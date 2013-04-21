#include "type.h"


void fileopen();

int open_file(int mode);

void truncate(MINODE *mip);

void closeFile();

int close_file(int fd);

void do_seek();

long llseek(int fd, long position);

void pfd();

void do_read();

int read_file();

int myread(int fd, char *buf, int nbytes);

void do_cat();

int catFile();
