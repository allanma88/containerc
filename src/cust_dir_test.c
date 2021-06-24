#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "cust_dir.h"

int main()
{
    char *path = "./mycontainer/rootfs/dev/pts";
    if (mkdirRecur(path) < 0)
    {
        printf("mkdirRecur %s error: %s\n", path, strerror(errno));
    }
    else
    {
        printf("mkdirRecur done\n");
    }
}
