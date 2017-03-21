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
 *
 * Close a file, i.e. removes the entry from the file descriptor table
*/
int ssfs_fclose(int fileID);

/*!
 * @name         ssfs_frseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 * @return       On success return 1, negative number otherwise.
 * 
 * Moves the read pointer to the location specified
*/
int ssfs_frseek(int fileID, int loc);

/*!
 * @name         ssfs_fwseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 * @return       On success return 1, negative number otherwise.
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
int ssfs_fread(int fileID, chr *buf, int length);

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
