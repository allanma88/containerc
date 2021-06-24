#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "cio.h"

int writeInt(char *path, int i)
{
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        return -1;
    }
    return writeInt1(fd, i);
}

int writeInt1(int fd, int i)
{
    int len = snprintf(NULL, 0, "%d", i);
    char str[len + 1];
    sprintf(str, "%d", i);
    return write(fd, str, len);
}

int readInt(char *path)
{
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        return -1;
    }
    return readInt1(fd);
}

int readInt1(int fd)
{
    char s[11] = {};
    if (read(fd, s, 10) < 0)
    {
        return -1;
    }
    int i = (int)strtol(s, NULL, 10);
    return i;
}


char *readtoend(char *path, char *modes)
{
    FILE *f = fopen(path, modes);
    if (f == NULL)
    {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET); /* same as rewind(f); */

    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);

    string[fsize] = 0;
    return string;
}