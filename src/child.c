#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "cust_dir.h"
#include "config.h"
#include "child.h"
#include "cio.h"
#include "log.h"

unsigned long int computeMountFlags(char **options, int optionsLen)
{
    unsigned long int flags = 0;
    for (int i = 0; i < optionsLen; i++)
    {
        if (!strncmp(options[i], "nosuid", 6))
        {
            flags |= MS_NOSUID;
        }
        else if (!strncmp(options[i], "strictatime", 11))
        {
            flags |= MS_STRICTATIME;
        }
        else if (!strncmp(options[i], "noexec", 6))
        {
            flags |= MS_NOEXEC;
        }
        else if (!strncmp(options[i], "nodev", 6))
        {
            flags |= MS_NODEV;
        }
        else if (!strncmp(options[i], "ro", 6))
        {
            flags |= MS_RDONLY;
        }
        else if (!strncmp(options[i], "relatime", 6))
        {
            flags |= MS_RELATIME;
        }
        else if (!strncmp(options[i], "rbind", 5))
        {
            flags |= MS_REC | MS_BIND;
        }
    }
    return flags;
}

static int runMountNamespace(containerConfig *config)
{
    mountConfig *mnt = config->mounts;

    for (int i = 0; i < config->mountslen; i++)
    {
        int mountFlags = computeMountFlags(mnt->options, mnt->optionsLen);
        if (mkdirRecur(mnt->destination) < 0)
        {
            logError("mkdirRecur %s error", mnt->destination);
            return -1;
        }
        if (!strncmp(mnt->type, "cgroup", 5))
        {
            if (mount("tmpfs", mnt->destination, "tmpfs", MS_NOSUID | MS_NOEXEC | MS_RELATIME, "mode=755") < 0)
            {
                logError("mount tmpfs %s tmpfs error", mnt->destination);
                return -1;
            }
            int len = strlen(mnt->destination);
            char cpuPath[len + 5];
            sprintf(cpuPath, "%s/cpu", mnt->destination);
            if (mkdirRecur(cpuPath) < 0)
            {
                logError("mkdirRecur %s error", cpuPath);
                return -1;
            }
            if (mount(mnt->source, cpuPath, mnt->type, mountFlags, "cpu") < 0)
            {
                logError("mount %s %s %s error", mnt->source, cpuPath, mnt->type);
                return -1;
            }
            char memoryPath[len + 7];
            sprintf(memoryPath, "%s/memory", mnt->destination);
            if (mkdirRecur(memoryPath) < 0)
            {
                logError("mkdirRecur %s error", memoryPath);
                return -1;
            }
            if (mount(mnt->source, memoryPath, mnt->type, mountFlags, "memory") < 0)
            {
                logError("mount %s %s %s error", mnt->source, memoryPath, mnt->type);
                return -1;
            }
        }
        else
        {
            if (mount(mnt->source, mnt->destination, mnt->type, mountFlags, NULL) < 0)
            {
                logError("mount %s %s %s error", mnt->source, mnt->destination, mnt->type);
                return -1;
            }
        }
        mnt++;
    }
    return 0;
}

static int runUser(userConfig *user)
{
    if (setuid(user->uid) < 0)
    {
        logError("set uid error");
        return -1;
    }
    if (seteuid(user->uid) < 0)
    {
        logError("set euid error");
        return -1;

    }
    if (setgid(user->gid) < 0)
    {
        logError("set gid error");
        return -1;
    }
    if (setegid(user->gid) < 0)
    {
        logError("set egid error");
        return -1;
    }
    return 0;
}

static int runProcess(processConfig *process)
{
    chdir(process->cwd);
    //todo: change to absolute path of exe
    if (execve(process->args[0], process->args, process->env) < 0)
    {
        logError("execute %s in child error", process->args[0]);
        return -1;
    }
}

static int pivotRoot(const char *new_root, const char *put_old)
{
    return syscall(SYS_pivot_root, new_root, put_old);
}

static int runRoot(rootConfig *root)
{
    // char *curDir = get_current_dir_name();
    // int curDirSize = strlen(curDir);
    // char rootDir[strlen(curDir) + 1 + strlen(root->path) + 1];
    // sprintf(rootDir, "%s/%s", curDir, root->path);
    // if (chroot(rootDir) < 0)
    // {
    //     logError("chroot error");
    //     return -1;
    // }
    // return 0;

    char *new_root = root->path;
    const char *put_old = "/oldrootfs";
    char path[PATH_MAX];

    /* Ensure that 'new_root' and its parent mount don't have
              shared propagation (which would cause pivot_root() to
              return an error), and prevent propagation of mount
              events to the initial mount namespace. */
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1)
    {
        logError("mount-MS_PRIVATE error");
        return -1;
    }

    /* Ensure that 'new_root' is a mount point. */
    if (mount(new_root, new_root, NULL, MS_BIND, NULL) == -1)
    {
        logError("mount-MS_BIND error");
        return -1;
    }

    /* Create directory to which old root will be pivoted. */
    snprintf(path, sizeof(path), "%s/%s", new_root, put_old);
    if (mkdir(path, 0777) == -1)
    {
        logError("mkdir error");
        return -1;
    }

    /* And pivot the root filesystem. */
    if (pivotRoot(new_root, path) == -1)
    {
        logError("pivot_root error");
        return -1;
    }

    /* Switch the current working directory to "/". */
    if (chdir("/") == -1)
    {
        logError("chdir error");
        return -1;
    }

    /* Unmount old root and remove mount point. */
    if (umount2(put_old, MNT_DETACH) == -1)
    {
        logError("umount2 error");
        return -1;
    }
    if (rmdir(put_old) == -1)
    {
        logError("rmdir error");
        return -1;
    }
    return 0;
}

int childChildMain(void *arg)
{
    cloneArgs *cArgs = (struct cloneArgs *)arg;
    containerConfig *config = cArgs->config;

    close(cArgs->sync_child_pipe[0]);
    close(cArgs->sync_child_pipe[1]);
    close(cArgs->sync_grandchild_pipe[1]);

    if (cArgs->cloneFlags | CLONE_NEWNS)
    {
        if(runMountNamespace(config) < 0)
        {
            return -1;
        }
    }

    if (sethostname(config->hostname, strlen(config->hostname)) == -1)
    {
        logError("set hostname error");
        return -1;
    }

    if (runRoot(config->root) < 0)
    {
        return -1;
    }

    if(runUser(config->process->user) < 0)
    {
        return -1;
    }

    if (writeInt1(cArgs->sync_grandchild_pipe[0], CREATERUNTIME) < 0)
    {
        logError("grandchild write CREATERUNTIME to sync_grandchild_pipe[%d] error", cArgs->sync_grandchild_pipe[0]);
        return -1;
    }

    int msg;
    if ((msg = readInt1(cArgs->sync_grandchild_pipe[0])) != CREATERUNTIMERESP)
    {
        logError("grandchild read %d from sync_grandchild_pipe is not CREATERUNTIMERESP error", msg);
        return -1;
    }
    close(cArgs->sync_grandchild_pipe[0]);

    if(runProcess(config->process) < 0)
    {
        return -1;
    }

    printf("child child done\n");
}

int childMain(void *arg)
{
    cloneArgs *cArgs = (cloneArgs *)arg;
    containerConfig *config = cArgs->config;
    int cloneFlags = cArgs->cloneFlags;

    close(cArgs->sync_child_pipe[1]);

    int msg;
    if ((msg = readInt1(cArgs->sync_child_pipe[0])) != PARENTOK)
    {
        logError("child read %d from sync_child_pipe is not PARENTOK", msg);
        return -1;
    }

    int grandChildPid = clone(childChildMain, child_stack + STACK_SIZE, (cloneFlags & ~CLONE_NEWUSER) | SIGCHLD, cArgs);
    if (grandChildPid == -1)
    {
        logError("clone error");
        return -1;
    }

    close(cArgs->sync_grandchild_pipe[0]);
    close(cArgs->sync_grandchild_pipe[1]);

    if (writeInt1(cArgs->sync_child_pipe[0], grandChildPid) < 0)
    {
        logError("child write childPid=%d to sync_child_pipe error", grandChildPid);
        return -1;
    }

    close(cArgs->sync_child_pipe[0]);

    if (waitpid(grandChildPid, NULL, 0) < 0)
    {
        logError("waitpid error");
        return -1;
    }

    printf("child done\n");
    return 0;
}