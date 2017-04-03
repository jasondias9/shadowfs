#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//store these in memory to ensure fast access
SuperBlock_t sb;
FBM_t fbm;
root_directory_t root;
inode_f_t inode_f;

void FBM_init(FBM_t *fbm) {
    for(int i = 0; i < NB_BLOCKS; i++) {
        if(i <= 18) { 
            fbm->fbm[i] = 0;
        } else {
            fbm->fbm[i] = 1;
        }

    }

}

void SuperBlock_init(SuperBlock_t *sb) {
    
    sb->magic_number = 0xACBD0005;
    sb->block_size = BLOCK_SIZE;
    sb->fs_size = 0; //is this supposed to be current FS size ? or max?  
    sb->num_inodes = 0;

    inode_t root = *(inode_t *)malloc(sizeof(inode_t));;
    root.size = 14;
    root.indirect = -1;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        root.direct[i] = i + JNODE_OFFSET;
    }
    sb->root = root; 
}

void inode_init(inode_t *inode) {
    inode->size = 0;
    inode->indirect = -1;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        //file points to no blocks for now.  
        inode->direct[i] = -1;
    } 
}

void prepare_blks(inode_f_t *inode_f_ptr, block_t *blks) { 
    for(int i = 0; i < MAX_DIRECTS; i++) {
        blks[i] =  *(block_t *)malloc(BLOCK_SIZE);
        memcpy(&blks[i], &(inode_f_ptr->inode_table[i*INODE_PER_BLOCK]), BLOCK_SIZE);
    }
}

void root_init(inode_t *root) {
    for(int i = 0; i < MAX_DIRECTS; i++) {
        if(i < 3) {
            root->direct[i] = i + ROOT_DIR_OFFSET; 
        } else {
            root->direct[i] = -1;
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

void mkssfs(int fresh) {
    
    if(fresh) {
        
        if(init_fresh_disk("test_disk", BLOCK_SIZE, NB_BLOCKS) < 0) {
            perror("Faild to create a new file system");
        }

        sb = *(SuperBlock_t *)malloc(sizeof(SuperBlock_t));
        fbm = *(FBM_t *)malloc(sizeof(FBM_t));
        root = *(root_directory_t *)malloc(sizeof(root_directory_t));
        inode_t inode = *(inode_t *)malloc(sizeof(inode_t));
        
        SuperBlock_init(&sb);        
        FBM_init(&fbm);
        
        //initialize empty inodes for the inode_file
        inode_init(&inode);
        inode_f = *(inode_f_t *)malloc(sizeof(inode_f_t));
        //allocate the initilized inodes into a table
        inode_alloc(&inode_f, inode);
       

        block_t *blks = (block_t *)malloc(BLOCK_SIZE * MAX_DIRECTS);
        //segment inode file into blocks
        prepare_blks(&inode_f, blks);
       
        

        write_blocks(0,1, &sb); 
        write_blocks(1,1, &fbm);
        for(int i = 5; i <= 18; i++) {
            //write the inode_file
            write_blocks(i, 1, &blks[i]);
        }

        
    } else {
        init_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
    }

    
    /* State of the file system at this point: 
     * fs = {0: sb, 1: fbm, [2-4]: root directory, [5, 18]: inode_file, [19, 1023] : free} 
     */
}

int main() {
    mkssfs(1);
}

int ssfs_fopen(char *name) {
    return 0;
}

int ssfs_fclose(int fileID) {
    return 0;
}

int ssfs_frseek(int fileID, int loc) {
    return 0;
}

int ssfs_fwseek(int fileID, int loc) {
    return 0;
}

int ssfs_fwrite(int fileID, char *buf, int length) {
    //block_t blk0 =*(struct block *)malloc(1024);
    //memcpy(&blk0, &sb, 1024);

    return 0;
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

