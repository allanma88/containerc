#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <sys/mount.h>

#include "run.h"
#include "config.h"
#include "json.h"
#include "print.h"
#include "parent.h"
#include "cio.h"
#include "cstr.h"
#include "path.h"
#include "log.h"

static char *makeRoot(char *image, char *tag, char *containerId)
{
    char *manifestFile = getManifestFilePath(image, tag);
    char *manifest = readtoend(manifestFile, "rb");
    cJSON *rootJson = cJSON_Parse(manifest);
    cJSON *manifestJson = cJSON_GetArrayItem(rootJson, 0);
    cJSON *layersJson = cJSON_GetObjectItemCaseSensitive(manifestJson, "Layers");
    cJSON *layerJson;
    int n = cJSON_GetArraySize(layersJson);
    char *layerPaths[n];
    int i = 0;
    cJSON_ArrayForEach(layerJson, layersJson)
    {
        char *layer = layerJson->valuestring;
        char *layerId = strtok(layer, "/");
        layerPaths[i++] = getRuntimeLayerPath(layerId);
    }

    char *containerImagePath = join1(":", n, layerPaths);
    char *containerLayerPath = getContainerLayerPath(containerId);
    char *containerRootfsPath = getContainerRootfsPath(containerId);
    char *containerWorkerPath = getContainerWorkerPath(containerId);
    mkdirRecur(containerLayerPath);
    mkdirRecur(containerRootfsPath);
    mkdirRecur(containerWorkerPath);

    char option[strlen(containerImagePath) + strlen(containerLayerPath) + strlen(containerWorkerPath) + 13 + 1];
    sprintf(option, "lowerdir=%s,upperdir=%s,workdir=%s", containerImagePath, containerLayerPath, containerWorkerPath);
    if (mount("overlay", containerRootfsPath, "overlay", 0, option) < 0)
    {
        logError("mount overlayfs error, option=%s, dir=%s", option, containerRootfsPath);
        return NULL;
    }
    return getContainerBasePath(containerId);
}

static int runContainer(containerConfig *config, char *rootPath)
{
    cloneArgs *cArgs = (cloneArgs *)malloc(sizeof(cloneArgs));
    cArgs->config = config;
    cArgs->rootPath = rootPath;
    return parentRun(cArgs);
}

int run(char *rootPath)
{
    if (rootPath == NULL)
    {
        return -1;
    }
    char buffer[PATH_MAX];
    char *realRootPath = realpath(rootPath, buffer);
    chdir(realRootPath);

    char *string = readtoend("config.json", "rb");
    if (string == NULL)
    {
        logError("read config error");
        return -1;
    }
    containerConfig *config = deserialize(string);
    if (config == NULL)
    {
        return -1;
    }
    // printConfig(config);
    // return 0;
    return runContainer(config, realRootPath);
}

char *run1(char *image, char *tag, char *entrypoint)
{
    char *containerId = randomstr();
    char *rootPath = makeRoot(image, tag, containerId);
    if (rootPath == NULL)
    {
        return NULL;
    }
    if (chdir(rootPath) < 0)
    {
        return NULL;
    }
    containerConfig *config = makeDefaultConfig();
    if (runContainer(config, rootPath) < 0)
    {
        return NULL;
    }
    return containerId;
}