/* string.h — String and memory functions for cc45 / MEGA65 */

#pragma once

/* String operations */
int strlen(char *s);
char *strcpy(char *dest, char *src);
char *strncpy(char *dest, char *src, int n);
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, int n);
char *strcat(char *dest, char *src);
char *strncat(char *dest, char *src, int n);
char *strchr(char *s, int c);
char *strrchr(char *s, int c);
char *strstr(char *haystack, char *needle);
char *strpbrk(char *s, char *accept);
int strspn(char *s, char *accept);
int strcspn(char *s, char *reject);
char *strtok(char *s, char *delim);

/* Memory operations */
void *memcpy(void *dest, void *src, int n);
void *memmove(void *dest, void *src, int n);
void *memset(void *s, int c, int n);
int memcmp(void *s1, void *s2, int n);
