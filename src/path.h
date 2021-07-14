#ifndef __PATH_H
#define __PATH_H

char *getConfigFilePath(char *image, char *tag, char *imageDigest);

char *getManifestFilePath(char *image, char *tag);

char *getLayerTarFilePath(char *layerId);

char *getLayerJsonFilePath(char *layerId);

#endif