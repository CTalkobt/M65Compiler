/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma once

#include <stddef.h>

_Noreturn void exit(int status);

int atoi(char *s);
char *itoa(int value, char *str, int base);
char *ltoa(long value, char *str, int base);

/* short aliases — cast through atoi/itoa (short == int on this target) */
#define atos(s) ((short)atoi(s))
#define stoa(value, str, base) itoa((int)(value), (str), (base))

int abs(int value);
int rand(void);
void srand(unsigned int seed);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
