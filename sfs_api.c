#include "disk_emu.h"
#include "sfs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SuperBlock_t sb;
RootDirectory_t rootdir;
FBM_t fbm;

void FBM_init(FBM_t *fbm) {
    for(int i = 0; i < NB_BLOCKS; i++) {
        if(i <= 16) { 
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
        root.direct[i] = i + PERM_ALLOCATED_BLOCK_OFFSET;
    }
    sb->root = root; 
}

void INodeFile_init(inode_t *inode) {
    inode->size = 0;
    inode->indirect = -1;
    for(int i = 0; i < MAX_DIRECTS; i++) {
        //file points to no blocks for now.  
        inode->direct[i] = -1;
    } 
}

void mkssfs(int fresh) {
    
    if(fresh) {
        
        if(init_fresh_disk("test_disk", BLOCK_SIZE, NB_BLOCKS) < 0) {
            perror("Faild to create a new file system");
        }

        sb = *(SuperBlock_t *)malloc(sizeof(SuperBlock_t));
        fbm = *(FBM_t *)malloc(sizeof(FBM_t));
        rootdir = *(RootDirectory_t *)malloc(sizeof(rootdir));
        inode_t inode = *(inode_t *)malloc(sizeof(inode_t));

        SuperBlock_init(&sb);        
        FBM_init(&fbm);
        INodeFile_init(&inode);

        for(int i = 0; i < 1; i++) {
            //requires null terminated string
            strcpy(rootdir.name[i], "\0");
            rootdir.inode_table[i] = inode; 
        }

        write_blocks(0,1, &sb); 
        write_blocks(1,1, &fbm); 
        write_blocks(2, 1, &rootdir);

    } else {
        init_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
    }

    
    /* State of the file system at this point: 
     * fs = {0: sb, 1: fbm, 2: rood directory, [3, 16]: inode_file, [4, 1023] : free} 
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

