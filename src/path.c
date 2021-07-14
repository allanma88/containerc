#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "cstr.h"

char *getConfigFilePath(char *image, char *tag, char *imageDigest)
{
    char *imageId = substr(imageDigest, ':');
    char *base = "/var/lib/ctrc/images";
    char *configFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(configFile, "%s/%s/%s/%s.json", base, image, tag, imageId);
    return configFile;
}

char *getManifestFilePath(char *image, char *tag)
{
    char *base = "/var/lib/ctrc/images";
    char *manifestFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(manifestFile, "%s/%s/%s/manifest.json", base, image, tag);
    return manifestFile;
}

char *getLayerTarFilePath(char *layerId)
{
    char *layDirBase = "/var/lib/ctrc/layers";
    char *layerTarFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerTarFile, "%s/%s/layer.tar", layDirBase, layerId);
    return layerTarFile;
}

char *getLayerJsonFilePath(char *layerId)
{
    char *layDirBase = "/var/lib/ctrc/layers";
    char *layerJsonFile = (char *)calloc(PATH_MAX, sizeof(char));
    sprintf(layerJsonFile, "%s/%s/json", layDirBase, layerId);
    return layerJsonFile;
}
