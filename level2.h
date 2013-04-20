#include "type.h"


void fileopen();

int open_file();

void truncate(MINODE *mip);

void closeFile();

int close_file(int fd);

void do_seek();

long llseek(int fd, long position);

void pfd();
