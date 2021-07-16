#ifndef __CONFIG_H
#define __CONFIG_H

#define PARENTOK 1
#define CREATERUNTIME 2
#define CREATERUNTIMERESP 3

typedef struct userConfig
{
    int uid;
    int gid;
} userConfig;

typedef struct processConfig
{
    char **args;
    int argc;
    char **env;
    int envc;
    char *cwd;
    userConfig *user;
} processConfig;

typedef struct rootConfig
{
    char *path;
    int readonly;
} rootConfig;

typedef struct mountConfig
{
    char *destination;
	char *source;
	char *type;
	char **options;
    int optionsLen;
} mountConfig;

typedef struct idMappingConfig
{
    int containerId;
    int hostId;
    long size;
} idMappingConfig;

typedef struct namespaceConfig
{
    char *type;
    char *path;
} namespaceConfig;

typedef struct cpuConfig
{
    int shares;
	int quota;
	int period;
} cpuConfig;

typedef struct memoryConfig
{
} memoryConfig;

typedef struct resourceConfig
{
    cpuConfig *cpu;
    memoryConfig *memory;
} resourceConfig;

typedef struct hookConfig
{
    char *path;
    char **args;
    int argc;
    char **env;
    int envc;
    int timeout;
} hookConfig;

typedef struct hooksConfig
{
    hookConfig *createRuntime;
    int createRuntimeLen;
} hooksConfig;

typedef struct linuxConfig
{
    namespaceConfig *namespaces;
    int namespaceLen;
    idMappingConfig *uidMappings;
    int uidMappingsLen;
    idMappingConfig *gidMappings;
    int gidMappingsLen;
    char *cgroupsPath;
    resourceConfig *resource;
} linuxConfig;

typedef struct containerConfig
{
    char *ociVersion;
    processConfig *process;
    rootConfig *root;
    char *hostname;
    mountConfig *mounts;
    int mountslen;
    hooksConfig *hooks;
    linuxConfig *Linux;
} containerConfig;

typedef struct imageManifest
{
    char *config;
    char **repoTags;
    int repoTagLen;
    char **layers;
    int layerLen;
} imageManifest;

void freeConfig(containerConfig *config);

containerConfig *makeDefaultConfig();

#endif