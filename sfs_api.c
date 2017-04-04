#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//store these in memory to ensure fast access
SuperBlock_t sb;
FBM_t fbm;
root_directory_t rootdir;
inode_f_t inode_f;
OFD_t ofd[NB_FILES];

void OFD_init() {
    for(int i = 0; i < NB_FILES; i++) {
        ofd[i] = *(OFD_t *)malloc(sizeof(OFD_t));
        ofd[i].inode_num = -1;
    }
}

void FBM_init(FBM_t *fbm) {
    for(int i = 0; i < NB_BLKS; i++) {
        if(i <= 18) { 
            fbm->fbm[i] = FULL;
        } else {
            fbm->fbm[i] = EMPTY;
        }
    }
}

void SuperBlock_init(SuperBlock_t *sb) { 
    sb->magic_number = MAGIC_NUMBER;
    sb->block_size = BLK_SIZE;
    sb->fs_size = 0; //is this supposed to be current FS size ? or max?  
    sb->num_inodes = 1; //counting the root (only 199 remain)

    inode_t root = *(inode_t *)malloc(sizeof(inode_t));;
    root.size = MAX_DIRECTS;
    root.indirect = UNDEF;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        root.direct[i] = i + JNODE_OFFSET;
    }
    sb->root = root; 
}

void inode_init(inode_t *inode) {
    inode->size = -1;
    inode->indirect = -1;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        //file points to no blocks for now.  
        inode->direct[i] = -1;
    } 
}

void segment_rootdir(root_directory_t *rootdir_ptr, block_t *blks) {
    for(int i = 0; i < ROOT_BLK_ALLOC; i++) {
        blks[i] = *(block_t *)malloc(BLK_SIZE);
        memcpy(&blks[i], &(rootdir_ptr->name[i*NAME_PER_BLK]), BLK_SIZE);
    }
}

void segment_inode_f(inode_f_t *inode_f_ptr, block_t *blks) { 
    for(int i = 0; i < MAX_DIRECTS; i++) {
        blks[i] =  *(block_t *)malloc(BLK_SIZE);
        memcpy(&blks[i], &(inode_f_ptr->inode_table[i*INODE_PER_BLK]), BLK_SIZE);
    }
}

void root_init(inode_t *root) {
    root->size = 2189;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        if(i < 3) {
            root->direct[i] = i + ROOT_DIR_OFFSET; 
        } else {
            root->direct[i] = UNDEF;
        }
    }
}

void inode_alloc(inode_f_t *inode_f, inode_t inode) {
    //initialize root
    inode_t root_inode = *(inode_t *)malloc(sizeof(inode_t));
    memcpy(&root_inode, &inode, sizeof(inode));
    root_init(&root_inode); 
    inode_f->inode_table[0] = root_inode;
    for(int i = 1; i < NB_FILES; i++) {
        inode_f->inode_table[i] = inode; 
    }
}

void rootdir_init(root_directory_t *rootdir) {
    //init root name;
    strcpy(rootdir->name[0], "/\0"); 
    for(int i = 1; i < NB_FILES; i++) {
        strcpy(rootdir->name[i], "\0"); 
    }
}

void mkssfs(int fresh) {
    OFD_init();
    if(fresh) {
        if(init_fresh_disk("test_disk", BLK_SIZE, NB_BLKS) < 0) {
            perror("Faild to create a new file system");
        }

        sb = *(SuperBlock_t *)malloc(sizeof(SuperBlock_t));
        fbm = *(FBM_t *)malloc(sizeof(FBM_t));
        rootdir = *(root_directory_t *)malloc(sizeof(root_directory_t));
        inode_t inode = *(inode_t *)malloc(sizeof(inode_t));
        
        SuperBlock_init(&sb);
        block_t blk0 = *(block_t *)malloc(BLK_SIZE);
        memcpy(&blk0, &sb, sizeof(sb));
        
        FBM_init(&fbm);
        block_t blk1 = *(block_t *)malloc(BLK_SIZE);
        memcpy(&blk1, &fbm, sizeof(fbm));
        
        //initialize empty inodes for the inode_file
        inode_init(&inode);
        inode_f = *(inode_f_t *)malloc(sizeof(inode_f_t));
        //allocate the initilized inodes into a table
        inode_alloc(&inode_f, inode); 

        block_t *inode_f_blks = (block_t *)malloc(BLK_SIZE * MAX_DIRECTS);
        //segment inode file into blocks
        segment_inode_f(&inode_f, inode_f_blks);
       
        //initialize root directory
        rootdir_init(&rootdir);
        block_t *rootdir_blks = (block_t *)malloc(BLK_SIZE * NB_FILES);
        //segment rootdir into blocks
        segment_rootdir(&rootdir, rootdir_blks); 

        write_blocks(0,1, &blk0); 
        write_blocks(1,1, &blk1);
        for(int i = 2; i <= 4; i++) {
            write_blocks(i, 1, &rootdir_blks[i-2]);
        }
        for(int i = 5; i <= 18; i++) {
            //write the inode_file
            write_blocks(i, 1, &inode_f_blks[i-5]);
        }
        
    } else {
        init_disk("test_disk", BLK_SIZE, NB_BLKS);
    } 
    /* State of the file system at this point: 
     * fs = {0: sb, 1: fbm, [2-4]: root directory, [5, 18]: inode_file, [19, 1023] : free} 
    */
        
}

int find_free_inode(inode_f_t *inode_f) {
    for(int i = 1; i < NB_FILES; i++) {
        if(inode_f->inode_table[i].size == -1) {
            return i;
        }
    }
    return -1;
}

int ssfs_fopen(char *name) {
    
    if(sb.magic_number != MAGIC_NUMBER) {
        sb = *(SuperBlock_t *)malloc(sizeof(SuperBlock_t));
        read_blocks(0, 1, &sb);
    } 
    if(sb.num_inodes == 200) {
        //no available inodes
        return -1;
    } 
    //check if root directory is already in cache
    if(strcmp(rootdir.name[0],"/") != 0) {
        printf("reloading root from disk...\n");
        rootdir = *(root_directory_t *)malloc(sizeof(root_directory_t));
        read_blocks(2, 3, &rootdir);
        //for now this works. Will need to check that EOF is correct.
        //can write simple test for this
    }
    int create = 1;
    int inode_num = -1;
    //for now linear search. can add optimization
    for(int i = 1; i < NB_FILES; i++) {
        if(strcmp(rootdir.name[i], name) == 0) {
            //I just need to open
            inode_num = i;
            create = 0; 
            break;
        }    
    }

    if(create) {
        //check if inode_f is already in cache 
        if(inode_f.inode_table[0].size != ROOT_DIR_SIZE) {
            printf("reloading inode file from disk...\n");
            inode_f = *(inode_f_t *)malloc(sizeof(inode_f_t));
            read_blocks(5, 14, &inode_f);
        }
        inode_num = find_free_inode(&inode_f);
        inode_f.inode_table[inode_num].size = 0;
        strcpy(rootdir.name[inode_num], name);
    }

    // added entry to open file descriptor table
    int fd;
    for(fd = 0; fd < NB_FILES; fd++) {
        if(ofd[fd].inode_num == -1) {
            ofd[fd].inode_num = inode_num;
            ofd[fd].r_ptr = 0;
            ofd[fd].w_ptr = inode_f.inode_table[inode_num].size;
            return fd;
        } 
    }
    return -1;
}

int ssfs_fclose(int fileID) {
    if(ofd[fileID].inode_num != -1) {
        ofd[fileID].inode_num = -1;
        ofd[fileID].w_ptr = 0;
        ofd[fileID].r_ptr = 0;
        return 0;
    } 
    //file specified is not initialized
    return -1;
    
}

int ssfs_frseek(int fileID, int loc) {
    if(ofd[fileID].inode_num != -1) {
        ofd[fileID].r_ptr = loc;
        return 0;
    }
    //file specified is not initialized
    return -1;
}

int ssfs_fwseek(int fileID, int loc) {
    if(ofd[fileID].inode_num != -1) {
        ofd[fileID].w_ptr = loc;
        return 0;
    }
    //file specified is not initialized
    return -1;
}

int find_free_block() {
    for(int i = 0; i < NB_BLKS; i++) {
        if(fbm.fbm[i] == EMPTY) {
            return i;
        }
    }
    return -1;
}

int write_cont(int length, int curr_blk, char **buf, int inode_num) {
    int initial_size = inode_f.inode_table[inode_num].size;
    while(length > 0) {
        int i = 0;
        int blk_add = find_free_block();
        if(blk_add == -1) {
            printf("No available blocks to write to\n");
            return -1;
        } 
        block_t blk0 = *(block_t *)malloc(BLK_SIZE);
        memcpy(&blk0, &buf[i*BLK_SIZE], BLK_SIZE);
        write_blocks(blk_add, 1, &blk0);
        length -= BLK_SIZE;
        if(inode_f.inode_table[inode_num].direct[curr_blk] == -1) {
            if(length-BLK_SIZE < 0) {
                inode_f.inode_table[inode_num].size += length;
            } else {
                inode_f.inode_table[inode_num].size += BLK_SIZE;
            } 
        }
        inode_f.inode_table[inode_num].direct[curr_blk] = blk_add;
        curr_blk++;
        i++;
    }
    return inode_f.inode_table[inode_num].size - initial_size;
}

int ssfs_fwrite(int fileID, char *buf, int length) { 
    int wrote = 0;
    if(length < 1) return -1;
    int w_ptr = ofd[fileID].w_ptr;
    
    int inode_num = ofd[fileID].inode_num;
    if(ofd[fileID].inode_num == -1) {
        //file not open
        return -1;
    }  
 
    if(w_ptr % BLK_SIZE == 0) {
        //start writing in curr_blk
        int curr_blk = w_ptr/BLK_SIZE; 
        //total blks required for write
        int tot_blks = length/BLK_SIZE + 1;
        w_ptr = 0;

        //write free blks
        wrote = write_cont(length, curr_blk, &buf, inode_num);
    } else { 
        //start writing in the curr_blk (where the w_ptr is)
        int curr_blk = w_ptr/BLK_SIZE;
    
        //actual write position in curr block
        w_ptr -= curr_blk*BLK_SIZE;

        // the additional blks required to write remaining length
        int req_blk = (length - (BLK_SIZE-w_ptr))/BLK_SIZE + 1;

        block_t blk0 = *(block_t *)malloc(BLK_SIZE);
        //get the actual blk address
        int blk_add = inode_f.inode_table[inode_num].direct[curr_blk]; //TODO: deal with indirect
        read_blocks(blk_add, 1, &blk0);
        memcpy(&blk0.data[w_ptr], &buf, BLK_SIZE - w_ptr); 
        //write the partial block
        write_blocks(blk_add, 1, &blk0);   

        wrote = (BLK_SIZE - w_ptr);
        length -= wrote; 
        w_ptr = 0;

        //write free blks
        wrote += write_cont(length, curr_blk, &buf, inode_num);
    }
    return wrote;
}

int main() {
    mkssfs(1);
    int fd = ssfs_fopen("hello world");
    printf("fd: %i\n", fd);
    int inode_num = ofd[0].inode_num;
    printf("inode_number: %i\n", inode_num);
    return 1;
}

int ssfs_fread(int fileID, char *buf, int length) {
    return 0;
}

int ssfs_remove(char *file) {
    return 0;
}

int ssfs_commit() {
    return 0;
}

int ssfs_restore(int cnum) {
    return 0;
}

