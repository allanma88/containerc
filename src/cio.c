#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>

#include "cio.h"
#include "cstr.h"

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

int mkdirRecur(const char *path)
{
    DIR *dir = opendir(path);
    if (dir != NULL)
    {
        closedir(dir);
        return 0;
    }

    char *p = NULL;
    size_t len = strlen(path);
    char tmp[len + 1];

    snprintf(tmp, len + 1, "%s", path);
    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0 && EEXIST != errno)
            {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0 && EEXIST != errno)
    {
        return -1;
    }
    return 0;
}

char *getdir(char *path)
{
    size_t n = strlen(path);
    if (path[n - 1] == '/')
    {
        return path;
    }
    else
    {
        char *pos = strrchr(path, '/');
        int len = pos - path + 1;
        char *dir = (char *)calloc(len + 1, sizeof(char));
        strncpy(dir, path, len);
        return dir;
    }
}

FILE *openFile(char *path)
{
    char *pos = strrchr(path, '/');
    int n = pos - path;
    char dir[n + 1];
    strncpy(dir, path, n);
    dir[n] = '\0';
    mkdirRecur(dir);

    FILE *file = fopen(path, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "open %s error: %s\n", path, strerror(errno));
        return NULL;
    }
    return file;
}

char *absDirPath(char *dir)
{
    char buffer[PATH_MAX];
    char *realPath = realpath(dir, buffer);
    return realPath;
}

char *absExePath(char *exePath)
{
    if (strchr(exePath, '/') != NULL)
    {
        return absDirPath(exePath);
    }

    char *pathEnv = getenv("PATH");
    char *dir = strtok(pathEnv, ":");
    char *absPath;
    struct stat st;
    while (dir != NULL)
    {
        absPath = join("/", 2, dir, exePath);
        if (stat(absPath, &st) == 0)
        {
            return absPath;
        }
        dir = strtok(NULL, ":");
    }
    return NULL;
}