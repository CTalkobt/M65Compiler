/* scanf_core.h — the scanf engine's interface (library-private)
 *
 * __vscanf_core reads a string (file NULL) or a FILE through __scan_getc /
 * __scan_ungetc: the engine's weak versions read only strings, stdio_format.c
 * (linked by fscanf and friends) has strong ones that also read a FILE, so
 * sscanf alone does not link the FILE code. %f %e %g go to __scan_float:
 * a weak stub in the engine (a matching failure), the real one in
 * scanf_float.c, which cc45 links (it references _scanf_float) when a
 * scanf, fscanf or sscanf call passes a float pointer, or a unit with float
 * code calls vscanf, vfscanf or vsscanf. No calls go through pointers.
 */
#pragma once

#include <stdarg.h>
#include <stdio.h>

#define SC_EOF  (-1)
#define SC_NONE (-2)

struct __scan_src {
    FILE *file;                 /* the stream, or NULL for a string */
    const unsigned char *str;   /* the string's next character */
    int la;                     /* lookahead, SC_NONE when not read yet */
    int count;                  /* characters consumed (%n) */
};

int __scan_getc(struct __scan_src *s);
void __scan_ungetc(struct __scan_src *s, int c);
int __scan_float(struct __scan_src *s, int width, float *dest);

static inline int sc_peek(struct __scan_src *s) {
    if (s->la == SC_NONE) s->la = __scan_getc(s);
    return s->la;
}

static inline void sc_next(struct __scan_src *s) {
    s->la = SC_NONE;
    s->count++;
}

int __vscanf_core(FILE *file, const char *str, const char *fmt, va_list ap);
