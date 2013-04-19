#include "type.h"
#include "level2.h"
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
  OFT *Oftp;
  long unsigned dev;

  if(pathname[0] == 0 || parameter[0] == 0)
    { 
      printf("open: enter pathname and mode = 0|1|2|3 for R|W|RW|APPEND\t");
      scanf("%s %d",pathname,&mode);
    }
  else
    sscanf(parameter, "%d",&mode);

  printf("pathname =%s mode =%d\n",pathname,mode);

  if(pathname[0] = '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
  
  ino = getino(&dev, pathname);
  
  if(ino == 0)
    return -1;

  mip = iget(dev,ino); 
  
  filemode = mip->INODE.i_mode;
  if ( ( mip->INODE.i_mode & 0040000) == 0040000) /// DIR
    {
      printf("%s is a DIR file\n",pathname);
      iput(mip);
      return -1;
    }
  
  if( mode >0 && mode < 4 && !(filemode &(1 << 7)))
    {
      printf("%s can't open for READ\n",pathname);
      iput(mip);
      return -1;
    }

  if((mode == 0 || mode ==2) &&! (filemode & ( 1 << 8)))
    {
      printf("%s can't open for WRITE\n",pathname);
      iput(mip);
      return -1;
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
  while(running->fd[i]!=NULL)
  {
	i++;
  }
  running->fd[i]=oftp;
  mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);

  if(mode)
    mip->dirty = 1;
 
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
  if(close_file(fd) < 0)
    printf("close: failed\n");

}

int close_file(int fd)
{
  //1. verify fd is within range.

  //2. verify running->fd[fd] is pointing at a OFT entry

  //3. The following code segments should be fairly obvious:
  //   oftp = running->fd[fd];
  //   running->fd[fd] = 0;
  //   oftp->refCount--;
  //  if (oftp->refCount > 0) return 0;

     // last user of this OFT entry ==> dispose of the Minode[]
  //   mip = oftp->inodeptr;
  //   iput(mip);

  //   fdalloc(oftp);   (optional, refCount==0 says it's FREE)
     return 0; 
}

long lseek(int fd, long position)
{
  //From fd, find the OFT entry. 

  //change OFT entry's offset to position but make sure NOT to over run
  //either end of the file.

  //return originalPosition
}
int pfd()
{
  //This function displays the currently opened files as follows:
  //     filename  fd  mode  offset
  //     --------  --  ----  ------ 
  //     /a/b/c     1  READ   1234       
  //to help the user know what files has been opened.
  printf("\t filename\tfd\tmode\toffset\n");
  printf("\t --------\t--\t----\t------\n");
  for(i=0;i<11;i++)
  {
 	printf("\t %s\t%d\t",running->fd[i]->inodeptr->name,i);
	switch(running->fd[i]->mode){
		case 0:
		       printf("R\t");
		       break;
		case 1:
		       printf("W\t");
		       break;
		case 2:
		       printf("RW\t");
		       break;
		case 3:
                       printf("APPEND\t");
		       break;
	}
	printf("%d\n",running->fd[i]->offset);
  } 
}

