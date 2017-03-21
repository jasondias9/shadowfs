#include "disk_emu.h"
#include <stdio.h>

void mkssfs(int fresh) {
    
    if(fresh) {
        init_fresh_disk("test_disk", 1024, 20);
    } else {
        init_disk("test_disk", 1024, 20);
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


