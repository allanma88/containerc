#ifndef __CHILD_H
#define __CHILD_H

struct cloneArgs
{
    containerConfig *config;
    int cloneFlags;
    int pipe_fd[2]; /* Pipe used to synchronize parent and child */
};

int childMain(void *arg);

#endif