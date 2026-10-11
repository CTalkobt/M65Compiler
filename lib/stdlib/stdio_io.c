/* stdio_io.c — Character and block I/O for MEGA65 C Compiler
 *
 * fgetc, fputc, ungetc, fread, fwrite implementations
 * Phase 1 of FILE I/O implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Buffer helpers shared with stdio_file.c */
int _fill_read_buffer(FILE *stream);
int _flush_write_buffer(FILE *stream);

/* ============================================================================
 * CHARACTER I/O
 * ============================================================================ */

/**
 * fgetc - Read a character from a stream
 */
int fgetc(FILE *stream) {
    if (!stream || !(stream->flags & _IOREAD)) {
        return EOF;
    }

    /* Check for pushback character */
    if (stream->ungetc_pending) {
        stream->ungetc_pending = 0;
        return stream->ungetc_char;
    }

    /* Check if we've already signaled EOF */
    if (stream->flags & _IOEOF) {
        return EOF;
    }

    /* If buffer is empty, fill it */
    if (stream->buf_pos >= stream->buf_len) {
        if (_fill_read_buffer(stream) < 0) {
            return EOF;
        }
        if (stream->buf_len == 0) {
            stream->flags |= _IOEOF;
            return EOF;
        }
    }

    /* Return next character from buffer */
    unsigned char c = stream->buffer[stream->buf_pos++];
    stream->position++;
    return (int)c;
}

/**
 * fputc - Write a character to a stream
 */
int fputc(int c, FILE *stream) {
    if (!stream || !(stream->flags & _IOWRITE)) {
        return EOF;
    }

    /* stdout/stderr are the screen: written directly */
    if (stream == stdout || stream == stderr) {
        putchar(c);
        stream->position++;
        return (unsigned char)c;
    }

    /* Add character to buffer */
    unsigned char byte = (unsigned char)c;

    if (stream->buf_len >= stream->buf_size) {
        /* Buffer full, flush it */
        if (_flush_write_buffer(stream) < 0) {
            return EOF;
        }
    }

    stream->buffer[stream->buf_len++] = byte;
    stream->position++;

    /* Auto-flush on newline for text streams */
    if (byte == '\n' && !(stream->flags & _IOBINARY)) {
        if (_flush_write_buffer(stream) < 0) {
            return EOF;
        }
    }

    return (int)byte;
}

/**
 * ungetc - Push a character back onto the stream
 *
 * Note: Standard C allows at least one character to be pushed back.
 * We maintain one ungetc_char field in FILE structure.
 */
int ungetc(int c, FILE *stream) {
    if (!stream || c == EOF) {
        return EOF;
    }

    /* Only one character can be pushed back */
    if (stream->ungetc_pending) {
        return EOF;  /* Already have a pushback character */
    }

    stream->ungetc_char = (unsigned char)c;
    stream->ungetc_pending = 1;
    stream->position--;  /* Back up position counter */

    return (int)(unsigned char)c;
}

/* ============================================================================
 * BLOCK I/O
 * ============================================================================ */

/**
 * fread - Read data from a stream
 *
 * Returns: number of complete items successfully read
 * Note: Returns fewer than nmemb on EOF or error
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream || !ptr || size == 0 || nmemb == 0) {
        return 0;
    }

    if (!(stream->flags & _IOREAD)) {
        return 0;
    }

    unsigned char *buf = (unsigned char *)ptr;
    size_t total_bytes = size * nmemb;
    size_t bytes_read = 0;

    /* Handle ungetc character first */
    if (stream->ungetc_pending) {
        if (total_bytes > 0) {
            stream->ungetc_pending = 0;
            buf[0] = stream->ungetc_char;
            buf++;
            bytes_read++;
            total_bytes--;
            stream->position++;
        }
    }

    /* Read remaining bytes */
    while (bytes_read < total_bytes) {
        if (stream->buf_pos >= stream->buf_len) {
            /* Buffer empty, refill */
            if (_fill_read_buffer(stream) < 0) {
                break;  /* Return partial read on error */
            }
            if (stream->buf_len == 0) {
                stream->flags |= _IOEOF;
                break;  /* EOF reached */
            }
        }

        /* Copy from buffer to user buffer */
        size_t available = stream->buf_len - stream->buf_pos;
        size_t to_copy = (total_bytes - bytes_read);
        if (to_copy > available) {
            to_copy = available;
        }

        memcpy(buf + bytes_read, stream->buffer + stream->buf_pos, to_copy);
        bytes_read += to_copy;
        stream->buf_pos += to_copy;
        stream->position += to_copy;
    }

    /* Return number of complete items read */
    return bytes_read / size;
}

/**
 * fwrite - Write data to a stream
 *
 * Returns: number of complete items successfully written
 * Note: Returns fewer than nmemb on error
 */
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (!stream || !ptr || size == 0 || nmemb == 0) {
        return 0;
    }

    if (!(stream->flags & _IOWRITE)) {
        return 0;
    }

    const unsigned char *buf = (const unsigned char *)ptr;
    size_t total_bytes = size * nmemb;
    size_t bytes_written = 0;

    /* stdout/stderr are the screen: written directly */
    if (stream == stdout || stream == stderr) {
        for (; bytes_written < total_bytes; bytes_written++) putchar(buf[bytes_written]);
        stream->position += total_bytes;
        return nmemb;
    }

    while (bytes_written < total_bytes) {
        /* Check if buffer is full */
        if (stream->buf_len >= stream->buf_size) {
            if (_flush_write_buffer(stream) < 0) {
                break;  /* Error during flush */
            }
        }

        /* Copy from user buffer to stream buffer */
        size_t available = stream->buf_size - stream->buf_len;
        size_t to_copy = total_bytes - bytes_written;
        if (to_copy > available) {
            to_copy = available;
        }

        memcpy(stream->buffer + stream->buf_len, buf + bytes_written, to_copy);
        stream->buf_len += to_copy;
        bytes_written += to_copy;
        stream->position += to_copy;
    }

    /* Return number of complete items written */
    return bytes_written / size;
}

