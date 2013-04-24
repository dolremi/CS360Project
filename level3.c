#include "type.h"
#include "level3.h"
#include "level1.h"
#include <stdlib.h>

void do_mount()
{
  if(mount() < 0)
    printf("can't mount\n");
}

int mount()    /*  Usage: mount filesys mount_point OR mount */
{
  int i,fd;
  int dev, ino;
  MINODE *mip;
  char buf[BLOCK_SIZE];
  int magic,nblocks, bfree, ninodes, ifree;
  
    
  if(pathname[0] == 0 && parameter[0] == 0)
    {  
      for(i = 0; i < NMOUNT; i++)
	{
	  if(mountTable[i] && mountTable[i]->dev)
	    {
	      printf("%s mounted on %s\n",mountTable[i]->mount_name,mountTable[i]->name);
	    }
	}
      return 0;

    }

 
  //2. Check whether filesys is already mounted: 
  for(i=0;i < NMOUNT;i++)
    {
      if(mountTable[i] && mountTable[i]->dev && (strcmp(mountTable[i]->mount_name,pathname)==0))
	{
	  printf("%s is already mounted\n",pathname);
	  return -1;
	}
    }

  i = 0;
  while(mountTable[i]&& mountTable[i]->dev)
    i++;

  fd = open(pathname,O_RDWR);

  if(fd < 0)
    {
      printf("open %s failed.\n",pathname);
      return -1;
    }

  // verify it is an EXT2 FS
  get_block(fd,SUPERBLOCK,buf);
    
  sp = (SUPER *)buf;
  magic = sp->s_magic;
  nblocks = sp->s_blocks_count;
  bfree = sp->s_free_blocks_count;
  ninodes = sp->s_inodes_count;
  ifree = sp->s_free_inodes_count;


 if(magic != SUPER_MAGIC)
   {
     printf("SUPER_MAGIC = %d it is not an EX2 Filesystem.\n",magic);
     return -1;
   }

  get_block(fd,GDBLOCK,buf);
  gp = (GD *)buf;
  printf("iblock = %d\n",gp->bg_inode_table);
  printf("bmap=%d\n",gp->bg_block_bitmap);
  printf("imap=%d\n",gp->bg_inode_bitmap);
  
  if(parameter[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  ino = getino(&dev, parameter);
  
  if(!ino)
    return -1;
  
  mip = iget(dev, ino);

  if((mip->INODE.i_mode & 0040000) != 0040000 || mip->refCount > 1)
    return -1;

  mountTable[i] = (MOUNT *)malloc(sizeof(MOUNT));
  mountTable[i]->ninodes = ninodes;
  mountTable[i]->nblocks = nblocks;
  mountTable[i]->dev = fd;
  mountTable[i]->mounted_inode = mip;
  mip->mounted = 1;
  mip->mountptr = mountTable[i];
  strcpy(mountTable[i]->mount_name,pathname);
  strcpy(mountTable[i]->name, parameter);

  printf("mount : %s mounted on %s \n",pathname,parameter); 
  printf("nblocks=%d bfree=%d  ninodes=%d ifree=%d\n",nblocks, bfree,ninodes,ifree);

   return 0;

}
  
void do_umount()
{
  if(umount()<0)
    {
      printf("can not umount the filesys %s\n",pathname);
    }
}

int umount()
{
  int i;
  int dev;
  int mounted_checker=0;
  MOUNT *temp;
  int freeBlocks, freeInodes;
  char buf[BLOCK_SIZE];
   
  if(pathname[0] == 0)
     {
       printf("umount : input pathname : ");
       fgets(pathname, 256,stdin);
       pathname[strlen(pathname) -1] = 0;
     }

   //1. Search the MOUNT table to check filesys is indeed mounted.
   for(i=0;i<NMOUNT;i++)
     {
       if(mountTable[i] && mountTable[i]->dev && strcmp(mountTable[i]->mount_name,pathname)==0)
	 {
	   printf("umount : %s\n",mountTable[i]->mount_name);
	   dev=mountTable[i]->dev;
	   mounted_checker=1;
	   temp=mountTable[i];
	   break;
	 }
     }
   
   if(mounted_checker==0)
     {
       return -1;
     }

   //2. Check whether any file is still active in the mounted filesys;
   //      e.g. someone's CWD or opened files are still there,
   //   if so, the mounted filesys is BUSY ==> cannot be umounted yet.
   //   HOW to check?      ANS: by checking all minode[].dev
   
   for(i=0;i<NMINODES;i++)
     {
       if(minode[i].refCount!=0 && minode[i].dev==dev)
	 {
	   printf("the mounted filesysis BUSY, canoot be unmounted yet!!\n");
	   return -1;
	 }
     }

   //3. Find the mount_point's inode (which should be in memory while it's mounted 
   //   on).  Reset it to "not mounted"; then 
   //         iput()   the minode.  (because it was iget()ed during mounting)
   get_block(temp->dev,SUPERBLOCK,buf);
   sp = (SUPER *)buf;
   freeBlocks = sp->s_free_blocks_count;
   freeInodes = sp->s_free_inodes_count;
   printf("nblocks=%4d freeBlocks=%4d usedBlocks=%4d\n",temp->nblocks, freeBlocks, temp->nblocks-freeBlocks);
   printf("ninodes=%4d freeInodes=%4d usedInodes=%4d\n",temp->ninodes, freeInodes, temp->ninodes-freeInodes);
   temp->mounted_inode->mounted=0;
   iput(temp->mounted_inode);
   temp->dev = 0;
   printf("%s unmounted\n",pathname);

   return 0;

}  

