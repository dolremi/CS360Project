#include "type.h"

// Initialization of the program 
void init();

void mount_root();

void pwd();

void do_pwd(MINODE *wd);

void quit();

void cd();

int do_cd();

void ls();

int do_ls();

void printChild(int devicename, MINODE *mp);

int my_mkdir(MINODE *pip, char *name);

int findparent(char *pathn);

int make_dir();

void do_mkdir();
