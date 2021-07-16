#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <gcrypt.h>
#include <limits.h>
#include <curl/curl.h>

#include "pull.h"
#include "help.h"
#include "json.h"
#include "cio.h"
#include "path.h"
#include "log.h"
#include "cstr.h"

struct memory
{
    char *response;
    size_t size;
};

static size_t write_callback(char *data, size_t size, size_t nmemb, void *userdata)
{
    size_t realsize = size * nmemb;
    struct memory *mem = (struct memory *)userdata;

    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0; /* out of memory! */

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

static char *get(char *url, struct curl_slist *headers, char *token)
{
    struct memory chunk = {0};
    CURL *curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (token != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
            curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token);
        }
        if (headers != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    }
#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    return chunk.response;
}

static int download(char *url, struct curl_slist *headers, char *token, FILE *file)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (token != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
            curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token);
        }
        if (headers != NULL)
        {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)file);
    }
#ifdef SKIP_PEER_VERIFICATION
    /*
     * If you want to connect to a site who isn't using a certificate that is
     * signed by one of the certs in the CA bundle you have, you can skip the
     * verification of the server's certificate. This makes the connection
     * A LOT LESS SECURE.
     *
     * If you have a CA cert for the server stored someplace else than in the
     * default bundle, then the CURLOPT_CAPATH option might come handy for
     * you.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

#ifdef SKIP_HOSTNAME_VERIFICATION
    /*
     * If the site you're connecting to uses a different host name that what
     * they have mentioned in their server certificate's commonName (or
     * subjectAltName) fields, libcurl will refuse to connect. You can skip
     * this check, but this will make the connection less secure.
     */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
#endif

    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return -1;
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    return 0;
}

static char *requestToken(char *image)
{
    char *authBase = "https://auth.docker.io";
    char *authService = "registry.docker.io";
    char scope[24 + strlen(image) + 1];
    sprintf(scope, "repository:library/%s:pull", image);
    char authUrl[strlen(authBase) + strlen(authService) + strlen(scope) + 22 + 1];
    sprintf(authUrl, "%s/token?service=%s&scope=%s", authBase, authService, scope);

    char *body = get(authUrl, NULL, NULL);
    cJSON *rootJson = cJSON_Parse(body);
    char *token = parseStr(rootJson, "token");
    return token;
}

static char *requestManifests(char *image, char *tag, char *token)
{
    char *registryBase = "https://registry-1.docker.io";
    char manifestsUrl[strlen(registryBase) + strlen(image) + strlen(tag) + 23 + 1];
    sprintf(manifestsUrl, "%s/v2/library/%s/manifests/%s", registryBase, image, tag);
    // printf("manifestsUrl: %s\n", manifestsUrl);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/vnd.docker.distribution.manifest.v2+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.docker.distribution.manifest.list.v2+json");
    headers = curl_slist_append(headers, "Accept: application/vnd.docker.distribution.manifest.v1+json");

    char *body = get(manifestsUrl, headers, token);
    return body;
}

static int requestBlob(char *image, char *tag, char *token, FILE *file)
{
    char *registryBase = "https://registry-1.docker.io";
    char blobUrl[strlen(registryBase) + strlen(image) + strlen(tag) + 19 + 1];
    sprintf(blobUrl, "%s/v2/library/%s/blobs/%s", registryBase, image, tag);
    // printf("manifestsUrl: %s\n", manifestsUrl);

    return download(blobUrl, NULL, token, file);
}

static char *findManifestDigest(char *manifests)
{
    char *digest;
    cJSON *rootJson = cJSON_Parse(manifests);
    cJSON *manifestsJson = cJSON_GetObjectItemCaseSensitive(rootJson, "manifests");
    cJSON *manifestJson;
    cJSON_ArrayForEach(manifestJson, manifestsJson)
    {
        cJSON *platformJson = cJSON_GetObjectItemCaseSensitive(manifestJson, "platform");
        char *os = parseStr(platformJson, "os");
        char *architecture = parseStr(platformJson, "architecture");
        if (!strncmp(os, "linux", 5) && !strncmp(architecture, "amd64", 5))
        {
            digest = parseStr(manifestJson, "digest");
            break;
        }
    }
    return digest;
}

static char *getConfigDigest(char *manifests)
{
    cJSON *rootJson = cJSON_Parse(manifests);
    cJSON *configJson = cJSON_GetObjectItemCaseSensitive(rootJson, "config");
    char *digest = parseStr(configJson, "digest");
    return digest;
}

static int saveManifest(char *image, char *tag, imageManifest *imgManifest)
{
    char *content = serialize(imgManifest);
    char *manifestFilePath = getManifestFilePath(image, tag);
    FILE *manifestFile = openFile(manifestFilePath);
    int len = strlen(content);
    int n = 0;
    while (n < len)
    {
        n += fwrite(content, 1, len, manifestFile);
    }
    return 0;
}

static int saveLayerJson(char *layerId, char *parentId, char *config)
{
    cJSON *rootJson = cJSON_CreateObject();
    cJSON *layerIdJson = cJSON_CreateString(layerId);
    cJSON_AddItemToObject(rootJson, "id", layerIdJson);
    if (parentId != NULL)
    {
        cJSON *parentIdJson = cJSON_CreateString(parentId);
        cJSON_AddItemToObject(rootJson, "parent", parentIdJson);
    }
    if (config == NULL)
    {
        cJSON *createdJson = cJSON_CreateString("0001-01-01T00:00:00Z");
        cJSON_AddItemToObject(rootJson, "created", createdJson);

        cJSON *containerConfigJson = cJSON_CreateObject();

        cJSON *hostnameJson = cJSON_CreateString("");
        cJSON_AddItemToObject(containerConfigJson, "Hostname", hostnameJson);

        cJSON *domainnameJson = cJSON_CreateString("");
        cJSON_AddItemToObject(containerConfigJson, "Domainname", domainnameJson);

        cJSON *userJson = cJSON_CreateString("");
        cJSON_AddItemToObject(containerConfigJson, "User", userJson);

        cJSON *attachStdinJson = cJSON_CreateBool(cJSON_False);
        cJSON_AddItemToObject(containerConfigJson, "AttachStdin", attachStdinJson);

        cJSON *attachStdoutJson = cJSON_CreateBool(cJSON_False);
        cJSON_AddItemToObject(containerConfigJson, "AttachStdout", attachStdoutJson);

        cJSON *attachStderrJson = cJSON_CreateBool(cJSON_False);
        cJSON_AddItemToObject(containerConfigJson, "AttachStderr", attachStderrJson);

        cJSON *openStdinJson = cJSON_CreateBool(cJSON_False);
        cJSON_AddItemToObject(containerConfigJson, "OpenStdin", openStdinJson);

        cJSON *stdinOnceJson = cJSON_CreateBool(cJSON_False);
        cJSON_AddItemToObject(containerConfigJson, "StdinOnce", stdinOnceJson);

        //TODO: add more

        cJSON_AddItemToObject(rootJson, "container_config", containerConfigJson);
    }
    else
    {
        cJSON *configRootJson = cJSON_Parse(config);

        cJSON *architectureJson = cJSON_GetObjectItem(configRootJson, "architecture");
        cJSON_AddItemToObject(rootJson, "architecture", architectureJson);

        cJSON *configJson = cJSON_GetObjectItem(configRootJson, "config");
        cJSON_AddItemToObject(rootJson, "config", configJson);

        cJSON *containerJson = cJSON_GetObjectItem(configRootJson, "container");
        cJSON_AddItemToObject(rootJson, "container", containerJson);

        cJSON *containerConfigJson = cJSON_GetObjectItem(configRootJson, "container_config");
        cJSON_AddItemToObject(rootJson, "container_config", containerConfigJson);

        cJSON *createdJson = cJSON_GetObjectItem(configRootJson, "created");
        cJSON_AddItemToObject(rootJson, "created", createdJson);

        cJSON *dockerVersionJson = cJSON_GetObjectItem(configRootJson, "docker_version");
        cJSON_AddItemToObject(rootJson, "docker_version", dockerVersionJson);

        cJSON *osJson = cJSON_GetObjectItem(configRootJson, "os");
        cJSON_AddItemToObject(rootJson, "os", osJson);
    }
    char *content = cJSON_Print(rootJson);
    char *layerJsonFilePath = getLayerJsonFilePath(layerId);
    FILE *layerJsonFile = openFile(layerJsonFilePath);
    int len = strlen(content);
    int n = 0;
    while (n < len)
    {
        n += fwrite(content, 1, len, layerJsonFile);
    }
    return 0;
}

static char *downloadConfig(char *image, char *tag, char *configDigest)
{
    char *imageId = substr(configDigest, ':');
    char *configFilePath = getConfigFilePath(image, tag, imageId);
    FILE *configFile = openFile(configFilePath);
    if (configFile != NULL)
    {
        char *token = requestToken(image);
        printf("downloading config manifest\n");
        requestBlob(image, configDigest, token, configFile);
        fclose(configFile);
    }
    char *config = calloc(strlen(imageId) + 5 + 1, sizeof(char));
    sprintf(config, "%s.json", imageId);
    return config;
}

static int extract(char *tarFile, char *destPath)
{
    char cmd[strlen(tarFile) + strlen(destPath) + 12 + 1];
    sprintf(cmd, "tar -xf %s -C %s", tarFile, destPath);
    return system(cmd);
}

static int downloadImage(char *image, char *tag, char *configManifest)
{
    char *token;
    imageManifest imgManifest = {};
    imgManifest.repoTagLen = 1;
    imgManifest.repoTags = (char **)calloc(1, sizeof(char *));
    imgManifest.repoTags[0] = (char *)calloc(sizeof(image) + sizeof(tag) + 2, sizeof(char));
    sprintf(imgManifest.repoTags[0], "%s:%s", image, tag);

    cJSON *rootJson = cJSON_Parse(configManifest);
    cJSON *configJson = cJSON_GetObjectItemCaseSensitive(rootJson, "config");
    char *configDigest = parseStr(configJson, "digest");
    // printf("configDigest: %s\n", configDigest);
    imgManifest.config = downloadConfig(image, tag, configDigest);

    char *configContent = readtoend(imgManifest.config, "rb");
    cJSON *layersJson = cJSON_GetObjectItemCaseSensitive(rootJson, "layers");
    cJSON *layerJson;
    char *layerId = "";
    char *parentId = NULL;
    char *layerTarFilePath;
    char *layer;
    imgManifest.layerLen = cJSON_GetArraySize(layersJson);
    imgManifest.layers = calloc(imgManifest.layerLen, sizeof(char *));
    int i = 0;
    cJSON_ArrayForEach(layerJson, layersJson)
    {
        char *layerDigest = parseStr(layerJson, "digest");
        printf("downlaoding layer %s\n", layerDigest);
        
        char buffer[strlen(layerDigest) + strlen(layerId) + 2];
        sprintf(buffer, "%s\n%s", layerId, layerDigest);
        layerId = hash(buffer);
        char *config = i == imgManifest.layerLen - 1 ? configContent : NULL;
        saveLayerJson(layerId, parentId, config);
        layerTarFilePath = getLayerTarFilePath(layerId);
        FILE *layerTarFile = openFile(layerTarFilePath);
        token = requestToken(image);
        requestBlob(image, layerDigest, token, layerTarFile);

        layer = (char*)calloc(strlen(layerId) + 10 + 1, sizeof(char));
        sprintf(layer, "%s/layer.tar", layerId);
        imgManifest.layers[i] = layer;

        fclose(layerTarFile);

        char *runtimeLayerPath = getRuntimeLayerPath(layerId);
        mkdirRecur(runtimeLayerPath);
        extract(layerTarFilePath, runtimeLayerPath);
        
        free(token);
        free(layerTarFilePath);
        parentId = layerId;
        i++;
    }

    saveManifest(image, tag, &imgManifest);

    return 0;
}

int pull(char *imgAddr)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);

    char *image = strtok(imgAddr, ":");
    char *tag = strtok(NULL, ":");
    if (image == NULL)
    {
        help();
        return 0;
    }
    if (tag == NULL)
    {
        tag = "latest";
    }

    char *token = requestToken(image);
    // printf("token: %s\n", token);
    char *manifestList = requestManifests(image, tag, token);
    // printf("manifestList: %s\n", manifestList);
    char *manifestDigest = findManifestDigest(manifestList);
    // printf("manifestDigest: %s\n", manifestDigest);
    char *configManifest = requestManifests(image, manifestDigest, token);
    // printf("config manifest: %s\n", configManifest);
    free(token);
    printf("request manifest done\n");

    downloadImage(image, tag, configManifest);

    curl_global_cleanup();
}
