#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <sys/wait.h>

#include "config.h"
#include "parser.h"
#include "print.h"
#include "parent.h"
#include "child.h"
#include "run.h"

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

int computeCloneFlags(linuxConfig *Linux)
{
    int cloneFlags = 0;
    for (int i = 0; i < Linux->namespaceLen; i++)
    {
        namespaceConfig namespace = Linux->namespaces[i];
        if (!strncmp(namespace.type, "pid", 3))
        {
            cloneFlags |= CLONE_NEWPID;
        }
        else if (!strncmp(namespace.type, "ipc", 3))
        {
            cloneFlags |= CLONE_NEWIPC;
        }
        else if (!strncmp(namespace.type, "uts", 3))
        {
            cloneFlags |= CLONE_NEWUTS;
        }
        else if (!strncmp(namespace.type, "mount", 5))
        {
            cloneFlags |= CLONE_NEWNS;
        }
        else if (!strncmp(namespace.type, "user", 4))
        {
            cloneFlags |= CLONE_NEWUSER;
        }
        else if (!strncmp(namespace.type, "cgroup", 6))
        {
            cloneFlags |= CLONE_NEWCGROUP;
        }
        else if (!strncmp(namespace.type, "network", 7))
        {
            cloneFlags |= CLONE_NEWNET;
        }
    }
    return cloneFlags;
}

int run()
{
    struct cloneArgs cloneArgs;
    pid_t pid;
    cloneArgs.config = parse();
    if (cloneArgs.config == NULL)
    {
        return -1;
    }
    // printConfig(cloneArgs.config);
    // return 0;

    if (pipe(cloneArgs.pipe_fd) == -1)
    {
        fprintf(stderr, "ERROR: pipe %s\n", strerror(errno));
        return -1;
    }

    cloneArgs.cloneFlags = computeCloneFlags(cloneArgs.config->Linux);
    int childPid = clone(childMain, child_stack + STACK_SIZE, (cloneArgs.cloneFlags & CLONE_NEWUSER) | SIGCHLD, &cloneArgs);
    if (childPid == -1)
    {
        printf("clone error: %s\n", strerror(errno));
        return -1;
    }

    parentRun(cloneArgs.cloneFlags, childPid, cloneArgs.config);

    close(cloneArgs.pipe_fd[1]);

    if (waitpid(childPid, NULL, 0) == -1)
    {
        printf("waitpid error: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}