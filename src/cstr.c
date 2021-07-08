#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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

char *substr(char *s, char c)
{
    char *pos = strchr(s, c);
    size_t n = strlen(pos + 1) + 1;
    char *s1 = malloc((n + 1) * sizeof(char));
    strncpy(s1, pos + 1, n);
    s1[n] = '\0';
    return s1;
}