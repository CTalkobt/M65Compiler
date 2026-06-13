/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma once

#include <stddef.h>

_Noreturn void exit(int status);
_Noreturn void _exit(int status);
_Noreturn void abort(void);

int atoi(char *s);
char *itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

/* short aliases — cast through atoi/itoa (short == int on this target) */
#define atos(s) ((short)atoi(s))
#define stoa(value, str, base) itoa((int)(value), (str), (base))

long strtol(char *nptr, char **endptr, int base);
unsigned long strtoul(char *nptr, char **endptr, int base);

#ifndef __NOMACRO_ABS
#define abs(x) ((x)<0?-(x):(x))
#else
int abs(int value);
#endif
#ifndef __NOMACRO_LABS
#define labs(x) ((long)((x)<0?-(x):(x)))
#else
long labs(long value);
#endif
int rand(void);
void srand(unsigned int seed);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
