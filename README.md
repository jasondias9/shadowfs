# SSFS (Simple Shadowing File System)

A simple shadow file system designed for the UNIX environment. 

 ## Limitations
* Limited filename lengths 
* No multi - user access / file protection support
* No subdirectories 
* Shadowing capability has not been completed yet.

## Implementation 
As part of the assignment I was provided with a constant-cost disk (CCDisk) emulator - implemented  as an array of blocks and saved as a file on the host machine's filesystem. Any block can be accessed randomly. We create a disk by specifiying the block size and the number of blocks giving us the totla size of the disk. 

## On disk data structures

__super block__ - the first block in the file system, defining the file system geometry. The magic number identifies the type of file system format. Other infomation held include the file system size, total number of i-nodes, and block addresses of the root directory. The root itself is a j-node.  

| Data  | Description |
| ------------- | ------------- |
| Magic number  | identifies the type of file system  |
| Block size | In bytes | 
| File system size | In blocks |
| Number of i-nodes | total number of i-nodes |
| Root directory | Addressing of the root directory | 

In the future I will add shadow root directories which will specify the block addressing for further saved states. (The shadowing feature) 

__j-node__
The j-node points to several data blocks that constitute the root directory file which itself holds the several i-nodes each addressing the blocks for a particular file. Unlike the UNIX file system where i-nodes are written directory on disk after the super block, SSFS writes the i-nodes to file on the file system addressed by the j-node. 

__Free Bit Map__
The FBM is used to determine the locations of the data bloks that are available for allocation. bit value 1 indicates the data block is unused, 0 indicates used. 

## C API

```C
void mkssfs(int fresh); // creates the file system
int ssfs_fopen(char *name); // opens the given file
int ssfs_fclose(int fileID); // closes the given file
int ssfs_frseek(int fileID, loc); // seek (Read) to the location from beginning
int ssfs_fwseek(int fileID, loc); // seek (Write) to the location from beginning
int ssfs_fwrite(int fileID, char *buf, int length); // write buf characters into disk
int ssfs_fread(int fileID, char *buf, int length); // read characters from disk into buf
int ssfs_remove(char *file); // removes a file from the filesystem
```
In development
```C
int ssfs_commit(); // create a shadow of the file system
int ssfs_restore(int cnum); // restore the file system to a previous shadow
```


