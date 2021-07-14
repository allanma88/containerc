#ifndef __JSON_H
#define __JSON_H

#include <cjson/cJSON.h>
#include "config.h"

containerConfig *deserialize();

char *serialize(imageManifest *manifest);

int parseBool(cJSON *json, char *path);

int parseInt(cJSON *json, char *path);

char *parseStr(cJSON *json, char *path);

int parseStrArray(cJSON *json, char *path, char ***strs);

#endif