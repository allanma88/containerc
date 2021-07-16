#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "log.h"
#include "run.h"
#include "help.h"
#include "pull.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        help();
        return 0;
    }

    if (!strncmp(argv[1], "run", 3) && argc > 2)
    {
        char *rootPath = NULL;
        char *imageAddr = NULL;
        char *entrypoint = NULL;

        if (!strncmp(argv[2], "-r", 2))
        {
            if (argc > 3)
            {
                rootPath = argv[3];
            }
            return run(rootPath);
        }
        else
        {
            imageAddr = argv[2];
            if (argc > 3)
            {
                entrypoint = argv[3];
            }
            char *image = strtok(imageAddr, ":");
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
            char *containerId = run1(image, tag, entrypoint);
            if (containerId == NULL)
            {
                logError("run error");
            }
            else
            {
                printf("%s started\n", containerId);
            }
        }
    }
    else if (!strncmp(argv[1], "pull", 4))
    {
        if (argc < 3)
        {
            help();
            return 0;
        }
        return pull(argv[2]);
    }
    else
    {
        help();
    }
    return 0;
}
