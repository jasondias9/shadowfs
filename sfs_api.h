#ifndef _SSHFS_
#define _SSHFS_

#define BLK_SIZE 1024
#define FS_SIZE 1048576
#define ROOT_DIR_SIZE 2189
#define INODE_SIZE 64
#define SB_SIZE 80

#define NB_BLKS 1024
#define NB_FILES 200
#define NB_SHADOWS 10

#define MAX_DIRECTS 14

#define MAX_NAME_LENGTH 10

#define JNODE_OFFSET 5
#define ROOT_DIR_OFFSET 2

#define INODE_PER_BLK 16
#define NAME_PER_BLK 93

#define ROOT_BLK_ALLOC 3
#define UNDEF -1

#define FULL 0
#define EMPTY 1

#define MAGIC_NUMBER 0xACBD0005


#include <stdint.h>


typedef struct OFD {
    int inode_num;
    int r_ptr;
    int w_ptr;
} OFD_t;

typedef struct inode {
    int size;
    //pointers to just the first 14 blocks. For larger files use the indirect
    int direct[MAX_DIRECTS];
    int indirect;
} inode_t;

typedef struct inode_f_t {
    inode_t inode_table[NB_FILES];
} inode_f_t;

typedef struct root_directory {
    char name[NB_FILES][MAX_NAME_LENGTH+1];
} root_directory_t;

typedef struct block {
    unsigned char data[BLK_SIZE];
} block_t;

typedef struct SuperBlock {
    int magic_number;
    int block_size;
    int fs_size;
    int num_inodes;
    inode_t root;
} SuperBlock_t;

typedef struct FBM {
   uint8_t fbm[NB_BLKS]; 
} FBM_t;

/*!
 * @name        mkssfs
 * @param       fresh      If 1, create the file system from scratch. 
 *                         Otherwise the file system is opened from disk
 * Formats the virtual disk implemented by the disk emulator and creates an instance
 * of SSFS 
 *
*/
void mkssfs(int fresh);

/*!
 * @name        ssfs_fopen
 * @param       name        Name of the file system
 * @return      integer corresponding to the index of the entry in the file
 *              descriptor table
 *
 * Opens the file in append mode (sets write file pointer to the end of the file, 
 * sets read file pointer to the beginning of the file. If the file does not exist, 
 * create a new file and sets size to 0. 
*/
int ssfs_fopen(char *name);

/*!
 * @name        ssfs_fclose
 * @param       fileID       ID of the file system
 * @return      On success return 0, negative number otherwise
 *
 * Close a file, i.e. removes the entry from the file descriptor table
*/
int ssfs_fclose(int fileID);

/*!
 * @name         ssfs_frseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 * @return       On success return 0, negative number otherwise.
 * 
 * Moves the read pointer to the location specified
*/
int ssfs_frseek(int fileID, int loc);

/*!
 * @name         ssfs_fwseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 * @return       On success return 0, negative number otherwise.
 * 
 * Moves the write pointer to the location specified.
*/
int ssfs_fwseek(int fileID, int loc);

/*!
 * @name          ssfs_fwrite
 * @param         fileID       ID of the file system
 * @param         buf          pointer to character stream to be written to disk
 * @param         length       length of the character stream
 * @return        On success the number of bytes written to the file system,
 *                else a negative number
 *
 * Writes the given number of bytes in buf into the open file, starting from the 
 * current write file pointer.
*/
int ssfs_fwrite(int fileID, char *buf, int length);


/*!
 * @name          ssfs_fread
 * @param         fileID       ID of the file system
 * @param         buf          pointer to character stream to be read into from disk
 * @param         length       length of the data to be read from the disk
 * @return        On success the number of bytes written to buf, else a negative number
 *
 * Reads the given number of bytes into buf from the open file, starting from the 
 * current read file pointer.
*/
int ssfs_fread(int fileID, char *buf, int length);

/*!
 * @name ssfs_remove
 * @param         file         pointer to the file to be removed
 * 
 * Removes the file from the file system
*/
int ssfs_remove(char *file);

/*!
 * @name ssfs_commit
 * 
 * Create a shadow of the file system
*/
int ssfs_commit();

/*!
 * @name ssfs_restore
 * @param       cnum            **NOT SURE** 
 *
 * Restore the file system to a previous shadow
*/
int ssfs_restore(int cnum);

#endif
