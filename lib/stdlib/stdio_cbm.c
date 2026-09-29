/* stdio_cbm.c — Commodore file I/O operations for MEGA65
 *
 * Low-level file operations using KERNAL routines
 * Implements: cbm_read, cbm_write, cbm_seek, cbm_tell, cbm_status, etc.
 *
 * Phase 1: Low-Level File I/O
 */

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

fd_t cbm_open(unsigned char device, const char *filename, unsigned char type) {
    if (!filename || device == 0) {
        return -1;
    }

    /* Find free logical file number */
    static unsigned char next_fd = 3;
    unsigned char fd = next_fd;

    if (fd > 14) {
        fd = 3;  /* Wrap around */
        next_fd = 3;
    }

    /* Determine secondary address from file type */
    unsigned char secondary = 0;  /* Read mode */
    if (type == CBM_T_CBM) {
        secondary = 0;  /* Directory listing */
    } else {
        secondary = 0;  /* Default to read */
    }

    /* Set filename */
    if (cbm_k_setnam(filename) != 0) {
        return -1;
    }

    /* Set file parameters */
    if (cbm_k_setlfs(fd, device, secondary) != 0) {
        return -1;
    }

    /* Open the file */
    unsigned char status = cbm_k_open();
    if (status != 0) {
        return -1;
    }

    /* Increment for next open */
    next_fd = (fd + 1 > 14) ? 3 : (fd + 1);

    return fd;
}

/**
 * cbm_open_ext - Extended file open with explicit secondary address
 */

fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary) {
    if (!filename || device == 0) {
        return -1;
    }

    /* Find free logical file number */
    static unsigned char next_fd_ext = 3;
    unsigned char fd = next_fd_ext;

    if (fd > 14) {
        fd = 3;
        next_fd_ext = 3;
    }

    /* Set filename */
    if (cbm_k_setnam(filename) != 0) {
        return -1;
    }

    /* Set file parameters with explicit secondary address */
    if (cbm_k_setlfs(fd, device, secondary) != 0) {
        return -1;
    }

    /* Open the file */
    unsigned char status = cbm_k_open();
    if (status != 0) {
        return -1;
    }

    next_fd_ext = (fd + 1 > 14) ? 3 : (fd + 1);

    return fd;
}

/**
 * cbm_close - Close a file
 */

int cbm_close(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return -1;
    }

    unsigned char status = cbm_k_close((unsigned char)fd);
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
    if (fd < 3 || fd > 14 || !buffer || length == 0) {
        return -1;
    }

    unsigned char *buf = (unsigned char *)buffer;
    unsigned int bytes_read = 0;

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

        if (status & STATUS_EOF) {
            /* EOF reached */
            cbm_k_clrchn();
            return bytes_read;
        }

        if (status & (STATUS_ERROR | STATUS_DEVICE | STATUS_NOTFND)) {
            /* Error occurred */
            cbm_k_clrchn();
            return -1;
        }

        /* Store byte and continue */
        buf[i] = c;
        bytes_read++;
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
    }

    /* Restore output and return */
    cbm_k_clrchn();
    return bytes_written;
}

/* ============================================================================
 * FILE POSITIONING
 * ============================================================================ */

/**
 * cbm_seek - Seek to position in file
 *
 * Note: Not all Commodore file types support seeking.
 * Sequential files (.seq) cannot seek reliably.
 * Relative files (.rel) support seeking.
 * Disk files may support seeking depending on drive firmware.
 *
 * For now, returns error as seeking requires device-specific commands.
 */

int cbm_seek(fd_t fd, unsigned long position) {
    if (fd < 3 || fd > 14) {
        return -1;
    }

    /* TODO: Implement seeking
     * This would require:
     * 1. Device-specific commands (e.g., POSITION for 1581)
     * 2. Or re-opening the file and reading forward
     * 3. For now, return error
     */

    return -1;
}

/**
 * cbm_tell - Get current position in file
 *
 * Note: Not reliably supported on Commodore drives.
 * Sequential files don't track position.
 */

unsigned long cbm_tell(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return 0;
    }

    /* TODO: Implement tell
     * Would require position tracking in drive firmware
     * For now, return 0
     */

    return 0;
}

/**
 * cbm_eof - Check if end-of-file reached
 *
 * Returns: 1 if at EOF, 0 otherwise
 */

int cbm_eof(fd_t fd) {
    if (fd < 3 || fd > 14) {
        return 0;
    }

    unsigned char status = cbm_k_readst();
    return (status & STATUS_EOF) ? 1 : 0;
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

/**
 * cbm_opendir - Open directory listing
 */

int cbm_opendir(unsigned char device) {
    /* TODO: Implement directory listing
     * Open "$" (directory) file for reading
     * Parse directory format from device
     */
    return -1;
}

/**
 * cbm_readdir - Read next directory entry
 */

int cbm_readdir(fd_t fd, char *filename, unsigned char *type, unsigned int *size) {
    /* TODO: Implement directory reading
     * Parse directory entry format:
     * - 2 bytes: size (little-endian)
     * - 16 bytes: filename (padded)
     * - 1 byte: file type
     */
    return 0;
}

/**
 * cbm_closedir - Close directory
 */

int cbm_closedir(fd_t fd) {
    return cbm_close(fd);
}

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================
 * (Stubs for Phase 4)
 */

/**
 * cbm_delete - Delete a file
 */

int cbm_delete(unsigned char device, const char *filename) {
    /* TODO: Implement file deletion
     * Use KERNAL SCRATCH command format:
     * Device responds to "S-FILENAME"
     */
    return -1;
}

/**
 * cbm_rename - Rename a file
 */

int cbm_rename(unsigned char device, const char *old_name, const char *new_name) {
    /* TODO: Implement file renaming
     * Use KERNAL RENAME command format:
     * Device responds to "R-NEWNAME=OLDNAME"
     */
    return -1;
}

/**
 * cbm_format - Format a disk
 */

int cbm_format(unsigned char device, const char *name, const char *id) {
    /* TODO: Implement disk formatting
     * Use KERNAL format command format
     * WARNING: This is destructive! Requires user confirmation.
     */
    return -1;
}

