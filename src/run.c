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
    int i = n-1;
    cJSON_ArrayForEach(layerJson, layersJson)
    {
        char *layer = layerJson->valuestring;
        char *layerId = strtok(layer, "/");
        layerPaths[i--] = getRuntimeLayerPath(layerId);
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
    char *realRootPath = absDirPath(rootPath);
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

static cJSON *getImageConfigJson(char *image, char *tag)
{
    char *manifestFilePath = getManifestFilePath(image, tag);
    char *manifest = readtoend(manifestFilePath, "rb");
    cJSON *manifestsJson = cJSON_Parse(manifest);
    cJSON *manifestJson = cJSON_GetArrayItem(manifestsJson, 0);
    char *configVal = parseStr(manifestJson, "Config");
    char *configDigest = strtok(configVal, ".");
    char *configFilePath = getConfigFilePath(image, tag, configDigest);
    char *config = readtoend(configFilePath, "rb");
    cJSON *configJson = cJSON_Parse(config);
    cJSON *containerConfigJson = cJSON_GetObjectItemCaseSensitive(configJson, "config");
    return containerConfigJson;
}

static void merge(containerConfig *config, cJSON *containerConfigJson)
{
    char **env;
    int envc = parseStrArray(containerConfigJson, "Env", &env);
    if (envc > 0)
    {
        free(config->process->env);
        config->process->env = env;
        config->process->envc = envc;
    }

    char *cwd = parseStr(containerConfigJson, "WorkingDir");
    if (cwd != NULL)
    {
        config->process->cwd = cwd;
    }

    char **entrypoint;
    char **cmd;
    int entrypointLen = parseStrArray(containerConfigJson, "Entrypoint", &entrypoint);
    int cmdLen = parseStrArray(containerConfigJson, "Cmd", &cmd);
    if (entrypointLen > 0 && cmdLen > 0)
    {
        free(config->process->args);
        char **newargs = (char **)calloc(entrypointLen + cmdLen, sizeof(char *));
        for (int i = 0; i < entrypointLen; i++)
        {
            newargs[i] = entrypoint[i];
        }
        for (int i = 0; i < cmdLen; i++)
        {
            newargs[entrypointLen + i] = cmd[i];
        }
        config->process->args = newargs;
        config->process->argc = entrypointLen + cmdLen;
    }
    else if (entrypointLen > 0)
    {
        free(config->process->args);
        config->process->args = entrypoint;
        config->process->argc = entrypointLen;
    }
    else if (cmdLen > 0)
    {
        free(config->process->args);
        config->process->args = cmd;
        config->process->argc = cmdLen;
    }
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
    cJSON *containerConfigJson = getImageConfigJson(image, tag);
    merge(config, containerConfigJson);
    if (runContainer(config, rootPath) < 0)
    {
        return NULL;
    }
    return containerId;
}