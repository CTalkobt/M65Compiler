/* stdio_rel.c — Relative (record-based) file operations
 *
 * Implements REL (Relative) file support for random-access record files.
 * REL files allow reading/writing fixed-size records at arbitrary positions.
 *
 * Phase 4 of FILE I/O implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations from stdio_cbm.c */
extern fd_t cbm_open(unsigned char device, const char *filename, unsigned char type);
extern int cbm_close(fd_t fd);
extern int cbm_read(fd_t fd, void *buffer, unsigned int length);
extern int cbm_write(fd_t fd, const void *buffer, unsigned int length);

/* ============================================================================
 * REL FILE HANDLE STRUCTURE
 * ============================================================================
 *
 * REL files on Commodore are accessed sequentially but can position to
 * arbitrary records using special control sequences.
 *
 * Record Format:
 *   - Each record has a fixed size (1-254 bytes)
 *   - Records are numbered starting from 1
 *   - Position marker: 0x00 (EOF), 0x01-0xFE (record data), 0xFF (error)
 */

typedef struct {
    fd_t fd;                       /* File descriptor */
    unsigned char record_size;     /* Bytes per record (1-254) */
    unsigned int current_record;   /* Current record number (1-based) */
    unsigned int total_records;    /* Total records in file */
    unsigned char buffer[256];     /* Record buffer (max 254 bytes + 2 overhead) */
    int dirty;                     /* Buffer modified, needs flush */
} RelFileHandle;

/* Global REL file handles (up to 8 like regular FILES) */
static RelFileHandle rel_files[8];
static int rel_handles_init = 0;

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/**
 * Initialize REL file handle table
 */
static void _rel_init_handles(void) {
    if (!rel_handles_init) {
        int i;
        for (i = 0; i < 8; i++) {
            rel_files[i].fd = -1;
            rel_files[i].current_record = 0;
        }
        rel_handles_init = 1;
    }
}

/**
 * Find free REL file handle slot
 */
static int _rel_find_slot(void) {
    int i;
    _rel_init_handles();
    for (i = 0; i < 8; i++) {
        if (rel_files[i].fd < 0) {
            return i;
        }
    }
    return -1;
}

/* ============================================================================
 * REL FILE OPERATIONS
 * ============================================================================ */

/**
 * cbm_rel_open - Open a REL (relative) file
 *
 * Opens a REL file for record-based access.
 * REL files support random access to fixed-size records.
 *
 * Parameters:
 *   device       — Device number (8=disk)
 *   filename     — REL file name
 *   record_size  — Size of each record (1-254 bytes)
 *   mode         — CBM_REL_READ or CBM_REL_WRITE
 *
 * Returns:
 *   REL file handle (0-7) on success, -1 on error
 *
 * Implementation Notes:
 *   - Opens file with secondary address that indicates record size
 *   - Commodore OPEN uses secondary address for record info
 *   - SA = record_size for REL files
 */
int cbm_rel_open(unsigned char device, const char *filename,
                 unsigned char record_size, int mode) {
    if (device == 0 || !filename || record_size == 0 || record_size > 254) {
        return -1;
    }

    int slot = _rel_find_slot();
    if (slot < 0) {
        return -1;  /* No free handles */
    }

    /* Open REL file
     * Commodore 1541/1581 expects secondary address = record size for REL files
     * For now, use standard open - actual record size negotiation happens via KERNAL
     */
    fd_t fd = cbm_open(device, filename, CBM_T_REL);

    if (fd < 0) {
        return -1;
    }

    /* Initialize REL handle */
    rel_files[slot].fd = fd;
    rel_files[slot].record_size = record_size;
    rel_files[slot].current_record = 0;
    rel_files[slot].total_records = 0;
    rel_files[slot].dirty = 0;

    return slot;
}

/**
 * cbm_rel_read - Read a record from REL file
 *
 * Reads a specific record from the REL file into a buffer.
 * Automatically positions to the correct record.
 *
 * Parameters:
 *   handle      — REL file handle from cbm_rel_open()
 *   record_num  — Record number to read (1-based)
 *   buffer      — Buffer to read into
 *   size        — Number of bytes to read (≤ record_size)
 *
 * Returns:
 *   Number of bytes read, 0 on EOF, -1 on error
 *
 * Implementation Notes:
 *   - Commodore REL files use position markers
 *   - To read record N: write position marker (0x00 byte, then record#)
 *   - Then read the record data
 *   - This is handled via KERNAL POSITION command (custom)
 */
int cbm_rel_read(int handle, unsigned int record_num, void *buffer, unsigned int size) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0 || !buffer) {
        return -1;
    }

    if (size > rel_files[handle].record_size) {
        size = rel_files[handle].record_size;
    }

    /* TODO: Implement POSITION command via KERNAL
     * Commodore POSITION format: WRITE position marker bytes to file
     * This would require special KERNAL calls to position in REL file
     *
     * For now, return error as this requires KERNAL integration
     */

    return -1;  /* Not yet implemented - requires KERNAL POSITION */
}

/**
 * cbm_rel_write - Write a record to REL file
 *
 * Writes a record to the REL file at a specific position.
 * Automatically positions to the correct record.
 *
 * Parameters:
 *   handle      — REL file handle from cbm_rel_open()
 *   record_num  — Record number to write (1-based)
 *   buffer      — Data to write
 *   size        — Number of bytes to write (≤ record_size)
 *
 * Returns:
 *   Number of bytes written, -1 on error
 */
int cbm_rel_write(int handle, unsigned int record_num, const void *buffer, unsigned int size) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0 || !buffer) {
        return -1;
    }

    if (size > rel_files[handle].record_size) {
        size = rel_files[handle].record_size;
    }

    /* TODO: Implement POSITION command via KERNAL
     * Similar to read, requires KERNAL support for positioning
     */

    return -1;  /* Not yet implemented - requires KERNAL POSITION */
}

/**
 * cbm_rel_position - Position to a specific record
 *
 * Positions the REL file pointer to a specific record.
 * This is the fundamental operation for random access.
 *
 * Parameters:
 *   handle      — REL file handle
 *   record_num  — Record number (1-based)
 *
 * Returns:
 *   0 on success, -1 on error
 *
 * Implementation Notes:
 *   - Uses KERNAL POSITION command
 *   - Format: 0x00, record_lo, record_hi
 *   - Positions file pointer to start of record
 */
int cbm_rel_position(int handle, unsigned int record_num) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0) {
        return -1;
    }

    if (record_num == 0) {
        return -1;  /* Record numbers are 1-based */
    }

    /* TODO: Implement POSITION via KERNAL
     * Would write position marker bytes to enable random access
     */

    rel_files[handle].current_record = record_num;
    return 0;  /* Placeholder */
}

/**
 * cbm_rel_tell - Get current record position
 *
 * Returns the current record number in the file.
 *
 * Parameters:
 *   handle — REL file handle
 *
 * Returns:
 *   Current record number (1-based), 0 if not positioned
 */
unsigned int cbm_rel_tell(int handle) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0) {
        return 0;
    }

    return rel_files[handle].current_record;
}

/**
 * cbm_rel_size - Get file size in records
 *
 * Returns the number of records in the file.
 *
 * Parameters:
 *   handle — REL file handle
 *
 * Returns:
 *   Number of records, 0 if unknown
 */
unsigned int cbm_rel_size(int handle) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0) {
        return 0;
    }

    return rel_files[handle].total_records;
}

/**
 * cbm_rel_close - Close a REL file
 *
 * Closes the REL file and frees the handle.
 *
 * Parameters:
 *   handle — REL file handle from cbm_rel_open()
 *
 * Returns:
 *   0 on success, -1 on error
 */
int cbm_rel_close(int handle) {
    if (handle < 0 || handle >= 8) {
        return -1;
    }

    if (rel_files[handle].fd < 0) {
        return -1;  /* Already closed */
    }

    /* Flush any pending writes */
    if (rel_files[handle].dirty) {
        /* TODO: Flush buffer if needed */
        rel_files[handle].dirty = 0;
    }

    /* Close the underlying file */
    int result = cbm_close(rel_files[handle].fd);

    /* Clear handle */
    rel_files[handle].fd = -1;
    rel_files[handle].current_record = 0;

    return result;
}

/* ============================================================================
 * REL FILE UTILITY FUNCTIONS
 * ============================================================================ */

/**
 * cbm_rel_record_size - Get record size for opened file
 *
 * Returns the record size that was specified when opening the file.
 *
 * Parameters:
 *   handle — REL file handle
 *
 * Returns:
 *   Record size in bytes (1-254), 0 on error
 */
unsigned char cbm_rel_record_size(int handle) {
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0) {
        return 0;
    }

    return rel_files[handle].record_size;
}

/**
 * cbm_rel_is_open - Check if REL file handle is open
 *
 * Parameters:
 *   handle — REL file handle
 *
 * Returns:
 *   1 if open, 0 if closed
 */
int cbm_rel_is_open(int handle) {
    if (handle < 0 || handle >= 8) {
        return 0;
    }

    return rel_files[handle].fd >= 0 ? 1 : 0;
}

/* ============================================================================
 * FUTURE PHASES
 * ============================================================================
 *
 * Phase 4b: KERNAL POSITION Command Integration
 *   - Implement actual POSITION command via KERNAL
 *   - Wire up cbm_rel_read() and cbm_rel_write()
 *   - Support record sizing negotiation
 *
 * Phase 4c: Record Caching
 *   - Cache current record in buffer
 *   - Detect sequential vs. random access patterns
 *   - Optimize for typical use cases
 *
 * Phase 4d: Extended Record Operations
 *   - Record append operations
 *   - Record deletion (sparse files)
 *   - Record update with overflow handling
 */

