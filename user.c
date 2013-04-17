#include "type.h"
#include "user.h"
#include "level1test.h"

functionTable myTable[] ={
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
