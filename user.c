#include "type.h"
#include "user.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"

functionTable myTable[] ={
  {"mv",do_mv},
  {"menu",menu},
  {"umount",do_umount},
  {"mount",do_mount},
  {"cp",do_cp},
  {"write",do_write},
  {"cat",do_cat},
  {"read",do_read},
  {"pfd",pfd},
  {"lseek",do_seek},
  {"close",closeFile},
  {"open",fileopen},
  {"chgrp",chgrp},
  {"chown",chown},
  {"chmod",chgmod},
  {"touch",touch},
  {"symlink",do_symlink},
  {"rm",do_rm},
  {"unlink",do_unlink},
  {"link",do_link},
  {"rmdir",do_rmdir},
  {"stat",stats},
  {"creat",do_creat},
  {"mkdir",do_mkdir},
  {"ls",ls},
  {"cd",cd},
  {"pwd",pwd},
  {"quit",quit},
  {0,0}
};

// check the command in the function pointer table
int checkFunct(char *command)
{
  int i = 0;
  for(i = 0; myTable[i].functionName;i++)
    {
      if(!strcmp(command,myTable[i].functionName))
	{
	  myTable[i].f();
	  return 1;
	}
    }
  printf("The command is invalid!\n");
  return 0;
}

void clearInput()
{
  int i = 0;
  for(i = 0; i < 256 ; i++)
    {
      pathname[i] = 0;
      parameter[i] = 0;
    }
}
