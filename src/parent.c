#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "cust_dir.h"
#include "config.h"
#include "parent.h"

static int update_map(idMappingConfig *idMappings, int idMappingLen, char *map_file)
{
    int fd = open(map_file, O_RDWR);
    if (fd == -1)
    {
        fprintf(stderr, "ERROR: open %s: %s\n", map_file, strerror(errno));
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
    printf("write '%s' to %s\n", mapping, map_file);
    if (write(fd, mapping, len) != len)
    {
        fprintf(stderr, "ERROR: write to %s: %s\n", map_file, strerror(errno));
        return -1;
    }
    close(fd);
    return 0;
}

static int proc_setgroups_write(pid_t child_pid, char *str)
{
    char setgroups_path[PATH_MAX];
    int fd;

    snprintf(setgroups_path, PATH_MAX, "/proc/%jd/setgroups", (intmax_t)child_pid);

    fd = open(setgroups_path, O_RDWR);
    if (fd == -1)
    {
        /* We may be on a system that doesn't support
        /proc/PID/setgroups. In that case, the file won't exist,
        and the system won't impose the restrictions that Linux 3.19
        added. That's fine: we don't need to do anything in order
        to permit 'gid_map' to be updated.

        However, if the error from open() was something other than
        the ENOENT error that is expected for that case,  let the
        user know. */
        if (errno != ENOENT)
            fprintf(stderr, "ERROR: open %s: %s\n", setgroups_path, strerror(errno));
        return -1;
    }

    if (write(fd, str, strlen(str)) == -1)
        fprintf(stderr, "ERROR: write %s: %s\n", setgroups_path, strerror(errno));

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
        update_map(Linux->uidMappings, Linux->uidMappingsLen, map_path);
    }
    if (Linux->gidMappingsLen > 0)
    {
        proc_setgroups_write(childPid, "deny");
        snprintf(map_path, PATH_MAX, "/proc/%jd/gid_map", (intmax_t)childPid);
        update_map(Linux->gidMappings, Linux->gidMappingsLen, map_path);
    }
}

static int writeInt(char *path, int i)
{
    //need error handler
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        return -1;
    }
    int len = snprintf(NULL, 0, "%d", i);
    char str[len + 1];
    sprintf(str, "%d", i);
    return write(fd, str, len);
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
        fprintf(stderr, "mkdirRecur %s error: %s\n", cgroupsFullPath, strerror(errno));
    }

    char *cpuShares = "cpu.shares";
    char cpuSharesFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuShares) + 2];
    sprintf(cpuSharesFilePath, "%s/%s", cgroupsFullPath, cpuShares);
    if (writeInt(cpuSharesFilePath, cpu->shares) < 0)
    {
        fprintf(stderr, "write %d to %s error: %s\n", cpu->shares, cpuSharesFilePath, strerror(errno));
    }

    char *cpuPeriod = "cpu.cfs_period_us";
    char cpuPeriodFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuPeriod) + 2];
    sprintf(cpuPeriodFilePath, "%s/%s", cgroupsFullPath, cpuPeriod);
    if (writeInt(cpuPeriodFilePath, cpu->period) < 0)
    {
        fprintf(stderr, "write %d to %s error: %s\n", cpu->period, cpuPeriodFilePath, strerror(errno));
    }

    char *cpuQuota = "cpu.cfs_quota_us";
    char cpuQuotaFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cpuQuota) + 2];
    sprintf(cpuQuotaFilePath, "%s/%s", cgroupsFullPath, cpuQuota);
    if (writeInt(cpuQuotaFilePath, cpu->quota) < 0)
    {
        fprintf(stderr, "write %d to %s error: %s\n", cpu->quota, cpuQuotaFilePath, strerror(errno));
    }

    char *tasks = "tasks";
    char tasksFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(tasks) + 2];
    sprintf(tasksFilePath, "%s/%s", cgroupsFullPath, tasks);
    if (writeInt(tasksFilePath, childPid) < 0)
    {
        fprintf(stderr, "write %d to %s error: %s\n", childPid, tasksFilePath, strerror(errno));
    }

    char *cgroupProcs = "cgroup.procs";
    char cgroupProcsFilePath[cgroupsPathLen + cgroupsRootPathLen + strlen(cgroupProcs) + 2];
    sprintf(cgroupProcsFilePath, "%s/%s", cgroupsFullPath, cgroupProcs);
    if (writeInt(cgroupProcsFilePath, childPid) < 0)
    {
        fprintf(stderr, "write %d to %s error: %s\n", childPid, cgroupProcsFilePath, strerror(errno));
    }
}

int parentRun(int cloneFlags, int childPid, containerConfig *config)
{
    if (cloneFlags | CLONE_NEWUSER)
    {
        runUserNamespace(childPid, config);
    }
    if (cloneFlags | CLONE_NEWCGROUP)
    {
        runCgroupNamespace(childPid, config->Linux);
    }
    return 0;
}
