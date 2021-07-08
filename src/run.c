#define _GNU_SOURCE
#include <stdlib.h>

#include "config.h"
#include "deserializer.h"
#include "print.h"
#include "parent.h"
#include "run.h"

int run()
{
    cloneArgs *cArgs = (cloneArgs*)malloc(sizeof(cloneArgs));
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