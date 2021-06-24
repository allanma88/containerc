#ifndef __CHILD_H
#define __CHILD_H

typedef struct cloneArgs
{
    containerConfig *config;
    int cloneFlags;
    int sync_child_pipe[2]; /* Pipe used to synchronize parent and child */
    int sync_grandchild_pipe[2]; /* Pipe used to synchronize parent and grand child */
} cloneArgs;

int childMain(void *arg);

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */
char child_stack[STACK_SIZE];

#endif