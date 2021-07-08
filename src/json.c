#include <stdlib.h>
#include <cjson/cJSON.h>

int parseStrArray(cJSON *json, char *path, char ***strs)
{
    cJSON *arrayJson = cJSON_GetObjectItemCaseSensitive(json, path);
    int n = cJSON_GetArraySize(arrayJson);
    if (cJSON_IsNull(arrayJson) || !n)
    {
        return 0;
    }
    if (!cJSON_IsArray(arrayJson))
    {
        // logError("%s is not array: %s", path, cJSON_Print(arrayJson));
        return -1;
    }
    *strs = (char **)calloc(n, sizeof(char *));
    int i = 0;
    cJSON *itemJson;
    cJSON_ArrayForEach(itemJson, arrayJson)
    {
        if (!cJSON_IsString(itemJson))
        {
            // logError("item is not string: %s", cJSON_Print(itemJson));
            return -1;
        }
        (*strs)[i++] = itemJson->valuestring;
    }
    return n;
}

char *parseStr(cJSON *json, char *path)
{
    cJSON *strJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsString(strJson))
    {
        return NULL;
    }
    return strJson->valuestring;
}

int parseInt(cJSON *json, char *path)
{
    cJSON *intJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsNumber(intJson))
    {
        //problem
        return -1;
    }
    return intJson->valueint;
}

int parseBool(cJSON *json, char *path)
{
    cJSON *intJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsBool(intJson))
    {
        return -1;
    }
    return intJson->valueint;
}