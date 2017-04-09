#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//*Cache*
FBM_t *fbm = NULL;
root_directory_t *rootdir = NULL;
OFD_t ofd[NB_FILES];

/* OFD Operations */

void OFD_init() {
    for(int i = 0; i < NB_FILES; i++) {
        ofd[i].inode_num = -1;
    }
}

/* SuperBlock Operations */

void SuperBlock_fetch(SuperBlock_t *sb) {
    block_t *blk0 = (block_t *)malloc(sizeof(block_t));
    memset(blk0, 0, sizeof(block_t));
    read_blocks(0, 1, blk0);
    memcpy(sb, blk0, sizeof(*sb));
    free(blk0);
}

void SuperBlock_init(SuperBlock_t *sb) { 
    sb->magic_number = MAGIC_NUMBER;
    sb->block_size = BLK_SIZE;
    sb->fs_size = 0;    
    sb->num_inodes = 1; 

    inode_t *root = (inode_t *)malloc(sizeof(inode_t));;
    memset(root, 0, sizeof(inode_t));
    root->size = sizeof(inode_f_t);
    root->indirect = UNDEF;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        root->direct[i] = i + JNODE_OFFSET;
    }
    sb->root = *root;
    free(root);
}

void SuperBlock_write() {
    SuperBlock_t *sb = (SuperBlock_t *)malloc(sizeof(SuperBlock_t));
    
    SuperBlock_init(sb);
    block_t *blk0 = (block_t *)malloc(BLK_SIZE);
    memset(blk0, 0, sizeof(block_t));
    memcpy(blk0, sb, sizeof(SuperBlock_t)); 
    write_blocks(0,1, blk0);
    free(sb);
    free(blk0);
}

void SuperBlock_save(SuperBlock_t *sb) {
    block_t *blk0 = (block_t *)malloc(sizeof(block_t));
    memset(blk0, 0, BLK_SIZE);
    memcpy(blk0, sb, sizeof(SuperBlock_t));
    write_blocks(0,1, blk0);
    free(blk0);
}

/* FBM Operations */

void FBM_save(FBM_t *fbm) {
    block_t *blk0 = (block_t *)malloc(sizeof(FBM_t));
    memcpy(blk0, fbm, sizeof(FBM_t));
    write_blocks(1, 1, blk0);
    free(blk0);
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

void FBM_write() {
    fbm = (FBM_t *)malloc(sizeof(FBM_t));
    FBM_init(fbm);
    block_t *blk1 = (block_t *)malloc(sizeof(block_t));
    memcpy(blk1, &fbm, sizeof(FBM_t)); 
    write_blocks(1,1, blk1);
    free(blk1);
}

/* root directory operations */

void rootdir_write(block_t *rootdir_blks) {
     for(int i = 0; i < 3; i++) {
        write_blocks(i+2, 1, &rootdir_blks[0]);
    }
}

void rootdir_segment(root_directory_t *rootdir_ptr, block_t *blks) {
    memcpy(&blks->data[0], &rootdir->name[0], BLK_SIZE);
    memcpy(&blks->data[BLK_SIZE], &rootdir->name[93], BLK_SIZE);
    memcpy(&blks->data[2*BLK_SIZE], &rootdir->name[186], 154);  
}

void rootdir_init(root_directory_t *rootdir) {
    //init root name;
    strcpy(rootdir->name[0], "/\0"); 
    for(int i = 1; i < NB_FILES; i++) {
        strcpy(rootdir->name[i], "\0"); 
    }
}

void rootdir_prepare() {
    rootdir = (root_directory_t *)malloc(sizeof(root_directory_t));
    memset(rootdir, 0, sizeof(root_directory_t));
    rootdir_init(rootdir);
    block_t *rootdir_blks = (block_t *)malloc(sizeof(block_t) * 4);
    memset(rootdir_blks, 0, sizeof(block_t) * ROOT_BLK_ALLOC);
    //segment rootdir into blocks
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
    root->size = 2189;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        if(i < 3) {
            root->direct[i] = i + ROOT_DIR_OFFSET; 
        } else {
            root->direct[i] = UNDEF;
        }
    }
}


void inode_f_fetch(inode_f_t *inode_f) {
    block_t *blks = (block_t *)malloc(sizeof(block_t)*MAX_DIRECTS);
    read_blocks(5, 14, blks);
    memcpy(inode_f, blks, sizeof(inode_f_t));
    free(blks);
}

void inode_f_save(block_t *inode_f_blks) {
    for(int i = 0; i < 12; i++) {
        //write the inode_file
        write_blocks(i + 5, 1, &inode_f_blks[i]);
    }
}

void inode_f_segment(inode_f_t *inode_f_ptr, block_t *blks) { 
    for(int i = 0; i < 12; i++) {
        memcpy(&blks[i], &inode_f_ptr->inode_table[i*INODE_PER_BLK], sizeof(block_t));
    }
    memcpy(&blks[12], &inode_f_ptr->inode_table[192], 512);
}

void inode_alloc(inode_f_t *inode_f, inode_t *inode) {
    //initialize root
    inode_t *root_inode = (inode_t *)malloc(sizeof(inode_t));
    memcpy(root_inode, &inode, sizeof(inode_t));
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
    for(int i = 0; i < MAX_DIRECTS; i++) {
        //file points to no blocks for now.  
        inode->direct[i] = -1;
    } 
}

void inode_f_prepare() {
    inode_t *inode = (inode_t *)malloc(sizeof(inode_t));
    inode_init(inode);
    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
    memset(inode_f, 0, NB_FILES);
    //allocate the initilized inodes into a table
    inode_alloc(inode_f, inode); 
    block_t *inode_f_blks = (block_t *)malloc(sizeof(block_t) * MAX_DIRECTS - 1);
    //segment inode file into blocks
    inode_f_segment(inode_f, inode_f_blks);
    inode_f_save(inode_f_blks);
    free(inode);
    free(inode_f);
    free(inode_f_blks);
}

/* Helper */

int invalid_fileID(int fileID) {
    if(fileID < 0 || fileID > 199) {
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


    OFD_init();
    if(fresh) {
        if(init_fresh_disk("test_disk", sizeof(block_t), NB_BLKS) < 0) {
            perror("Failed to create a new file system");
        }

        SuperBlock_write();
        FBM_write();    
        rootdir_prepare();
        inode_f_prepare();
    } else {
        init_disk("test_disk", sizeof(block_t), NB_BLKS);
    } 
    /* State of the file system at this point: 
     * fs = {0: sb, 1: fbm, [2-4]: root directory, [5, 18]: inode_file, [19, 1023] : free} 
    */
        
}

int ssfs_fopen(char *name) {
    SuperBlock_t *sb = (SuperBlock_t *)malloc(sizeof(SuperBlock_t));
    SuperBlock_fetch(sb);

    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
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
        block_t *inode_f_blks = (block_t *)malloc(sizeof(block_t) * MAX_DIRECTS);
        inode_f_segment(inode_f, inode_f_blks);
        inode_f_save(inode_f_blks);
        free(inode_f_blks);
        
        strncpy(rootdir->name[inode_num], name, strlen(name));
        block_t *rootdir_blks = (block_t *)malloc(sizeof(block_t) * NB_FILES);
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
            SuperBlock_save(sb); 
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
    
    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
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
    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
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
        if(fbm->fbm[i] == EMPTY) {
            return i;
        }
    }
    return -1;
}

int write_cont(int length, int curr_blk, char *buf, int inode_num, inode_f_t *inode_f) {
    int initial_size = inode_f->inode_table[inode_num].size;
    int i = 0;
    while(length > 0) { 
  
        int blk_add = find_free_block();
        printf("block address %i\n\n\n", blk_add);
        if(blk_add < 0) {
            printf("No available blocks to write to\n");
            return -1;
        } 
        block_t *blk0 = (block_t *)malloc(sizeof(block_t));
        memset(blk0, 0, BLK_SIZE); 
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
    if(invalid_fileID(fileID)) {
        return -1;
    } 
    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
    inode_f_fetch(inode_f);
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
        w_ptr = 0;
        //write free blks
        wrote = write_cont(length, curr_blk, buf, inode_num, inode_f);
    } else { 
        //start writing in the curr_blk (where the w_ptr is)
        int curr_blk = w_ptr/BLK_SIZE;
        int write_size = length;

        //actual write position in curr block
        w_ptr -= curr_blk*BLK_SIZE;
        
        if(length > BLK_SIZE - w_ptr) {
            write_size = BLK_SIZE - w_ptr;
        }
        // the additional blks required to write remaining length
        block_t *blk0 = (block_t *)malloc(sizeof(block_t));

        //get the actual blk address
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
            wrote += write_cont(length, curr_blk+1, buf + write_size, inode_num, inode_f);
        }
    }

    block_t *blks = (block_t *)malloc(sizeof(block_t)*MAX_DIRECTS);
    inode_f_segment(inode_f, blks);
    inode_f_save(blks);
    
    free(blks);
    FBM_save(fbm);
    ofd[fileID].w_ptr = inode_f->inode_table[inode_num].size;
    free(inode_f);
    return wrote;
}


/*int test_seek(int file_id, int file_size, int write_ptr, char *write_buf, int num_file, int offset){
  int res;
  for(int i = 0; i < num_file; i++){
    //Just testing the shift for beyond seek boundaries before actually doing it. 
    res = ssfs_frseek(file_id, -1);
    if(res >= 0)
      fprintf(stderr, "Warning: ssfs_frseek returned positive. Negative seek location attempted. Potential frseek fail?\n");
    res = ssfs_frseek(file_id, file_size + 100);
    if(res >= 0)
      fprintf(stderr, "Warning: ssfs_frseek returned positive. Seek location beyond file size attempted. Potential frseek fail?\n");
    res = ssfs_fwseek(file_id, -1);
    if(res >= 0)
      fprintf(stderr, "Warning: ssfs_frseek returned positive. Negative seek location attempted. Potential fwseek fail?\n");
    res = ssfs_fwseek(file_id, file_size + 100);
    if(res >= 0)
      fprintf(stderr, "Warning: ssfs_frseek returned positive. Seek location beyond file size attempted. Potential fwseek fail?\n");
    res = ssfs_frseek(file_id, file_size - offset);
    if(res < 0)
      fprintf(stderr, "Warning: ssfs_frseek returned negative. Potential frseek fail?\n");
    res = ssfs_fwseek(file_id, file_size - offset);
    if(res < 0)
      fprintf(stderr, "Warning: ssfs_fwseek returned negative. Potential fwseek fail?\n");
    write_ptr -= offset;
    if(write_ptr < 0)
      write_ptr = 0;
    //file_size[i] -= 10;
  }
  return 0;
}

int main() {
    mkssfs(1);
    int f1_fd = ssfs_fopen("f1");
    //int f2_fd = ssfs_fopen("f2"); 
    //int f3_fd = ssfs_fopen("f3");
    char buf[27] = "hello this is a test string";

    char *ret = (char *)malloc(27);
    //test_seek(f1_fd, 18*140, 10, buf, 1, 10);
    
    ssfs_fwrite(f1_fd, buf, 27); 
    ssfs_fread(f1_fd, ret, 27);

    printf("%s", ret);
    free(ret);
    free(fbm);
    free(rootdir);
    return 1;
}*/

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
    inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
    inode_f_fetch(inode_f); 
    
    block_t *blks = (block_t *)malloc(sizeof(block_t) * blks_to_read);
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
        for(inode_num = 0; inode_num < NB_FILES; inode_num++) {
            if(strcmp(rootdir->name[inode_num], file) == 0) {
                inode_f_t *inode_f = (inode_f_t *)malloc(sizeof(inode_f_t));
                inode_f_fetch(inode_f);
                inode_f->inode_table[inode_num].size = 0;
                for(int i = 0; i < MAX_DIRECTS; i++) {
                    if(inode_f->inode_table[inode_num].direct[i] != -1) {
                        fbm->fbm[inode_f->inode_table[inode_num].direct[i]] = 1;
                        inode_f->inode_table[inode_num].direct[i] = -1;
                        FBM_save(fbm);
                        block_t *blks = (block_t *)malloc(sizeof(block_t) * MAX_DIRECTS);
                        inode_f_segment(inode_f, blks);
                        inode_f_save(blks);
                        free(blks);
                        free(inode_f);
                        return 0;
                    }
                }
                return -1;
            }
        }
        
        return -1;
}
