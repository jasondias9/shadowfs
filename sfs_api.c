#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int BLK_SIZE, ROOT_DIR_SIZE, INODE_FILE_SIZE, INODE_SIZE, SUPER_BLOCK_SIZE, FBM_SIZE; 
int ROOT_DIR_BLK_SIZE, INODE_FILE_BLK_SIZE;

//*Cache*
fbm_t *fbm = NULL;
root_directory_t *rootdir = NULL;
ofd_t ofd[NB_FILES];

/* OFD Operations */

void OFD_init() {
    for(int i = 0; i < NB_FILES; i++) {
        ofd[i].inode_num = -1;
    }
}

/* SuperBlock Operations */


void * alloc(int size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr; 
}

void super_block_fetch(super_block_t *sb) {
    block_t *blk0 = alloc(BLK_SIZE);
    read_blocks(SUPER_BLOCK_ADD, 1, blk0);
    memcpy(sb, blk0, SUPER_BLOCK_SIZE);
    free(blk0);
}

void super_block_init(super_block_t *sb) { 
    sb->magic_number = MAGIC_NUMBER;
    sb->block_size = BLK_SIZE;
    //define as full even when these blks are not completely filled
    sb->fs_size = DATA_ADD * BLK_SIZE;    
    sb->num_inodes = 1; 

    inode_t *root = alloc(INODE_SIZE);;
    root->size = INODE_FILE_SIZE;
    root->indirect = UNDEF;
    for(int i = 0; i < NB_DIR_PTR; i++) {
        root->direct[i] = i + INODE_FILE_ADD;
    }
    sb->root = *root;
    free(root);
}

void super_block_write() {
    super_block_t *sb = alloc(SUPER_BLOCK_SIZE); 
    super_block_init(sb);
    block_t *blk0 = alloc(BLK_SIZE);
    memcpy(blk0, sb, SUPER_BLOCK_SIZE); 
    write_blocks(SUPER_BLOCK_ADD, 1, blk0);
    free(sb);
    free(blk0);
}

void super_block_save(super_block_t *sb) {
    block_t *blk0 = alloc(BLK_SIZE);
    memcpy(blk0, sb, SUPER_BLOCK_SIZE);
    write_blocks(SUPER_BLOCK_ADD, 1, blk0);
    free(blk0);
}

/* FBM Operations */

void fbm_save(fbm_t *fbm) {
    block_t *blk0 = alloc(FBM_SIZE);
    memcpy(blk0, fbm, FBM_SIZE);
    write_blocks(FBM_ADD, 1, blk0);
    free(blk0);
}

void fbm_init(fbm_t *fbm) {
    for(int i = 0; i < NB_BLKS; i++) {
        if(i < DATA_ADD) { 
            fbm->fbm[i] = FULL;
        } else {
            fbm->fbm[i] = AVAIL;
        }
    }
}

void fbm_write() {
    fbm = alloc(FBM_SIZE);
    fbm_init(fbm);
    block_t *blk1 = alloc(BLK_SIZE);
    memcpy(blk1, &fbm, FBM_SIZE); 
    write_blocks(FBM_ADD, 1, blk1);
    free(blk1);
}

void fbm_fetch() {
    fbm = alloc(FBM_SIZE);
    block_t *blk0 = alloc(BLK_SIZE);
    read_blocks(FBM_ADD, 1, blk0);
    memcpy(fbm, blk0, FBM_SIZE);
    free(blk0);
}

/* root directory operations */

void rootdir_fetch() {
    rootdir = alloc(ROOT_DIR_SIZE);
    block_t *blks = alloc(BLK_SIZE * ROOT_DIR_BLK_SIZE);
    read_blocks(ROOT_DIR_ADD, ROOT_DIR_BLK_SIZE, blks);
    memcpy(rootdir, blks, ROOT_DIR_SIZE);
    free(blks);
}

void rootdir_write(block_t *rootdir_blks) {
     for(int i = 0; i < ROOT_DIR_BLK_SIZE - 1; i++) {
        write_blocks(i+2, 1, &rootdir_blks[i]);
    }
}

//TODO: clean
void rootdir_segment(root_directory_t *rootdir_ptr, block_t *blks) {
    memcpy(&blks[0], &rootdir->name[0], BLK_SIZE);
    memcpy(&blks[1], &rootdir->name[93], BLK_SIZE);
    //write remaining bytes to the 3rd block
    memcpy(&blks[2], &rootdir->name[186], 154);  
}

void rootdir_init(root_directory_t *rootdir) {
    strcpy(rootdir->name[0], "/\0"); 
    for(int i = 1; i < NB_FILES; i++) {
        strcpy(rootdir->name[i], "\0"); 
    }
}

void rootdir_prepare() {
    rootdir = alloc(ROOT_DIR_SIZE);
    rootdir_init(rootdir);
    block_t *rootdir_blks = alloc(ROOT_DIR_SIZE * ROOT_DIR_BLK_SIZE);
    rootdir_segment(rootdir, rootdir_blks);
    rootdir_write(rootdir_blks);
    free(rootdir_blks);
}

/* inode file operations */
int find_free_inode(inode_f_t *inode_f) {
    for(int i = 1; i < NB_FILES; i++) {
        if(inode_f->inode_table[i].size == -1) {
            return i;
        }
    }
    return -1;
}

void root_init(inode_t *root) {
    root->size = ROOT_DIR_SIZE;
    for(int i = 0; i < NB_DIR_PTR; i++) {
        if(i < 3) {
            root->direct[i] = i + ROOT_DIR_ADD; 
        } else {
            root->direct[i] = UNDEF;
        }
    }
}

void inode_f_fetch(inode_f_t *inode_f) {
    block_t *blks = alloc(BLK_SIZE * INODE_FILE_BLK_SIZE);
    read_blocks(INODE_FILE_ADD, INODE_FILE_BLK_SIZE, blks);
    memcpy(inode_f, blks, INODE_FILE_SIZE);
    free(blks);
}

void inode_f_save(block_t *inode_f_blks) {
    for(int i = 0; i < INODE_FILE_BLK_SIZE; i++) {
        //write the inode_file
        write_blocks(i + INODE_FILE_ADD, 1, &inode_f_blks[i]);
    }
}

void inode_f_segment(inode_f_t *inode_f_ptr, block_t *blks) { 
    for(int i = 0; i < INODE_FILE_BLK_SIZE - 1; i++) {
        memcpy(&blks[i], &inode_f_ptr->inode_table[i*INODE_PER_BLK], BLK_SIZE);
    }
    //write remaining bytes to the 13th block
    memcpy(&blks[INODE_FILE_BLK_SIZE - 1], &inode_f_ptr->inode_table[192], 512);
}

void inode_alloc(inode_f_t *inode_f, inode_t *inode) {
    //initialize root
    inode_t *root_inode = alloc(INODE_SIZE);
    root_init(root_inode); 
    inode_f->inode_table[0] = *root_inode;
    for(int i = 1; i < NB_FILES; i++) {
        inode_f->inode_table[i] = *inode; 
    }
    free(root_inode);
}

void inode_init(inode_t *inode) {
    inode->size = -1;
    inode->indirect = -1;
    for(int i = 0; i < NB_DIR_PTR; i++) {
        //file points to no blocks for now.  
        inode->direct[i] = -1;
    } 
}

void inode_f_prepare() {
    inode_t *inode = alloc(INODE_SIZE);
    inode_init(inode);
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    //allocate the initilized inodes into a table
    inode_alloc(inode_f, inode); 
    block_t *inode_f_blks = alloc(INODE_FILE_SIZE * INODE_FILE_BLK_SIZE);
    //segment inode file into blocks
    inode_f_segment(inode_f, inode_f_blks);
    inode_f_save(inode_f_blks);
    free(inode);
    free(inode_f);
    free(inode_f_blks);
}

/* Helper */

int invalid_fileID(int fileID) {
    if(fileID < 0 || fileID > NB_FILES - 1) {
        return 1;
    }
    return 0;
}

int *intdup(int *arr, int length) {
    int *p = (int *)malloc(length*sizeof(int));
    memcpy(p, arr, length);
    return p;
}

/* Start of ssfs API */

void mkssfs(int fresh) {
    BLK_SIZE = sizeof(block_t);
    ROOT_DIR_SIZE = sizeof(root_directory_t);
    ROOT_DIR_BLK_SIZE = ROOT_DIR_SIZE/BLK_SIZE + 1;
    FBM_SIZE = sizeof(fbm_t);
    SUPER_BLOCK_SIZE = sizeof(super_block_t);
    INODE_FILE_SIZE = sizeof(inode_f_t);
    INODE_FILE_BLK_SIZE = INODE_FILE_SIZE/BLK_SIZE + 1;
    INODE_SIZE = sizeof(inode_t);

    OFD_init();
    if(fresh) {
        if(init_fresh_disk("test_disk", sizeof(block_t), NB_BLKS) < 0) {
            perror("Failed to create a new file system");
        }
        super_block_write();
        fbm_write();    
        rootdir_prepare();
        inode_f_prepare();
    } 
    else {
        init_disk("test_disk", sizeof(block_t), NB_BLKS);
        rootdir_fetch();
        fbm_fetch();
        for(int i = 0; i < NB_FILES; i++) {
            printf("file names: %s\n", rootdir->name[i]);
        }
    } 

    /* State of the file system at this point: 
     * fs = {0: sb, 1: fbm, [2-4]: root directory, [5, 17]: inode_file, [18, 1023] : free} 
    */
        
}

int ssfs_fopen(char *name) {
    super_block_t *sb = alloc(SUPER_BLOCK_SIZE);
    super_block_fetch(sb);
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f);

    int create = 1;
    int inode_num = -1;

    for(int i = 1; i < NB_FILES; i++) {
        if(strcmp(rootdir->name[i], name) == 0) {
            //I just need to open
            inode_num = i;
            create = 0; 
            break;
        }    
    }
    if(create) {
        inode_num = find_free_inode(inode_f);
        inode_f->inode_table[inode_num].size = 0;
        sb->num_inodes++;
        block_t *inode_f_blks = alloc(BLK_SIZE * INODE_FILE_BLK_SIZE);
        inode_f_segment(inode_f, inode_f_blks);
        inode_f_save(inode_f_blks);
        free(inode_f_blks);
        
        strncpy(rootdir->name[inode_num], name, strlen(name));
        block_t *rootdir_blks = alloc(BLK_SIZE * ROOT_DIR_BLK_SIZE);
        rootdir_segment(rootdir, rootdir_blks);
        rootdir_write(rootdir_blks);
        free(rootdir_blks);

    } 
    // added entry to open file descriptor table
    int fd;
    for(fd = 0; fd < NB_FILES; fd++) {
        if(ofd[fd].inode_num == -1) {
            ofd[fd].inode_num = inode_num;
            ofd[fd].r_ptr = 0;
            ofd[fd].w_ptr = inode_f->inode_table[inode_num].size;
            
            super_block_save(sb); 
            free(sb);
            free(inode_f);
            return fd;
        } 
    }
    free(sb);
    free(inode_f);
    return -1;
}

int ssfs_fclose(int fileID) {
    if(invalid_fileID(fileID)) {
        return -1;
    }
    if(ofd[fileID].inode_num != -1) {
        ofd[fileID].inode_num = -1;
        ofd[fileID].w_ptr = 0;
        ofd[fileID].r_ptr = 0;
        return 0;
    } 
    return -1;  
}

int ssfs_frseek(int fileID, int loc) {
    if(invalid_fileID(fileID)) {
        return -1;
    }

    int inode_num = ofd[fileID].inode_num; 
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f);
    int size = inode_f->inode_table[inode_num].size;
    free(inode_f);
     
    if(ofd[fileID].inode_num != -1 && loc >= 0 && loc < size + 1) {
        ofd[fileID].r_ptr = loc;
        return 0;
    }
    return -1;
}

int ssfs_fwseek(int fileID, int loc) {
    if(invalid_fileID(fileID)) {
        return -1;
    }

    int inode_num = ofd[fileID].inode_num;
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f);
    int size = inode_f->inode_table[inode_num].size;
    free(inode_f);
    
    if(ofd[fileID].inode_num != -1 && loc >= 0 && loc < size + 1) {
        ofd[fileID].w_ptr = loc;
        return 0;
    }
    //file specified is not initialized
    return -1;
}

int inode_num_fetch(int fileID) {
    if(invalid_fileID(fileID)) {
        return -1;
    }
    return ofd[fileID].inode_num;
}

int find_free_block() {
    for(int i = 0; i < NB_BLKS; i++) {
        if(fbm->fbm[i] == AVAIL) {
            return i;
        }
    }
    return -1;
}

int write_block(int length, int curr_blk, char *buf, int inode_num, inode_f_t *inode_f) {
    int initial_size = inode_f->inode_table[inode_num].size;
    int i = 0;
    while(length > 0) { 
  
        int blk_add = find_free_block();
        if(blk_add < 0) {
            printf("No available blocks to write to\n");
            return -1;
        } 
        block_t *blk0 = alloc(BLK_SIZE);

        if(length > BLK_SIZE) {
            memcpy(blk0, buf + i*BLK_SIZE, BLK_SIZE);
        } else {
            memcpy(blk0, buf, length);
        }

        write_blocks(blk_add, 1, blk0);
        fbm->fbm[blk_add] = 0; 
        free(blk0);

        if(length < BLK_SIZE) {
            inode_f->inode_table[inode_num].size += length;
        } else {
            inode_f->inode_table[inode_num].size += BLK_SIZE;
        }

        length -= BLK_SIZE;
        inode_f->inode_table[inode_num].direct[curr_blk] = blk_add;
        curr_blk++;
        i++;
    }
    return inode_f->inode_table[inode_num].size - initial_size;
}

int ssfs_fwrite(int fileID, char *buf, int length) { 
    
    if(invalid_fileID(fileID) || length < 1) {
        return -1;
    }

    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f);

    int wrote = 0;
    int w_ptr = ofd[fileID].w_ptr; 
    int inode_num = ofd[fileID].inode_num;    
   
    if(ofd[fileID].inode_num == -1) {
        //file doesnt exist 
        return -1;
    }  

    if(w_ptr % BLK_SIZE == 0) {
        //start writing in curr_blk
        int curr_blk = w_ptr/BLK_SIZE; 
        w_ptr = 0;
        //write free blks
        wrote = write_block(length, curr_blk, buf, inode_num, inode_f);
    } else { 
        //start writing in the curr_blk (where the w_ptr is)
        int curr_blk = w_ptr/BLK_SIZE;
        int write_size = length;
        w_ptr -= curr_blk*BLK_SIZE;
        
        if(length > BLK_SIZE - w_ptr) {
            write_size = BLK_SIZE - w_ptr;
        }
        block_t *blk0 = alloc(BLK_SIZE);
        int blk_add = inode_f->inode_table[inode_num].direct[curr_blk]; //TODO: deal with indirect
        read_blocks(blk_add, 1, blk0);
        memcpy((char *)blk0 + w_ptr, buf, write_size);
        //write the partial block
        write_blocks(blk_add, 1, blk0);
        free(blk0);

        int blk_occupied = inode_f->inode_table[inode_num].size - (BLK_SIZE*curr_blk);
        int actual_b_written;
        if(w_ptr > blk_occupied) {
            actual_b_written = write_size;
        } else {
            actual_b_written = write_size - (blk_occupied - w_ptr);
        }
        wrote = write_size;
        length -= wrote; 
        inode_f->inode_table[inode_num].size += actual_b_written;
        w_ptr = 0; 
        //write remaining buff if avail
        if(length) {
            wrote += write_block(length, curr_blk+1, buf + write_size, inode_num, inode_f);
        }
    } 
    block_t *blks = alloc(BLK_SIZE * INODE_FILE_BLK_SIZE);
    inode_f_segment(inode_f, blks);
    inode_f_save(blks); 
    free(blks);

    fbm_save(fbm);
    ofd[fileID].w_ptr = inode_f->inode_table[inode_num].size;
    free(inode_f);

    super_block_t *sb = alloc(SUPER_BLOCK_SIZE);
    sb->fs_size += wrote;
    super_block_save(sb);
    free(sb);

    return wrote;
}

int ssfs_fread(int fileID, char *buf, int length) {
    if(invalid_fileID(fileID)) {
        return -1;
    } 
 
    int r_ptr = ofd[fileID].r_ptr;
    int inode_num = ofd[fileID].inode_num;

    if(inode_num < 0) {
        return -1;
    }

    int curr_blk = r_ptr/BLK_SIZE;
    int blks_to_read = (length - r_ptr)/BLK_SIZE + 1;
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f); 
    
    block_t *blks = alloc(BLK_SIZE * blks_to_read);
    for(int i = curr_blk; i < blks_to_read; i++) {
        read_blocks(inode_f->inode_table[inode_num].direct[i], 1, &blks[i]);
    }

    memcpy(buf, (char *)blks + r_ptr, length);
    free(inode_f);
    free(blks);
    return length;
}

int ssfs_remove(char *file) {
        int inode_num;
        //never delete root - protection
        for(inode_num = 1; inode_num < NB_FILES; inode_num++) {
            if(strcmp(rootdir->name[inode_num], file) == 0) {
                inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
                inode_f_fetch(inode_f);
                super_block_t *sb = alloc(SUPER_BLOCK_SIZE);
                super_block_fetch(sb);
                
                sb->fs_size -= inode_f->inode_table[inode_num].size;
                sb->num_inodes--;
                inode_f->inode_table[inode_num].size = 0;
                super_block_save(sb);
                free(sb);
            
                for(int i = 0; i < NB_DIR_PTR; i++) {
                    if(inode_f->inode_table[inode_num].direct[i] != UNDEF) {
                        fbm->fbm[inode_f->inode_table[inode_num].direct[i]] = AVAIL;
                        inode_f->inode_table[inode_num].direct[i] = UNDEF;      
                    }
                }

                fbm_save(fbm);
                block_t *blks = alloc(BLK_SIZE * INODE_FILE_BLK_SIZE);
                inode_f_segment(inode_f, blks);  
                free(inode_f);
                
                inode_f_save(blks);
                free(blks);   
                return 0;
            }
        }
        //unable to find the file
        return -1;
}
