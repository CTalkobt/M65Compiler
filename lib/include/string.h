/* string.h — String and memory functions for cc45 / MEGA65 */

#pragma once

#ifndef NULL
#define NULL 0
#endif

/* String operations */
__fastcall__ int strlen(char *s);
__fastcall__ char *strcpy(char *dest, char *src);
__fastcall__ char *strncpy(char *dest, char *src, int n);
__fastcall__ signed int strcmp(char *s1, char *s2);
__fastcall__ signed int strncmp(char *s1, char *s2, int n);
__fastcall__ char *strcat(char *dest, char *src);
__fastcall__ char *strncat(char *dest, char *src, int n);
__fastcall__ char *strchr(char *s, int c);
__fastcall__ char *strrchr(char *s, int c);
char *strstr(char *haystack, char *needle);
char *strpbrk(char *s, char *accept);
int strspn(char *s, char *accept);
int strcspn(char *s, char *reject);
__fastcall__ char *strtok(char *s, char *delim);
__fastcall__ char *strtok_r(char *s, char *delim, char **saveptr);
int strnlen(const char *s, int maxlen);
int strlcpy(char *dst, const char *src, int size);
char *strdup(const char *s);

/* Memory operations */
__fastcall__ void *memcpy(void *dest, void *src, int n);
__fastcall__ void *memmove(void *dest, void *src, int n);
__fastcall__ void *memset(void *s, int c, int n);
__fastcall__ signed int memcmp(void *s1, void *s2, int n);
void *memchr(void *s, int c, int n);
char *strerror(int errnum);

/* GCC __builtin_* equivalents */
void *__builtin_memcpy(void *dest, void *src, unsigned int n);
void *__builtin_memset(void *dest, int c, unsigned int n);
void *__builtin_memmove(void *dest, void *src, unsigned int n);
signed int __builtin_memcmp(void *s1, void *s2, unsigned int n);
unsigned int __builtin_strlen(char *s);
char *__builtin_strcpy(char *dest, char *src);
signed int __builtin_strcmp(char *s1, char *s2);
