#include "level1test.h"
#include "user.h"
#include "type.h"

void main()
{
  char line[128],cname[64];

  init();

  while(1){
    printf("P%d running: ",running->pid);
    printf("input command : ");
    fgets(line,128,stdin);
    line[strlen(line)-1] = 0;
    if(line[0] == 0) continue;

    clearInput();
    sscanf(line, "%s %s %64c",cname, pathname, parameter);

    checkFunct(cname);
  }
}
