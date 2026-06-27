/* stdio.h — Standard I/O for cc45 / MEGA65
 *
 * Provides character and string output via KERNAL CHROUT ($FFD2).
 */

#pragma once

int getchar(void);
int putchar(int c);
int puts(char *s);
int sprintf(char *buf, char *fmt, ...);
int snprintf(char *buf, int size, char *fmt, ...);
int printf(char *fmt, ...);
int sscanf(char *str, char *fmt, ...);

/* Variadic-list formatting (requires <stdarg.h>) */
typedef __builtin_va_list va_list;
int vsprintf(char *buf, char *fmt, va_list ap);
int vsnprintf(char *buf, int size, char *fmt, va_list ap);
