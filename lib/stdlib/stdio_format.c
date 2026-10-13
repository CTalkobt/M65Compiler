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
extern int vfscanf(FILE *stream, const char *format, va_list ap);

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
int __vformat(void (*put)(int c, void *ctx), void *ctx, char *fmt, va_list ap,
              int (*ffmt)(char *, float, int, int, int));

/* Writes a character to the FILE given as the context */
static void vf_file_put(int c, void *ctx) {
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

/**
 * vfscanf - Formatted input from FILE (va_list version)
 *
 * Reads formatted data from stream.
 * Currently stubbed - requires complex scanf implementation.
 *
 * Returns: number of input items successfully matched and assigned
 */
int vfscanf(FILE *stream, const char *format, va_list ap) {
    if (!stream || !format) {
        return EOF;
    }

    /* TODO: Implement fscanf parsing
     * This requires a full scanf-style parser which is complex.
     * For now, return 0 (no items matched).
     */

    return 0;
}

/**
 * fscanf - Formatted input from FILE
 *
 * Reads formatted data from stream.
 * Currently stubbed - implement if needed.
 *
 * Returns: number of input items successfully matched and assigned
 */
int fscanf(FILE *stream, const char *format, ...) {
    if (!stream || !format) {
        return EOF;
    }

    va_list ap;
    va_start(ap, format);
    int result = vfscanf(stream, format, ap);
    va_end(ap);

    return result;
}

/* ============================================================================
 * FILE POSITIONING
 * ============================================================================ */

