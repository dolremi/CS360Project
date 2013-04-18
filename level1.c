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
  iput(wd);
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


	      printf(" %d %d %d %d",temp->INODE.i_links_count, temp->INODE.i_uid,temp->INODE.i_gid, temp->INODE.i_size);
	      	   
	      printf(" %s %s",ctime(&(temp->INODE.i_atime)),namebuf);
	      
	      // if symblink needs to the file link to 
	      if((mode & 0120000) == 0120000)
		printf(" => %s\n",(char *)(temp->INODE.i_block));
	      else
		printf("\n");

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
  char *parent, *child;
  if(pathname[0] == 0)
    { 
      printf("mkdir : input pathname :");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
    }
 

  //check if it is absolute path to determine where the inode comes from
  if(pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
 
   
  if(findparent(pathname))
    {  
      parent = dirname(pathname);
      child = basename(pathname);
      ino = getino(&dev, parent);

      if(ino == 0)
	return -1;
      pip = iget(dev,ino);

    }
  else
    {
      pip = iget(running->cwd->dev,running->cwd->ino);
      child = (char *)malloc((strlen(pathname) + 1)*sizeof(char));
      strcpy(child, pathname);
    }


  // verify INODE is a DIR and child does not exists in the parent directory
  if((pip->INODE.i_mode & 0040000) != 0040000) 
    {
      printf("%s is not a directory.\n",parent);
      iput(pip);
      return -1;
    }
  
  if(search(pip,child))
    {
      printf("%s already exists.\n",child);
      iput(pip);
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
      strncpy(dp->name,name,dp->name_len);
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
    strncpy(dirp->name,name,dirp->name_len);
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
  char *parent, *child;
 
  if(pathname[0] == 0)
    { 
      printf("input filename :");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
    }
 

  //check if it is absolute path to determine where the inode comes from
  if(pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;
 
   
  if(findparent(pathname))
    {  
      parent = dirname(pathname);
      child = basename(pathname);
      ino = getino(&dev, parent);
      if(ino == 0)
	return -1;
      pip = iget(dev,ino);

    }
  else
    {
      pip = iget(running->cwd->dev,running->cwd->ino);
      child = (char *)malloc((strlen(pathname) + 1)*sizeof(char));
      strcpy(child, pathname);
    }


  // verify INODE is a DIR and child does not exists in the parent directory
  if((pip->INODE.i_mode & 0040000) != 0040000) 
    {
      printf("%s is not a directory.\n",parent);
      iput(pip);
      return -1;
    }
  
  if(search(pip,child))
    {
      printf("%s already exists.\n",child);
      iput(pip);
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
      strncpy(dp->name,name,dp->name_len);
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
    strncpy(dirp->name,name,dirp->name_len);
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

void do_rmdir()
{
  if(rmdir() < 0)
    printf("can't rmdir.\n");
}


int rmdir()
{
  int dev,i;
  unsigned long parent,ino;
  char *my_name;
  MINODE *mip,*pip;
  char path[256]; 

  strcpy(path, pathname);
  dev = running->cwd->dev; 
  ino = getino(&dev,pathname);


  // path doesn't exist
  if(ino == 0)
    return -1;

  mip = iget(dev,ino);
 
  /* level 3 only */
  // if not super user and uid is not matched
  if(running->uid  && running->uid != mip->INODE.i_uid)
    {
      printf("Error! The user mode doesn't match.\n");
      iput(mip);     
      return -1;
    } 
  
  // if(!mip->mounted)
  //     mount();

 
  /**********************************************/
  // check if not DIR || busy || not empty
  if(((mip->INODE.i_mode)&0040000) != 0040000)
    {
      printf("invalid pathname.\n");
      iput(mip);
      return -1;
    }

  if(mip->refCount>1)
    {
      printf("DIR is being used.\n");
      iput(mip);
      return -1;
    }

  if(!isEmpty(mip)) {
    printf("DIR not empty\n");
    iput(mip);
    return -1;
  }


  for(i = 0; i < 12; i++){
    if(mip->INODE.i_block[i] == 0)
      continue;
    bdealloc(mip->dev, mip->INODE.i_block[i]);
  }

  idealloc(mip->dev,mip->ino);
  iput(mip);

  findino(mip,&ino,&parent);
  pip = iget(mip->dev,parent);

  if(findparent(path))
    my_name = basename(path);
  else{
    my_name = (char *)malloc((strlen(path)+1)*sizeof(char));
    strcpy(my_name,path);
  }
  
  rm_child(pip,my_name);
  pip->INODE.i_links_count--;
  pip->INODE.i_atime = pip->INODE.i_mtime = time(0L);
  pip->dirty = 1;
  iput(pip);
  return 0;
}

// return 0 for not empty
int isEmpty(MINODE *mip)
{
  int i;
  char *cp;
  char buf[BLOCK_SIZE],namebuf[256];

  if(mip->INODE.i_links_count > 2)
    return 0;

  if(mip->INODE.i_links_count == 2)
    {
      for(i = 0; i < 12; i++)
	{
	  if(mip->INODE.i_block[i])
	    {
	      get_block(mip->dev,mip->INODE.i_block[i],buf); 
	      cp = buf;
	      dp = (DIR *)buf;

	      while(cp < &buf[BLOCK_SIZE])
		{
		  strncpy(namebuf,dp->name,dp->name_len);
		  namebuf[dp->name_len] = 0;

		  if(strcmp(namebuf,".")&&strcmp(namebuf,".."))
		    return 0;

		  cp+=dp->rec_len;
		  dp=(DIR *)cp;
		}
	    }
	 
	}

      return 1;
    }
}

int rm_child(MINODE *parent, char *my_name)
{
  int i,j, total_length,next_length,removed_length,previous_length;
  char *cp, *cNext;
  DIR *dNext;
  char buf[BLOCK_SIZE],namebuf[256],temp[BLOCK_SIZE];

  for(i = 0; i < 12 ; i++ )
    {
      if(parent->INODE.i_block[i])
	{
	  get_block(parent->dev,parent->INODE.i_block[i],buf);
	  dp=(DIR *)buf;
	  cp = buf;
	  j = 0;
	  total_length = 0;
	  while(cp < &buf[BLOCK_SIZE])
	    {
	      strncpy(namebuf,dp->name,dp->name_len);
	      namebuf[dp->name_len] = 0;
	      total_length+= dp->rec_len;

	      // found my_name
	      if(!strcmp(namebuf,my_name))
		{
		  
		  // if not first entry in data block
		  if(j)
		    {
                 
		      // if my_name at the last entry in the data block
		      if(total_length == BLOCK_SIZE)
			{
			  removed_length = dp->rec_len;
			  cp-= previous_length;
			  dp=(DIR *)cp;
			  dp->rec_len += removed_length;
			  strncpy(namebuf,dp->name,dp->name_len);
			  namebuf[dp->name_len] = 0;
			  put_block(parent->dev,parent->INODE.i_block[i],buf);
			  return 0;
			}

		      // my_name is not at the last entry in the data block
		      // move following entries forward in the data block
		      removed_length = dp->rec_len;
		      cNext = cp + dp->rec_len;
		      dNext = (DIR *)cNext;
		      while(total_length + dNext->rec_len < BLOCK_SIZE)
			{
			  total_length += dNext->rec_len;
			  next_length = dNext->rec_len;
			  dp->inode = dNext->inode;
			  dp->rec_len = dNext->rec_len;
			  dp->name_len = dNext->name_len;
			  strncpy(dp->name,dNext->name,dNext->name_len);
			  dp->name[dp->name_len] = 0;
			  cNext += next_length;
			  dNext = (DIR *)cNext;
			  cp+= next_length;
			  dp = (DIR *)cp;
			 
			}
		      dp->inode = dNext->inode;
		      dp->rec_len = dNext->rec_len + removed_length;
		      dp->name_len = dNext->name_len;
		      strncpy(namebuf,dNext->name,dNext->name_len);
		      namebuf[dp->name_len] = 0;
		      strcpy(dp->name,namebuf);
			 
		      put_block(parent->dev,parent->INODE.i_block[i],buf);
		      return 0;
		    }
		  else
		    {
		    
		      bdealloc(parent->dev,parent->INODE.i_block[i]);
		      memset(temp,0,BLOCK_SIZE);
		      put_block(parent->dev,parent->INODE.i_block[i],temp);
		      parent->INODE.i_size-=BLOCK_SIZE;
		      parent->INODE.i_block[i] = 0;
		      return 0;
		    }
		}
	      j++;
	      previous_length = dp->rec_len;
	      cp+=dp->rec_len;
	      dp = (DIR *)cp;
	    }

	}
    }
  return -1;

}

void do_link()
{
  if(link() < 0)
    printf("can't link\n");
}

int link()
{
  char OldPath[256],NewPath[256],parent[256],child[256], paths[256],buf[BLOCK_SIZE],buf2[BLOCK_SIZE];
  unsigned long inumber,oldIno;
  int dev,newDev,i,rec_length,need_length,ideal_length,datab;
  MINODE *mip;
  char *cp;
  DIR *dirp;

  if(pathname[0] == 0 || parameter[0] == 0) 
    {
      printf("input OLD_filename: ");
      fgets(OldPath,256,stdin);
      OldPath[strlen(OldPath)-1] = 0;
      printf("input NEW_filename: ");
      fgets(NewPath,256,stdin);
      NewPath[strlen(NewPath)-1] = 0;
    }
  else
    {
      strcpy(OldPath,pathname);
      strcpy(NewPath,parameter);
    }
      printf("link: %s %s\n",OldPath,NewPath);
    
      dev = root->dev;
      strcpy(paths,OldPath);
      oldIno = getino(&dev,paths);
      if(oldIno == 0)
	  return -1;

      mip = iget(dev,oldIno);

      // make sure it is REG file
      if(((mip->INODE.i_mode)&0100000)!= 0100000)  
	{
	  printf("%s is not REG file.\n",OldPath);
	  iput(mip);
	  return -1;
	}
       
      iput(mip);
      newDev = running->cwd->dev;
      if(findparent(NewPath))
	{
	  strcpy(parent,dirname(NewPath));
	  strcpy(child,basename(NewPath));

	  strcpy(paths,parent);
	  inumber = getino(&newDev,paths);
     
	  if(inumber == 0)
	    return -1;
	  
	  if(newDev != dev)
	    {
	      printf("not on the same device.\n");
	      return -1;
	    }
 
	  mip = iget(newDev,inumber);

	  // make sure parent is DIR file
	  if(((mip->INODE.i_mode)&0040000)!= 0040000)  
	    {
	      printf("%s is not DIR file.\n",parent);
	      iput(mip);
	      return -1;
	    }

	  if(search(mip,child))
	    {
	      printf("%s already exists.\n",child);
	      iput(mip);
	      return -1;
	    }
	}
      else
	{
	  strcpy(parent,".");
	  strcpy(child,NewPath);

	  if(running->cwd->dev != dev)
	    {
	      printf("not on the same device.\n");
	      return -1;
	    }

	  if(search(running->cwd,child))
	    {
	      printf("%s already exists.\n",child);
	      return -1;
	    }
	  mip = iget(running->cwd->dev,running->cwd->ino);
	}

      printf("parent=%s  child=%s\n",parent,child);     
 
      // add an entry to the parent's data block
      i = 0;
      while(mip->INODE.i_block[i])
	i++;
  
      i--;  
 
      get_block(mip->dev,mip->INODE.i_block[i],buf);
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

      need_length = 4 * ((8 + strlen(child) + 3)/4);
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
	  dp->name_len = strlen(child);
	  strncpy(dp->name,child,dp->name_len);
	  dp->inode = oldIno;
    
	  // write the new block back to the disk
	  put_block(mip->dev,mip->INODE.i_block[i],buf);
  
	}
      else{
	// otherwise allocate a new data block 
	i++;
	datab = balloc(mip->dev);
	mip->INODE.i_block[i] = datab;
	get_block(mip->dev, datab, buf2);

	// enter the new entry as the first entry 
	dirp = (DIR *)buf2;
	dirp->rec_len = BLOCK_SIZE;
	dirp->name_len = strlen(child);
	strncpy(dirp->name,child,dirp->name_len);
	dirp->inode = oldIno;

	// write the new block back to the disk
	put_block(mip->dev,mip->INODE.i_block[i],buf2);
   
      }

      mip->INODE.i_links_count++;
      mip->INODE.i_atime = time(0L);
      mip->dirty = 1;
      printf("inode's link_count=%d\n",mip->INODE.i_links_count);
      iput(mip);

      return 0;
}

void do_unlink()
{

  if(unlink() < 0)
    printf("can't unlink\n");
}

void do_rm()
{
  if(unlink()< 0)
    printf("can't rm\n");
}

int unlink()
{
  char path[256],parent[256],child[256];
  MINODE *mip;
  int dev,i,m,l;
  int *k,*j,*t;
  int block[15];
  char buf[BLOCK_SIZE],buf2[BLOCK_SIZE];
  unsigned long inumber;

  if(pathname[0] == 0)
    {
      printf("input pathname: ");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
    }

 
  dev = root->dev;
  strcpy(path,pathname);
  inumber = getino(&dev,path);
  if(inumber == 0)
    return -1;

  mip = iget(dev, inumber);

  // make sure it is FILE
  if(((mip->INODE.i_mode)&0040000)== 0040000)
    {
      printf("%s is not a REG file.\n",pathname);
      iput(mip);
      return -1;
    }

  mip->INODE.i_links_count--;

  /* DISK BLOCK deallocation  */

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
 
  // deallocate its INODE
  idealloc(mip->dev, mip->ino);
  iput(mip);

   if(findparent(pathname))
     {
       strcpy(parent,dirname(pathname));
       strcpy(child,basename(pathname));
       strcpy(path,parent);
       inumber = getino(&dev,path);
       mip = iget(dev,inumber);
     }
   else{
     strcpy(parent,".");
     strcpy(child,pathname);

     mip = iget(running->cwd->dev,running->cwd->ino);
   }

   printf("parent=%s  child=%s\n",parent,child);

   rm_child(mip,child);
   mip->INODE.i_links_count--;

   printf("inode's link_count=%d\n",mip->INODE.i_links_count);
   mip->INODE.i_atime=mip->INODE.i_mtime = time(0L);
   mip->dirty = 1;
   iput(mip);

   return 0;
}

void do_symlink()
{
  if(symlink() < 0)
    printf("can't symlink\n");
}

int symlink()
{  
  char OldPath[256],NewPath[256],paths[256],buf[BLOCK_SIZE];
  unsigned long inumber;
  int dev,i,r;
  MINODE *mip,*pip;
  char *cp, *parent, *child;
  DIR *dirp;

  if(pathname[0] == 0 || parameter[0] == 0) 
    {
      printf("input OLD_filename: ");
      fgets(OldPath,256,stdin);
      OldPath[strlen(OldPath)-1] = 0;
      printf("input NEW_filename: ");
      fgets(NewPath,256,stdin);
      NewPath[strlen(NewPath)-1] = 0;
    }
  else
    {
      strcpy(OldPath,pathname);
      strcpy(NewPath,parameter);
    }
      printf("symlink: %s %s\n",OldPath,NewPath);
    
      dev = root->dev;
      strcpy(paths,OldPath);
      inumber = getino(&dev,paths);
      
      // file does not exist
      if(inumber == 0)
	  return -1;

      mip = iget(dev,inumber);

      // make sure it is REG file
      if(((mip->INODE.i_mode)&0100000)!= 0100000 && (((mip->INODE.i_mode)&0040000)!= 0040000))  
	{
	  printf("%s is not a REG file or DIR.\n",OldPath);
	  iput(mip);
	  return -1;
	}
       
      iput(mip);

      if(NewPath[0] == '/')
	dev = root->dev;
      else
	dev = running->cwd->dev;
 
   
      if(findparent(NewPath))
	{  
	  parent = dirname(NewPath);
	  child = basename(NewPath);
	  strcpy(paths,parent);
	  inumber = getino(&dev, paths);
	  
	  if(inumber == 0)
	    return -1;

	  pip = iget(dev,inumber);
	}
      else
	{
	  pip = iget(running->cwd->dev,running->cwd->ino);
	  child = (char *)malloc((strlen(NewPath) + 1)*sizeof(char));
	  strcpy(child, NewPath);
	  parent = (char *)malloc(2*sizeof(char));
	  strcpy(parent,".");
	}

      printf("parent=%s  child=%s\n",parent,child);

      if((pip->INODE.i_mode & 0040000) != 0040000) 
	{
	  printf("%s is not a directory.\n",parent);
	  iput(pip);
	  return -1;
	}
  
      if(search(pip,child))
	{
	  printf("%s already exists.\n",child);
	  iput(pip);
	  return -1;
	}

      printf("CALL MYCREAT\n");
      r = my_creat(pip, child);
      printf("AFTER CREAT\n");
      printf("now pathname is %s\n",NewPath);
      // get the new path into memory
      strcpy(paths,NewPath);
      inumber = getino(&dev,paths);

      if(inumber == 0)
	return -1;
      printf("ndev=%d ino=%d\n",dev,inumber);
      printf("pathname=%s\n",OldPath);
      mip = iget(dev,inumber);
      mip->INODE.i_mode = 0xA1FF;
      strcpy((char *)(mip->INODE.i_block),OldPath);
      printf("iblock=%s\n",(char *)(mip->INODE.i_block));
      printf("name=%s ",OldPath);
      printf("type=%4x ",mip->INODE.i_mode);
      mip->INODE.i_size = strlen(OldPath);
      printf("size=%d ",mip->INODE.i_size);
      printf("refCount=%d\n",mip->refCount);
      printf("symlink %s %s OK\n",OldPath,NewPath);
      mip->dirty = 1;
      iput(mip);
      return r;
}

void touch()
{
  if(do_touch() < 0)
    printf("can't touch\n");
}

int do_touch()
{
  int dev,inumber;
  MINODE *mip;

  if(pathname[0] == 0)
    {
      printf("input pathname: ");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
    }

  dev = root->dev;
  inumber = getino(&dev,pathname);

  if(inumber == 0)
    return -1;

  mip = iget(dev,inumber);

  mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
  mip->dirty = 1;

  iput(mip);

  return 0;
}

void chgmod()
{
  if(do_chmod() < 0)
    printf("can't change mode\n");
}

int do_chmod()
{
  int mode, dev,inumber;
  MINODE *mip;

  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("input pathname: ");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
      printf("input the new mode: ");
      fgets(parameter,256,stdin);
      parameter[strlen(parameter)-1] = 0;
    }

  sscanf(parameter,"%x",&mode);
  dev = root->dev;
  inumber = getino(&dev,pathname);

  if(inumber == 0)
    return -1;

  mip = iget(dev,inumber);

  mip->INODE.i_mode = mode;
  mip->dirty = 1;

  iput(mip);
}

void chown()
{
  if(do_chown() < 0)
    printf("can't change the owner\n");
}

int do_chown()
{
  int owner, dev,inumber;
  MINODE *mip;

  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("input pathname: ");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
      printf("input the new owner: ");
      fgets(parameter,256,stdin);
      parameter[strlen(parameter)-1] = 0;
    }

  sscanf(parameter,"%d",&owner);
  dev = root->dev;
  inumber = getino(&dev,pathname);

  if(inumber == 0)
    return -1;

  mip = iget(dev,inumber);

  mip->INODE.i_uid = owner;
  mip->dirty = 1;

  iput(mip);
}

void chgrp()
{
  if(do_chgrp() < 0)
    printf("can't change the owner\n");
}

int do_chgrp()
{
  int group, dev,inumber;
  MINODE *mip;

  if(pathname[0] == 0 || parameter[0] == 0)
    {
      printf("input pathname: ");
      fgets(pathname,256,stdin);
      pathname[strlen(pathname)-1] = 0;
      printf("input the new group: ");
      fgets(parameter,256,stdin);
      parameter[strlen(parameter)-1] = 0;
    }

  sscanf(parameter,"%d",&group);
  dev = root->dev;
  inumber = getino(&dev,pathname);

  if(inumber == 0)
    return -1;

  mip = iget(dev,inumber);

  mip->INODE.i_gid = group;
  mip->dirty = 1;

  iput(mip);
}
