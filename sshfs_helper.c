#include "disk_emu.h"
#include "sfs_api.h"

int inode_id = 0;

void INodeTable_init(INodeTable *inat) {
    
    for(inode_id = 2; inode_id < NB_FILES - 1; inode_id++) { 
        //inat->table[inode_id]; //do some initialization
    }
    inat->size = 0;
}

void SuperBlock_init(SuperBlock *sb) {
    
    sb->magic_number = 0xACBD0005;
    sb->block_size = BLOCK_SIZE;
    sb->fs_size = DISK_SIZE; 
    sb->num_inodes = NB_FILES;

    i_node j_node;

}

