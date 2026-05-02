/* stdio.h — Standard I/O for cc45 / MEGA65
 *
 * Provides character and string output via KERNAL CHROUT ($FFD2).
 */

#pragma include_once

int putchar(int c);
int puts(char *s);
int sprintf(char *buf, char *fmt, ...);
int printf(char *fmt, ...);
int sscanf(char *str, char *fmt, ...);
