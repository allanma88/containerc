#define _GNU_SOURCE
#include <stdio.h>

#include "cstr.h"
#include "config.h"
#include "print.h"
#include "log.h"
#include "cio.h"

static void mkdirRecur_test()
{
    char *path = "./mycontainer/rootfs/dev/pts";
    if (mkdirRecur(path) < 0)
    {
        logError("mkdirRecur %s error\n", path);
    }
    else
    {
        printf("mkdirRecur done\n");
    }
}

static void randomstr_test()
{
    char *guid = randomstr();
    printf("new guid is %s\n", guid);
}

static void hash_test()
{
    char *h = hash("1");
    printf("hash value of 1 is %s\n", h);
}

static void join_test()
{
    char *str = join(":", 2, "lower1/", "lower2/");
    printf("%s\n", str);

    str = join(":", 1, "lower1/");
    printf("%s\n", str);

    char *strs[2];
    strs[0] = "lower1/";
    strs[1] = "lower2/";
    str = join1(":", 2, strs);
    printf("%s\n", str);

    char *strs1[1];
    strs1[0] = "lower1/";
    str = join1(":", 1, strs1);
    printf("%s\n", str);
}

static void makeDefaultConfig_test()
{
    containerConfig *container = makeDefaultConfig();
    printConfig(container);
}

static void format_test()
{
    int grandChildPid = 976;
    char *rootPath = "/var/lib";
    char *childPidEnv = format("ChildPid=%d", grandChildPid);
    char *containerBaseEnv = format("ContainerBase=%s", rootPath);
    printf("childPidEnv: %s\n", childPidEnv);
    printf("containerBaseEnv: %s\n", containerBaseEnv);
}

static void absExePath_test()
{
    char *absPath = absExePath("sh");
    printf("sh absolute path is %s\n", absPath);
}

int main(int argc, char **argv)
{
    // mkdirRecur_test();
    // randomstr_test();
    // hash_test();
    // join_test();
    // makeDefaultConfig_test();
    // format_test();
    // absExePath_test();
}