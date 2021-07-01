#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cstr.h"

char *format(char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int n = vsnprintf(NULL, 0, format, ap);
    char *s = (char *)calloc(n + 1, sizeof(char));
    vsnprintf(s, n + 1, format, ap);
    va_end(ap);
    return s;
}