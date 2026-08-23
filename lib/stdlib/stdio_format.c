/* stdio_format.c — Formatted I/O for FILE streams
 *
 * Implements:
 * - fprintf, vfprintf — formatted output to FILE
 * - fscanf, vfscanf — formatted input from FILE
 * - fgets, fputs — line-oriented I/O
 *
 * Phase 2 of FILE I/O implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/* Forward declarations - these exist in stdlib */
extern int vprintf(const char *format, va_list ap);
extern int vsprintf(char *str, const char *format, va_list ap);
extern int vfscanf(FILE *stream, const char *format, va_list ap);

/* ============================================================================
 * LINE-ORIENTED I/O
 * ============================================================================ */

/**
 * fgets - Read a line from file
 *
 * Reads up to (n-1) characters from stream into s, stops at newline.
 * Stores newline in string. Null-terminates.
 *
 * Returns: s on success, NULL on EOF or error
 */
char *fgets(char *s, int n, FILE *stream) {
    if (!s || n <= 0 || !stream) {
        return NULL;
    }

    int ch;
    int i = 0;

    /* Read characters until newline or EOF */
    while (i < n - 1) {
        ch = fgetc(stream);

        if (ch == EOF) {
            if (i == 0) {
                /* EOF with no characters read */
                return NULL;
            }
            /* EOF after reading some characters */
            break;
        }

        s[i++] = (char)ch;

        if (ch == '\n') {
            break;  /* Stop at newline */
        }
    }

    s[i] = '\0';  /* Null-terminate */
    return s;
}

/**
 * fputs - Write a string to file
 *
 * Writes string s to stream (without the null terminator).
 *
 * Returns: non-negative on success, EOF on error
 */
int fputs(const char *s, FILE *stream) {
    if (!s || !stream) {
        return EOF;
    }

    int count = 0;

    for (int i = 0; s[i]; i++) {
        int ch = fputc(s[i], stream);
        if (ch == EOF) {
            return EOF;
        }
        count++;
    }

    return count;
}

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
int vfprintf(FILE *stream, const char *format, va_list ap) {
    if (!stream || !format) {
        return -1;
    }

    /* Use a stack buffer for formatting (1KB max) */
    char buffer[1024];

    /* Format into buffer using vsprintf */
    int n = vsprintf(buffer, format, ap);

    if (n < 0) {
        return -1;
    }

    /* Write buffer to stream */
    int written = fwrite(buffer, 1, n, stream);

    return written;
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

/**
 * fseek - Seek to position in file
 *
 * Sets the file position indicator for the stream.
 * Limited support on Commodore (sequential files don't support seeking).
 *
 * Returns: 0 on success, -1 on error
 */
int fseek(FILE *stream, long offset, int whence) {
    if (!stream) {
        return -1;
    }

    /* TODO: Implement seeking via CBM_SEEK
     * Commodore file seeking is limited:
     * - Sequential files (.seq) don't support seeking
     * - Relative files (.rel) support seeking
     * - Would need device-specific commands
     */

    return -1;  /* Not supported */
}

/**
 * ftell - Get current file position
 *
 * Returns the current position in the file.
 * Not reliably supported on Commodore drives.
 *
 * Returns: current position, or -1L on error
 */
long ftell(FILE *stream) {
    if (!stream) {
        return -1L;
    }

    /* Return tracked position from FILE structure */
    return (long)stream->position;
}

/**
 * rewind - Seek to beginning of file
 *
 * Sets position to start of file and clears error/EOF flags.
 */
void rewind(FILE *stream) {
    if (!stream) {
        return;
    }

    fseek(stream, 0L, SEEK_SET);
    clearerr(stream);
}

/**
 * fgetpos - Get current file position
 *
 * Stores current position in fpos_t variable.
 *
 * Returns: 0 on success, -1 on error
 */
int fgetpos(FILE *stream, fpos_t *pos) {
    if (!stream || !pos) {
        return -1;
    }

    *pos = stream->position;
    return 0;
}

/**
 * fsetpos - Set file position
 *
 * Sets position from fpos_t variable.
 *
 * Returns: 0 on success, -1 on error
 */
int fsetpos(FILE *stream, const fpos_t *pos) {
    if (!stream || !pos) {
        return -1;
    }

    return fseek(stream, (long)*pos, SEEK_SET);
}

