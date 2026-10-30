/* stdlib.h — General utilities for cc45 / MEGA65 */

#pragma once

#include <stddef.h>

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

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
__fastcall__ char *ultoa(unsigned long value, char *str, int base);
/* f as printf's %g writes it, at s (oscar64's) */
void ftoa(float f, char *s);

/* short aliases — cast through atoi/itoa (short == int on this target) */
#define atos(s) ((short)atoi(s))
#define stoa(value, str, base) itoa((int)(value), (str), (base))

__fastcall__ signed long strtol(char *restrict nptr, char **restrict endptr, int base);
__fastcall__ unsigned long strtoul(char *restrict nptr, char **restrict endptr, int base);
float strtof(char *restrict nptr, char **restrict endptr);
float atof(char *nptr);
#define strtod(nptr, endptr) ((double)strtof((nptr), (endptr)))

#ifndef __NOMACRO_ABS
#define abs(x) ((x)<0?-(x):(x))
#else
__fastcall__ signed int abs(signed int value) __attribute__((const));
#endif
#ifndef __NOMACRO_LABS
#define labs(x) ((long)((x)<0?-(x):(x)))
#else
__fastcall__ signed long labs(signed long value) __attribute__((const));
#endif
#define RAND_MAX 32767
__fastcall__ int rand(void);
__fastcall__ void srand(unsigned int seed);

/* Up to 32 functions, called in reverse order by exit() and by a return
 * from main (not by _exit or abort) */
int atexit(void (*func)());

typedef struct {
    signed int quot;
    signed int rem;
} div_t;

typedef struct {
    signed long quot;
    signed long rem;
} ldiv_t;

div_t div(signed int numer, signed int denom);
ldiv_t ldiv(signed long numer, signed long denom);

__fastcall__ void *malloc(size_t size);
__fastcall__ void free(void *ptr);
__fastcall__ void *calloc(size_t nmemb, size_t size);
__fastcall__ void *realloc(void *ptr, size_t size);
/* size bytes at a multiple of alignment (a power of two) in *memptr, freed
 * with free; 0, EINVAL or ENOMEM */
int posix_memalign(void **memptr, size_t alignment, size_t size);

/* cc65's heap functions: the free bytes, the largest malloc that can
 * succeed, a block's size, and memory given to the heap */
__fastcall__ size_t _heapmemavail(void);
__fastcall__ size_t _heapmaxavail(void);
__fastcall__ size_t _heapblocksize(const void *block);
__fastcall__ void _heapadd(void *mem, size_t size);
/* oscar64's name for _heapmemavail */
__fastcall__ unsigned int heapfree(void);

/* The environment (empty at the start; putenv's "NAME=value" strings) */
char *getenv(const char *name);
int putenv(char *string);
/* No command processor: 0 for NULL, -1 for a command */
int system(const char *command);

/* srand with a seed from the hardware random number generator */
__fastcall__ void _randomize(void);
/* The size bytes at p and q exchanged */
__fastcall__ void _swap(void *p, void *q, size_t size);

/* Sorting and searching */
void qsort(void *base, int nmemb, int size, int (*cmpfunc)());
void *bsearch(const void *key, const void *base, int nmemb, int size,
              int (*cmpfunc)());
