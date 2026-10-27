/* string.h — String and memory functions for cc45 / MEGA65 */

#pragma once

#ifndef NULL
#define NULL 0
#endif

/* String operations */
__fastcall__ int strlen(const char *s) __pure;
__fastcall__ char *strcpy(char *restrict dest, const char *restrict src);
__fastcall__ char *strncpy(char *restrict dest, const char *restrict src, int n);
__fastcall__ signed int strcmp(const char *s1, const char *s2) __pure;
__fastcall__ signed int strncmp(const char *s1, const char *s2, int n) __pure;
__fastcall__ char *strcat(char *restrict dest, const char *restrict src);
__fastcall__ char *strncat(char *restrict dest, const char *restrict src, int n);
__fastcall__ char *strchr(const char *s, int c) __pure;
__fastcall__ char *strrchr(const char *s, int c) __pure;
char *strstr(const char *haystack, const char *needle) __pure;
char *strpbrk(const char *s, const char *accept) __pure;
int strspn(const char *s, const char *accept) __pure;
int strcspn(const char *s, const char *reject) __pure;
__fastcall__ char *strtok(char *restrict s, const char *restrict delim);
__fastcall__ char *strtok_r(char *restrict s, const char *restrict delim, char **restrict saveptr);
int strnlen(const char *s, int maxlen) __pure;
int strlcpy(char *restrict dst, const char *restrict src, int size);
/* Which of several strings s is: 1 for the first equal one, 0 for none;
 * the list ends with NULL (cc45 turns a call whose candidates are all
 * string literals into __stridx_table, where the NULL may be left out) */
int stridx(const char *s, ...);
int __stridx_table(const char *s, const unsigned char *table);
char *strdup(const char *s);

/* Memory operations */
__fastcall__ void *memcpy(void *restrict dest, const void *restrict src, int n);
__fastcall__ void *memmove(void *dest, const void *src, int n);
__fastcall__ void *memset(void *s, int c, int n);
__fastcall__ signed int memcmp(const void *s1, const void *s2, int n) __pure;
void *memchr(const void *s, int c, int n);
char *strerror(int errnum);

/* GCC __builtin_* equivalents */
void *__builtin_memcpy(void *restrict dest, const void *restrict src, unsigned int n);
void *__builtin_memset(void *dest, int c, unsigned int n);
void *__builtin_memmove(void *dest, const void *src, unsigned int n);
signed int __builtin_memcmp(const void *s1, const void *s2, unsigned int n);
unsigned int __builtin_strlen(const char *s);
char *__builtin_strcpy(char *restrict dest, const char *restrict src);
signed int __builtin_strcmp(const char *s1, const char *s2);
