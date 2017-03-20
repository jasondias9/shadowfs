/*!
 * @name        mkssfs
 * @param       fresh      If 1, create the file system from scratch. 
 *                         Otherwise the file system is opened from disk
 * Creates a new file system
 *
*/
void mkssfs(int fresh);

/*!
 * @name        ssfs_fopen
 * @param       name        Name of the file system
 *
 * Opens the file system
*/
int ssfs_fopen(char *name);

/*!
 * @name        ssfs_fclose
 * @param       fileID       ID of the file system
 *
 * Closes the given file system
*/
int ssfs_fclose(int fileID);

/*!
 * @name         ssfs_frseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 *
 * Seek (read) to the location from the beginning
*/
int ssfs_frseek(int fileID, int loc);

/*!
 * @name         ssfs_fwseek
 * @param        fileID       ID of the file system
 * @param        loc          location to seek until
 * 
 * Seek (write) to the location from the beginning
*/
int ssfs_fwseek(int fileID, int loc);

/*!
 * @name          ssfs_fwrite
 * @param         fileID       ID of the file system
 * @param         buf          pointer to character stream to be written to disk
 * @param         length       length of the character stream
 *
 * Write buffer characters onto the disk
*/
int ssfs_fwrite(int fileID, char *buf, int length);


/*!
 * @name          ssfs_fread
 * @param         fileID       ID of the file system
 * @param         buf          pointer to character stream to be read into from disk
 * @param         length       length of the data to be read from the disk
 *
 * Read characters from disk into buffer
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

