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
        if(i <= 15) { 
            fbm->fbm[i] = 0;
        } else {
            fbm->fbm[i] = 1;
        }

    }

}

void SuperBlock_init(SuperBlock_t *sb) {
    
    sb->magic_number = 0xACBD0005;
    sb->block_size = BLOCK_SIZE;
    sb->fs_size = FS_SIZE; 
    sb->num_inodes = NB_FILES-1;
    inode_t root;
    root.size = 13;
    for(int i = 0; i < NB_BLOCKS-1; i++) {
        if(i >= 2 && i < 15) { 
            root.direct[i] = i;
        } else {
            root.direct[i] = -1; 
        }
    }
    sb->root = root; 
}

void mkssfs(int fresh) {
    
    if(fresh) {

        init_fresh_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
        rootdir = *(struct RootDirectory *)malloc(sizeof(rootdir));

        SuperBlock_init(&sb);        
        FBM_init(&fbm);
        
        write_blocks(0,1, &sb); 
        write_blocks(1,1, &fbm);

    } else {
        init_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
    }
    inode_t inode = *(inode_t *)malloc(sizeof(inode_t));
    printf("%lu\n", sizeof(inode_t));
    for(int i = 15; i < NB_FILES; i++) {
        rootdir.name[i] = "nRx4jdF2YQ";
        rootdir.inode_table[i] = inode;
    }
    /*
     * 1) current inode size is 4100 - 1024 blocks * 4 (size of int) + 4 bytes (size) = 4100
     * 2) what calculation are they doing to get 64 bytes per inode?
     *
     * */

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

