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

/**
 * cbm_opendir - Open directory listing
 *
 * Opens the special "$" file which contains the directory listing.
 * On Commodore, directory is read just like a file.
 *
 * Parameters:
 *   device — Device number (8=disk, etc.)
 *
 * Returns:
 *   Directory handle (pseudo file descriptor) on success
 *   -1 on error
 */
int cbm_opendir(unsigned char device) {
    if (device == 0) {
        return -1;  /* Invalid device */
    }

    /* Open "$" (directory) as a SEQ file for reading */
    fd_t fd = cbm_open(device, "$", CBM_T_SEQ);

    if (fd < 0) {
        return -1;
    }

    /* Return the fd as a directory handle
     * (caller treats it as a directory, we treat it as a file) */
    return (int)fd;
}

/**
 * cbm_readdir - Read next directory entry
 *
 * Reads the next entry from the directory listing.
 * Parses the Commodore directory format and fills in the provided structures.
 * Properly extracts file type and flags from the directory entry.
 *
 * Parameters:
 *   fd       — Directory handle from cbm_opendir()
 *   filename — Buffer for filename (16+ bytes recommended)
 *   type     — Pointer to file type byte (includes flags: CBM_FILE_LOCKED, CBM_FILE_SPLAT, CBM_FILE_CLOSED)
 *   size     — Pointer to file size in blocks
 *
 * Returns:
 *   0 on success
 *   1 at end of directory
 *   -1 on error
 *
 * File Type Byte Format (entry[18]):
 *   Bit 7: Closed flag (1=properly closed, 0=open/error)
 *   Bit 6: Locked flag (1=locked/protected, 0=not locked)
 *   Bit 5: Splat flag (1=marked for delete, 0=normal)
 *   Bits 3-0: File type (0x1=SEQ, 0x2=PRG, 0x3=USR, 0x4=REL)
 *
 * Example:
 *   type = 0xC2  →  Closed (0x80) | Locked (0x40) | PRG type (0x2)
 *   type = 0x81  →  Normal SEQ file
 */
int cbm_readdir(int fd_handle, char *filename, unsigned char *type, unsigned int *size) {
    if (fd_handle < 0 || !filename || !type || !size) {
        return -1;
    }

    /* Read 32-byte directory entry */
    unsigned char entry[32];
    int nread = cbm_read((fd_t)fd_handle, entry, 32);

    if (nread < 32) {
        /* EOF or error */
        return 1;  /* End of directory */
    }

    /* Parse entry */
    *size = entry[0] | (entry[1] << 8);  /* Little-endian size in blocks */

    /* Copy filename, removing padding ($A0 padding) */
    int i, j;
    i = 0;
    j = 0;
    while (i < 16 && entry[i + 2] != 0xA0) {
        filename[j] = entry[i + 2];
        i++;
        j++;
    }
    filename[j] = '\0';  /* Null-terminate */

    /* Extract file type with all flags
     * Bit 7: Closed flag (1=closed, 0=open)
     * Bit 6: Locked flag (1=locked, 0=unlocked)
     * Bit 5: Splat flag (1=scratched, 0=normal)
     * Bits 3-0: File type code (1=SEQ, 2=PRG, 3=USR, 4=REL, 8=DIR)
     */
    *type = entry[18];

    return 0;  /* Success, more entries possible */
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
        case 0x02:  return "PRG";
        case 0x01:  return "SEQ";
        case 0x03:  return "USR";
        case 0x04:  return "REL";
        case 0x08:  return "DIR";
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
 *   Type code: 0x01=SEQ, 0x02=PRG, 0x03=USR, 0x04=REL, 0x08=DIR
 */
unsigned char cbm_dir_get_type(unsigned char type) {
    return type & CBM_FILE_TYPE_MASK;
}

