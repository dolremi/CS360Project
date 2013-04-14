#include "type.h"
#include "level1test.h"
#include "utility.h"
#include <stdlib.h>

extern char pathname[256];

void init(){
  int i = 0;

  proc[0].uid = 0;
  proc[1].uid = 1;
  proc[0].gid = 0;
  proc[1].gid = 0;
  proc[0].cwd = 0;
  proc[1].cwd = 0;

  for(i = 0; i < NMINODES; i++)
    minode[i].refCount = 0;

  root = 0;

  mount_root();

  running = &proc[0];
}

void mount_root()
{
  char buf[BLOCK_SIZE];
  int dev;
  char devicename[128];
  int magic,nblocks, bfree, ninodes, ifree;

  printf("mounting root\n");
  printf("enter root name (RETURN for /dev/fd0) :");
 
  fgets(devicename,128,stdin);
  devicename[strlen(devicename)-1] = 0;
 
  if(devicename[0] == 0)
    {
      strcpy(devicename, "/dev/fd0/");
      printf("Now the device is %s\n",devicename);
      exit(1);
    }
  else{
    dev = open(devicename,O_RDWR);
 
    if(dev < 0)
      {
	printf("open %s failed.\n",devicename);
	exit(1);
      }
 
    // verify it is an EXT2 FS
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
	root = iget(dev, ROOT_INODE);
	printf("mounted root\n");
	printf("creating P0, P1\n");

	proc[0].cwd = root;
	proc[1].cwd = root;
	root->refCount = 3;
      }
    else{
      printf("SUPER_MAGIC = %d it is not an EX2 Filesystem.\n",magic);
      printf("Program aborted.\n");
      exit(1);
  }
  }
}


void quit()
{
  int i;
  for(i = 0; i < NMINODES; i++)
    {
      while(minode[i].refCount)
	iput(&minode[i]);
    }
  printf("program finished.\n");
  exit(1);
}

void pwd()
{
  if(running -> cwd == root)
    printf("/");
  else
    do_pwd(running->cwd);
 
 printf("\n");
}

void do_pwd(MINODE *wd)
{
  struct DIR *dirp;
  char myname[256];
  unsigned long myino,parentino;
  
  if(wd == root)
    {
      return;
    }

  findino(wd,&myino,&parentino);
  wd = iget(wd->dev, parentino);
  do_pwd(wd);
  findmyname(wd,myino,myname);
  printf("/%s",myname);
}

void cd()
{
  if(do_cd() < 0)
    printf("cd: no such directory\n");
} 

int do_cd()
{

  unsigned long  ino;
  int device;
  MINODE *mp;
 
  // cd to the root
  if(pathname[0] == 0)
    {
      iput(running->cwd);
      running->cwd = root;
      root->refCount++;
      return 0;
    }

  device = running->cwd->dev;
  ino = getino(&device, pathname);

  // no such directory
  if(ino == 0)
    return -1;
    
  mp = iget(device, ino);

  // check DIR type
  if(((mp->INODE.i_mode) & 0040000)!= 0040000)
    {
      printf("%s is not a directory\n",pathname);
      iput(mp);
      return -1;
    }

  // dispose of original CWD
  iput(running->cwd);

  running->cwd = mp;
  return 0;
}

void stats()
{
  struct stat mystat;
  char path[256];
  
  if(do_stat(pathname,&mystat) < 0)
    printf("bad directory.\n"); 
 
}

int do_stat(char *path, struct stat *stPtr)
{
  unsigned long ino;
  MINODE *mip;
  int device = running->cwd->dev;
 
  // stat will default to cwd if no pathname specified
  if(path[0] == 0)
    ino = running->cwd->ino;
  else
    ino = getino(&device,path);

  // no such path in the stat
  if(ino == 0)
    return -1;

  mip = iget(device, ino);

  stPtr->st_dev = device;
  stPtr->st_ino = ino;
 
  stPtr->st_mode = mip->INODE.i_mode;
  stPtr->st_nlink = mip->INODE.i_links_count;
  stPtr->st_uid = mip->INODE.i_uid;
  stPtr->st_gid = mip->INODE.i_gid;
  stPtr->st_size = mip->INODE.i_size;
  stPtr->st_blksize = BLOCK_SIZE;
  stPtr->st_blocks = mip->INODE.i_blocks;
  stPtr->st_atime = mip->INODE.i_atime;
  stPtr->st_mtime = mip->INODE.i_mtime;
  stPtr->st_ctime = mip->INODE.i_ctime;

  // print out all the fields in stat
  printf("\n********** stat **********\n");
  printf("dev=%d   ",stPtr->st_dev);
  printf("ino=%d   ",stPtr->st_ino);
  printf("mod=%4x\n", stPtr->st_mode);
  printf("uid=%d   ",stPtr->st_uid);
  printf("gid=%d   ",stPtr->st_gid);
  printf("nlink=%d\n",stPtr->st_nlink);
  printf("size=%d ",stPtr->st_size); 
  printf("time=%s\n",ctime(&(stPtr->st_ctime)));
  printf("**************************\n");
 
  iput(mip);

  return 0;
}

void ls()
{
  if(do_ls(pathname) < 0)
    printf("ls : %s doesn't exist.\n",pathname);
 
}

int do_ls(char *path)
{
  unsigned long  ino;
  MINODE *mip;
  int device = running->cwd->dev;

  if(path[0] == 0)
    {
      mip = iget(device, running->cwd->ino);
      printChild(device, mip);
    }
  else
    {
      ino = getino(&device, path);
     
      // pathname doesn't exist
      if(!ino)
	return -1;
      
      mip = iget(device, ino);
      printChild(device, mip);
    }

  iput(mip);
  return 0;
}


void printChild(int devicename, MINODE *mp)
{

  char buf[BLOCK_SIZE], namebuf[256];
  char *cp;
  DIR *dirp;
  unsigned short mode;
  int i,ino;
  MINODE *temp;

  for(i = 0; i < 12 ; i++)
    {
      if(mp->INODE.i_block[i])
	{
	  get_block(devicename, mp->INODE.i_block[i], buf);
	  cp = buf;
	  dirp = (DIR *)buf;

	  while(cp <&buf[BLOCK_SIZE])
	    {
	      strncpy(namebuf, dirp->name, dirp->name_len);
	      namebuf[dirp->name_len] = 0;
	   	     
	      ino = dirp->inode;
	      temp = iget(devicename,ino); 
	      mode = temp->INODE.i_mode;
	     
	      // print out info in the file as ls -l in linux
	      if((mode & 0120000) == 0120000)
		printf("l");
	      else if((mode & 0040000) == 0040000)
		printf("d");
	      else if((mode & 0100000) == 0100000)
		printf("-");
	   

	      if((mode & (1 << 8)))
		printf("r");
	      else
		printf("-");

	      if((mode & (1 << 7)) )
		printf("w");
	      else
		printf("-");
	      
	      if((mode & (1 << 6)) )
		printf("x");
	      else
		printf("-");

	      if((mode & (1 << 5)) )
		printf("r");
	      else
		printf("-");

	      if((mode & ( 1 << 4)) )
		printf("w");
	      else
		printf("-");

	      if((mode & (1 << 3)) )
		printf("x");
	      else
		printf("-");
	      
	      if((mode & (1 << 2)) )
		printf("r");
	      else
		printf("-");

	      if((mode & (1 << 1)) )
		printf("w");
	      else
		printf("-");
	      
	      if(mode & 1)
		printf("x");
	      else
		printf("-");


	      printf(" %4d %d %d %d",temp->INODE.i_links_count, temp->INODE.i_uid,temp->INODE.i_gid, temp->INODE.i_size);
	      	   
	      printf(" %s %s\n",ctime(&(temp->INODE.i_atime)),namebuf);
	      
	      iput(temp);
		
	      cp+=dirp->rec_len;
	      dirp = (DIR *)cp;
	    }

	}
    }
  printf("*****************************************************\n");
}

void do_mkdir()
{
  if(make_dir() < 0)
    printf("can't mkdir.\n");
}

int make_dir()
{
  int ino, r,dev;
  MINODE *pip;
  char path[256];
  char *parent, *child;
 
  printf("mkdir : input pathname :");
  fgets(path,256,stdin);
  path[strlen(path)-1] = 0;


  //check if it is absolute path to determine where the inode comes from
  if(path[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
 
   
  if(findparent(path))
    {  
      parent = dirname(path);
      child = basename(path);
      ino = getino(&dev, parent);
      if(ino == 0)
	return -1;
      pip = iget(dev,ino);

    }
  else
    {
      pip = iget(running->cwd->dev,running->cwd->ino);
      child = (char *)malloc((strlen(path) + 1)*sizeof(char));
      strcpy(child, path);
    }


  // verify INODE is a DIR and child does not exists in the parent directory
  if((pip->INODE.i_mode & 0040000) != 0040000) 
    {
      printf("%s is not a directory.\n",parent);
      return -1;
    }
  
  if(search(pip,child))
    {
      printf("%s already exists.\n",child);
      return -1;
    }
  r = my_mkdir(pip, child);

  return r;
}

int findparent(char *pathn)
{
  int i = 0;
  while(i < strlen(pathn))
    {
      if(pathn[i] == '/')
	return 1;
      i++;
    }
  return 0;
}

int my_mkdir(MINODE *pip, char *name)
{
  unsigned long inumber,bnumber;
  int i = 0,datab;
  char buf[BLOCK_SIZE],buf2[BLOCK_SIZE];
  char *cp;
  int need_length, ideal_length,rec_length;
  DIR *dirp;
  MINODE *mip;
 
  // allocate an inode and a disk block for the new directory
  inumber = ialloc(pip->dev);
  bnumber = balloc(pip->dev);
 
  mip = iget(pip->dev, inumber);

  /* write the content to mip->INODE*/
  mip->INODE.i_mode = 0x41ED;
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = 1024;
  mip->INODE.i_links_count = 2;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 2;    /* Blocks count in 512-byte blocks */
  mip->dirty = 1;

  for(i = 1; i < 15; i++)
    mip->INODE.i_block[i] = 0;
  mip->INODE.i_block[0] = bnumber;

  iput(mip);
  
   // write the . and .. entries into a buf[] of BLOCK_SIZE
  memset(buf,0,BLOCK_SIZE);

  dp = (DIR *)buf;
  
  dp->inode = inumber;
  strncpy(dp->name,".",1);
  dp->name_len = 1;
  dp->rec_len = 12;

  cp = buf;
  cp += dp->rec_len;
  dp = (DIR *)cp;

  dp->inode = pip->ino;
  dp->name_len = 2;
  strncpy(dp->name,"..",2);
  dp->rec_len = BLOCK_SIZE -12;

  put_block(pip->dev,bnumber,buf);

  //Finally enter name into parent's directory, assume all direct data blocks
  i = 0;
  while(pip->INODE.i_block[i])
    i++;
  
  i--;  
 
  get_block(pip->dev,pip->INODE.i_block[i],buf);
  dp = (DIR *)buf;
  cp = buf;
  rec_length = 0;

  // step to the last entry in a data block
  while(dp->rec_len + rec_length < BLOCK_SIZE)
    {
      rec_length+=dp->rec_len;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

 
  need_length = 4 * ((8 + strlen(name) + 3)/4);
  ideal_length = 4 *((8 + dp->name_len +3)/4);
  rec_length = dp->rec_len;

  // check if it can enter the new entry as the last entry
  if((rec_length - ideal_length) >=need_length)
    {
      // trim the previous entry to its ideal_length
      dp->rec_len = ideal_length;
      cp+=dp->rec_len;
      dp = (DIR *)cp;
      dp->rec_len = rec_length - ideal_length;
      dp->name_len = strlen(name);
      strcpy(dp->name,name);
      dp->inode = inumber;
    
      // write the new block back to the disk
      put_block(pip->dev,pip->INODE.i_block[i],buf);
  
   }
  else{
    // otherwise allocate a new data block 
    i++;
    datab = balloc(pip->dev);
    pip->INODE.i_block[i] = datab;
    get_block(pip->dev, datab, buf2);

    // enter the new entry as the first entry 
    dirp = (DIR *)buf2;
    dirp->rec_len = BLOCK_SIZE;
    dirp->name_len = strlen(name);
    strcpy(dirp->name,name);
    dirp->inode = inumber;

  // write the new block back to the disk
    put_block(pip->dev,pip->INODE.i_block[i],buf2);
   
  }

  pip->INODE.i_links_count++;
  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;
 
  iput(pip);

  return 0;
}

void do_creat()
{
  if(creat_file() < 0)
    printf("can't creat.\n");
}

int creat_file()
{
  int ino,r,dev;
  MINODE *pip;
  char path[256];
  char *parent, *child;
 
  printf("input filename :");
  fgets(path,256,stdin);
  path[strlen(path)-1] = 0;


  //check if it is absolute path to determine where the inode comes from
  if(path[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
 
   
  if(findparent(path))
    {  
      parent = dirname(path);
      child = basename(path);
      ino = getino(&dev, parent);
      if(ino == 0)
	return -1;
      pip = iget(dev,ino);

    }
  else
    {
      pip = iget(running->cwd->dev,running->cwd->ino);
      child = (char *)malloc((strlen(path) + 1)*sizeof(char));
      strcpy(child, path);
    }


  // verify INODE is a DIR and child does not exists in the parent directory
  if((pip->INODE.i_mode & 0040000) != 0040000) 
    {
      printf("%s is not a directory.\n",parent);
      return -1;
    }
  
  if(search(pip,child))
    {
      printf("%s already exists.\n",child);
      return -1;
    }
  r = my_creat(pip, child);

  return r;
}

int my_creat(MINODE *pip, char *name)
{
  unsigned long inumber;
  int i = 0,datab;
  char buf[BLOCK_SIZE],buf2[BLOCK_SIZE];
  char *cp;
  int need_length, ideal_length,rec_length;
  DIR *dirp;
  MINODE *mip;
 
  // allocate an inode and a disk block for the new directory
  inumber = ialloc(pip->dev);

  mip = iget(pip->dev, inumber);

  /* write the content to mip->INODE*/
  mip->INODE.i_mode = 0x81A4;
  mip->INODE.i_uid = running->uid;
  mip->INODE.i_gid = running->gid;
  mip->INODE.i_size = 0;
  mip->INODE.i_links_count = 1;
  mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
  mip->INODE.i_blocks = 2;    /* Blocks count in 512-byte blocks */
  mip->dirty = 1;

  for(i = 0; i < 15; i++)
    mip->INODE.i_block[i] = 0;

  iput(mip);
  
  //Finally enter name into parent's directory, assume all direct data blocks
  i = 0;
  while(pip->INODE.i_block[i])
    i++;
  
  i--;  
 
  get_block(pip->dev,pip->INODE.i_block[i],buf);
  dp = (DIR *)buf;
  cp = buf;
  rec_length = 0;

  // step to the last entry in a data block
  while(dp->rec_len + rec_length < BLOCK_SIZE)
    {
      rec_length+=dp->rec_len;
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }

 
  need_length = 4 * ((8 + strlen(name) + 3)/4);
  ideal_length = 4 *((8 + dp->name_len +3)/4);
  rec_length = dp->rec_len;

  // check if it can enter the new entry as the last entry
  if((rec_length - ideal_length) >=need_length)
    {
      // trim the previous entry to its ideal_length
      dp->rec_len = ideal_length;
      cp+=dp->rec_len;
      dp = (DIR *)cp;
      dp->rec_len = rec_length - ideal_length;
      dp->name_len = strlen(name);
      strcpy(dp->name,name);
      dp->inode = inumber;
    
      // write the new block back to the disk
      put_block(pip->dev,pip->INODE.i_block[i],buf);
  
   }
  else{
    // otherwise allocate a new data block 
    i++;
    datab = balloc(pip->dev);
    pip->INODE.i_block[i] = datab;
    get_block(pip->dev, datab, buf2);

    // enter the new entry as the first entry 
    dirp = (DIR *)buf2;
    dirp->rec_len = BLOCK_SIZE;
    dirp->name_len = strlen(name);
    strcpy(dirp->name,name);
    dirp->inode = inumber;

  // write the new block back to the disk
    put_block(pip->dev,pip->INODE.i_block[i],buf2);
   
  }

  pip->INODE.i_links_count++;
  pip->INODE.i_atime = time(0L);
  pip->dirty = 1;
 
  iput(pip);

  return 0;
}
