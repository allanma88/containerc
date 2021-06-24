#define _GNU_SOURCE
#include <stdlib.h>

#include "config.h"
#include "parser.h"
#include "print.h"
#include "parent.h"
#include "run.h"

int run()
{
    cloneArgs *cArgs = (cloneArgs*)malloc(sizeof(cloneArgs));
    pid_t pid;
    cArgs->config = parse();
    if (cArgs->config == NULL)
    {
        return -1;
    }
    // printConfig(cloneArgs.config);
    // return 0;

    return parentRun(cArgs);
}