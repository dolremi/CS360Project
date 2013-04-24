#include "type.h"

// Initialization of the program 
void menu();

void init();

void mount_root();

void pwd();

void do_pwd(MINODE *wd);

void quit();

void cd();

int do_cd();

void ls();

int do_ls();

void printFile(MINODE *mip, char *namebuf);

void printChild(int devicename, MINODE *mp);

int my_mkdir(MINODE *pip, char *name);

int findparent(char *pathn);

int make_dir();

void do_mkdir();

void do_creat();

int creat_file();

int my_creat(MINODE *pip, char *name);

void stats();

int do_stat(char *path, struct stat *stPtr);

void do_rmdir();

int rmdir();

int rm_child(MINODE *parent, char *my_name);

void do_link();

int link();

void do_unlink();

int unlink();

void do_rm();

void do_symlink();

int symlink();

void touch();

int do_touch();

void chgmod();

int do_chmod();

void chown();

int do_chown();

void chgrp();

int do_chgrp();
