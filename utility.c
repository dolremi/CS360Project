#include "type.h"
#include "utility.h"
#include <stdlib.h>

void get_block(int fd,int block, char *buf)
{
  lseek(fd,(long)(BLOCK_SIZE*block),0);
  read(fd,buf,BLOCK_SIZE);
}


void put_block(int fd, int block, char *buf)
{
  lseek(fd, (long)(BLOCK_SIZE*block),0);
  write(fd, buf, BLOCK_SIZE);
}


int token_path(char *path)
{
  char *pch;
  int i = 0;
  
  pch = strtok(path, "/");
  while(pch != NULL)
    {
      name[i] = pch;
      pch = strtok(NULL, "/");
      i++;
    }
  return i;
}

char *dirname(char *pathname)
{
  
  int i = strlen(pathname) - 1;
  char *dir;
  while(pathname[i] != '/')
    {
      i--;
    }
  if(i == 0)
    {
      dir = (char *)malloc(2*sizeof(char));
      strcpy(dir, "/");
    }
  else{
  dir = (char *)malloc((i+1)*sizeof(char));
  strncpy(dir, pathname, i);
  dir[i] = 0;
  } 

  return dir;
}


char *basename(char *pathname)
{
  int i = strlen(pathname) - 1;
  char *base;
  while(pathname[i] != '/')
    i--;
  base = (char *)malloc((strlen(pathname)-i)*sizeof(char));
  strcpy(base,&pathname[i+1]);
  return base;
}


unsigned long  getino(int *device, char *pathname){
  int i,n;
  unsigned long inumber;
  char buf[BLOCK_SIZE],path[256];
  char firstC;
  MINODE *mip;
 

  // start from the INODE of root OR CWD level1 implementation 
  if(pathname[0] == '/')
    {
      *device = root->dev;
      inumber = root->ino;
    }
  else
    {
      *device = running->cwd->dev;
      inumber = running->cwd->ino;
    }
 
  // keep the pathname
  strcpy(path,pathname);
  n = token_path(path);
      
  for(i = 0; i < n ; i++)
    {
      mip = iget(*device, inumber);  
      inumber = search(mip, name[i]);

      // The file is not found
      if(inumber == 0)
	{
	  printf("%s does not exist.\n",name[i]);
	  iput(mip);
	  return 0;
	}
	  
      if((mip->INODE.i_mode & 0040000) != 0040000)
	{
	  printf(" %s is not a directory!\n",name[i]);
	  iput(mip);
	  return 0;
	}
      iput(mip);
    }

  return inumber;
}

// search function will be updated to cater the need of level 3
unsigned long  search(MINODE * mip, char *filename)
{
  int i;
  char buf[BLOCK_SIZE], namebuf[256];
  char *cp;
  
  for(i = 0; i <= 11 ; i ++)
    {
      if(mip->INODE.i_block[i])
	{
	 
	  get_block(mip->dev, mip->INODE.i_block[i], buf);
	  dp = (DIR *)buf;
	  cp = buf;
     
	  while(cp < &buf[BLOCK_SIZE])
	    {
	      strncpy(namebuf,dp->name,dp->name_len);
	      namebuf[dp->name_len] = 0;
	
	      if(!strcmp(namebuf, filename))
		return  dp->inode;
	      cp +=dp->rec_len;
	      dp = (DIR *)cp;
	    }
	}    
    }     
  return 0;
  
}

MINODE * iget(int dev, unsigned long ino){
  int i, nodeIndex, blockIndex;
  INODE *cp;
  char buf[BLOCK_SIZE];
  for(i = 0; i < NMINODES; i++)
    {
      if(minode[i].refCount){
	if(minode[i].dev == dev && minode[i].ino == ino)
	  {
	    minode[i].refCount++;
	    return &minode[i];
	  }
      }
    }

   for(i = 0; i < NMINODES; i++)
    {
      if(minode[i].refCount == 0)
	{
	  //here comes the Mailman's Algorithm
	  nodeIndex = (ino - 1) % INODES_PER_BLOCK;
	  blockIndex = (ino - 1) / INODES_PER_BLOCK + INODEBLOCK;
	  get_block(dev,blockIndex,buf);
	  cp = (INODE *)buf;
	  cp+= nodeIndex;
	  minode[i].INODE = *cp;
	  minode[i].dev = dev;
	  minode[i].ino = ino;
	  minode[i].refCount = 1;
	  minode[i].dirty = 0;
	  return &minode[i];
	}
    }
}

void iput(MINODE *mip){

  int nodeIndex,blockIndex;
  char buf[BLOCK_SIZE];

  mip->refCount--;
 
  // some other PROC still used it
  if(mip->refCount)
    return;
  
  // INODE hasn't been modified
  if(mip->dirty == 0)
    return;

  //Now the INODE has been modified and no other PROC used it
  
  //here comes the Mailman's Algorithm
  nodeIndex = (mip->ino -1 ) % INODES_PER_BLOCK;
  blockIndex = (mip->ino -1) / INODES_PER_BLOCK + INODEBLOCK;
  
  get_block(mip->dev,blockIndex, buf);
  ip = (INODE *)buf;
  ip += nodeIndex;
  *ip = mip->INODE;
  put_block(mip->dev,blockIndex,buf);
    
}

int findmyname(MINODE *parent, unsigned long myino, char *myname)
{
  int i;
  char buf[BLOCK_SIZE], namebuf[256];
  char *cp;
   
  for(i = 0; i <= 11 ; i ++)
    {
      if(parent->INODE.i_block[i] != 0)
	{
	
	  get_block(parent->dev, parent->INODE.i_block[i], buf);
	  dp = (DIR *)buf;
	  cp = buf;
     
	  while(cp < &buf[BLOCK_SIZE])
	    {
	      strncpy(namebuf,dp->name,dp->name_len);
	      namebuf[dp->name_len] = 0;
	
	      if(dp->inode == myino)
		{
		  strcpy(myname, namebuf);
		  return 0;
		}
	      cp +=dp->rec_len;
	      dp = (DIR *)cp;
	    }
	}    
    }     
  return -1;
  
}
  
int findino(MINODE *mip, unsigned long *myino, unsigned long *parentino)
{
  int i;
  char buf[BLOCK_SIZE], namebuf[256];
  char *cp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
     
  *myino = dp->inode;
  cp +=dp->rec_len;
  dp = (DIR *)cp;
  *parentino = dp->inode;	    	
  return 0;
}


unsigned long ialloc(int dev)
{
  int i;
  char buf[BLOCK_SIZE];
  int ninodes;
  SUPER *temp;

  // get total number of inodes
  get_block(dev,SUPERBLOCK,buf);
  temp = (SUPER *)buf;
  ninodes = temp->s_inodes_count;
  put_block(dev,SUPERBLOCK,buf);

  get_block(dev, IBITMAP,buf);

  for(i = 0; i < ninodes ; i++)
    {
      if(tst_bit(buf,i) == 0){
	set_bit(buf,i);
	put_block(dev,IBITMAP,buf);

	decFreeInodes(dev);
	return i+1;
      }
    }
  
  return 0;

}

unsigned long idealloc(int dev, unsigned long ino)
{
  int i ;
  char buf[BLOCK_SIZE];

  // get inode bitmap block
  get_block(dev, IBITMAP, buf);
  clear_bit(buf,ino-1);

  put_block(dev, IBITMAP,buf);

  incFreeInodes(dev);
}

unsigned long balloc(int dev)
{
  int i;
  char buf[BLOCK_SIZE];
  int nblocks;
  SUPER *temp;

  // get total number of blocks
  get_block(dev,SUPERBLOCK,buf);
  temp = (SUPER *)buf;
  nblocks = temp->s_blocks_count;
  put_block(dev,SUPERBLOCK,buf);

  get_block(dev, BBITMAP,buf);

  for(i = 0; i < nblocks ; i++)
    {
      if(tst_bit(buf,i) == 0){
	set_bit(buf,i);
	put_block(dev,BBITMAP,buf);

	decFreeBlocks(dev);
	return i+1;
      }
    }
  return 0;

}

unsigned long bdealloc(int dev, unsigned long iblock)
{
  int i ;
  char buf[BLOCK_SIZE];

  // get inode bitmap block
  get_block(dev, BBITMAP, buf);
  clear_bit(buf,iblock-1);

  put_block(dev, BBITMAP,buf);

  incFreeBlocks(dev);
}


void decFreeInodes(int dev)
{

  char buf[BLOCK_SIZE];
  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, SUPERBLOCK,buf);

  get_block(dev, GDBLOCK,buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, GDBLOCK,buf);

}

void incFreeInodes(int dev)
{

  char buf[BLOCK_SIZE];
  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, SUPERBLOCK,buf);

  get_block(dev, GDBLOCK,buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, GDBLOCK,buf);

}


void incFreeBlocks(int dev)
{

  char buf[BLOCK_SIZE];
  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, SUPERBLOCK,buf);

  get_block(dev, GDBLOCK,buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, GDBLOCK,buf);

}

void decFreeBlocks(int dev)
{

  char buf[BLOCK_SIZE];
  get_block(dev, SUPERBLOCK, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, SUPERBLOCK,buf);

  get_block(dev, GDBLOCK,buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, GDBLOCK,buf);

}


int tst_bit(char *buf, int BIT)
{
  int i, j;
  i = BIT / 8;
  j = BIT % 8;
  return buf[i] & ( 1 << j);
}

int set_bit(char *buf, int BIT)
{
  int i, j;
  i = BIT / 8;
  j = BIT % 8;
  buf[i] |= (1 << j);
  return 1;
}


int clear_bit(char *buf, int BIT)
{
  int i, j;
  i = BIT / 8;
  j = BIT % 8;
  buf[i] &= ~( 1 << j);
  return 1;
}



