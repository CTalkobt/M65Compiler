/* stdio_line.c — line-oriented I/O for FILE streams: fgets, fputs
 *
 * Kept apart from stdio_format.c (fprintf/fscanf): ln45 links whole
 * objects, and these would otherwise bring in the printf machinery.
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>

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

