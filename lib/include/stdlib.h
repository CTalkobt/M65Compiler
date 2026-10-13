/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma once

#include <stddef.h>

__fastcall__ _Noreturn void exit(int status);
_Noreturn void _exit(int status);
__fastcall__ _Noreturn void abort(void);
_Noreturn void __builtin_trap(void);
_Noreturn void __builtin_unreachable(void);
int __builtin_abs(int x);
long __builtin_labs(long x);
int __builtin_ffs(int x);
int __builtin_clz(unsigned int x);
int __builtin_ctz(unsigned int x);
int __builtin_popcount(unsigned int x);
unsigned int __builtin_bswap16(unsigned int x);

__fastcall__ signed int atoi(char *s);
signed long atol(char *s);
__fastcall__ char *itoa(signed int value, char *str, int base);
__fastcall__ char *utoa(unsigned int value, char *str, int base);
__fastcall__ char *ltoa(signed long value, char *str, int base);

/* short aliases — cast through atoi/itoa (short == int on this target) */
#define atos(s) ((short)atoi(s))
#define stoa(value, str, base) itoa((int)(value), (str), (base))

signed long strtol(char *nptr, char **endptr, int base);
unsigned long strtoul(char *nptr, char **endptr, int base);
float strtof(char *nptr, char **endptr);
float atof(char *nptr);
#define strtod(nptr, endptr) ((double)strtof((nptr), (endptr)))

#ifndef __NOMACRO_ABS
#define abs(x) ((x)<0?-(x):(x))
#else
__fastcall__ signed int abs(signed int value);
#endif
#ifndef __NOMACRO_LABS
#define labs(x) ((long)((x)<0?-(x):(x)))
#else
__fastcall__ signed long labs(signed long value);
#endif
__fastcall__ int rand(void);
__fastcall__ void srand(unsigned int seed);

int atexit(void (*func)());

__fastcall__ void *malloc(size_t size);
__fastcall__ void free(void *ptr);
__fastcall__ void *calloc(size_t nmemb, size_t size);
__fastcall__ void *realloc(void *ptr, size_t size);

/* Sorting and searching */
void qsort(void *base, int nmemb, int size, int (*cmpfunc)());
void *bsearch(const void *key, const void *base, int nmemb, int size,
              int (*cmpfunc)());
