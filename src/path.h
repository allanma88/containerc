#ifndef __PATH_H
#define __PATH_H

char *getConfigFilePath(char *image, char *tag, char *imageDigest);

char *getManifestFilePath(char *image, char *tag);

char *getLayerTarFilePath(char *layerId);

char *getLayerJsonFilePath(char *layerId);

char *getRuntimeLayerPath(char *layerId);

char *getRuntimeConfigFilePath(char *layerId);

char *getContainerBasePath(char *containerId);

char *getContainerRootfsPath(char *containerId);

char *getContainerLayerPath(char *containerId);

char *getContainerWorkerPath(char *containerId);

#endif