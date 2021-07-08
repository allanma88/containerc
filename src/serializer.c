#include <cjson/cJSON.h>

#include "serializer.h"
#include "config.h"

char *serialize(imageManifest *manifest)
{
    cJSON *manifestJson = cJSON_CreateObject();

    cJSON *configJson = cJSON_CreateString(manifest->config);
    cJSON_AddItemToObject(manifestJson, "Config",configJson);

    cJSON *repoTagsJson = cJSON_CreateArray();
    for (int i = 0; i < manifest->repoTagLen; i++)
    {
        cJSON *repoTagJson = cJSON_CreateString(manifest->repoTags[i]);
        cJSON_AddItemToArray(repoTagsJson, repoTagJson);
    }
    cJSON_AddItemToObject(manifestJson, "RepoTags", repoTagsJson);
    
    cJSON *layersJson = cJSON_CreateArray();
    for (int i = 0; i < manifest->layerLen; i++)
    {
        cJSON *layerJson = cJSON_CreateString(manifest->layers[i]);
        cJSON_AddItemToArray(layersJson, layerJson);
    }
    cJSON_AddItemToObject(manifestJson, "Layers", layersJson);
    
    cJSON *manifestsJson = cJSON_CreateArray();
    cJSON_AddItemToArray(manifestsJson, manifestJson);

    return cJSON_Print(manifestsJson);
}