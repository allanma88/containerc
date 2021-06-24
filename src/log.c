#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "log.h"

void logError(char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);

    fprintf(stderr, ": %s\n", strerror(errno));
}