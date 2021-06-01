#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "cust_dir.h"
#include "config.h"
#include "child.h"

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

static unsigned long int computeMountFlags(char **options, int optionsLen)
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
            fprintf(stderr, "mkdirRecur %s error: %s\n", mnt->destination, strerror(errno));
        }
        if (!strncmp(mnt->type, "cgroup", 5))
        {
            if (mount("tmpfs", mnt->destination, "tmpfs", MS_NOSUID | MS_NOEXEC | MS_RELATIME, "mode=755") < 0)
            {
                fprintf(stderr, "mount tmpfs %s tmpfs error: %s\n", mnt->destination, strerror(errno));
            }
            int len = strlen(mnt->destination);
            char cpuPath[len + 5];
            sprintf(cpuPath, "%s/cpu", mnt->destination);
            if (mkdirRecur(cpuPath) < 0)
            {
                fprintf(stderr, "mkdirRecur %s error: %s\n", cpuPath, strerror(errno));
            }
            if (mount(mnt->source, cpuPath, mnt->type, mountFlags, "cpu") < 0)
            {
                fprintf(stderr, "mount %s %s %s error: %s\n", mnt->source, cpuPath, mnt->type, strerror(errno));
            }
            char memoryPath[len + 7];
            sprintf(memoryPath, "%s/memory", mnt->destination);
            if (mkdirRecur(memoryPath) < 0)
            {
                fprintf(stderr, "mkdirRecur %s error: %s\n", memoryPath, strerror(errno));
            }
            if (mount(mnt->source, memoryPath, mnt->type, mountFlags, "memory") < 0)
            {
                fprintf(stderr, "mount %s %s %s error: %s\n", mnt->source, memoryPath, mnt->type, strerror(errno));
            }
        }
        else
        {
            if (mount(mnt->source, mnt->destination, mnt->type, mountFlags, NULL) < 0)
            {
                fprintf(stderr, "mount %s %s %s error: %s\n", mnt->source, mnt->destination, mnt->type, strerror(errno));
            }
        }
        mnt++;
    }
}

static int runUser(userConfig *user)
{
    if (setuid(user->uid) < 0)
    {
        fprintf(stderr, "set uid error: %s\n", strerror(errno));
    }
    if (seteuid(user->uid) < 0)
    {
        fprintf(stderr, "set euid error: %s\n", strerror(errno));
    }
    if (setgid(user->gid) < 0)
    {
        fprintf(stderr, "set gid error: %s\n", strerror(errno));
    }
    if (setegid(user->gid) < 0)
    {
        fprintf(stderr, "set egid error: %s\n", strerror(errno));
    }
}

static int runProcess(processConfig *process)
{
    chdir(process->cwd);
    //todo: change to absolute path of exe
    if (execve(process->args[0], process->args, process->env) < 0)
    {
        fprintf(stderr, "execute %s in child error: %s\n", process->args[0], strerror(errno));
        return -1;
    }
}

static int pivot_root(const char *new_root, const char *put_old)
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
    //     fprintf(stderr, "chroot error: %s\n", strerror(errno));
    // }

    char *new_root = root->path;
    const char *put_old = "/oldrootfs";
    char path[PATH_MAX];

    /* Ensure that 'new_root' and its parent mount don't have
              shared propagation (which would cause pivot_root() to
              return an error), and prevent propagation of mount
              events to the initial mount namespace. */

    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1)
        fprintf(stderr, "mount-MS_PRIVATE error: %s\n", strerror(errno));

    /* Ensure that 'new_root' is a mount point. */

    if (mount(new_root, new_root, NULL, MS_BIND, NULL) == -1)
        fprintf(stderr, "mount-MS_BIND error: %s\n", strerror(errno));

    /* Create directory to which old root will be pivoted. */

    snprintf(path, sizeof(path), "%s/%s", new_root, put_old);
    if (mkdir(path, 0777) == -1)
        fprintf(stderr, "mkdir error: %s\n", strerror(errno));

    /* And pivot the root filesystem. */

    if (pivot_root(new_root, path) == -1)
        fprintf(stderr, "pivot_root error: %s\n", strerror(errno));

    /* Switch the current working directory to "/". */

    if (chdir("/") == -1)
        fprintf(stderr, "chdir error: %s\n", strerror(errno));

    /* Unmount old root and remove mount point. */

    if (umount2(put_old, MNT_DETACH) == -1)
        fprintf(stderr, "umount2 error: %s\n", strerror(errno));
    if (rmdir(put_old) == -1)
        fprintf(stderr, "rmdir error: %s\n", strerror(errno));
}

int childChildMain(void *arg)
{
    struct cloneArgs *cloneArgs = (struct cloneArgs *)arg;
    containerConfig *config = cloneArgs->config;
    
    if (sethostname(config->hostname, strlen(config->hostname)) == -1)
    {
        fprintf(stderr, "set hostname error: %s\n", strerror(errno));
    }

    runRoot(config->root);

    runUser(config->process->user);

    if (cloneArgs->cloneFlags | CLONE_NEWNS)
    {
        runMountNamespace(config);
    }

    runProcess(config->process);

    printf("child child done\n");
}

int childMain(void *arg)
{
    struct cloneArgs *cloneArgs = (struct cloneArgs *)arg;
    containerConfig *config = cloneArgs->config;
    char ch;

    /* Wait until the parent has updated the UID and GID mappings.
       See the comment in main(). We wait for end of file on a
       pipe that will be closed by the parent process once it has
       updated the mappings. */

    /* Close our descriptor for the write end of the pipe so that we see EOF when parent closes its descriptor. */
    close(cloneArgs->pipe_fd[1]);
    if (read(cloneArgs->pipe_fd[0], &ch, 1) != 0)
    {
        fprintf(stderr, "Failure in child: read from pipe returned != 0\n");
        return -1;
    }

    close(cloneArgs->pipe_fd[0]);

    struct cloneArgs childCloneArgs = {.config = cloneArgs->config, .cloneFlags = cloneArgs->cloneFlags};
    int childPid = clone(childChildMain, child_stack + STACK_SIZE, (cloneArgs->cloneFlags & ~CLONE_NEWUSER) | SIGCHLD, &childCloneArgs);
    if (childPid == -1)
    {
        printf("clone error: %s\n", strerror(errno));
        return -1;
    }

    if (waitpid(childPid, NULL, 0) == -1)
    {
        printf("waitpid error: %s\n", strerror(errno));
        return -1;
    }

    printf("child done\n");
    return 0;
}