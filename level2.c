#include "level2.h"
#include "level1.h"
#include "utility.h"
#include <stdlib.h>


void fileopen()
{
  int mode;
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

  if(open_file(mode) < 0)
    printf("can't open the file\n");
}

int open_file(int mode)
{
  int filemode;
  MINODE *mip;
  OFT *oftp;
  int dev,i;
  unsigned long ino;

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
 
  if(fd < 0 || fd  >= NFD)
    {
      printf("invalid fd\n");
      return -1;
    }
 
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
	  printf("   %d   ",running->fd[i]->offset);
	  printf("     %d  ",running->fd[i]->refCount);
	  printf("   [%d,%d]\n",running->fd[i]->inodeptr->dev, running->fd[i]->inodeptr->ino);   
	  i++;
	}
      printf("=======================================\n");
    }
  else
    {
      printf("no opened files\n");
    }
}

void do_read()
{

  if(read_file() < 0)
    printf("can't read file\n");

}

int read_file()
{
  int fd, nbytes;
  char buf[BLOCK_SIZE];

  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("read : input fd  nbytes to read: ");
      fgets(pathname,256, stdin);
      pathname[strlen(pathname) -1 ] = 0;
      sscanf(pathname, "%d %d",&fd, &nbytes);
    }
  else
    {
      sscanf(pathname,"%d",&fd);
      sscanf(parameter,"%d",&nbytes);
    }

  if(fd < 0 || fd  >= NFD)
    {
      printf("invalid fd\n");
      return -1;
    }
 
  if(running->fd[fd] == 0)
    {
      printf("fd doesn't exist\n");
      return -1;
    }

  if( running->fd[fd]->mode ==1 || running->fd[fd]->mode == 3)
    {
      printf("bad mode\n");
      return -1;
    }
  return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes)
{ 
 int block[15];
 int size,lbk, startByte,blk,i,m,secLbk, secSb,remain,readIn,count;
 int  *j, *k, *t;
 char buffer[BLOCK_SIZE], buffer2[BLOCK_SIZE], readbuf[BLOCK_SIZE];
 char *cp, *cq;
 OFT *oftp = running->fd[fd];
 MINODE *mip = oftp->inodeptr;
 
 count = 0;
 cq = buf;

 for(i = 0 ; i <15; i++)
   block[i] = mip ->INODE.i_block[i];

 size = oftp->inodeptr->INODE.i_size - oftp->offset;

 while( nbytes > 0 && size > 0)
   {
     lbk = oftp->offset / BLOCK_SIZE;
     startByte = oftp->offset % BLOCK_SIZE;

     if(lbk < 12){
       blk = mip->INODE.i_block[lbk];
     }
     else if(lbk >=12 && lbk < 256 + 12){
       get_block(mip->dev,block[12],buffer); 
       k = (int *) buffer;
       i = 0;
       while(i < lbk - 12)
	 {
	   k++;
	   i++;
	 }
       blk = *k;
     }
     else{
       get_block(mip->dev, block[13],buffer);
       secLbk = (lbk - 267) / 256;
       secSb = (lbk - 267) % 256;
       t = (int *)buffer;
       i = 0;
       while(i < secLbk)
	 {
	   t++;
	   i++;
	 }

       get_block(mip->dev, *t, buffer2);
       j = (int *)buffer2;
       i = 0;
       while(i < secSb)
	 {
	   j++;
	   i++;
	 }
       blk = *j;
     }

     get_block(mip->dev,blk, readbuf);
     cp = readbuf + startByte;
     remain = BLOCK_SIZE - startByte;
     readIn = min(remain, size, nbytes);
     strncpy(cq, cp, readIn);
     cq += readIn;
     oftp->offset += readIn;
     count += readIn;
     size -= readIn;
     nbytes -= readIn;

   }
 printf("myread : read %d char from file %d\n",count, fd);

 return count;

}


int min(int a, int b, int c)
{
  int minValue;
  if(a < b)
    {
      minValue = a;
      if(c < a)
	minValue = c;
    }
  else
    {
      minValue = b;
      if(c < b)
	minValue = c;
    }

  return minValue;

}

void do_cat()
{
  if(catFile() < 0)
    printf("can't cat file\n");
}

int catFile()
{
  int fd, n;
  char mybuf[BLOCK_SIZE];

  if(pathname[0] == 0)
    {
      printf("cat : Input filename : ");
      fgets(pathname, 256, stdin);
      pathname[strlen(pathname) - 1 ] = 0;
    }

  if((fd = open_file(0)) < 0)
    {
      printf("file open error\n");
      return -1;
    }

  while( n = myread(fd,mybuf, BLOCK_SIZE))
    {
      mybuf[n] = 0;
      printf("%s",mybuf);
    }

  close_file(fd);

  return 0;
} 

void do_write()
{
  if(write_file() < 0)
    printf("write file failed\n");

}

int write_file()
{
  int fd,nbytes;
  char line[BLOCK_SIZE];
  char buf[256];

  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("write : input  fd  text : ");
      fgets(line, BLOCK_SIZE, stdin);
      pathname[strlen(line) - 1] = 0;
      sscanf(line, "%d %s", &fd, buf);
    }
  else
    {
      sscanf(pathname, "%d", &fd);
      strcpy(buf, pathname);
    }

  if(fd < 0 || fd  >= NFD)
    {
      printf("invalid fd\n");
      return -1;
    }
 
  if(running->fd[fd] == 0)
    {
      printf("fd doesn't exist\n");
      return -1;
    }

  if(running->fd[fd]->mode == 0)
    {
      printf("bad mode\n");
      return -1;
    }

  nbytes = strlen(buf);
  return mywrite(fd, buf, nbytes); 


}

int mywrite(int fd, char *buf, int nbytes)
{
  int block[15];
  int size, lbk, startByte, blk, i, m, secLbk, secSb, remain, writeIn, count;
  int *j, *k, *t;
  char buffer[BLOCK_SIZE], buffer2[BLOCK_SIZE],wbuf[BLOCK_SIZE];
  char *cq, *cp;
  OFT *oftp = running->fd[fd];
  MINODE *mip = oftp->inodeptr;

  count = 0;
  cq = buf;

 for(i = 0 ; i <15; i++)
   block[i] = mip ->INODE.i_block[i];

  while(nbytes)
    {
      lbk = oftp->offset / BLOCK_SIZE;
      startByte = oftp->offset % BLOCK_SIZE;

      if(lbk < 12){
	if(mip->INODE.i_block[lbk] == 0)
	  mip->INODE.i_block[lbk] = balloc(mip->dev);
	blk = mip->INODE.i_block[lbk];
      }
      else if (lbk >= 12 && lbk < 256 + 12){
	get_block(mip->dev, block[12], buffer);
	k = (int *)buffer;
	i = 0;
	while(i < lbk - 12)
	  {
	    k++;
	    i++;
	  }

	if(*k == 0)
	  *k = balloc(mip->dev);

	blk = *k;
      }
      else{
	get_block(mip->dev, block[13],buffer);
       secLbk = (lbk - 267) / 256;
       secSb = (lbk - 267) % 256;
       t = (int *)buffer;
       i = 0;
       while(i < secLbk)
	 {
	   t++;
	   i++;
	 }

       get_block(mip->dev, *t, buffer2);
       j = (int *)buffer2;
       i = 0;
       while(i < secSb)
	 {
	   j++;
	   i++;
	 }

       if(*j == 0)
	 *j = balloc(mip->dev);

       blk = *j;
      }

      get_block(mip->dev, blk, wbuf);
      cp = wbuf + startByte;
      remain = BLOCK_SIZE - startByte;


      // check the min 

      put_block(mip->dev, blk, wbuf);
    }

  mip->dirty = 1;
  printf("wrote %d char into file fd = %d\n",nbytes,fd);
  return nbytes;
}
