#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define BASE "/var/lib/ctrc"
#define IMAGES "images"
#define LAYERS "layers"
#define CONTAINERS "containers"
#define RUNTIME "runtime"

char *getConfigFilePath(char *image, char *tag, char *imageId)
{
    char *configFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(configFile, "%s/%s/%s/%s/%s.json", BASE, IMAGES, image, tag, imageId);
    return configFile;
}

char *getManifestFilePath(char *image, char *tag)
{
    char *manifestFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(manifestFile, "%s/%s/%s/%s/manifest.json", BASE, IMAGES, image, tag);
    return manifestFile;
}

char *getLayerTarFilePath(char *layerId)
{
    char *layerTarFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerTarFile, "%s/%s/%s/layer.tar", BASE, LAYERS, layerId);
    return layerTarFile;
}

char *getLayerJsonFilePath(char *layerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/json", BASE, LAYERS, layerId);
    return layerJsonFile;
}

char *getRuntimeLayerPath(char *layerId)
{
    char *runtimeLayerPath = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(runtimeLayerPath, "%s/%s/%s/", BASE, RUNTIME, layerId);
    return runtimeLayerPath;
}


char *getRuntimeConfigFilePath(char *layerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/config.json", BASE, RUNTIME, layerId);
    return layerJsonFile;
}

char *getContainerBasePath(char *containerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/", BASE, CONTAINERS, containerId);
    return layerJsonFile;
}

char *getContainerRootfsPath(char *containerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/rootfs/", BASE, CONTAINERS, containerId);
    return layerJsonFile;
}

char *getContainerLayerPath(char *containerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/layer/", BASE, CONTAINERS, containerId);
    return layerJsonFile;
}

char *getContainerWorkerPath(char *containerId)
{
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/%s/work/", BASE, CONTAINERS, containerId);
    return layerJsonFile;
}
