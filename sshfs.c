#include "disk_emu.h"
#include "sshfs_helper.c" //update to header
#include "sshfs.h"
#include <stdio.h>

INodeTable inat;


void mkssfs(int fresh) {
    
    if(fresh) {
        INodeTable_init(&inat);      
        init_fresh_disk("test_disk", BLOCK_SIZE, NB_BLOCKS);
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

int ssfs_commit() {
    return 0;
}

int ssfs_restore(int cnum) {
    return 0;
}

int main() {
    mkssfs(1);
    return 0;
}


