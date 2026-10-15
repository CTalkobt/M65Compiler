/* stdio_directory.c — Directory and file operations
 *
 * Implements Commodore file system operations:
 * - cbm_opendir, cbm_readdir, cbm_closedir — directory listing
 * - cbm_delete — delete files
 * - cbm_rename — rename files
 * - cbm_format — format disks
 *
 * Phase 3 of FILE I/O implementation
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations from stdio_cbm.c */
extern fd_t cbm_open(unsigned char device, const char *filename, unsigned char type);
extern int cbm_close(fd_t fd);
extern int cbm_read(fd_t fd, void *buffer, unsigned int length);
extern int cbm_status(fd_t fd);

/* ============================================================================
 * INTERNAL DIRECTORY PARSER
 * ============================================================================
 *
 * Commodore directory listing format:
 * Each entry is 32 bytes:
 *   Offset  Size  Description
 *   ------  ----  -----------
 *   0-1     2     File size in blocks (little-endian)
 *   2-17    16    Filename (padded with $A0)
 *   18      1     File type (0x82=PRG, 0x81=SEQ, 0x84=USR, 0x85=REL)
 *   19-23   5     (depends on drive type)
 *
 * Directory is opened as a special "$" file and read sequentially.
 */

typedef struct {
    unsigned short size;           /* File size in blocks */
    char filename[16];             /* Filename (padded) */
    unsigned char type;            /* File type */
    unsigned char flags;           /* Flags (locked, etc.) */
    unsigned long reserved;        /* Reserved for future use */
} DirectoryEntry;

/* Directory handle structure */
typedef struct {
    fd_t fd;                       /* Open file descriptor for directory */
    unsigned char device;          /* Device number */
    unsigned char eof;             /* EOF flag */
} DirectoryHandle;

/* ============================================================================
 * DIRECTORY OPERATIONS
 * ============================================================================ */

/* The drive sends the directory as a BASIC program (secondary address 0):
 * a load address, then lines of a 2-byte link (0 at the end), a 2-byte
 * line number (the size in blocks) and text up to a 0 byte. The first line
 * is the header (the disk name), the last "BLOCKS FREE.". One line into
 * *num and text; 1 at the end */
static int dir_line(fd_t fd, unsigned int *num, unsigned char *text, int max) {
    unsigned char b[2];
    unsigned char c;
    int n = 0;
    if (cbm_read(fd, b, 2) != 2 || (b[0] == 0 && b[1] == 0)) return 1;
    if (cbm_read(fd, b, 2) != 2) return 1;
    *num = b[0] | ((unsigned int)b[1] << 8);
    for (;;) {
        if (cbm_read(fd, &c, 1) != 1) return 1;
        if (c == 0) break;
        if (n < max - 1) text[n++] = c;
    }
    text[n] = 0;
    return 0;
}

/**
 * cbm_opendir - Open a device's directory; returns a handle for
 * cbm_readdir / cbm_closedir, or -1
 */
int cbm_opendir(unsigned char device) {
    unsigned char load[2];
    unsigned char text[40];
    unsigned int num;
    if (device == 0) {
        return -1;  /* Invalid device */
    }

    fd_t fd = cbm_open(device, "$", CBM_T_SEQ);
    if (fd < 0) {
        return -1;
    }
    /* Skip the load address and the header line */
    if (cbm_read(fd, load, 2) != 2 || dir_line(fd, &num, text, sizeof text) != 0 ||
        cbm_drive_status(device) >= 20) {
        cbm_close(fd);
        return -1;
    }
    return (int)fd;
}

/* The directory listing's type names (as the drive sends them; cc45 writes
 * "del" with the same bytes) */
static const char dir_types[7][4] = { "del", "seq", "prg", "usr", "rel", "cbm", "dir" };

/**
 * cbm_readdir - Read the next directory entry
 *
 *   filename — buffer for the name (17 bytes)
 *   type     — the type byte as in the directory: the type code (CBM_T_*
 *              & $0F) | CBM_FILE_CLOSED unless the listing marks the file
 *              unclosed ("*SEQ") | CBM_FILE_LOCKED for "SEQ<"
 *   size     — the size in blocks
 *
 * Returns 0 for an entry, 1 at the end of the directory, -1 on error.
 */
int cbm_readdir(int fd_handle, char *filename, unsigned char *type, unsigned int *size) {
    unsigned char text[40];
    unsigned int num;
    int i, j, t;
    unsigned char flags = CBM_FILE_CLOSED;
    if (fd_handle < 0 || !filename || !type || !size) {
        return -1;
    }
    if (dir_line((fd_t)fd_handle, &num, text, sizeof text) != 0) return 1;

    /* The name between quotes; "BLOCKS FREE." has none */
    for (i = 0; text[i] && text[i] != '"'; i++) ;
    if (!text[i]) return 1;
    i++;
    for (j = 0; text[i] && text[i] != '"' && j < 16; i++, j++) filename[j] = (char)text[i];
    filename[j] = '\0';
    if (text[i] == '"') i++;

    /* The type: [*]XXX[<] */
    while (text[i] == ' ') i++;
    if (text[i] == '*') { flags = 0; i++; }
    for (t = 0; t < 7; t++)
        if (text[i] == (unsigned char)dir_types[t][0] && text[i + 1] == (unsigned char)dir_types[t][1] &&
            text[i + 2] == (unsigned char)dir_types[t][2]) break;
    if (t == 7) return -1;
    if (text[i + 3] == '<') flags |= CBM_FILE_LOCKED;

    *size = num;
    *type = (unsigned char)(flags | t);
    return 0;
}

/**
 * cbm_closedir - Close directory listing
 *
 * Closes the directory file handle.
 *
 * Parameters:
 *   fd — Directory handle from cbm_opendir()
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cbm_closedir(int fd_handle) {
    if (fd_handle < 0) {
        return -1;
    }

    return cbm_close((fd_t)fd_handle);
}

/* ============================================================================
 * FILE OPERATIONS
 * ============================================================================ */

/**
 * cbm_delete - Delete a file
 *
 * Deletes a file using the Commodore SCRATCH command.
 * Device interprets "S-FILENAME" as a scratch command.
 *
 * Parameters:
 *   device   — Device number
 *   filename — File to delete
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cbm_delete(unsigned char device, const char *filename) {
    if (device == 0 || !filename) {
        return -1;
    }

    /* Commodore deletion uses special SCRATCH command
     * Format: "S-FILENAME" or ":FILENAME,S"
     * We'll use the simpler approach: open for write with special handling
     *
     * TODO: Implement actual SCRATCH command via SETNAM
     * For now, return error as this requires special KERNAL handling
     */

    return -1;  /* Not yet implemented */
}

/**
 * cbm_rename - Rename a file
 *
 * Renames a file using the Commodore RENAME command.
 * Device interprets "R-NEWNAME=OLDNAME" as a rename command.
 *
 * Parameters:
 *   device   — Device number
 *   old_name — Original filename
 *   new_name — New filename
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cbm_rename(unsigned char device, const char *old_name, const char *new_name) {
    if (device == 0 || !old_name || !new_name) {
        return -1;
    }

    /* Commodore renaming uses special RENAME command
     * Format: "R-NEWNAME=OLDNAME"
     *
     * TODO: Implement actual RENAME command via SETNAM
     * For now, return error as this requires special KERNAL handling
     */

    return -1;  /* Not yet implemented */
}

/**
 * cbm_format - Format a disk
 *
 * Formats a disk with a new name and ID.
 * WARNING: This is DESTRUCTIVE and erases all data!
 *
 * Parameters:
 *   device — Device number
 *   name   — Volume name (16 characters max)
 *   id     — Disk ID (typically "00")
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Note: Requires explicit confirmation or should be protected
 */
int cbm_format(unsigned char device, const char *name, const char *id) {
    if (device == 0 || !name || !id) {
        return -1;
    }

    /* Commodore formatting uses special NEW/FORMAT command
     * Format: "N-DISKNAME,ID"
     *
     * WARNING: This is DESTRUCTIVE!
     *
     * TODO: Implement actual FORMAT command via SETNAM
     * Should require explicit confirmation from user
     */

    return -1;  /* Not yet implemented */
}

/* ============================================================================
 * DIRECTORY ITERATION HELPER
 * ============================================================================
 *
 * Convenience function for iterating through directory entries.
 * Usage:
 *   int dir = cbm_opendir(8);
 *   if (dir >= 0) {
 *       char filename[17];
 *       unsigned char type;
 *       unsigned int size;
 *       while (cbm_readdir(dir, filename, &type, &size) == 0) {
 *           printf("%s (%d blocks)\n", filename, size);
 *       }
 *       cbm_closedir(dir);
 *   }
 */

/**
 * cbm_dir_type_name - Get human-readable file type name
 *
 * Converts a Commodore file type code to a string.
 * Extracts the type code from the directory entry byte and ignores flags.
 *
 * Parameters:
 *   type — File type code from directory (may include flags in upper bits)
 *
 * Returns:
 *   Pointer to string: "PRG", "SEQ", "USR", "REL", "DIR", or "???"
 *
 * Example:
 *   cbm_dir_type_name(0xC2)  →  "PRG"  (flags ignored, code 0x2=PRG)
 *   cbm_dir_type_name(0x81)  →  "SEQ"  (no flags, code 0x1=SEQ)
 */
const char *cbm_dir_type_name(unsigned char type) {
    /* Mask to get just the type code */
    unsigned char type_code = type & CBM_FILE_TYPE_MASK;

    switch (type_code) {
        case 0x00:  return "DEL";
        case 0x01:  return "SEQ";
        case 0x02:  return "PRG";
        case 0x03:  return "USR";
        case 0x04:  return "REL";
        case 0x05:  return "CBM";
        case 0x06:  return "DIR";
        default:    return "???";
    }
}

/**
 * cbm_dir_is_locked - Check if file is locked
 *
 * Parameters:
 *   type — File type byte from directory entry
 *
 * Returns:
 *   1 if locked, 0 if not locked
 */
int cbm_dir_is_locked(unsigned char type) {
    return (type & CBM_FILE_LOCKED) ? 1 : 0;
}

/**
 * cbm_dir_is_scratched - Check if file is scratched/marked for delete
 *
 * Parameters:
 *   type — File type byte from directory entry
 *
 * Returns:
 *   1 if scratched, 0 if not scratched
 */
int cbm_dir_is_scratched(unsigned char type) {
    return (type & CBM_FILE_SPLAT) ? 1 : 0;
}

/**
 * cbm_dir_is_closed - Check if file was properly closed
 *
 * Parameters:
 *   type — File type byte from directory entry
 *
 * Returns:
 *   1 if closed, 0 if open or error
 */
int cbm_dir_is_closed(unsigned char type) {
    return (type & CBM_FILE_CLOSED) ? 1 : 0;
}

/**
 * cbm_dir_get_type - Extract pure file type code
 *
 * Removes all flags and returns only the file type code.
 *
 * Parameters:
 *   type — File type byte from directory entry
 *
 * Returns:
 *   Type code: 0x00=DEL, 0x01=SEQ, 0x02=PRG, 0x03=USR, 0x04=REL, 0x05=CBM, 0x06=DIR
 */
unsigned char cbm_dir_get_type(unsigned char type) {
    return type & CBM_FILE_TYPE_MASK;
}

