#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int BLK_SIZE, ROOT_DIR_SIZE, INODE_FILE_SIZE, INODE_SIZE, SUPER_BLOCK_SIZE, FBM_SIZE, INDIRECT_BLK_SIZE; 
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

/* save a modified superblock to file */
void super_block_save(super_block_t *sb) {
    block_t *blk0 = alloc(BLK_SIZE);
    memcpy(blk0, sb, SUPER_BLOCK_SIZE);
    write_blocks(SUPER_BLOCK_ADD, 1, blk0);
    free(blk0);
}

/* write a new superblock to file */
void super_block_prepare() {
    super_block_t *sb = alloc(SUPER_BLOCK_SIZE); 
    super_block_init(sb);
    super_block_save(sb);
    free(sb);
}

/* FBM Operations */

/* save a modified fbm to file */
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

void fbm_prepare() {
    fbm = alloc(FBM_SIZE);
    fbm_init(fbm);
    fbm_save(fbm);
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

/* segment the rootdirectory into blks */
void rootdir_segment(root_directory_t *rootdir_ptr, block_t *blks) {
    memcpy(&blks[0], &rootdir->name[0], BLK_SIZE);
    memcpy(&blks[1], &rootdir->name[NAME_PER_BLK], BLK_SIZE);
    //write remaining bytes to the 3rd block
    memcpy(&blks[2], &rootdir->name[NAME_PER_BLK*2], 154);  
}

void rootdir_save(root_directory_t *rootdir) {
     block_t *rootdir_blks = alloc(BLK_SIZE * ROOT_DIR_BLK_SIZE);
     rootdir_segment(rootdir, rootdir_blks);
        
     for(int i = 0; i < ROOT_DIR_BLK_SIZE - 1; i++) {
        write_blocks(i+2, 1, &rootdir_blks[i]);
    }
    free(rootdir_blks);
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
    rootdir_save(rootdir);
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

void inode_f_segment(inode_f_t *inode_f_ptr, block_t *blks) { 
    for(int i = 0; i < INODE_FILE_BLK_SIZE - 1; i++) {
        memcpy(&blks[i], &inode_f_ptr->inode_table[i*INODE_PER_BLK], BLK_SIZE);
    }
    //write remaining bytes to the 13th block
    memcpy(&blks[INODE_FILE_BLK_SIZE - 1], &inode_f_ptr->inode_table[192], 512);
}

void inode_f_save(inode_f_t *inode_f) {
    block_t *blks = alloc(BLK_SIZE * INODE_FILE_BLK_SIZE);
    inode_f_segment(inode_f, blks);
    
    for(int i = 0; i < INODE_FILE_BLK_SIZE; i++) {
        write_blocks(i + INODE_FILE_ADD, 1, &blks[i]);
    }
    free(blks);
}

void inode_alloc(inode_f_t *inode_f, inode_t *inode) {
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
    inode_f_save(inode_f);
    free(inode);
    free(inode_f);
}

/* Helper */

int invalid_fileID(int fileID) {
    if(fileID < 0 || fileID > NB_FILES - 1) {
        return 1;
    }
    return 0;
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
    INDIRECT_BLK_SIZE = sizeof(indirect_blk_t);
    OFD_init();

    if(fresh) {
        if(init_fresh_disk("test_disk", sizeof(block_t), NB_BLKS) < 0) {
            perror("Failed to create a new file system");
        }
        super_block_prepare();
        fbm_prepare();    
        rootdir_prepare();
        inode_f_prepare();
    } 
    else {
        init_disk("test_disk", sizeof(block_t), NB_BLKS);
        rootdir_fetch();
        fbm_fetch(); 
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
    
    //create a copy in case we need to rollback
    root_directory_t *rootdir_cpy = alloc(ROOT_DIR_SIZE);
    memcpy(rootdir_cpy, rootdir, ROOT_DIR_SIZE);

    if(sb->num_inodes >= 199) {
        return -1;
    }
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
        strncpy(rootdir->name[inode_num], name, strlen(name)); 
    } 
    int fd;
    for(fd = 0; fd < NB_FILES; fd++) {
        if(ofd[fd].inode_num == -1) {
            ofd[fd].inode_num = inode_num;
            ofd[fd].r_ptr = 0;
            ofd[fd].w_ptr = inode_f->inode_table[inode_num].size; 
            
            super_block_save(sb); 
            rootdir_save(rootdir);
            inode_f_save(inode_f);
            free(rootdir_cpy);
            free(sb);
            free(inode_f);
            return fd;
        } 
    }
    memset(rootdir, 0, ROOT_DIR_SIZE);
    memcpy(rootdir, rootdir_cpy, ROOT_DIR_SIZE);
    free(rootdir_cpy);
    //don't save sb and inode_f rollsback since they arent cached
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

int write_block(int length, int curr_blk, char *buf, int inode_num, inode_f_t *inode_f) {
    int initial_size = inode_f->inode_table[inode_num].size;
    int i = 0;  

    indirect_blk_t *indir_blk = NULL; 
    int indir_add = -1;
    while(length > 0) { 
        if(curr_blk == NB_DIR_PTR) {
            indir_add = find_free_block();
            indir_blk = alloc(INDIRECT_BLK_SIZE);
        }

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
        fbm->fbm[blk_add] = FULL; 
        free(blk0);

        if(length < BLK_SIZE) {
            inode_f->inode_table[inode_num].size += length;
        } else {
            inode_f->inode_table[inode_num].size += BLK_SIZE;
        }

        length -= BLK_SIZE;
        if(indir_add > -1) {
            indir_blk->direct[curr_blk - NB_DIR_PTR] = blk_add; 
        } else {
            inode_f->inode_table[inode_num].direct[curr_blk] = blk_add;
        }
        curr_blk++;
        
        i++;
    }
    if(indir_add > -1) {
        write_blocks(indir_add, 1, indir_blk);
        free(indir_blk);
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
        return -1;
    }  

    if(w_ptr % BLK_SIZE == 0) {
        //start writing in curr_blk
        int curr_blk = w_ptr/BLK_SIZE;  
        w_ptr = 0;
        //write free blks
        wrote = write_block(length, curr_blk, buf, inode_num, inode_f);
    } else { 
        //start writing in the curr_blk at the w_ptr
        int curr_blk = w_ptr/BLK_SIZE;
        int write_size = length;
        w_ptr -= curr_blk*BLK_SIZE;
        
        //determine bytes to write in the partial block
        if(length > BLK_SIZE - w_ptr) {
            write_size = BLK_SIZE - w_ptr;
        }
        
        //get the curr_blk from memory
        block_t *blk0 = alloc(BLK_SIZE);
        int blk_add = inode_f->inode_table[inode_num].direct[curr_blk];
        read_blocks(blk_add, 1, blk0);
        memcpy((char *)blk0 + w_ptr, buf, write_size);
        
        //write the partial block 
        write_blocks(blk_add, 1, blk0);
        free(blk0);
        
        //determine the true number of bytes written
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
    
    inode_f_save(inode_f); 

    fbm_save(fbm);
    ofd[fileID].w_ptr += wrote;
    free(inode_f);

    super_block_t *sb = alloc(SUPER_BLOCK_SIZE);
    sb->fs_size += wrote;
    super_block_save(sb);
    free(sb);
    
    return wrote;
}

int indirect_blk_fetch(indirect_blk_t *indir_blk, int blk_add) {
    read_blocks(blk_add, 1, indir_blk);
}

int ssfs_fread(int fileID, char *buf, int length) {
    
    int inode_num = ofd[fileID].inode_num;
    if(length < 1 || invalid_fileID(fileID) || inode_num < 0) {
        return -1;
    }
    
    inode_f_t *inode_f = alloc(INODE_FILE_SIZE);
    inode_f_fetch(inode_f); 
    int file_size = inode_f->inode_table[inode_num].size; 
    
    if(length > file_size) {
        free(inode_f);
        return -1;
    }
    
    int r_ptr = ofd[fileID].r_ptr;
    int rem_bytes = file_size - r_ptr;
    if(length > rem_bytes) {
        //just read to the end of the file
        length = rem_bytes;
    }

    int nb_blks = file_size / BLK_SIZE + 1; 
    block_t *blks = alloc(BLK_SIZE * nb_blks);
    int indir_add = inode_f->inode_table[inode_num].indirect;
    for(int i = 0; i < nb_blks; i++) {
        read_blocks(inode_f->inode_table[inode_num].direct[i], 1, &blks[i]); 
    } 
    
    //retrieve and read indirect block pointers
    if(nb_blks > NB_DIR_PTR && indir_add != UNDEF) {
        indirect_blk_t *indir_blk = alloc(INDIRECT_BLK_SIZE); 
        indirect_blk_fetch(indir_blk, indir_add);
        for(int i = 0; i < BLK_SIZE; i++) {
            if(indir_blk->direct[i] != -1) {
                read_blocks(indir_blk->direct[i], 1, &blks[i + nb_blks]);  
            }
        }
        free(indir_blk);
    }
     
    ofd[fileID].r_ptr += length;
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

                strcpy(rootdir->name[inode_num], "\0"); 
                
                sb->fs_size -= inode_f->inode_table[inode_num].size;
                sb->num_inodes--;
                inode_f->inode_table[inode_num].size = 0;
                for(int i = 0; i < NB_DIR_PTR; i++) {
                    if(inode_f->inode_table[inode_num].direct[i] != UNDEF) {
                        fbm->fbm[inode_f->inode_table[inode_num].direct[i]] = AVAIL;
                        if(inode_f->inode_table[inode_num].indirect != UNDEF) {
                            fbm->fbm[inode_f->inode_table[inode_num].indirect] = AVAIL;
                        }
                        inode_f->inode_table[inode_num].direct[i] = UNDEF;
                        inode_f->inode_table[inode_num].indirect = UNDEF;
                    }
                }
                fbm_save(fbm);                
                inode_f_save(inode_f);
                super_block_save(sb);
                rootdir_save(rootdir);
                free(sb);
                free(inode_f);
                return 0;
            }
        }
        //unable to find the file       
        return -1;
}
