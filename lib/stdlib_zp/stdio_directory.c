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
 *
 * Parameters:
 *   fd       — Directory handle from cbm_opendir()
 *   filename — Buffer for filename (16+ bytes recommended)
 *   type     — Pointer to file type byte
 *   size     — Pointer to file size in blocks
 *
 * Returns:
 *   0 on success
 *   1 at end of directory
 *   -1 on error
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
    *size = entry[0] | (entry[1] << 8);  /* Little-endian size */

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

    *type = entry[18];  /* File type byte */

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
 * Converts a Commodore file type byte to a string.
 *
 * Parameters:
 *   type — File type byte (0x81, 0x82, 0x84, etc.)
 *
 * Returns:
 *   Pointer to string: "PRG", "SEQ", "USR", "REL", or "???"
 */
const char *cbm_dir_type_name(unsigned char type) {
    switch (type) {
        case CBM_T_PRG:  return "PRG";
        case CBM_T_SEQ:  return "SEQ";
        case CBM_T_USR:  return "USR";
        case CBM_T_REL:  return "REL";
        case CBM_T_CBM:  return "CBM";
        default:         return "???";
    }
}

