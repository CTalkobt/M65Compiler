/* stdio_file.c — FILE I/O implementation for MEGA65 C Compiler
 *
 * Core FILE operations: fopen, fclose, freopen, fflush
 * Phase 1 of FILE I/O implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * STATIC STATE
 * ============================================================================ */

/* Static buffer for stdin/stdout/stderr */
static unsigned char _stdin_buf[BUFSIZ];
static unsigned char _stdout_buf[BUFSIZ];
static unsigned char _stderr_buf[BUFSIZ];

/* Standard file pointers */
static FILE _stdin = {
    .handle = STDIN_FILENO,
    .mode = _IOREAD,
    .flags = 0,
    .position = 0,
    .file_size = 0,
    .buffer = _stdin_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

static FILE _stdout = {
    .handle = STDOUT_FILENO,
    .mode = _IOWRITE,
    .flags = 0,
    .position = 0,
    .file_size = 0,
    .buffer = _stdout_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

static FILE _stderr = {
    .handle = STDERR_FILENO,
    .mode = _IOWRITE,
    .flags = 0,
    .position = 0,
    .file_size = 0,
    .buffer = _stderr_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

FILE *stdin = &_stdin;
FILE *stdout = &_stdout;
FILE *stderr = &_stderr;

/* File descriptor allocation table */
static FILE *_open_files[FOPEN_MAX] = {
    &_stdin, &_stdout, &_stderr,
    NULL, NULL, NULL, NULL, NULL
};

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Find an available file descriptor slot
 * Returns slot index (0-FOPEN_MAX) or -1 if no slots available
 */
static int _find_free_slot(void) {
    for (int i = 3; i < FOPEN_MAX; i++) {
        if (_open_files[i] == NULL) {
            return i;
        }
    }
    return -1;
}

/**
 * Allocate a new FILE structure
 */
static FILE *_alloc_file(void) {
    FILE *fp = (FILE *)malloc(sizeof(FILE));
    if (!fp) return NULL;

    memset(fp, 0, sizeof(FILE));
    return fp;
}

/**
 * Parse mode string and set FILE flags
 * Returns flags on success, 0 on error
 */
static unsigned char _parse_mode(const char *mode, unsigned char *cbm_type) {
    unsigned char flags = 0;
    int has_plus = 0;
    int is_binary = 0;

    if (!mode || !mode[0]) return 0;

    /* Check for binary mode suffix */
    for (int i = 0; mode[i]; i++) {
        if (mode[i] == 'b') {
            is_binary = 1;
        } else if (mode[i] == '+') {
            has_plus = 1;
        }
    }

    /* Parse primary mode */
    switch (mode[0]) {
        case 'r':
            flags = _IOREAD;
            *cbm_type = is_binary ? CBM_T_USR : CBM_T_SEQ;
            if (has_plus) flags |= _IOWRITE;
            break;

        case 'w':
            flags = _IOWRITE | _IOTRUNC;
            *cbm_type = is_binary ? CBM_T_USR : CBM_T_SEQ;
            if (has_plus) flags |= _IOREAD;
            break;

        case 'a':
            flags = _IOWRITE | _IOAPPEND;
            *cbm_type = is_binary ? CBM_T_USR : CBM_T_SEQ;
            if (has_plus) flags |= _IOREAD;
            break;

        default:
            return 0;
    }

    if (is_binary) flags |= _IOBINARY;

    return flags;
}

/**
 * Open file via Commodore KERNAL
 * Returns file handle (0-15) on success, -1 on error
 */
static int _kernal_open(const char *filename, unsigned char cbm_type, unsigned char flags) {
    /* TODO: Implement KERNAL SETNAM, SETLFS, OPEN calls
     * For now, return error
     */
    return -1;
}

/**
 * Flush FILE write buffer to device
 * Returns 0 on success, EOF on error
 */
static int _flush_write_buffer(FILE *stream) {
    if (!stream || !(stream->flags & _IOWRITE)) {
        return 0;
    }

    if (stream->buf_len > 0) {
        int written = cbm_write(stream->handle, stream->buffer, stream->buf_len);
        if (written < 0 || written < stream->buf_len) {
            stream->flags |= _IOERR;
            return EOF;
        }
        stream->buf_pos = 0;
        stream->buf_len = 0;
    }

    return 0;
}

/**
 * Fill FILE read buffer from device
 * Returns 0 on success, EOF on error or EOF
 */
static int _fill_read_buffer(FILE *stream) {
    if (!stream || !(stream->flags & _IOREAD)) {
        return EOF;
    }

    int nread = cbm_read(stream->handle, stream->buffer, stream->buf_size);
    if (nread < 0) {
        stream->flags |= _IOERR;
        return EOF;
    }

    if (nread == 0) {
        stream->flags |= _IOEOF;
        return EOF;
    }

    stream->buf_pos = 0;
    stream->buf_len = nread;
    return 0;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

/**
 * fopen - Open a file
 */
FILE *fopen(const char *filename, const char *mode) {
    if (!filename || !mode) {
        return NULL;
    }

    /* Find free slot */
    int slot = _find_free_slot();
    if (slot < 0) {
        return NULL;  /* Too many open files */
    }

    /* Allocate FILE structure */
    FILE *fp = _alloc_file();
    if (!fp) {
        return NULL;
    }

    /* Parse mode string */
    unsigned char cbm_type = CBM_T_SEQ;
    unsigned char flags = _parse_mode(mode, &cbm_type);
    if (!flags) {
        free(fp);
        return NULL;
    }

    /* Open file via KERNAL */
    int handle = _kernal_open(filename, cbm_type, flags);
    if (handle < 0) {
        free(fp);
        return NULL;
    }

    /* Initialize FILE structure */
    fp->handle = handle;
    fp->mode = flags;
    fp->flags = 0;
    fp->position = 0;
    fp->file_size = 0;
    fp->backend = CBM_DEVICE_KERNAL;

    /* Allocate buffer */
    fp->buffer = (unsigned char *)malloc(BUFSIZ);
    if (!fp->buffer) {
        cbm_close(handle);
        free(fp);
        return NULL;
    }

    fp->buf_size = BUFSIZ;
    fp->buf_pos = 0;
    fp->buf_len = 0;
    fp->flags |= _IOMYBUF;  /* Mark buffer as malloc'd */

    /* Store in open files table */
    _open_files[slot] = fp;

    return fp;
}

/**
 * fclose - Close a file
 */
int fclose(FILE *stream) {
    if (!stream) {
        return EOF;
    }

    /* Prevent closing stdin/stdout/stderr */
    if (stream == stdin || stream == stdout || stream == stderr) {
        return 0;  /* Silently succeed */
    }

    /* Flush write buffer */
    if (stream->flags & _IOWRITE) {
        if (_flush_write_buffer(stream) < 0) {
            /* Continue closing even if flush fails */
        }
    }

    /* Close file handle */
    cbm_close(stream->handle);

    /* Free buffer if malloc'd */
    if (stream->flags & _IOMYBUF && stream->buffer) {
        free(stream->buffer);
    }

    /* Remove from open files table */
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (_open_files[i] == stream) {
            _open_files[i] = NULL;
            break;
        }
    }

    /* Free FILE structure */
    free(stream);

    return 0;
}

/**
 * freopen - Reopen a file
 */
FILE *freopen(const char *filename, const char *mode, FILE *stream) {
    if (!stream || !filename || !mode) {
        return NULL;
    }

    /* Close existing file */
    if (stream->handle != STDIN_FILENO &&
        stream->handle != STDOUT_FILENO &&
        stream->handle != STDERR_FILENO) {
        cbm_close(stream->handle);
    }

    /* Parse mode string */
    unsigned char cbm_type = CBM_T_SEQ;
    unsigned char flags = _parse_mode(mode, &cbm_type);
    if (!flags) {
        return NULL;
    }

    /* Open new file */
    int handle = _kernal_open(filename, cbm_type, flags);
    if (handle < 0) {
        return NULL;
    }

    /* Reset FILE structure */
    stream->handle = handle;
    stream->mode = flags;
    stream->flags = 0;
    stream->position = 0;
    stream->file_size = 0;
    stream->buf_pos = 0;
    stream->buf_len = 0;

    return stream;
}

/**
 * fflush - Flush stream buffer
 */
int fflush(FILE *stream) {
    if (!stream) {
        /* Flush all open files */
        for (int i = 0; i < FOPEN_MAX; i++) {
            if (_open_files[i] && (_open_files[i]->flags & _IOWRITE)) {
                if (_flush_write_buffer(_open_files[i]) < 0) {
                    return EOF;
                }
            }
        }
        return 0;
    }

    /* Flush specific stream */
    if (stream->flags & _IOWRITE) {
        return _flush_write_buffer(stream);
    }

    return 0;
}

/* ============================================================================
 * STATUS FUNCTIONS
 * ============================================================================ */

/**
 * feof - Test end-of-file indicator
 */
int feof(FILE *stream) {
    if (!stream) return 0;
    return (stream->flags & _IOEOF) != 0;
}

/**
 * ferror - Test error indicator
 */
int ferror(FILE *stream) {
    if (!stream) return 0;
    return (stream->flags & _IOERR) != 0;
}

/**
 * clearerr - Clear error and EOF indicators
 */
void clearerr(FILE *stream) {
    if (!stream) return;
    stream->flags &= ~(_IOEOF | _IOERR);
}

/**
 * fileno - Get file descriptor from FILE
 */
int fileno(FILE *stream) {
    if (!stream) return -1;
    return stream->handle;
}

