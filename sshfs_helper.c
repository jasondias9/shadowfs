#include "disk_emu.h"
#include "sshfs.h"

int inode_id = 0;

void INodeTable_init(INodeTable *inat) {
    
    for(inode_id = 2; inode_id < NB_FILES - 1; inode_id++) { 
        //inat->table[inode_id]; //do some initialization
    }
    inat->size = 0;
}

