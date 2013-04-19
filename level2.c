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
  Oft *Oftp;
  long unsigned dev;


  if(pathname[0] == 0 || parameter[0] == 0)
    { 
      printf("open: enter pathname and mode = 0|1|2|3 for R|W|RW|APPEND\t");
      scanf("%s %d",pathname,&mode);
    }
  else
    sscanf(parameter, "%d",&mode);

  printf("pathname =%s mode =%d\n",pathname,mode);
  
  ino = getino(&dev, pathname);
  
  if(ino == 0)
    return -1;

  mip = iget(dev,ino); 
  
 
  if ( ( mip->INODE.i_mode & 0040000) == 0040000) /// DIR
    {
      printf(" %s is a DIR file",pathname);
      return -1;
    }

  //if ( (st_mode & (1 << 8)) ) // Owner can r
  //if ( (st_mode & (1 << 7)) ) // Owner can w
  //if ( (st_mode & (1 << 6)) ) // Owner can x
  oftp = malloc();       // get a FREE OFT
  oftp->mode = mode;     // open mode 
  oftp->refCount = 1;
  oftp->inodeptr = mip;  // point at the file's minode[]
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
  running->fd[i]->inodeptr->dirty=1;
  running->fd[i]->inodeptr->INODE.i_atime = time(0L);
  return i;

}

void truncate(MINODE *mip)
{
  int i=0;
  int m=0;
  int *k,*j,*t;
  char buf[BLOCK_SIZE],buf2[BLOCK_SIZE];
  //1. release mip->INODE's data blocks;a file may have 12 direct blocks, 256 indirect blocks and 256*256double indirect data blocks. release them all.
  //direct block
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
  //2. update INODE's time field
  mip->INODE.i_atime = time(0L);
  //3. set INODE's size to 0 and mark Minode[ ] dirty
  mip->INODE.i_size=0;
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

