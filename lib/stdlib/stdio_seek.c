/* stdio_seek.c — fseek, ftell, rewind, fgetpos, fsetpos
 *
 * Apart from stdio_file.c so that a program that does not seek does not
 * link them (ln45 links whole objects).
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <errno.h>

int _fill_read_buffer(FILE *stream);
int _kernal_open(const char *filename, unsigned char cbm_type, unsigned char flags);
extern int cbm_close(fd_t fd);

/* A read stream: consume bytes up to position target (EOF stops early) */
static void _skip_to(FILE *stream, unsigned long target) {
    while (stream->position < target) {
        if (stream->buf_pos >= stream->buf_len) {
            if (_fill_read_buffer(stream) < 0 || stream->buf_len == 0) return;
        }
        unsigned long left = target - stream->position;
        unsigned int avail = stream->buf_len - stream->buf_pos;
        unsigned int step = left < avail ? (unsigned int)left : avail;
        stream->buf_pos += step;
        stream->position += step;
    }
}

/**
 * fseek - Seek to position in file
 *
 * A memory stream (r+, w+, a+) seeks anywhere; writing past its end fills
 * the gap with zeros. A stream opened for reading seeks forward by reading
 * and backward by reopening the file; SEEK_END reads to the end first. A
 * stream opened for writing ("w", "a") can only stay where it is: ESPIPE.
 */
int fseek(FILE *stream, long offset, int whence) {
    if (!stream || stream == stdin || stream == stdout || stream == stderr) {
        errno = ESPIPE;
        return -1;
    }
    int mem = stream->backend == _IOBACKEND_MEMORY;
    long base;
    if (whence == SEEK_SET) {
        base = 0;
    } else if (whence == SEEK_CUR) {
        base = mem ? (long)stream->buf_pos : (long)stream->position;
    } else if (whence == SEEK_END) {
        if (mem) {
            base = (long)stream->buf_len;
        } else if (stream->flags & _IOREAD) {
            stream->ungetc_pending = 0;
            _skip_to(stream, 0xFFFFFFFFUL);  /* to the end: the length */
            base = (long)stream->position;
        } else {
            base = (long)stream->position;  /* "w"/"a": written so far */
        }
    } else {
        errno = EINVAL;
        return -1;
    }
    long target = base + offset;
    if (target < 0) {
        errno = EINVAL;
        return -1;
    }

    stream->ungetc_pending = 0;
    if (mem) {
        if (target > 0xFF00L) {
            errno = EINVAL;
            return -1;
        }
        stream->buf_pos = (unsigned int)target;
    } else if (stream->flags & _IOREAD) {
        if ((unsigned long)target < stream->position) {
            /* back: reopen the file and read from the start */
            cbm_close(stream->handle);
            int h = _kernal_open(stream->name, stream->cbm_type, _IOREAD);
            if (h < 0) {
                stream->flags |= _IOERR;
                return -1;
            }
            stream->handle = (unsigned char)h;
            stream->position = 0;
            stream->buf_pos = 0;
            stream->buf_len = 0;
        }
        stream->flags &= ~_IOEOF;
        _skip_to(stream, (unsigned long)target);
        if (stream->position < (unsigned long)target) stream->position = (unsigned long)target;
    } else if ((unsigned long)target != stream->position) {
        /* a sequential file being written cannot move */
        errno = ESPIPE;
        return -1;
    }
    stream->flags &= ~_IOEOF;
    return 0;
}

/**
 * ftell - Get current position in file
 */
long ftell(FILE *stream) {
    if (!stream) return -1L;
    if (stream->backend == _IOBACKEND_MEMORY) return (long)stream->buf_pos - stream->ungetc_pending;
    return (long)stream->position;
}

/**
 * rewind - Seek to beginning of file
 */
void rewind(FILE *stream) {
    if (!stream) return;
    fseek(stream, 0L, SEEK_SET);
    clearerr(stream);
}

/**
 * fgetpos - Get current file position
 */
int fgetpos(FILE *stream, fpos_t *pos) {
    if (!stream || !pos) return -1;
    *pos = (fpos_t)ftell(stream);
    return 0;
}

/**
 * fsetpos - Set file position
 */
int fsetpos(FILE *stream, const fpos_t *pos) {
    if (!stream || !pos) return -1;
    return fseek(stream, (long)*pos, SEEK_SET);
}

