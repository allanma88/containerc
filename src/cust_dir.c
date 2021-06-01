#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "cust_dir.h"

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
                fprintf(stderr, "mkdir %s error: %s\n", tmp, strerror(errno));
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) < 0 && EEXIST != errno)
    {
        fprintf(stderr, "mkdir %s error: %s\n", tmp, strerror(errno));
        return -1;
    }
    return 0;
}
