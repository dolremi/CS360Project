#ifndef UTILITY_H
#define UTILITY_H
#include "type.h"

void get_block(int fd,int block, char *buf);

void put_block(int fd, int block, char *buf);

// break up the pathname applied to absolute path only!
int token_path(char *pathname);

// break the pathname into dirname
char *dirname(char *pathname);

// break the pathname into basename
char *basename(char *pathname);

// return the inumber of the pathname currently only for level 1 use
unsigned long  getino(int *dev, char *pathname);

// return the inumber given INODE for level 1 use
unsigned long  search(MINODE *mip, char *filename);

// load the inode into MINODE 
MINODE *iget(int dev, unsigned long ino);

// put the MINODE back to disk
void iput(MINODE *mip);

// find the name string given parent DIR and ino, return -1 if not found
int findmyname(MINODE *parent, unsigned long myino, char *myname);

// extact inumbers of . and .. for a DIR Minode
int findino(MINODE *mip, unsigned long *myino, unsigned long *parentino);

// allocate the inode in the disk
unsigned long ialloc(int dev);
/*
// deallocate the inode and write back to the disk
unsigned long idealloc(int dev, unsigned long ino)
*/
// allocate the block in the disk
unsigned long balloc(int dev);

// decrease free inode count in SUPER and GD on dev
void decFreeInodes(int dev);

// decrease free block count in SUPER and GD on dev
void decFreeBlocks(int dev);

// increase free block count in SUPER and GD on dev
void incFreeInodes(int dev);

// increase free block count in SUPER and GD on dev
void incFreeBlocks(int dev);

// deacllocate the block on the device 
unsigned long bdealloc(int dev, unsigned long iblock)

// test BITs
int tst_bit(char *buf, int BIT);

// set BITs
int set_bit(char *buf, int BIT);

// clear BITs
int clear_bit(char *buf, int BIT);


#endif
