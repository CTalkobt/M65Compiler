/* stdio_file.c — FILE I/O implementation for MEGA65 C Compiler
 *
 * Core FILE operations: fopen, fclose, freopen, fflush
 * Phase 1 of FILE I/O implementation
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* ============================================================================
 * STATIC STATE
 * ============================================================================ */

/* Static buffer for stdin/stdout/stderr */
static unsigned char _stdin_buf[BUFSIZ];
static unsigned char _stdout_buf[BUFSIZ];
static unsigned char _stderr_buf[BUFSIZ];

/* Standard file pointers - allocated as globals */
FILE _stdin_file = {
    .handle = STDIN_FILENO,
    .mode = _IOREAD,
    .flags = _IOREAD,
    .position = 0,
    .file_size = 0,
    .buffer = _stdin_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

FILE _stdout_file = {
    .handle = STDOUT_FILENO,
    .mode = _IOWRITE,
    .flags = _IOWRITE,
    .position = 0,
    .file_size = 0,
    .buffer = _stdout_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

FILE _stderr_file = {
    .handle = STDERR_FILENO,
    .mode = _IOWRITE,
    .flags = _IOWRITE,
    .position = 0,
    .file_size = 0,
    .buffer = _stderr_buf,
    .buf_size = BUFSIZ,
    .buf_pos = 0,
    .buf_len = 0,
    .backend = CBM_DEVICE_KERNAL
};

FILE *stdin = &_stdin_file;
FILE *stdout = &_stdout_file;
FILE *stderr = &_stderr_file;

/* File descriptor allocation table */
static FILE *_open_files[FOPEN_MAX] = {
    &_stdin_file, &_stdout_file, &_stderr_file,
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
/* lib/stdlib_asm/cbm_parse_mode.s45 */
__fastcall__ unsigned char __cbm_parse_mode(const char *mode, unsigned char *cbm_type);
#define _parse_mode __cbm_parse_mode

/* Forward declarations from stdio_cbm.c */
extern fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary);
__fastcall__ int cbm_drive_status(unsigned char device);
/* lib/stdlib_asm/cbm_dos_name.s45 */
__fastcall__ void __cbm_dos_name(char *dst, const char *name, unsigned char flags, unsigned char cbm_type);
extern int cbm_read(fd_t fd, void *buffer, unsigned int length);
extern int cbm_write(fd_t fd, const void *buffer, unsigned int length);
extern int cbm_close(fd_t fd);

/**
 * Open file via Commodore KERNAL
 * Returns file handle (0-15) on success, -1 on error
 */
int _kernal_open(const char *filename, unsigned char cbm_type, unsigned char flags) {
    /* Determine device number (default: disk) */
    unsigned char device = CBM_DEVICE_DISK;
    /* The DOS takes the type and direction from the name: "NAME,S,R",
     * "@0:NAME,S,W" (the @ replaces an existing file, as C's "w" truncates),
     * "NAME,S,A"; U for a USR ("b") file. A name with a ',' is passed as is
     * (lib/stdlib_asm/cbm_dos_name.s45). */
    static char name[48];
    __cbm_dos_name(name, filename, flags, cbm_type);

    /* Each open file on its own drive channel (secondary address = its
     * logical file number) */
    fd_t fd = cbm_open_ext(device, name, cbm_type, 0xFF);
    if (fd < 0) return -1;

    /* OPEN succeeds even for a missing file: the drive reports errors
     * (62 FILE NOT FOUND, ...) on its command channel */
    signed int st = cbm_drive_status(device);
    if (st >= 20) {
        cbm_close(fd);
        return -1;
    }
    return (int)fd;
}

/**
 * Flush FILE write buffer to device
 * Returns 0 on success, EOF on error
 */
int _flush_write_buffer(FILE *stream) {
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
int _fill_read_buffer(FILE *stream) {
    /* a memory stream's buffer is the whole file: nothing more to read */
    if (!stream || !(stream->flags & _IOREAD) || stream->backend == _IOBACKEND_MEMORY) {
        if (stream) stream->flags |= _IOEOF;
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

/* A memory stream ('+' modes): grow the buffer to hold at least need bytes */
int _mem_reserve(FILE *fp, unsigned int need) {
    if (need <= fp->buf_size) return 0;
    unsigned int cap = fp->buf_size ? fp->buf_size : 256;
    while (cap < need) cap = cap > 0x7F80 ? 0xFF00 : cap * 2;
    unsigned char *nb = (unsigned char *)realloc(fp->buffer, cap);
    if (!nb) { errno = ENOMEM; return -1; }
    fp->buffer = nb;
    fp->buf_size = cap;
    return 0;
}

/* Write a memory stream's contents back to its file ("@0:" replaces it) */
static int _mem_writeback(FILE *fp) {
    int h = _kernal_open(fp->name, fp->cbm_type, _IOWRITE | _IOTRUNC);
    if (h < 0) { fp->flags |= _IOERR; return EOF; }
    unsigned int done = 0;
    while (done < fp->buf_len) {
        unsigned int chunk = fp->buf_len - done;
        if (chunk > 254) chunk = 254;
        int w = cbm_write(h, fp->buffer + done, chunk);
        if (w <= 0) { cbm_close(h); fp->flags |= _IOERR; return EOF; }
        done += (unsigned int)w;
    }
    cbm_close(h);
    fp->dirty = 0;
    return 0;
}

/* Open filename into fp (its fields cleared but name/buffer kept free).
 * '+' modes load the file into memory; "w+" creates it empty at once. */
static int _open_stream(FILE *fp, const char *filename, const char *mode) {
    unsigned char cbm_type = CBM_T_SEQ;
    unsigned char flags = _parse_mode(mode, &cbm_type);
    if (!flags) { errno = EINVAL; return -1; }

    fp->name = (char *)malloc(strlen(filename) + 1);
    if (!fp->name) { errno = ENOMEM; return -1; }
    strcpy(fp->name, filename);
    fp->cbm_type = cbm_type;
    fp->mode = flags;
    fp->flags = flags & (_IOREAD | _IOWRITE | _IOAPPEND | _IOBINARY);
    fp->position = 0;
    fp->file_size = 0;
    fp->buf_pos = 0;
    fp->buf_len = 0;
    fp->ungetc_pending = 0;
    fp->dirty = 0;

    if ((flags & _IOREAD) && (flags & _IOWRITE)) {
        /* r+, w+, a+: the whole file in memory */
        fp->backend = _IOBACKEND_MEMORY;
        fp->buffer = NULL;
        fp->buf_size = 0;
        fp->flags |= _IOMYBUF;
        if (flags & _IOTRUNC) {
            /* w+: an empty file, created now */
            fp->dirty = 1;
            if (_mem_writeback(fp) < 0) return -1;
            return 0;
        }
        int h = _kernal_open(filename, cbm_type, _IOREAD);
        if (h < 0) {
            if (!(flags & _IOAPPEND)) return -1;  /* r+: the file must exist */
            fp->dirty = 1;                        /* a+: created on write back */
            return 0;
        }
        for (;;) {
            if (fp->buf_len > 0xFF00 - 254 || _mem_reserve(fp, fp->buf_len + 254) < 0) {
                cbm_close(h);
                errno = ENOMEM;
                return -1;
            }
            int n = cbm_read(h, fp->buffer + fp->buf_len, 254);
            if (n <= 0) break;
            fp->buf_len += (unsigned int)n;
        }
        cbm_close(h);
        return 0;
    }

    int handle = _kernal_open(filename, cbm_type, flags);
    if (handle < 0) return -1;
    fp->handle = (unsigned char)handle;
    fp->backend = CBM_DEVICE_KERNAL;
    fp->buffer = (unsigned char *)malloc(BUFSIZ);
    if (!fp->buffer) {
        cbm_close(handle);
        errno = ENOMEM;
        return -1;
    }
    fp->buf_size = BUFSIZ;
    fp->flags |= _IOMYBUF;  /* Mark buffer as malloc'd */
    return 0;
}

/* Release what _open_stream allocated */
static void _free_stream(FILE *fp) {
    if ((fp->flags & _IOMYBUF) && fp->buffer) free(fp->buffer);
    fp->buffer = NULL;
    if (fp->name) free(fp->name);
    fp->name = NULL;
}

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

    if (_open_stream(fp, filename, mode) < 0) {
        _free_stream(fp);
        free(fp);
        return NULL;
    }

    /* Store in open files table */
    _open_files[slot] = fp;

    return fp;
}

/* Finish a stream: write out what is buffered, close the file */
static int _close_stream(FILE *stream) {
    int r = 0;
    if (stream->backend == _IOBACKEND_MEMORY) {
        if (stream->dirty && _mem_writeback(stream) < 0) r = EOF;
    } else {
        if ((stream->flags & _IOWRITE) && _flush_write_buffer(stream) < 0) r = EOF;
        cbm_close(stream->handle);
    }
    _free_stream(stream);
    return r;
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

    int r = _close_stream(stream);

    /* Remove from open files table */
    for (int i = 0; i < FOPEN_MAX; i++) {
        if (_open_files[i] == stream) {
            _open_files[i] = NULL;
            break;
        }
    }

    /* Free FILE structure */
    free(stream);

    return r;
}

/**
 * freopen - Reopen a file
 */
FILE *freopen(const char *filename, const char *mode, FILE *stream) {
    if (!stream || !filename || !mode) {
        return NULL;
    }

    /* Close existing file (writing out what is still buffered) */
    if (stream != stdin && stream != stdout && stream != stderr) {
        _close_stream(stream);
    }
    stream->flags = 0;
    stream->buffer = NULL;
    stream->name = NULL;

    if (_open_stream(stream, filename, mode) < 0) {
        _free_stream(stream);
        return NULL;
    }
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
                if (fflush(_open_files[i]) < 0) {
                    return EOF;
                }
            }
        }
        return 0;
    }

    /* Flush specific stream */
    if (stream->backend == _IOBACKEND_MEMORY) {
        return stream->dirty ? _mem_writeback(stream) : 0;
    }
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
 * setvbuf - Set buffering for a stream
 */
int setvbuf(FILE *stream, char *buffer, int mode, size_t size) {
    if (!stream) return -1;
    /* TODO: Implement custom buffering modes (fully buffered, line buffered, unbuffered) */
    return 0;
}

/**
 * setbuf - Set buffer (simplified setvbuf)
 */
void setbuf(FILE *stream, char *buffer) {
    if (!stream) return;
    if (buffer) {
        stream->buffer = (unsigned char *)buffer;
        stream->buf_size = BUFSIZ;
    } else {
        /* Unbuffered mode - TODO */
        stream->buf_size = 0;
    }
}

/**
 * tmpfile - Create temporary file
 */
FILE *tmpfile(void) {
    /* TODO: Implement temporary file creation */
    return NULL;
}

/**
 * tmpnam - Generate temporary filename
 */
char *tmpnam(char *s) {
    /* TODO: Implement temporary name generation */
    static char buf[L_tmpnam];
    if (!s) s = buf;
    return s;
}

