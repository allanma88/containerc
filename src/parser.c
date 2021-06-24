#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#include "config.h"
#include "parser.h"
#include "cio.h"
#include "log.h"

static int parseStrArray(cJSON *json, char *path, char ***strs)
{
    cJSON *arrayJson = cJSON_GetObjectItemCaseSensitive(json, path);
    int n = cJSON_GetArraySize(arrayJson);
    if (cJSON_IsNull(arrayJson) || !n)
    {
        return 0;
    }
    if (!cJSON_IsArray(arrayJson))
    {
        logError("%s is not array: %s", path, cJSON_Print(arrayJson));
        return -1;
    }
    *strs = (char **)calloc(n, sizeof(char *));
    int i = 0;
    cJSON *itemJson;
    cJSON_ArrayForEach(itemJson, arrayJson)
    {
        if (!cJSON_IsString(itemJson))
        {
            logError("item is not string: %s", cJSON_Print(itemJson));
            return -1;
        }
        (*strs)[i++] = itemJson->valuestring;
    }
    return n;
}

static char *parseStr(cJSON *json, char *path)
{
    cJSON *strJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsString(strJson))
    {
        return NULL;
    }
    return strJson->valuestring;
}

static int parseInt(cJSON *json, char *path)
{
    cJSON *intJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsNumber(intJson))
    {
        //problem
        return -1;
    }
    return intJson->valueint;
}

static int parseBool(cJSON *json, char *path)
{
    cJSON *intJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsBool(intJson))
    {
        return -1;
    }
    return intJson->valueint;
}

static userConfig *parseUser(cJSON *json, char *path)
{
    cJSON *userJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsObject(userJson))
    {
        logError("%s is not obj: %s", path, cJSON_Print(userJson));
        return NULL;
    }
    userConfig *user = (userConfig *)calloc(1, sizeof(userConfig));
    user->uid = parseInt(userJson, "uid");
    user->gid = parseInt(userJson, "uid");
    return user;
}

static processConfig *parseProcess(cJSON *json, char *path)
{
    cJSON *processJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsObject(processJson))
    {
        logError("%s is not obj: %s", path, cJSON_Print(processJson));
        return NULL;
    }

    processConfig *process = (processConfig *)calloc(1, sizeof(processConfig));
    process->user = parseUser(processJson, "user");
    process->argc = parseStrArray(processJson, "args", &process->args);
    process->envc = parseStrArray(processJson, "env", &process->env);
    process->cwd = parseStr(processJson, "cwd");
    if (process->argc < 0 || process->envc < 0)
    {
        //free object in error
        return NULL;
    }
    return process;
}

static rootConfig *parseRoot(cJSON *json, char *path)
{
    cJSON *rootJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsObject(rootJson))
    {
        logError("%s is not obj: %s", path, cJSON_Print(rootJson));
        return NULL;
    }

    rootConfig *root = (rootConfig *)calloc(1, sizeof(rootConfig));
    root->path = parseStr(rootJson, "path");
    root->readonly = parseInt(rootJson, "readonly");
    if (root->readonly < 0)
    {
        root->readonly = 0;
    }
    if (root->path == NULL)
    {
        //free object in error
        free(root);
        return NULL;
    }
    return root;
}

static int parseMount(cJSON *json, mountConfig *mount)
{
    mount->destination = parseStr(json, "destination");
    mount->type = parseStr(json, "type");
    mount->source = parseStr(json, "source");
    mount->optionsLen = parseStrArray(json, "options", &mount->options);
    if (mount->destination == NULL)
    {
        logError("destination is not string: %s", cJSON_Print(json));
        return -1;
    }
    return 0;
}

static int parseMountArray(cJSON *json, char *path, mountConfig **mounts)
{
    cJSON *arrayJson = cJSON_GetObjectItemCaseSensitive(json, path);
    int n = cJSON_GetArraySize(arrayJson);
    if (cJSON_IsNull(arrayJson) || !n)
    {
        logError("%s is empty", path);
        return 0;
    }
    if (!cJSON_IsArray(arrayJson))
    {
        logError("%s is not array: %s", path, cJSON_Print(arrayJson));
        return -1;
    }
    *mounts = (mountConfig *)calloc(n, sizeof(mountConfig));
    mountConfig *mount = *mounts;
    cJSON *itemJson;
    cJSON_ArrayForEach(itemJson, arrayJson)
    {
        if (!cJSON_IsObject(itemJson))
        {
            logError("item is not object: %s", cJSON_Print(itemJson));
            return -1;
        }
        if (parseMount(itemJson, mount) < 0)
        {
            logError("item parse fail: %s", cJSON_Print(itemJson));
            return -1;
        }
        mount++;
    }
    return n;
}

static int parseNamespace(cJSON *json, namespaceConfig *namespace)
{
    namespace->path = parseStr(json, "path");
    namespace->type = parseStr(json, "type");
    if (namespace->type == NULL)
    {
        logError("type is not string: %s", cJSON_Print(json));
        return -1;
    }
    return 0;
}

static int parseNamespaceArray(cJSON *json, char *path, namespaceConfig **namespaces)
{
    cJSON *arrayJson = cJSON_GetObjectItemCaseSensitive(json, path);
    int n = cJSON_GetArraySize(arrayJson);
    if (cJSON_IsNull(arrayJson) || !n)
    {
        logError("%s is empty", path);
        return 0;
    }
    if (!cJSON_IsArray(arrayJson))
    {
        logError("%s is not array: %s", path, cJSON_Print(arrayJson));
        return -1;
    }
    *namespaces = (namespaceConfig *)calloc(n, sizeof(namespaceConfig));
    namespaceConfig *namespace = *namespaces;
    cJSON *itemJson;
    cJSON_ArrayForEach(itemJson, arrayJson)
    {
        if (!cJSON_IsObject(itemJson))
        {
            logError("item is not object: %s", cJSON_Print(itemJson));
            return -1;
        }
        if (parseNamespace(itemJson, namespace) < 0)
        {
            logError("item parse fail: %s", cJSON_Print(itemJson));
            return -1;
        }
        namespace ++;
    }
    return n;
}

static int parseIdMapping(cJSON *json, idMappingConfig *idMapping)
{
    //TODO: error handler?
    idMapping->containerId = parseInt(json, "containerID");
    idMapping->hostId = parseInt(json, "hostID");
    idMapping->size = parseInt(json, "size");
    return 0;
}

static int parseIdMappingArray(cJSON *json, char *path, idMappingConfig **idMappings)
{
    cJSON *arrayJson = cJSON_GetObjectItemCaseSensitive(json, path);
    int n = cJSON_GetArraySize(arrayJson);
    if (cJSON_IsNull(arrayJson) || !n)
    {
        logError("%s is empty", path);
        return 0;
    }
    if (!cJSON_IsArray(arrayJson))
    {
        logError("%s is not array: %s", path, cJSON_Print(arrayJson));
        return -1;
    }
    *idMappings = (idMappingConfig *)calloc(n, sizeof(idMappingConfig));
    idMappingConfig *idMapping = *idMappings;
    cJSON *itemJson;
    cJSON_ArrayForEach(itemJson, arrayJson)
    {
        if (!cJSON_IsObject(itemJson))
        {
            logError("item is not object: %s", cJSON_Print(itemJson));
            return -1;
        }
        if (parseIdMapping(itemJson, idMapping) < 0)
        {
            logError("item parse fail: %s", cJSON_Print(itemJson));
            return -1;
        }
        idMapping++;
    }
    return n;
}

static resourceConfig *parseResource(cJSON *json, char *path)
{
    //TODO: error handler?
    cJSON *resourceJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsObject(resourceJson))
    {
        logError("%s is not object: %s", path, cJSON_Print(json));
        return NULL;
    }
    cJSON *cpuJson = cJSON_GetObjectItemCaseSensitive(resourceJson, "cpu");
    if (!cJSON_IsObject(cpuJson))
    {
        logError("%s is not object: %s", "cpu", cJSON_Print(resourceJson));
        return NULL;
    }
    resourceConfig *resource = (resourceConfig *)malloc(sizeof(resourceConfig));
    resource->cpu = (cpuConfig *)malloc(sizeof(cpuConfig));
    resource->cpu->shares = parseInt(cpuJson, "shares");
    resource->cpu->quota = parseInt(cpuJson, "quota");
    resource->cpu->period = parseInt(cpuJson, "period");
    return resource;
}

static linuxConfig *parseLinux(cJSON *json, char *path)
{
    cJSON *linuxJson = cJSON_GetObjectItemCaseSensitive(json, path);
    if (!cJSON_IsObject(linuxJson))
    {
        logError("%s is not obj: %s", path, cJSON_Print(linuxJson));
        return NULL;
    }
    linuxConfig *Linux = (linuxConfig *)calloc(1, sizeof(linuxConfig));
    Linux->namespaceLen = parseNamespaceArray(linuxJson, "namespaces", &Linux->namespaces);
    Linux->uidMappingsLen = parseIdMappingArray(linuxJson, "uidMappings", &Linux->uidMappings);
    Linux->gidMappingsLen = parseIdMappingArray(linuxJson, "gidMappings", &Linux->gidMappings);
    Linux->cgroupsPath = parseStr(linuxJson, "cgroupsPath");
    Linux->resource = parseResource(linuxJson, "resources");
    if (Linux->namespaceLen < 0)
    {
        free(Linux);
        //TODO: free Linux
        return NULL;
    }
    return Linux;
}

containerConfig *parse()
{
    char *string = readtoend("config.json", "rb");
    if (string == NULL)
    {
        logError("read config error");
        return NULL;
    }
    cJSON *rootJson = cJSON_Parse(string);
    // char *output = cJSON_Print(json);
    // printf("%s\n", output);
    containerConfig *config = (containerConfig *)calloc(1, sizeof(containerConfig));
    if (!cJSON_IsObject(rootJson))
    {
        logError("root is not obj: %s", cJSON_Print(rootJson));
        goto ERROR;
    }
    if ((config->process = parseProcess(rootJson, "process")) == NULL)
    {
        goto ERROR;
    }
    if ((config->root = parseRoot(rootJson, "root")) == NULL)
    {
        goto ERROR;
    }
    if ((config->hostname = parseStr(rootJson, "hostname")) == NULL)
    {
        logError("hostname is not string: %s", cJSON_Print(rootJson));
        goto ERROR;
    }
    if ((config->mountslen = parseMountArray(rootJson, "mounts", &config->mounts)) < 0)
    {
        goto ERROR;
    }
    if ((config->Linux = parseLinux(rootJson, "linux")) == NULL)
    {
        goto ERROR;
    }
    return config;
ERROR:
    freeConfig(config);
    cJSON_free(rootJson);
    free(string);
    return NULL;
}