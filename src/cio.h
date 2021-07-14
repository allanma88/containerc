#ifndef __CIO_H
#define __CIO_H

#include <stdio.h>

int writeInt(char *path, int i);

int writeInt1(int fd, int i);

int readInt(char *path);

int readInt1(int fd);

char *readtoend(char *path, char *modes);

int mkdirRecur(const char *path);

char *getdir(char *path);

FILE *openFile(char *path);

#endif