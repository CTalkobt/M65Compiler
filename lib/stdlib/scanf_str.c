/* scanf_str.c — the scanf engine's default character source and float
 * reader (weak)
 *
 * In a member of their own, so that the engine's calls go through the
 * linker: stdio_format.c's strong __scan_getc/__scan_ungetc (a FILE too) and
 * scanf_float.c's strong __scan_float replace them when those are linked.
 */

#pragma cc45 signed_default

#include "scanf_core.h"

/* A string's characters (stdio_format.c replaces these with versions that
 * also read a FILE) */
#pragma cc45 weak
int __scan_getc(struct __scan_src *s) {
    unsigned char c = *s->str;
    if (!c) return SC_EOF;
    s->str++;
    return c;
}

#pragma cc45 weak
void __scan_ungetc(struct __scan_src *s, int c) {
    (void)c;
    s->str--;
}

/* No float scanf linked: %f is a matching failure (scanf_float.c) */
#pragma cc45 weak
int __scan_float(struct __scan_src *s, int width, float *dest) {
    (void)s; (void)width; (void)dest;
    return 0;
}

