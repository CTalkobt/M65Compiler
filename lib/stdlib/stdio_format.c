/* stdio_format.c — Formatted I/O for FILE streams
 *
 * Implements:
 * - fprintf, vfprintf — formatted output to FILE
 * - fscanf, vfscanf — formatted input from FILE
 * (fgets and fputs are in stdio_line.c, so that line I/O does not link
 *  the printf/scanf machinery)
 *
 * Phase 2 of FILE I/O implementation
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Forward declarations - these exist in stdlib */

/* ============================================================================
 * FORMATTED OUTPUT
 * ============================================================================ */

/**
 * vfprintf - Formatted output to FILE (va_list version)
 *
 * Internal helper that uses a temporary buffer and fwrite.
 *
 * Returns: number of characters written, or -1 on error
 */
/* __vformat (stdlib_asm/__vformat.s45) and its callbacks use the ZP convention */
__fastcall__ int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));

/* Writes a character to the FILE given as the context */
static __fastcall__ void vf_file_put(int c, void *ctx) {
    fputc(c, (FILE *)ctx);
}

int vfprintf(FILE *stream, const char *format, va_list ap) {
    if (!stream || !format || !(stream->flags & _IOWRITE)) {
        return -1;
    }
    unsigned char err = stream->flags & _IOERR;
    int n = __vformat(vf_file_put, stream, (char *)format, ap, 0);
    if (!err && (stream->flags & _IOERR)) return -1;
    return n;
}

/**
 * fprintf - Formatted output to FILE
 *
 * Outputs formatted string to stream.
 *
 * Returns: number of characters printed, or -1 on error
 */
int fprintf(FILE *stream, const char *format, ...) {
    if (!stream || !format) {
        return -1;
    }

    va_list ap;
    va_start(ap, format);
    int result = vfprintf(stream, format, ap);
    va_end(ap);

    return result;
}

/* ============================================================================
 * FORMATTED INPUT
 * ============================================================================ */

#include "scanf_core.h"

/* The scanf engine's character source: a FILE as well as a string (strong:
 * replace the engine's string-only versions) */
int __scan_getc(struct __scan_src *s) {
    unsigned char c;
    if (s->file) return fgetc(s->file);
    c = *s->str;
    if (!c) return SC_EOF;
    s->str++;
    return c;
}

void __scan_ungetc(struct __scan_src *s, int c) {
    if (s->file) ungetc(c, s->file);
    else s->str--;
}

/**
 * vfscanf / fscanf / vscanf / scanf - Formatted input from a FILE / stdin
 * (the scanf engine of scanf_core.c; the character after the input used is
 * pushed back with ungetc)
 */
int vfscanf(FILE *stream, const char *format, va_list ap) {
    if (!stream || !format) {
        return EOF;
    }
    return __vscanf_core(stream, NULL, format, ap);
}

int fscanf(FILE *stream, const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vfscanf(stream, format, ap);
    va_end(ap);
    return result;
}

int vscanf(const char *format, va_list ap) {
    return vfscanf(stdin, format, ap);
}

int scanf(const char *format, ...) {
    va_list ap;
    int result;
    va_start(ap, format);
    result = vfscanf(stdin, format, ap);
    va_end(ap);
    return result;
}

/* ============================================================================
 * FILE POSITIONING
 * ============================================================================ */

