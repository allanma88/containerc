#include <stdlib.h>

#include "config.h"
#include "cstr.h"

static userConfig *makeUserConfig()
{
    userConfig *user = (userConfig *)calloc(1, sizeof(userConfig));
    user->uid = 0;
    user->gid = 0;
    return user;
}

static processConfig *makeProcessConfig()
{
    processConfig *process = (processConfig *)calloc(1, sizeof(processConfig));

    int argc = 1;
    char **args = calloc(argc, sizeof(char *));
    args[0] = "/bin/sh";
    process->args = args;
    process->argc = argc;

    int envc = 2;
    char **env = calloc(envc, sizeof(char *));
    env[0] = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
    env[1] = "TERM=xterm";
    process->envc = envc;
    process->env = env;

    process->cwd = "/";
    process->user = makeUserConfig();

    return process;
}

static rootConfig *makeRootConfig()
{
    rootConfig *root = (rootConfig *)calloc(1, sizeof(rootConfig));
    root->path = "rootfs";
    root->readonly = 1;
    return root;
}

static hookConfig *makeHookConfig(int n)
{
    hookConfig *hooks = (hookConfig *)calloc(n, sizeof(hookConfig));
    hooks[0].path = "/mnt/d/OpenSource/containerc/src/nssetup.sh";
    return hooks;
}

static hooksConfig *makeHooksConfig()
{
    hooksConfig *hooks = (hooksConfig *)calloc(1, sizeof(hooksConfig));
    int createRuntimeLen = 1;
    hooks->createRuntimeLen = createRuntimeLen;
    hooks->createRuntime = makeHookConfig(createRuntimeLen);
    return hooks;
}

static namespaceConfig *makeNamespaceConfig(int n)
{
    namespaceConfig *namespaces = (namespaceConfig *)calloc(n, sizeof(namespaceConfig));
    namespaces[0].type = "pid";
    namespaces[1].type = "network";
    namespaces[2].type = "ipc";
    namespaces[3].type = "uts";
    namespaces[4].type = "mount";
    namespaces[5].type = "user";
    namespaces[6].type = "cgroup";
    return namespaces;
}

static idMappingConfig *makeIdMappingConfig(int n, long **idmappings)
{
    idMappingConfig *mappings = (idMappingConfig *)calloc(n, sizeof(idMappingConfig));
    for (int i = 0; i < n; i++)
    {
        mappings[i].containerId = (int)idmappings[i][0];
        mappings[i].hostId = (int)idmappings[i][1];
        mappings[i].size = idmappings[i][2];
    }
    return mappings;
}

static resourceConfig *makeResourceConfig()
{
    resourceConfig *resource = (resourceConfig *)calloc(1, sizeof(resourceConfig));
    
    resource->cpu = (cpuConfig *)calloc(1, sizeof(cpuConfig));
    resource->cpu->shares = 1024;
    resource->cpu->quota = 1000000;
    resource->cpu->period = 500000;

    resource->memory = (memoryConfig *)calloc(1, sizeof(memoryConfig));

    return resource;
}

static linuxConfig *makeLinuxConfig()
{
    linuxConfig *Linux = (linuxConfig *)calloc(1, sizeof(linuxConfig));

    int namespacelen = 7;
    Linux->namespaceLen = namespacelen;
    Linux->namespaces = makeNamespaceConfig(namespacelen);

    int uidmappingslen = 1;
    Linux->uidMappingsLen = uidmappingslen;
    long *uidmappings[uidmappingslen];
    long uidmapping[3] = {0, 0, 4294967295};
    uidmappings[0] = uidmapping;
    Linux->uidMappings = makeIdMappingConfig(uidmappingslen, uidmappings);

    int gidmappingslen = 1;
    Linux->gidMappingsLen = gidmappingslen;
    long *gidmappings[gidmappingslen];
    long gidmapping[3] = {0, 0, 4294967295};
    gidmappings[0] = gidmapping;
    Linux->gidMappings = makeIdMappingConfig(gidmappingslen, gidmappings);

    Linux->cgroupsPath = "/myRuntime";
    Linux->resource = makeResourceConfig();

    return Linux;
}

static mountConfig *makeMountsConfig(int mountslen)
{
    mountConfig *mounts = (mountConfig *)calloc(mountslen, sizeof(mountConfig));

    mounts[0].destination = "/proc";
    mounts[0].type = "proc";
    mounts[0].source = "proc";

    mounts[1].destination = "/dev";
    mounts[1].type = "tmpfs";
    mounts[1].source = "tmpfs";
    mounts[1].optionsLen = 4;
    mounts[1].options = (char **)calloc(4, sizeof(char *));
    mounts[1].options[0] = "nosuid";
    mounts[1].options[1] = "strictatime";
    mounts[1].options[2] = "mode=755";
    mounts[1].options[3] = "size=65536k";

    mounts[2].destination = "/dev/pts";
    mounts[2].type = "devpts";
    mounts[2].source = "devpts";
    mounts[2].optionsLen = 5;
    mounts[2].options = (char **)calloc(5, sizeof(char *));
    mounts[2].options[0] = "nosuid";
    mounts[2].options[1] = "noexec";
    mounts[2].options[2] = "newinstance";
    mounts[2].options[3] = "ptmxmode=0666";
    mounts[2].options[4] = "mode=0620";

    mounts[3].destination = "/dev/shm";
    mounts[3].type = "tmpfs";
    mounts[3].source = "shm";
    mounts[3].optionsLen = 5;
    mounts[3].options = (char **)calloc(5, sizeof(char *));
    mounts[3].options[0] = "nosuid";
    mounts[3].options[1] = "noexec";
    mounts[3].options[2] = "nodev";
    mounts[3].options[3] = "mode=1777";
    mounts[3].options[4] = "size=65536k";

    mounts[4].destination = "/dev/mqueue";
    mounts[4].type = "mqueue";
    mounts[4].source = "mqueue";
    mounts[4].optionsLen = 3;
    mounts[4].options = (char **)calloc(3, sizeof(char *));
    mounts[4].options[0] = "nosuid";
    mounts[4].options[1] = "noexec";
    mounts[4].options[2] = "nodev";

    mounts[5].destination = "/sys";
    mounts[5].type = "sysfs";
    mounts[5].source = "sysfs";
    mounts[5].optionsLen = 4;
    mounts[5].options = (char **)calloc(4, sizeof(char *));
    mounts[5].options[0] = "nosuid";
    mounts[5].options[1] = "noexec";
    mounts[5].options[2] = "nodev";
    mounts[5].options[3] = "ro";

    mounts[6].destination = "/sys/fs/cgroup";
    mounts[6].type = "cgroup";
    mounts[6].source = "cgroup";
    mounts[6].optionsLen = 5;
    mounts[6].options = (char **)calloc(5, sizeof(char *));
    mounts[6].options[0] = "nosuid";
    mounts[6].options[1] = "noexec";
    mounts[6].options[2] = "nodev";
    mounts[6].options[3] = "relatime";
    mounts[6].options[4] = "ro";

    return mounts;
}

containerConfig *makeDefaultConfig()
{
    int mountslen = 7;
    containerConfig *container = (containerConfig *)calloc(1, sizeof(containerConfig));
    container->ociVersion = "1.0.2-dev";
    container->process = makeProcessConfig();
    container->root = makeRootConfig();
    container->hostname = "runc";
    container->mounts = makeMountsConfig(mountslen);
    container->mountslen = mountslen;
    container->hooks = makeHooksConfig();
    container->Linux = makeLinuxConfig();
    return container;
}

void freeConfig(containerConfig *config)
{
    if (config == NULL)
    {
        return;
    }
    //free the field of config
    free(config);
}