/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma include_once

#include <stddef.h>

_Noreturn void exit(int status);

int atoi(char *s);
char *itoa(int value, char *str, int base);
int abs(int value);
int rand(void);
void srand(unsigned int seed);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
