#include <stdio.h>

#include "config.h"
#include "print.h"
#include "cstr.h"

static void printProcessConfig(processConfig *process)
{
    printf("================Process============\n");
    printf("args: ");
    for (int i = 0; i < process->argc; i++)
    {
        printf("%s ", process->args[i]);
    }
    printf("\n");
    printf("env: ");
    for (int i = 0; i < process->envc; i++)
    {
        printf("%s ", process->env[i]);
    }
    printf("\n");
}

static void printRootConfig(rootConfig *root)
{
    printf("================Root============\n");
    printf("path: %s, readonly: %d\n", root->path, root->readonly);
}

static void printMountsConfig(mountConfig *mounts, int mountsLen)
{
    printf("================Mounts============\n");
    for (int i = 0; i < mountsLen; i++)
    {
        char *option = join1(" ", mounts[i].optionsLen, mounts[i].options);
        printf("destination=%s, type=%s, source=%s, options: %s\n", mounts[i].destination, mounts[i].type, mounts[i].source, option);
    }
}

static void printHooksConfig(hooksConfig *hooks)
{
    printf("================Hooks============\n");
    printf("createRuntime: \n");
    for (int i = 0; i < hooks->createRuntimeLen; i++)
    {
        printf("path=%s\n", hooks->createRuntime[i].path);
    }
}

static void printIdMappingsConfig(idMappingConfig *idMappings, int idMappingsLen)
{
    for (int i = 0; i < idMappingsLen; i++)
    {
        printf("containerId=%d, hostId=%d, size=%ld\n", idMappings[i].containerId, idMappings[i].hostId, idMappings[i].size);
    }
}

static void printResourceConfig(resourceConfig *resource)
{
    printf("cpu: shares=%d, quota=%d, period=%d\n", resource->cpu->shares, resource->cpu->quota, resource->cpu->period);
}

static void printLinuxConfig(linuxConfig *Linux)
{
    printf("================Linux============\n");
    printf("namespaces: \n");
    for (int i = 0; i < Linux->namespaceLen; i++)
    {
        char *path = Linux->namespaces[i].path != NULL ? Linux->namespaces[i].path : "";
        printf("type=%s path=%s\n", Linux->namespaces[i].type, path);
    }

    printf("uidMappings: \n");
    printIdMappingsConfig(Linux->uidMappings, Linux->uidMappingsLen);

    printf("gidMappings: \n");
    printIdMappingsConfig(Linux->gidMappings, Linux->gidMappingsLen);

    printf("cgroupsPath: %s\n", Linux->cgroupsPath);
    printf("resource: \n");
    printResourceConfig(Linux->resource);
}

void printConfig(containerConfig *config)
{
    printProcessConfig(config->process);
    printRootConfig(config->root);
    printf("hostname: %s\n", config->hostname);
    printMountsConfig(config->mounts, config->mountslen);
    printHooksConfig(config->hooks);
    printLinuxConfig(config->Linux);
}
