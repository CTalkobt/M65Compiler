/* stdio_rel.c — REL (relative) files: fixed-size records by number (#783)
 *
 * The drive does the work, as CBM DOS has it (the MEGA65's DOS, the 1581):
 *   - open: "name,L," + the record length (1-254) creates the file or opens
 *     it (an existing file's length must be the same, else 50/51 errors);
 *     each REL file on a channel of its own (secondary address = its
 *     logical file number, 3-14)
 *   - before a record is read or written: "P" + (96 + secondary address) +
 *     record low + high + byte 1 on the command channel (15); a record past
 *     the end answers 50 RECORD NOT PRESENT: writing it extends the file
 *     (the records between are created empty)
 *   - read: the record's bytes up to its last non-zero one (EOI after it);
 *     write: the bytes, the rest of the record filled with 0 by the drive
 * The record count is found at open: a binary search with P for the last
 * record present. cbm_rel_truncate copies the records kept into a new REL
 * file, scratches the old one and renames the copy (#764).
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>

extern unsigned char cbm_k_chkin(unsigned char fd);
extern unsigned char cbm_k_chkout(unsigned char fd);
extern unsigned char cbm_k_clrchn(void);
extern unsigned char cbm_k_chrin(void);
extern void cbm_k_chrout(unsigned char c);
extern unsigned char cbm_k_readst(void);
extern fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary);
extern int cbm_close(fd_t fd);
__fastcall__ int cbm_drive_status(unsigned char device);
int __cbm_dos_cmd(unsigned char device, const char *cmd, unsigned char *count);

#define REL_NAME_MAX 17            /* a CBM name: 16 characters */

typedef struct {
    signed char fd;                /* logical file (= secondary address), -1: closed */
    unsigned char device;
    unsigned char record_size;     /* 1-254 */
    unsigned int current_record;   /* the last record read or written (1-based) */
    unsigned int total_records;    /* records in the file */
    int last_error;                /* CBM_REL_ERR_* */
    char name[REL_NAME_MAX];
} RelFileHandle;

static RelFileHandle rel_files[8];
static unsigned char rel_init;

static void rel_setup(void) {
    unsigned char i;
    if (rel_init) return;
    for (i = 0; i < 8; i++) rel_files[i].fd = -1;
    rel_init = 1;
}

/* the handle, or 0 when it is not an open REL file */
static RelFileHandle *rel_get(int handle) {
    rel_setup();
    if (handle < 0 || handle >= 8 || rel_files[handle].fd < 0) return 0;
    return &rel_files[handle];
}

/* a drive status number as a CBM_REL_ERR_* */
static int rel_error(int st) {
    switch (st) {
    case 0: case 1: return CBM_REL_ERR_NONE;
    case -1: return CBM_REL_ERR_DEVICE;
    case 62: return CBM_REL_ERR_NOTFOUND;
    case 26: return CBM_REL_ERR_WPROT;
    case 52: case 72: return CBM_REL_ERR_DISKFULL;   /* FILE TOO LARGE, DISK FULL */
    case 60: return CBM_REL_ERR_FILEOPEN;            /* WRITE FILE OPEN */
    case 50: case 51: case 64: return CBM_REL_ERR_CORRUPT;   /* RECORD NOT PRESENT, OVERFLOW IN RECORD, FILE TYPE MISMATCH */
    default: return CBM_REL_ERR_UNKNOWN;
    }
}

/* P on the command channel: the file at record (1-based), its first byte;
 * the drive's status (0, 50 past the end, ...) */
static int rel_position(RelFileHandle *r, unsigned int record) {
    char cmd[6];
    cmd[0] = 0x50;                         /* P (PETSCII; cc45's 'P' is shifted) */
    cmd[1] = (char)(96 + r->fd);
    cmd[2] = (char)(record & 0xFF);
    cmd[3] = (char)(record >> 8);
    cmd[4] = 1;
    cmd[5] = 0;
    /* (the bytes may be 0: sent one by one, not as a string) */
    if (cbm_drive_status(r->device) < 0) return -1;
    if (cbm_k_chkout(15) != 0) { cbm_k_clrchn(); return -1; }
    {
        unsigned char k;
        for (k = 0; k < 5; k++) cbm_k_chrout((unsigned char)cmd[k]);
    }
    cbm_k_chrout(13);
    cbm_k_clrchn();
    return cbm_drive_status(r->device);
}

/* 1 when the record (present) is empty: the DOS's $FF alone, as it fills
 * the records it creates (the rest of a block when the file grows, the
 * records skipped by a write past the end) */
static int rel_empty(RelFileHandle *r, unsigned int record) {
    unsigned char c, st;
    if (rel_position(r, record) != 0) return 0;
    if (cbm_k_chkin((unsigned char)r->fd) != 0) { cbm_k_clrchn(); return 0; }
    c = cbm_k_chrin();
    st = cbm_k_readst();
    cbm_k_clrchn();
    return c == CBM_REL_DELETED && (st & 0x40);
}

/* the number of records: the last one P finds (a binary search), less the
 * empty ones the DOS put after the last written */
static unsigned int rel_count(RelFileHandle *r) {
    unsigned int lo = 0, hi = 65535u, mid;
    if (rel_position(r, 1) != 0) return 0;
    lo = 1;
    while (lo < hi) {
        mid = lo + (unsigned int)((hi - lo + 1u) >> 1);
        if (rel_position(r, mid) == 0) lo = mid;
        else hi = mid - 1;
    }
    while (lo > 0 && rel_empty(r, lo)) lo--;
    return lo;
}

/* OPEN "name,L,<size>" on a channel of its own */
static int rel_open_file(RelFileHandle *r) {
    char spec[REL_NAME_MAX + 6];
    unsigned char n = (unsigned char)strlen(r->name);
    int st;
    /* "0:name,L,<size>" (L as PETSCII, as BASIC 65's DOPEN sends it) */
    spec[0] = '0';
    spec[1] = ':';
    memcpy(spec + 2, r->name, n);
    spec[n + 2] = ',';
    spec[n + 3] = 0x4C;
    spec[n + 4] = ',';
    spec[n + 5] = (char)r->record_size;
    spec[n + 6] = 0;
    r->fd = (signed char)cbm_open_ext(r->device, spec, CBM_T_REL, 0xFF);
    if (r->fd < 0) { r->fd = -1; return CBM_REL_ERR_DEVICE; }
    st = cbm_drive_status(r->device);
    if (st >= 20) {
        cbm_close(r->fd);
        r->fd = -1;
        return rel_error(st);
    }
    r->total_records = rel_count(r);
    r->current_record = 0;
    return CBM_REL_ERR_NONE;
}

/**
 * cbm_rel_open - Open a REL file, creating it if there is none
 *
 * record_size: 1-254 (an existing file's own); mode: CBM_REL_READ or
 * CBM_REL_WRITE (the drive opens a REL file for both).
 * Returns the handle (0-7), -1 on an error.
 */
int cbm_rel_open(unsigned char device, const char *filename, unsigned char record_size, int mode) {
    unsigned char i;
    RelFileHandle *r = 0;
    (void)mode;
    rel_setup();
    if (device == 0 || !filename || !*filename || strlen(filename) >= REL_NAME_MAX || record_size == 0 ||
        record_size > 254)
        return -1;
    for (i = 0; i < 8; i++)
        if (rel_files[i].fd < 0) { r = &rel_files[i]; break; }
    if (!r) return -1;
    r->device = device;
    r->record_size = record_size;
    r->last_error = CBM_REL_ERR_NONE;
    strcpy(r->name, filename);
    if (rel_open_file(r) != CBM_REL_ERR_NONE) return -1;
    return (int)i;
}

/**
 * cbm_rel_read - Read a record (1-based) into buffer (at most size bytes)
 *
 * Returns the bytes read (the record up to its last non-zero byte, the rest
 * of buffer up to size cleared), 0 for an empty record and past the last
 * one, -1 on an error. (A record of the single byte $FF reads as empty: it
 * is how the DOS marks one.)
 */
int cbm_rel_read(int handle, unsigned int record_num, void *buffer, unsigned int size) {
    RelFileHandle *r = rel_get(handle);
    unsigned char *p = (unsigned char *)buffer;
    unsigned int n = 0;
    int st;
    if (!r || !buffer || record_num == 0) return -1;
    if (size > r->record_size) size = r->record_size;
    memset(buffer, 0, size);
    if (record_num > r->total_records) return 0;
    st = rel_position(r, record_num);
    if (st != 0) { r->last_error = rel_error(st); return st == 50 ? 0 : -1; }
    if (cbm_k_chkin((unsigned char)r->fd) != 0) { cbm_k_clrchn(); r->last_error = CBM_REL_ERR_DEVICE; return -1; }
    while (n < size) {
        p[n++] = cbm_k_chrin();
        if (cbm_k_readst() & 0x40) break;   /* EOI: the record's last byte */
    }
    cbm_k_clrchn();
    if (n == 1 && p[0] == CBM_REL_DELETED) {   /* an empty (or deleted) record */
        p[0] = 0;
        n = 0;
    }
    r->current_record = record_num;
    r->last_error = CBM_REL_ERR_NONE;
    return (int)n;
}

/**
 * cbm_rel_write - Write a record (1-based): size bytes (at most the record
 * size; the rest of the record 0). Past the end the file grows to it.
 *
 * Returns the bytes written, -1 on an error.
 */
int cbm_rel_write(int handle, unsigned int record_num, const void *buffer, unsigned int size) {
    RelFileHandle *r = rel_get(handle);
    const unsigned char *p = (const unsigned char *)buffer;
    unsigned int n;
    int st;
    if (!r || !buffer || record_num == 0) return -1;
    if (size > r->record_size) size = r->record_size;
    st = rel_position(r, record_num);
    if (st != 0 && st != 50) { r->last_error = rel_error(st); return -1; }
    if (cbm_k_chkout((unsigned char)r->fd) != 0) { cbm_k_clrchn(); r->last_error = CBM_REL_ERR_DEVICE; return -1; }
    for (n = 0; n < size; n++) cbm_k_chrout(p[n]);
    cbm_k_clrchn();
    st = cbm_drive_status(r->device);
    if (st >= 20 && st != 50) { r->last_error = rel_error(st); return -1; }
    if (record_num > r->total_records) r->total_records = record_num;
    r->current_record = record_num;
    r->last_error = CBM_REL_ERR_NONE;
    return (int)size;
}

/**
 * cbm_rel_position - Position the file at a record (1-based): the next
 * cbm_rel_tell. 0, or -1 (a record past the end is allowed: written, the
 * file grows to it).
 */
int cbm_rel_position(int handle, unsigned int record_num) {
    RelFileHandle *r = rel_get(handle);
    int st;
    if (!r || record_num == 0) return -1;
    st = rel_position(r, record_num);
    if (st != 0 && st != 50) { r->last_error = rel_error(st); return -1; }
    r->current_record = record_num;
    return 0;
}

/* the record last read, written or positioned at (0: none yet) */
unsigned int cbm_rel_tell(int handle) {
    RelFileHandle *r = rel_get(handle);
    return r ? r->current_record : 0;
}

/* the number of records in the file: up to the last one not empty (the
 * empty records after it, which the DOS creates a block at a time, do not
 * count) */
unsigned int cbm_rel_size(int handle) {
    RelFileHandle *r = rel_get(handle);
    return r ? r->total_records : 0;
}

/* 0, or -1 when the handle is not open */
int cbm_rel_close(int handle) {
    RelFileHandle *r = rel_get(handle);
    if (!r) return -1;
    cbm_close(r->fd);
    r->fd = -1;
    return 0;
}

unsigned char cbm_rel_record_size(int handle) {
    RelFileHandle *r = rel_get(handle);
    return r ? r->record_size : 0;
}

int cbm_rel_is_open(int handle) {
    return rel_get(handle) != 0;
}

/* a record after the last: its number, -1 on an error */
int cbm_rel_append(int handle, const void *buffer, unsigned int size) {
    RelFileHandle *r = rel_get(handle);
    unsigned int rec;
    if (!r || !buffer) return -1;
    rec = r->total_records + 1;
    if (rec == 0) return -1;
    return cbm_rel_write(handle, rec, buffer, size) < 0 ? -1 : (int)rec;
}

/* empties a record: CBM_REL_DELETED ($FF) alone, the DOS's empty record
 * (the record stays: a REL file cannot lose records in the middle; it
 * reads back as 0 bytes; the last records deleted no longer count) */
int cbm_rel_delete(int handle, unsigned int record_num) {
    RelFileHandle *r = rel_get(handle);
    unsigned char marker = CBM_REL_DELETED;
    if (!r || record_num == 0 || record_num > r->total_records) return -1;
    if (cbm_rel_write(handle, record_num, &marker, 1) < 0) return -1;
    if (record_num == r->total_records)
        while (r->total_records > 0 && rel_empty(r, r->total_records)) r->total_records--;
    return 0;
}

/* an existing record rewritten: the bytes written, -1 when it does not exist */
int cbm_rel_update(int handle, unsigned int record_num, const void *buffer, unsigned int size) {
    RelFileHandle *r = rel_get(handle);
    if (!r || !buffer || record_num == 0 || record_num > r->total_records) return -1;
    return cbm_rel_write(handle, record_num, buffer, size);
}

/**
 * cbm_rel_truncate - Keep the first num_records records (#764)
 *
 * A REL file cannot be shortened in place: the records kept are copied into
 * a new REL file ("RELTRUNC.TMP"), the old file is scratched and the copy
 * renamed to its name, then reopened on the same handle. Longer than the
 * file: it grows to num_records (the records added empty). Needs room on
 * the disk for the copy.
 *
 * Returns 0, or -1 on an error (the handle then closed if the file could
 * not be reopened; the copy may be left on the disk).
 */
int cbm_rel_truncate(int handle, unsigned int num_records) {
    static char tmpname[] = "RELTRUNC.TMP";
    static char cmd[REL_NAME_MAX + 4];
    unsigned char buf[254];
    RelFileHandle *r = rel_get(handle);
    RelFileHandle t;
    unsigned int k;
    int n, err;
    if (!r || num_records == 0) return -1;
    if (num_records == r->total_records) return 0;
    if (num_records > r->total_records) {   /* longer: the last record written (a 0 byte) */
        buf[0] = 0;
        return cbm_rel_write(handle, num_records, buf, 1) < 0 ? -1 : 0;
    }
    /* the copy */
    t.device = r->device;
    t.record_size = r->record_size;
    strcpy(t.name, tmpname);
    cmd[0] = 0x53; cmd[1] = '0'; cmd[2] = ':'; strcpy(cmd + 3, tmpname);   /* S0: */
    __cbm_dos_cmd(t.device, cmd, 0);          /* (one left by an error before) */
    err = rel_open_file(&t);
    if (err != CBM_REL_ERR_NONE) { r->last_error = err; return -1; }
    for (k = 1; k <= num_records; k++) {
        n = cbm_rel_read(handle, k, buf, r->record_size);
        if (n < 0) break;
        /* (an empty record copied as the DOS's empty record) */
        {
            int st = rel_position(&t, k);
            if ((st != 0 && st != 50) || cbm_k_chkout((unsigned char)t.fd) != 0) { n = -1; cbm_k_clrchn(); break; }
        }
        if (n == 0) cbm_k_chrout(CBM_REL_DELETED);
        else {
            int b;
            for (b = 0; b < n; b++) cbm_k_chrout(buf[b]);
        }
        cbm_k_clrchn();
    }
    cbm_close(t.fd);
    if (n < 0 || cbm_drive_status(t.device) >= 20) { r->last_error = CBM_REL_ERR_UNKNOWN; return -1; }
    /* the old file replaced by the copy */
    cbm_close(r->fd);
    r->fd = -1;
    strcpy(cmd + 3, r->name);
    if (__cbm_dos_cmd(r->device, cmd, 0) != 1) { r->last_error = CBM_REL_ERR_UNKNOWN; return -1; }
    {
        static char ren[REL_NAME_MAX + REL_NAME_MAX + 7];
        ren[0] = 0x52; ren[1] = '0'; ren[2] = ':';   /* R0:name=0:tmp */
        strcpy(ren + 3, r->name);
        strcat(ren, "=0:");
        strcat(ren, tmpname);
        if (__cbm_dos_cmd(r->device, ren, 0) != 0) { r->last_error = CBM_REL_ERR_UNKNOWN; return -1; }
    }
    err = rel_open_file(r);
    r->last_error = err;
    return err == CBM_REL_ERR_NONE ? 0 : -1;
}

/* the last error of a handle (CBM_REL_ERR_INVALID: not open) */
int cbm_rel_lasterror(int handle) {
    RelFileHandle *r = rel_get(handle);
    return r ? r->last_error : CBM_REL_ERR_INVALID;
}

const char *cbm_rel_strerror(int error) {
    switch (error) {
    case CBM_REL_ERR_NONE:      return "No error";
    case CBM_REL_ERR_DEVICE:    return "Device not present";
    case CBM_REL_ERR_NOTFOUND:  return "File not found";
    case CBM_REL_ERR_FILEOPEN:  return "File already open";
    case CBM_REL_ERR_DISKFULL:  return "Disk full";
    case CBM_REL_ERR_WPROT:     return "Write protected";
    case CBM_REL_ERR_CORRUPT:   return "Data corruption detected";
    case CBM_REL_ERR_TIMEOUT:   return "Operation timeout";
    case CBM_REL_ERR_INVALID:   return "Invalid parameter";
    case CBM_REL_ERR_UNKNOWN:   return "Unknown error";
    default:                    return "Unrecognized error code";
    }
}
