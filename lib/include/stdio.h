/* stdio.h — Standard I/O for cc45 / MEGA65
 *
 * Provides traditional C FILE I/O (fopen, fread, fwrite, etc.) and
 * Commodore-style file handle I/O for native disk/tape operations.
 */

#pragma once

#include <stddef.h>
#include <stdarg.h>

/* ============================================================================
 * CHARACTER & STRING I/O (KERNAL-based)
 * ============================================================================ */

int getchar(void);
int putchar(int c);
int puts(char *s);
int sprintf(char *buf, char *fmt, ...);
int snprintf(char *buf, int size, char *fmt, ...);
int printf(char *fmt, ...);
int sscanf(char *str, char *fmt, ...);
int vsprintf(char *buf, char *fmt, va_list ap);
int vsnprintf(char *buf, int size, char *fmt, va_list ap);

/* ============================================================================
 * FILE STRUCTURE & TRADITIONAL FILE I/O
 * ============================================================================ */

/* FILE structure for C standard library I/O
 *
 * On MEGA65/Commodore, this wraps the native Commodore file handle
 * and provides buffering. Can use either disk45 or KERNAL for backend.
 */
typedef struct {
    unsigned char handle;           /* Commodore file handle (0-255) */
    unsigned char mode;              /* File mode (READ, WRITE, APPEND) */
    unsigned char flags;             /* _IOREAD, _IOWRITE, _IOEOF, _IOERR, _IOMYBUF */
    unsigned long position;          /* Current position in file */
    unsigned long file_size;         /* Total file size (0 if unknown) */
    unsigned char *buffer;           /* I/O buffer (if buffered) */
    unsigned int buf_size;           /* Buffer size */
    unsigned int buf_pos;            /* Position in buffer */
    unsigned int buf_len;            /* Bytes in buffer */
    unsigned char backend;           /* 0=KERNAL, 1=disk45, 2=memory */
    unsigned char ungetc_char;       /* Character pushed back by ungetc() */
} FILE;

/* File position type */
typedef unsigned long fpos_t;

/* File open modes */
#define _IOREAD     0x01             /* File opened for reading */
#define _IOWRITE    0x02             /* File opened for writing */
#define _IOAPPEND   0x04             /* File opened for appending */
#define _IOBINARY   0x08             /* Binary mode (vs text) */
#define _IOEOF      0x10             /* EOF reached */
#define _IOERR      0x20             /* Error occurred */
#define _IOMYBUF    0x40             /* Buffer allocated by FILE */
#define _IOTRUNC    0x80             /* Truncate file on open (for write mode) */
#define _IOUNGETC   0x01             /* Ungetc character pending (reuses as needed) */

/* Standard file pointers */
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

/* File operation constants */
#define EOF         (-1)
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2
#define BUFSIZ      256              /* Default buffer size */
#define FILENAME_MAX 16              /* Max filename length (Commodore) */
#define FOPEN_MAX   8                /* Max simultaneous open files */
#define L_tmpnam    32               /* Length of temp filename */

/* Stream file operations */
FILE *fopen(const char *filename, const char *mode);
FILE *freopen(const char *filename, const char *mode, FILE *stream);
int fclose(FILE *stream);
int fflush(FILE *stream);

/* Character I/O */
int fgetc(FILE *stream);
int fputc(int c, FILE *stream);
char *fgets(char *str, int size, FILE *stream);
int fputs(const char *str, FILE *stream);
int ungetc(int c, FILE *stream);

/* Block I/O */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

/* Formatted I/O */
int fprintf(FILE *stream, const char *format, ...);
int vfprintf(FILE *stream, const char *format, va_list ap);
int fscanf(FILE *stream, const char *format, ...);
int vfscanf(FILE *stream, const char *format, va_list ap);

/* File positioning */
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int fgetpos(FILE *stream, fpos_t *pos);
int fsetpos(FILE *stream, const fpos_t *pos);

/* File status */
int feof(FILE *stream);
int ferror(FILE *stream);
void clearerr(FILE *stream);
int fileno(FILE *stream);

/* Temporary files */
FILE *tmpfile(void);
char *tmpnam(char *str);

/* Buffer control */
int setvbuf(FILE *stream, char *buffer, int mode, size_t size);
void setbuf(FILE *stream, char *buffer);

/* ============================================================================
 * COMMODORE FILE HANDLE I/O (Unix-style, lightweight)
 * ============================================================================
 *
 * Lower-level file handle operations that directly map to Commodore
 * file operations. More efficient for embedded systems with limited
 * resources. Can be used alongside FILE operations.
 */

typedef int fd_t;                    /* File descriptor (Commodore device/channel) */

/* Standard file descriptors */
#define STDIN_FILENO    0            /* Standard input (keyboard/serial) */
#define STDOUT_FILENO   1            /* Standard output (screen) */
#define STDERR_FILENO   2            /* Standard error (screen) */

/* Device types for file operations */
#define CBM_DEVICE_DISK     8        /* 1541/1571/1581 disk drive */
#define CBM_DEVICE_TAPE     1        /* Cassette tape */
#define CBM_DEVICE_SERIAL   0        /* Serial/Network (future) */
#define CBM_DEVICE_KERNAL   255      /* Use KERNAL routines */

/* File access modes (for open) */
#define O_RDONLY    0x00             /* Read only */
#define O_WRONLY    0x01             /* Write only */
#define O_RDWR      0x02             /* Read and write */
#define O_CREAT     0x04             /* Create if doesn't exist */
#define O_TRUNC     0x08             /* Truncate to zero length */
#define O_APPEND    0x10             /* Append mode */
#define O_BINARY    0x20             /* Binary mode */

/* File types (Commodore) */
#define CBM_T_PRG   0x82             /* Program file */
#define CBM_T_SEQ   0x81             /* Sequential file */
#define CBM_T_USR   0x84             /* User file */
#define CBM_T_REL   0x85             /* Relative file */
#define CBM_T_CBM   0x88             /* Directory */
#define CBM_T_DIR   0x88             /* Directory alias */

/* File type masks and flags (directory entry byte 18) */
#define CBM_FILE_TYPE_MASK  0x0F     /* Mask for file type in lower nibble */
#define CBM_FILE_CLOSED     0x80     /* Bit 7: File closed properly (1=closed) */
#define CBM_FILE_LOCKED     0x40     /* Bit 6: File locked (1=locked) */
#define CBM_FILE_SPLAT      0x20     /* Bit 5: Splat/Scratched (1=marked for delete) */
#define CBM_FILE_NEWFILE    0x10     /* Bit 4: New file entry indicator (future use) */

/* File handle operations */
fd_t cbm_open(unsigned char device, const char *filename, unsigned char type);
fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary);
int cbm_close(fd_t fd);
int cbm_read(fd_t fd, void *buffer, unsigned int length);
int cbm_write(fd_t fd, const void *buffer, unsigned int length);
int cbm_seek(fd_t fd, unsigned long position);
unsigned long cbm_tell(fd_t fd);
int cbm_eof(fd_t fd);
int cbm_status(fd_t fd);

/* Directory operations */
int cbm_opendir(unsigned char device);
int cbm_readdir(fd_t fd, char *filename, unsigned char *type, unsigned int *size);
int cbm_closedir(fd_t fd);

/* Directory entry flag helpers */
const char *cbm_dir_type_name(unsigned char type);      /* Get type name (handles flags) */
unsigned char cbm_dir_get_type(unsigned char type);     /* Extract pure type code */
int cbm_dir_is_locked(unsigned char type);              /* Check locked bit */
int cbm_dir_is_scratched(unsigned char type);           /* Check scratched/splat bit */
int cbm_dir_is_closed(unsigned char type);              /* Check closed bit */

/* Device operations */
int cbm_format(unsigned char device, const char *name, const char *id);
int cbm_delete(unsigned char device, const char *filename);
int cbm_rename(unsigned char device, const char *old_name, const char *new_name);

/* REL (Relative) file operations — random-access record files */
int cbm_rel_open(unsigned char device, const char *filename,
                 unsigned char record_size, int mode);
int cbm_rel_read(int handle, unsigned int record_num, void *buffer, unsigned int size);
int cbm_rel_write(int handle, unsigned int record_num, const void *buffer, unsigned int size);
int cbm_rel_position(int handle, unsigned int record_num);
unsigned int cbm_rel_tell(int handle);
unsigned int cbm_rel_size(int handle);
int cbm_rel_close(int handle);
unsigned char cbm_rel_record_size(int handle);
int cbm_rel_is_open(int handle);

#define CBM_REL_READ    0               /* Open for reading */
#define CBM_REL_WRITE   1               /* Open for writing */

/* Low-level Commodore KERNAL calls (advanced) */
unsigned char cbm_k_chkin(unsigned char fd);      /* Redirect input */
unsigned char cbm_k_chkout(unsigned char fd);     /* Redirect output */
unsigned char cbm_k_clrchn(void);                 /* Clear channels */
unsigned char cbm_k_readst(void);                 /* Read status byte */
unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary);
unsigned char cbm_k_setnam(const char *filename); /* Set filename */
unsigned char cbm_k_open(void);                   /* Open file */
unsigned char cbm_k_close(unsigned char fd);      /* Close file */

/* ============================================================================
 * CONVENIENCE MACROS
 * ============================================================================ */

/* Convert between FILE* and file descriptor */
#define fileno(stream)      ((stream)->handle)
#define fdopen(fd, mode)    ((FILE *)(fd))  /* Simple cast for advanced use */
