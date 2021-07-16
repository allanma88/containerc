#ifndef __CSTR_H
#define __CSTR_H

char *format(char *format, ...);

char *substr(char *s, char c);

char *hash(char *buffer);

char *randomstr();

char *join(char *sep, int n, ...);

char *join1(char *sep, int n, char **strs);

#endif