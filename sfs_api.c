#include "disk_emu.h"
#include "sshfs_helper.c" //update to header
#include "sfs_api.h"
#include <stdio.h>

INodeTable inat;
SuperBlock sb;


/*
 * 1) j-node is an inode that points to every inode?
 * 2) is an inode just a 1-1 mapping to a file? 
 * 3) free bitmap just maps free data blocks?
 * 4) same for write mask
 * 5) shadow nodes are just root nodes from prior saved states?
 * but these are just pointers? where does the data get saved?
 * 6) root dir?
 *
 * */

void mkssfs(int fresh) {
    
    if(fresh) {
        INodeTable_init(&inat);   
        SuperBlock_init(&sb);        

        init_fresh_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);

        //write the inode allaction table

        write_blocks(0, 1, (void *) &inat);
        //write_blocks(offset,1, (void *) &sb);
        
    } else {
        init_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
    }
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

