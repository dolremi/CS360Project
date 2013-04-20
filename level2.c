#include "level2.h"
#include "level1.h"
#include "utility.h"
#include <stdlib.h>


void fileopen()
{
  if(open_file() < 0)
    printf("can't open the file\n");
}

int open_file()
{
  int mode,filemode;
  MINODE *mip;
  OFT *oftp;
  int dev,i;
  unsigned long ino;
  char line[256];

  if(pathname[0] == 0 || parameter[0] == 0)
    { 
      printf("open: enter pathname and mode = 0|1|2|3 for R|W|RW|APPEND : ");
      fgets(line,256,stdin);
      line[strlen(line)-1] = 0;
      sscanf(line, "%s %d",pathname,&mode);
    }
  else
    sscanf(parameter, "%d",&mode);

   if(pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
  
  ino = getino(&dev, pathname);
  
  if(ino == 0)
    return -1;

  mip = iget(dev,ino); 
  
  filemode = mip->INODE.i_mode;
  if ( ( filemode & 0040000) == 0040000) /// DIR
    {
      printf("%s is a DIR file\n",pathname);
      iput(mip);
      return -1;
    }
  
  if( mode >0 && mode < 4 &&!(filemode &(1 << 7)))
    {
      printf("%s can't open for WRITE\n",pathname);
      iput(mip);
      return -1;
    } 

  if((mode == 0 || mode ==2) &&! (filemode & ( 1 << 8)))
    {
      printf("%s can't open for READ\n",pathname);
      iput(mip);
      return -1;
    }
 
  i = 0;
  while(running->fd[i])
    {
      if(running->fd[i]->inodeptr->ino == ino && mode)
	{
	  printf("file %s is already opened with incompatible mode\n",pathname);
	  iput(mip);
	  return -1;
	}
      i++; 
    }

  oftp =(OFT *) malloc(sizeof(OFT));       // get a FREE OFT
  oftp->mode = mode;                       // open mode 
  oftp->refCount = 1;
  oftp->inodeptr = mip;                    // point at the file's minode[]
 
  switch(mode){
  case 0 : oftp->offset = 0; 
    break;
  case 1 : truncate(mip);        // W : truncate file to 0 size
    oftp->offset = 0;
    break;
  case 2 : oftp->offset = 0;    // RW does NOT truncate file
    break;
  case 3 : oftp->offset =  mip->INODE.i_size;  // APPEND mode
    break;
  default: printf("invalid mode\n");
    return(-1);
  }

  i = 0;
  while(running->fd[i])
  {
    i++;
  }
  printf("fd = %d dev = %d ino = %d\n", i, dev, ino);
  running->fd[i]=oftp;
  mip->INODE.i_atime = time(0L);

  if(mode)
    {
    mip->dirty = 1;
    mip->INODE.i_mtime = time(0L);
    }

  return i;

}

void truncate(MINODE *mip)
{
  int i, m;
  int block[15];
  int *k,*j,*t;
  char buf[BLOCK_SIZE],buf2[BLOCK_SIZE];

  for(i = 0; i <= 14; i++)
    block[i] = mip->INODE.i_block[i];

  // directed block
  for(i = 0; i <=11; i++)
    {
      if(block[i])
	bdealloc(mip->dev,block[i]);
    }

  // indirect block
  if(block[12])
    {
      get_block(mip->dev,block[12],buf);
      k = (int *)buf;
      for(i = 0; i < 256; i++)
	{
	  if(*k)
	    bdealloc(mip->dev,*k);
	  k++;
	}
    }

  // double indirect
  if(block[13])
    {
      get_block(mip->dev,block[13],buf);
      t = (int *)buf;
      for(i = 0; i < 256 ; i++)
	{
	  if(*t)
	    {
	      get_block(mip->dev, *t,buf2);
	      j = (int *)buf2;
	      for(m = 0; m < 256; m++)
		{
		  if(*j)
		    bdealloc(mip->dev,*j);
		  j++;
		}
	    }
	  t++;
	}
    }
 
  mip->INODE.i_atime = mip->INODE.i_mtime= time(0L);
  mip->INODE.i_size=0;
  mip->dirty = 1;
}

void closeFile()
{

  int fd;
  MINODE *mip;

  if(pathname[0] == 0)
    {
      printf("close : input fd number : ");
      fgets(pathname, 256,stdin);
      pathname[strlen(pathname) - 1] = 0;
    }

  sscanf(pathname, "%d",&fd);

  if(close_file(fd) < 0)
    printf("close: failed\n");

}

int close_file(int fd)
{

  OFT *oftp;
  MINODE *mip;
  //1. verify fd is within range.
  if(fd < 0 || fd  >= NFD)
    {
      printf("invalid fd\n");
      return -1;
    }
 
  //2. verify running->fd[fd] is pointing at a OFT entry
  if(running->fd[fd] == 0)
    {
      printf("fd doesn't exist\n");
      return -1;
    }
  
  oftp = running->fd[fd];
  running->fd[fd] = 0;
  oftp->refCount--;
  
  if (oftp->refCount > 0)
    return 0;

  mip = oftp->inodeptr;
  iput(mip);
  oftp->refCount = 0;
  printf("refCount = %d, it is FREE\n",oftp->refCount);
  free(oftp);  
  oftp = 0;
  return 0; 
}

void do_seek()
{
  int fd, position;
  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("lseek : input fd position :");
      fgets(pathname, 256,stdin);
      pathname[strlen(pathname)-1] = 0;
      sscanf(pathname,"%d %d",&fd, &position);
    }
  else
    {
      sscanf(pathname, "%d", &fd);
      sscanf(parameter,"%d",&position);
    }
 
  if(llseek(fd,position) < 0)
    printf("lseek failed\n");
}


long llseek(int fid, long position)
{
  
  long OriginPosition;

  if(fid < 0 || fid  >= NFD)
    {
      printf("invalid fd\n");
      return -1;
    }
 
  if(running->fd[fid] == 0)
    {
      printf("fd doesn't exist\n");
      return -1;
    }

  OriginPosition = running->fd[fid]->offset;

  // make sure not over run either end of the file
  if(running->fd[fid]->inodeptr->INODE.i_size < position)
    running->fd[fid]->offset = running->fd[fid]->inodeptr->INODE.i_size;
  else if (position < 0)
    running->fd[fid]->offset = 0;
  else
    running->fd[fid]->offset = position;

  return OriginPosition;
  
}


void pfd()
{

  int i = 0;
  if(running->fd[0])
    {
      printf("============== pid = %d ==============\n",running->pid);
      printf("fd  mode     offset   count  [dev,ino]\n");
      printf("--  ----     ------   -----  ---------\n");

      while(running->fd[i])
	{	
     	  printf("%d   ",i);
	  switch(running->fd[i]->mode){
	  case 0:
	    printf("READ    ");
	    break;
	  case 1:
	    printf("WRITE   ");
	    break;
	  case 2:
	    printf("RW      ");
	    break;
	  case 3:
	    printf("APPEND  ");
	    break;
	  }
	  printf("     %d     ",running->fd[i]->offset);
	  printf("     %d     ",running->fd[i]->refCount);
	  printf("[%d,%d]\n",running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);   
	  i++;
	}
      printf("=======================================\n");
    }
  else
    {
      printf("no opened files\n");
    }
}

