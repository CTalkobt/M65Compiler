/* stdio_cbm.c — Commodore file I/O operations for MEGA65
 *
 * Low-level file operations using KERNAL routines
 * Implements: cbm_read, cbm_write, cbm_seek, cbm_tell, cbm_status, etc.
 *
 * Phase 1: Low-Level File I/O
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations from stdio_kernal.c */
extern unsigned char cbm_k_chkin(unsigned char fd);
extern unsigned char cbm_k_chkout(unsigned char fd);
extern unsigned char cbm_k_clrchn(void);
extern unsigned char cbm_k_chrin(void);
extern void cbm_k_chrout(unsigned char c);
extern unsigned char cbm_k_readst(void);
extern unsigned char cbm_k_setnam(const char *filename);
extern unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary);
extern unsigned char cbm_k_open(void);
extern unsigned char cbm_k_close(unsigned char fd);

/* KERNAL I/O status (READST) bits */
#define STATUS_TIMEOUT_W 0x01   /* serial bus write timeout */
#define STATUS_TIMEOUT_R 0x02   /* serial bus read timeout */
#define STATUS_VERIFY    0x10   /* verify / read error */
#define STATUS_EOF       0x40   /* end of file */
#define STATUS_DEVICE    0x80   /* device not present */
#define STATUS_ERROR     (STATUS_TIMEOUT_W | STATUS_TIMEOUT_R | STATUS_VERIFY)
/* A missing file or write protection is reported by the drive's error
 * channel, not in the status byte */
#define STATUS_NOTFND    0x00
#define STATUS_WPROT     0x00

/* ============================================================================
 * FILE OPENING
 * ============================================================================ */

/**
 * cbm_open - Open a file on a Commodore device
 *
 * Parameters:
 *   device   - Device number (8=disk, 1=tape, 0=keyboard)
 *   filename - Null-terminated filename string
 *   type     - CBM file type (CBM_T_PRG, CBM_T_SEQ, CBM_T_USR, etc.)
 *
 * Returns:
 *   File descriptor (3-14) on success, -1 on error
 *
 * Implementation:
 * 1. Find free logical file number (3-14)
 * 2. Determine secondary address from type
 * 3. Set filename and file parameters
 * 4. Open the file
 * 5. Return file descriptor or error
 */

/* Logical file numbers 3-14 in use (15 is used for the drive's command
 * channel), and whether a file's read reached its end */
static unsigned char lfn_used[15];
static unsigned char lfn_eof[15];
/* For cbm_tell / cbm_seek: bytes read or written, whether the file was
 * written, and what reopens it (device, secondary address, name; a name
 * that does not fit can't be reopened) */
#define LFN_NAME_MAX 24
static unsigned long lfn_pos[15];
static unsigned char lfn_wrote[15];
static unsigned char lfn_dev[15];
static unsigned char lfn_sa[15];
static char lfn_name[15][LFN_NAME_MAX];

static int alloc_lfn(void) {
    unsigned char i;
    for (i = 3; i <= 14; i++) {
        if (!lfn_used[i]) {
            lfn_used[i] = 1; lfn_eof[i] = 0; lfn_pos[i] = 0; lfn_wrote[i] = 0;
            return i;
        }
    }
    return -1;
}

/* OPEN logical file lfn on device with secondary address sa */
static fd_t open_lfn(unsigned char device, const char *filename, unsigned char sa) {
    int fd;
    if (!filename || device == 0) return -1;
    fd = alloc_lfn();
    if (fd < 0) return -1;
    if (sa == 0xFF) sa = (unsigned char)fd;
    lfn_dev[fd] = device;
    lfn_sa[fd] = sa;
    if (strlen(filename) < LFN_NAME_MAX) strcpy(lfn_name[fd], filename);
    else lfn_name[fd][0] = 0;
    cbm_k_setnam(filename);
    cbm_k_setlfs((unsigned char)fd, device, sa);
    if (cbm_k_open() != 0) {
        lfn_used[fd] = 0;
        return -1;
    }
    return fd;
}

/* type is not used: the DOS takes the file type and direction from the
 * name ("NAME,S,W"); secondary address 0 reads (a directory "$" as a
 * BASIC listing) */
fd_t cbm_open(unsigned char device, const char *filename, unsigned char type) {
    (void)type;
    return open_lfn(device, filename, 0);
}

/**
 * cbm_open_ext - open with an explicit secondary address; $FF uses the
 * logical file number (3-14), which gives each open file its own channel
 */
fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary) {
    (void)type;
    return open_lfn(device, filename, secondary);
}

/**
 * cbm_drive_status - the number of the drive's status message (command
 * channel 15): 0 = OK, >= 20 an error; -1 if the channel can't be opened
 */
/* The device the drive's command channel (logical file 15) is open on, 0 =
 * none (lib/stdlib_asm/cbm_drive_status.s45 opens it on first use and keeps
 * it open: closing it makes the drive close every file it has open) */
extern unsigned char __cbm_cmd_device;

/**
 * cbm_close - Close a file
 */

int cbm_close(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return -1;
    }

    unsigned char status = cbm_k_close((unsigned char)fd);
    unsigned char i;
    lfn_used[fd] = 0;
    /* the last file closed: the command channel too */
    if (__cbm_cmd_device) {
        for (i = 3; i <= 14 && !lfn_used[i]; i++) ;
        if (i > 14) { cbm_k_close(15); __cbm_cmd_device = 0; }
    }
    return (status == 0) ? 0 : -1;
}

/* ============================================================================
 * FILE I/O
 * ============================================================================ */

/**
 * cbm_read - Read data from a file
 *
 * Parameters:
 *   fd       - File descriptor from cbm_open()
 *   buffer   - Buffer to read into
 *   length   - Number of bytes to read
 *
 * Returns:
 *   Number of bytes read, or -1 on error
 *
 * Implementation:
 * 1. Redirect input from file via CHKIN
 * 2. Read bytes one at a time via CHRIN
 * 3. Check status after each read for EOF
 * 4. Restore input via CLRCHN
 * 5. Return bytes read
 */

int cbm_read(fd_t fd, void *buffer, unsigned int length) {
    /* stdin: a line typed on the keyboard (getchar: GETIN, waiting), echoed
       through the ROM (putchar), DEL taking a character back, RETURN ending
       it: its '\n' ($0D) included; control characters other than those
       are dropped */
    if (fd == STDIN_FILENO && buffer && length) {
        unsigned char *line = (unsigned char *)buffer;
        unsigned int n = 0;
        while (n < length) {
            unsigned char c = (unsigned char)getchar();
            if (c == '\n') {
                putchar('\n');
                line[n++] = c;
                break;
            }
            if (c == 0x14) {
                if (n) {
                    n--;
                    putchar(0x14);
                }
            } else if ((c & 0x7F) >= 0x20) {
                putchar(c);
                line[n++] = c;
            }
        }
        return (int)n;
    }
    if (fd < 3 || fd > 14 || !buffer || length == 0) {
        return -1;
    }

    unsigned char *buf = (unsigned char *)buffer;
    unsigned int bytes_read = 0;

    /* The end was reached by an earlier read */
    if (lfn_eof[fd]) return 0;

    /* Redirect input from file */
    if (cbm_k_chkin((unsigned char)fd) != 0) {
        return -1;
    }

    /* Read bytes from file */
    for (unsigned int i = 0; i < length; i++) {
        /* Read one byte */
        unsigned char c = cbm_k_chrin();

        /* Check status */
        unsigned char status = cbm_k_readst();

        if (status & (STATUS_ERROR | STATUS_DEVICE)) {
            /* Error occurred */
            cbm_k_clrchn();
            return bytes_read ? (int)bytes_read : -1;
        }

        /* Store byte; the EOF status comes with the last byte, which is
           data */
        buf[i] = c;
        bytes_read++;
        lfn_pos[fd]++;
        if (status & STATUS_EOF) {
            lfn_eof[fd] = 1;
            break;
        }
    }

    /* Restore input and return */
    cbm_k_clrchn();
    return bytes_read;
}

/**
 * cbm_write - Write data to a file
 *
 * Parameters:
 *   fd       - File descriptor from cbm_open()
 *   buffer   - Buffer to write from
 *   length   - Number of bytes to write
 *
 * Returns:
 *   Number of bytes written, or -1 on error
 *
 * Implementation:
 * 1. Redirect output to file via CHKOUT
 * 2. Write bytes one at a time via CHROUT
 * 3. Check status after each write
 * 4. Restore output via CLRCHN
 * 5. Return bytes written
 */

int cbm_write(fd_t fd, const void *buffer, unsigned int length) {
    if (fd < 3 || fd > 14 || !buffer || length == 0) {
        return -1;
    }

    const unsigned char *buf = (const unsigned char *)buffer;
    unsigned int bytes_written = 0;

    /* Redirect output to file */
    if (cbm_k_chkout((unsigned char)fd) != 0) {
        return -1;
    }

    /* Write bytes to file */
    for (unsigned int i = 0; i < length; i++) {
        /* Write one byte */
        cbm_k_chrout(buf[i]);

        /* Check status */
        unsigned char status = cbm_k_readst();

        if (status & (STATUS_ERROR | STATUS_DEVICE | STATUS_WPROT)) {
            /* Error occurred */
            cbm_k_clrchn();
            return -1;
        }

        bytes_written++;
        lfn_pos[fd]++;
    }
    lfn_wrote[fd] = 1;

    /* Restore output and return */
    cbm_k_clrchn();
    return bytes_written;
}

/* ============================================================================
 * FILE POSITIONING
 * ============================================================================ */

/**
 * cbm_seek - Move a file opened for reading to byte position
 *
 * Forward by reading, backward by reopening the file (same device,
 * secondary address and name) and reading from the start. A file that was
 * written can only stay where it is. Returns 0, or -1 when the position
 * can't be reached (past the end: the file is then at its end).
 */
int cbm_seek(fd_t fd, unsigned long position) {
    unsigned char c;
    if (fd < 3 || fd > 14 || !lfn_used[fd]) {
        return -1;
    }
    if (position == lfn_pos[fd]) return 0;
    if (lfn_wrote[fd]) return -1;
    if (position < lfn_pos[fd]) {
        if (!lfn_name[fd][0]) return -1;
        cbm_k_close((unsigned char)fd);
        cbm_k_setnam(lfn_name[fd]);
        cbm_k_setlfs((unsigned char)fd, lfn_dev[fd], lfn_sa[fd]);
        if (cbm_k_open() != 0) {
            lfn_used[fd] = 0;
            return -1;
        }
        lfn_pos[fd] = 0;
        lfn_eof[fd] = 0;
    }
    while (lfn_pos[fd] < position) {
        if (cbm_read(fd, &c, 1) != 1) return -1;
    }
    return 0;
}

/**
 * cbm_tell - The number of bytes read from or written to the file
 */
unsigned long cbm_tell(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return 0;
    }
    return lfn_pos[fd];
}

/**
 * cbm_eof - 1 once a read has reached the end of the file, else 0
 */
int cbm_eof(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return 0;
    }
    return lfn_eof[fd];
}

/**
 * cbm_status - Get current status byte
 *
 * Returns: Status byte from KERNAL READST
 *
 * Status bits:
 * - Bit 6: EOF
 * - Bit 7: Device not present
 */

int cbm_status(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return -1;
    }

    return cbm_k_readst();
}

/* ============================================================================
 * DIRECTORY OPERATIONS
 * ============================================================================
 * (Stubs for Phase 4)
 */

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================
 * (Stubs for Phase 4)
 */

