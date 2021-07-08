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
    int opt;
    char *rootPath = NULL;
    if (argc < 2)
    {
        help();
        return 0;
    }

    if (!strncmp(argv[1], "run", 3))
    {
        if (argc > 2 && !strncmp(argv[2], "-r", 2))
        {
            if (argc > 3)
            {
                rootPath = argv[3];
            }
            if (rootPath != NULL)
            {
                chdir(rootPath);
            }
        }
        run();
    }
    else if (!strncmp(argv[1], "pull", 4))
    {
        if (argc < 3)
        {
            help();
            return 0;
        }
        pull(argv[2]);
    }
    else
    {
        logError("unkown command: %s", argv[1]);
    }
    return 0;
}
