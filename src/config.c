#include <stdlib.h>
#include "config.h"

void freeConfig(containerConfig *config)
{
    if (config == NULL)
        return;
    //free the field of config
    free(config);
}