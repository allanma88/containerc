#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "json.h"
#include "print.h"
#include "parent.h"
#include "run.h"
#include "cio.h"

static int extract(char *path)
{
    char *dir = getdir(path);
    char cmd[strlen(path) + strlen(dir) + 12 + 1];
    sprintf(cmd,"tar -xf %s -C %s", path, dir);
    return system(cmd);
}

int run(char *image, char *entrypoint)
{

    char *rootPath;
    if (rootPath != NULL)
    {
        chdir(rootPath);
    }

    cloneArgs *cArgs = (cloneArgs *)malloc(sizeof(cloneArgs));
    pid_t pid;
    cArgs->config = deserialize();
    if (cArgs->config == NULL)
    {
        return -1;
    }
    // printConfig(cArgs->config);
    // return 0;

    return parentRun(cArgs);
}