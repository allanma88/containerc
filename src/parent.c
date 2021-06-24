#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>

#include "cust_dir.h"
#include "config.h"
#include "parent.h"
#include "child.h"
#include "cio.h"
#include "log.h"

static int computeCloneFlags(linuxConfig *Linux)
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

static int updateIdMap(idMappingConfig *idMappings, int idMappingLen, char *map_file)
{
    int fd = open(map_file, O_RDWR);
    if (fd == -1)
    {
        logError("open %s error", map_file);
        return -1;
    }

    int len = 0;
    int n = 0;
    int nums[idMappingLen];
    idMappingConfig *idMapping = idMappings;
    for (int i = 0; i < idMappingLen; i++)
    {
        nums[i] = snprintf(NULL, 0, "%d %d %d\n", idMapping->containerId, idMapping->hostId, idMapping->size);
        len += nums[i];
        idMapping++;
    }
    char mapping[len + 1];
    mapping[len] = '\0';

    idMapping = idMappings;
    for (int i = 0; i < idMappingLen; i++)
    {
        snprintf(mapping + n, nums[i] + 1, "%d %d %d\n", idMapping->containerId, idMapping->hostId, idMapping->size);
        n += nums[i];
        idMapping++;
    }
    if (write(fd, mapping, len) != len)
    {
        logError("write to %s error", map_file);
        return -1;
    }
    close(fd);
    return 0;
}

static int updateSetgroups(pid_t child_pid, char *str)
{
    char setgroups_path[PATH_MAX];
    int fd;

    snprintf(setgroups_path, PATH_MAX, "/proc/%jd/setgroups", (intmax_t)child_pid);

    fd = open(setgroups_path, O_RDWR);
    if (fd == -1 && errno != ENOENT)
    {
        /* We may be on a system that doesn't support
        /proc/PID/setgroups. In that case, the file won't exist,
        and the system won't impose the restrictions that Linux 3.19
        added. That's fine: we don't need to do anything in order
        to permit 'gid_map' to be updated.

        However, if the error from open() was something other than
        the ENOENT error that is expected for that case,  let the
        user know. */
        logError("open %s error", setgroups_path);
        return -1;
    }

    if (write(fd, str, strlen(str)) == -1)
    {
        logError("write %s error", setgroups_path);
        return -1;
    }

    close(fd);
    return 0;
}

static int runUserNamespace(int childPid, containerConfig *config)
{
    linuxConfig *Linux = config->Linux;
    char map_path[PATH_MAX];
    if (Linux->uidMappingsLen > 0)
    {
        snprintf(map_path, PATH_MAX, "/proc/%jd/uid_map", (intmax_t)childPid);
        if(updateIdMap(Linux->uidMappings, Linux->uidMappingsLen, map_path) < 0)
        {
            return -1;
        }
    }
    if (Linux->gidMappingsLen > 0)
    {
        if(updateSetgroups(childPid, "deny") < 0)
        {
            return -1;
        }
        snprintf(map_path, PATH_MAX, "/proc/%jd/gid_map", (intmax_t)childPid);
        if(updateIdMap(Linux->gidMappings, Linux->gidMappingsLen, map_path) < 0)
        {
            return -1;
        }
    }
    return 0;
}

static int runCgroupNamespace(int childPid, linuxConfig *Linux)
{
    char *cgroupsPath = Linux->cgroupsPath;
    cpuConfig *cpu = Linux->resource->cpu;
    int cgroupsPathLen = strlen(cgroupsPath);
    char *cgroupsRootPath = "/sys/fs/cgroup/cpu";
    int cgroupsRootPathLen = strlen(cgroupsRootPath);

    char cgroupsFullPath[cgroupsPathLen + cgroupsRootPathLen + 1];
    sprintf(cgroupsFullPath, "%s%s", cgroupsRootPath, cgroupsPath);
    if (mkdirRecur(cgroupsFullPath) < 0)
    {
        logError("mkdirRecur %s error", cgroupsFullPath);
        return -1;
    }

    char *cpuShares = "cpu.shares";
    char cpuSharesFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuShares) + 2];
    sprintf(cpuSharesFilePath, "%s/%s", cgroupsFullPath, cpuShares);
    if (writeInt(cpuSharesFilePath, cpu->shares) < 0)
    {
        logError("write %d to %s error", cpu->shares, cpuSharesFilePath);
        return -1;
    }

    char *cpuPeriod = "cpu.cfs_period_us";
    char cpuPeriodFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuPeriod) + 2];
    sprintf(cpuPeriodFilePath, "%s/%s", cgroupsFullPath, cpuPeriod);
    if (writeInt(cpuPeriodFilePath, cpu->period) < 0)
    {
        logError("write %d to %s error", cpu->period, cpuPeriodFilePath);
        return -1;
    }

    char *cpuQuota = "cpu.cfs_quota_us";
    char cpuQuotaFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuQuota) + 2];
    sprintf(cpuQuotaFilePath, "%s/%s", cgroupsFullPath, cpuQuota);
    if (writeInt(cpuQuotaFilePath, cpu->quota) < 0)
    {
        logError("write %d to %s error", cpu->quota, cpuQuotaFilePath);
        return -1;
    }

    char *tasks = "tasks";
    char tasksFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(tasks) + 2];
    sprintf(tasksFilePath, "%s/%s", cgroupsFullPath, tasks);
    if (writeInt(tasksFilePath, childPid) < 0)
    {
        logError("write %d to %s error", childPid, tasksFilePath);
        return -1;
    }

    char *cgroupProcs = "cgroup.procs";
    char cgroupProcsFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cgroupProcs) + 2];
    sprintf(cgroupProcsFilePath, "%s/%s", cgroupsFullPath, cgroupProcs);
    if (writeInt(cgroupProcsFilePath, childPid) < 0)
    {
        logError("write %d to %s error", childPid, cgroupProcsFilePath);
        return -1;
    }
    return 0;
}

int parentRun(cloneArgs *cArgs)
{
    cArgs->cloneFlags = computeCloneFlags(cArgs->config->Linux);
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, cArgs->sync_child_pipe) < 0)
    {
        logError("child pipe error");
        return -1;
    }

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, cArgs->sync_grandchild_pipe) < 0)
    {
        logError("grand child pipe error");
        return -1;
    }

    int childPid = clone(childMain, child_stack + STACK_SIZE, (cArgs->cloneFlags & CLONE_NEWUSER) | SIGCHLD, cArgs);
    if (childPid == -1)
    {
        logError("clone error");
        return -1;
    }

    close(cArgs->sync_child_pipe[0]);

    if (cArgs->cloneFlags | CLONE_NEWUSER)
    {
        if(runUserNamespace(childPid, cArgs->config) < 0)
        {
            return -1;
        }
    }
    if (cArgs->cloneFlags | CLONE_NEWCGROUP)
    {
        if(runCgroupNamespace(childPid, cArgs->config->Linux) < 0)
        {
            return -1;
        }
    }

    if (writeInt1(cArgs->sync_child_pipe[1], PARENTOK) < 0)
    {
        logError("parent write PARENTOK to sync_child_pipe error");
        return -1;
    }

    int grandChildPid;
    if ((grandChildPid = readInt1(cArgs->sync_child_pipe[1])) < 0)
    {
        logError("parent read grandChildPid from sync_child_pipe error");
        return -1;
    }
    close(cArgs->sync_grandchild_pipe[0]);

    int msg;
    if ((msg = readInt1(cArgs->sync_grandchild_pipe[1])) != CREATERUNTIME)
    {
        logError("parent read %d from sync_grandchild_pipe is not CREATERUNTIME", msg);
        return -1;
    }

    system("/mnt/d/OpenSource/containerc/src/nssetup.sh");

    if (writeInt1(cArgs->sync_grandchild_pipe[1], CREATERUNTIMERESP) < 0)
    {
        logError("parent write CREATERUNTIMERESP to sync_grandchild_pipe error");
        return -1;
    }

    close(cArgs->sync_child_pipe[1]);
    close(cArgs->sync_grandchild_pipe[1]);

    if (waitpid(childPid, NULL, 0) == -1)
    {
        logError("waitpid error");
        return -1;
    }
    return 0;
}