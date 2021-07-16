#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <gcrypt.h>

#include "cstr.h"

char *format(char *format, ...)
{
    va_list ap, apc;
    va_start(ap, format);
    va_copy(apc, ap);

    int n = vsnprintf(NULL, 0, format, ap);
    char *s = (char *)calloc(n + 1, sizeof(char));
    vsnprintf(s, n + 1, format, apc);
    
    va_end(ap);
    va_end(apc);
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

static char *hex(unsigned char *string, int n)
{
    char *hex = (char *)calloc(n * 2 + 1, sizeof(char));
    for (unsigned i = 0; i < n; i++)
    {
        sprintf(hex + i * 2, "%02x", string[i]); /* print the result */
    }
    return hex;
}

char *hash(char *buffer)
{
    int algo = GCRY_MD_SHA256;
    unsigned int l = gcry_md_get_algo_dlen(algo); /* get digest length (used later to print the result) */
    gcry_md_hd_t h;
    gcry_md_open(&h, algo, GCRY_MD_FLAG_SECURE); /* initialise the hash context */
    gcry_md_write(h, buffer, strlen(buffer));    /* hash some text */
    unsigned char *x = gcry_md_read(h, algo);    /* get the result */
    char *hash = hex(x, l);
    gcry_md_close(h);
    return hash;
}

char *randomstr()
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    int n = 16;
    char buffer[n + 1];
    int result = read(fd, buffer, n);
    close(fd);
    if (result > 0)
    {
        return hash(buffer);
    }
    return NULL;
}

char *join(char *sep, int n, ...)
{
    char *strs[n];
    int lens[n];
    int total = 0;
    va_list ap;

    va_start(ap, n);
    for (int i = 0; i < n; i++)
    {
        char *arg = va_arg(ap, char *);
        lens[i] = strlen(arg);
        strs[i] = arg;
        total += lens[i];
    }
    va_end(ap);

    int seplen = strlen(sep);
    total = total + (n - 1) * seplen + 1;
    char *str = (char *)calloc(total, sizeof(char));
    if (str == NULL)
    {
        return NULL;
    }
    strncpy(str, strs[0], lens[0]);

    for (int i = 1; i < n; i++)
    {
        strncat(str, sep, seplen);
        strncat(str, strs[i], lens[i]);
    }
    return str;
}

char *join1(char *sep, int n, char **strs)
{
    if (n == 0)
    {
        return NULL;
    }
    int lens[n];
    int total = 0;

    for (int i = 0; i < n; i++)
    {
        lens[i] = strlen(strs[i]);
        total += lens[i];
    }

    int seplen = strlen(sep);
    total = total + (n - 1) * seplen + 1;
    char *str = (char *)calloc(total, sizeof(char));
    if (str == NULL)
    {
        return NULL;
    }
    strncpy(str, strs[0], lens[0]);
    for (int i = 1; i < n; i++)
    {
        strncat(str, sep, seplen);
        strncat(str, strs[i], lens[i]);
    }
    return str;
}
