#include "type.h"

typedef struct function_table{
  char *functionName;
  void (*f)();
}functionTable;

extern functionTable myTable[];


int checkFunct(char *command);

void clearInput();
