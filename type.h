/*	type.h for CS360 Project             */
#ifndef TYPE_H
#define TYPE_H

#include <stdio.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/ext2_fs.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>

// define shorter TYPES, save typing efforts
typedef struct ext2_group_desc  GD;
typedef struct ext2_super_block SUPER;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;    // need this for new version of e2fs

GD    *gp;
SUPER *sp;
INODE *ip;
DIR   *dp; 

#ifndef BLOCK_SIZE
#define BLOCK_SIZE        1024
#endif

#define BITS_PER_BLOCK    (8*BLOCK_SIZE)
#define INODES_PER_BLOCK  (BLOCK_SIZE/sizeof(INODE))

// Block number of EXT2 FS on FD
#define SUPERBLOCK        1
#define GDBLOCK           2
#define BBITMAP           3
#define IBITMAP           4
#define INODEBLOCK        5
#define ROOT_INODE        2

// Default dir and regulsr file modes
#define DIR_MODE          0040777 
#define FILE_MODE         0100644
#define SUPER_MAGIC       0xEF53
#define SUPER_USER        0

// Proc status
#define FREE              0
#define BUSY              1
#define KILLED            2

// Table sizes
#define NMINODES          100
#define NMOUNT            10
#define NPROC             10
#define NFD               10
#define NOFT              50

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  struct Minode *inodeptr;
  long  offset;
} OFT;

// PROC structure
typedef struct Proc{
  int   uid;
  int   pid;
  int   gid;
  int   ppid;
  struct Proc *parent;
  int   status;

  struct Minode *cwd;
  OFT   *fd[NFD];
} PROC;
      
// In-memory inodes structure
typedef struct Minode{		
  INODE    INODE;               // disk inode
  ushort   dev;
  unsigned long ino;
  ushort   refCount;
  ushort   dirty;
  ushort   mounted;
  struct Mount *mountptr;
  char     name[128];           // name string of file
} MINODE;

// Mount Table structure
typedef struct Mount{
        int    ninodes;
        int    nblocks;
        int    dev, busy;   
        struct Minode *mounted_inode;
        char   name[256]; 
        char   mount_name[64];
} MOUNT;

PROC proc[2];
MINODE minode[NMINODES];
int dev;
MINODE *root;
PROC *running;
char pathname[256];
char parameter[256];

// components of pathname after calling token_path function
char *name[256];

#endif 
