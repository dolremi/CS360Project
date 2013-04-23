#include "type.h"
#include "level3.h"
#include "level1.h"


int mount()    /*  Usage: mount filesys mount_point OR mount */
{
   char *filesys,*mount_point;
   int i;
   //int dev;
   Mount *free_mount_table_entry;
   int ino;
   MINODE *mip;
   char buf[BLOCK_SIZE];
   int magic,nblocks, bfree, ninodes, ifree;
//1. Ask for filesys (a pathname) and mount_point (a pathname also).
  // If mount with no parameters: display current mounted filesystems.
   printf("enter a filesys and mount_point. If mount with no parameters: display current mounted filesystems\n");
   sscanf("%s %s",filesys,mount_point);
   if(*filesys==NULL)
   {
        printf("mout with no parameters\n");
	printf("current mounted filesystem ->%s\n",running->cwd->mountptr->mount_name);
	return 0;
   }
//2. Check whether filesys is already mounted: 
//   (you may store the name of mounted filesys in the MOUNT table entry). 
//   If already mounted, reject;
//   else: allocate a free MOUNT table entry (whose dev == 0 means FREE).
   for(i=0;i<10;i++)
	{
		if(strcmp(mn[i]->mount_name,filesys)==0)
			{
				printf("the filesys %s already mounted !!!!",filesys);
				return 0;
			}
	}
     free_mount_table_entry= malloc(sizeof*Mount);
     free_mount_table_entry->dev=0;
//3. open filesys for RW; use its fd number as the NEW dev;
//   Check whether it's an EXT2 filesys: if not, reject.
    dev = open(&fliesys,O_RDWR);
   
    get_block(dev,SUPERBLOCK,buf);
    sp = (SUPER *)buf;
    magic = sp->s_magic;
    nblocks = sp->s_blocks_count;
    bfree = sp->s_free_blocks_count;
    ninodes = sp->s_inodes_count;
    ifree = sp->s_free_inodes_count;
  
    if(magic == SUPER_MAGIC)
      {
        get_block(dev,GDBLOCK,buf);
	gp = (GD *)buf;
	printf("iblock = %d\n",gp->bg_inode_table);
	printf("bmap=%d\n",gp->bg_block_bitmap);
	printf("imap=%d\n",gp->bg_inode_bitmap);
	printf("mount : %s mounted on / \n",devicename); 
	printf("nblocks=%d bfree=%d  ninodes=%d ifree=%d\n",nblocks, bfree,ninodes,ifree);
	//root = iget(dev, ROOT_INODE);
	//printf("mounted root\n");
	//printf("creating P0, P1\n");

	//proc[0].cwd = root;
	//proc[1].cwd = root;
	//root->refCount = 3;
      }
    else{
      printf("SUPER_MAGIC = %d it is not an EX2 Filesystem.\n",magic);
      printf("Program aborted.\n");
      exit(1);
  }
//4. find the ino, and then the minode of mount_point:
//    call  ino  = get_ino(&dev, pathname);  to get ino:
//    call  mip  = iget(dev, ino);           to load its inode into memory;    
   ino=get_ino(&dev,&mount_point);
   mip=iget(dev,ino):

//5. Check mount_point is a DIR.  
//   Check mount_point is NOT busy (e.g. can't be someone's CWD)

6. Record NEW dev in the MOUNT table entry;

   (For convenience, store the filsys name in the Mount table, and also
                     store its ninodes, nblocks)

7. Mark mount_point's minode as being mounted on and let it point at the
   MOUNT table entry, which points back to the mount_point minode.

8. return 0;

}
  

int umount(char *filesys)
{
   int i;
   MINODE *temp=root;
   Mount *mountp;
//1. Search the MOUNT table to check filesys is indeed mounted.
   for(i=0;i<10;i++)
	{
		if(strcmp(mn[i]->mount_name,filesys)==0)
			{
				printf("the filesys %s is indeed mounted !!!!",filesys);
			}
	}
//2. Check whether any file is still active in the mounted filesys;
//      e.g. someone's CWD or opened files are still there,
//   if so, the mounted filesys is BUSY ==> cannot be umounted yet.
//   HOW to check?      ANS: by checking all minode[].dev
   mountp=search_mount_table(&filesys);
   for(i=0;i<50;i++)
	{
		if(temp->dev==mountp->dev)
			{
				printf("the mounted filesysis BUSY, canoot be unmounted yet!!\n");
				mountp->busy=1;
				return 0;
			}
                temp++;
	}
3. Find the mount_point's inode (which should be in memory while it's mounted 
   on).  Reset it to "not mounted"; then 
         iput()   the minode.  (because it was iget()ed during mounting)

4. return(0);

}  
Mount *search_mount_table(char * filesys)
{
  int i;
	for(i=0;i<10;i++)
	{
		if(strcmp(mn[i]->mount_name,filesys)==0)
			{
				printf("find filesys %s!!!!",filesys);
				return &mn[i];
			}
	}
        printf("can not find the %s in mount table\n",filesys);
        
}
