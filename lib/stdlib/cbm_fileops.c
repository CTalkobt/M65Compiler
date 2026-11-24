/* cbm_fileops.c — cbm_delete, cbm_rename, cbm_format: DOS commands on the
 * drive's command channel (#762); remove() and rename() are made of the
 * first two (stdio_remove.c)
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>

extern fd_t cbm_open_ext(unsigned char device, const char *filename, unsigned char type, unsigned char secondary);
extern int cbm_close(fd_t fd);
__fastcall__ int cbm_drive_status(unsigned char device);
extern char __cbm_dos_msg[];

/* lib/stdlib/cbm_dos_cmd.c: a DOS command on the command channel; the
 * drive's status number, -1 when the channel can't be opened */
int __cbm_dos_cmd(unsigned char device, const char *cmd, unsigned char *count);

/* The command letters as PETSCII codes, as BASIC 65 sends them (cc45's 'S'
 * is the shifted letter, which the DOS does not take for the command), and
 * drive 0 named in every part (a rename without them: 79 SINGLE DRIVE D0
 * ONLY) */
#define DOS_S 0x53
#define DOS_R 0x52
#define DOS_N 0x4E
#define DOS_R_READ 0x52            /* R: OPEN for reading */

/* append s to cmd at *n (the command channel's 40 characters at most); 0,
 * or -1 when it does not fit */
static int put(char *cmd, unsigned char *n, const char *s) {
    while (*s) {
        if (*n >= 40) return -1;
        cmd[(*n)++] = *s++;
    }
    cmd[*n] = 0;
    return 0;
}

/* 1 when the drive has the file, 0 when not, -1 on an error: OPEN of
 * "0:name,S,R" (a file of another type answers 64 FILE TYPE MISMATCH) */
static int file_exists(unsigned char device, const char *name) {
    char spec[48];
    unsigned char n;
    fd_t fd;
    int st;
    spec[0] = '0';
    spec[1] = ':';
    spec[2] = 0;
    n = 2;
    if (put(spec, &n, name) || put(spec, &n, ",")) return -1;
    spec[n++] = DOS_S;
    spec[n++] = ',';
    spec[n++] = DOS_R_READ;
    spec[n] = 0;
    fd = cbm_open_ext(device, spec, 0, 0xFF);
    if (fd < 0) return -1;
    st = cbm_drive_status(device);
    cbm_close(fd);
    return st == 0 || st == 64 ? 1 : st == 62 ? 0 : -1;
}

/* "<c>0:" */
static void start(char *cmd, unsigned char *n, char c) {
    cmd[0] = c;
    cmd[1] = '0';
    cmd[2] = ':';
    cmd[3] = 0;
    *n = 3;
}

/**
 * cbm_delete - Delete (scratch) a file: "S0:name" on the command channel
 * (wildcards as the DOS has them: "TEMP*")
 *
 * Returns:
 *   0 when the drive scratched at least one file, -1 otherwise (no such
 *   file: "01, FILES SCRATCHED,00,00"; a drive error; no drive)
 */
int cbm_delete(unsigned char device, const char *filename) {
    char cmd[41];
    unsigned char count;
    unsigned char n;
    if (device == 0 || !filename || !*filename) return -1;
    start(cmd, &n, DOS_S);
    if (put(cmd, &n, filename)) return -1;
    return __cbm_dos_cmd(device, cmd, &count) == 1 && count > 0 ? 0 : -1;
}

/**
 * cbm_rename - Rename a file: "R0:new=0:old" on the command channel
 *
 * Returns:
 *   0 on success, -1 on an error (62 FILE NOT FOUND; 63 FILE EXISTS, which
 *   is checked here: the MEGA65's DOS would make a second file of the name;
 *   the drive's message is in __cbm_dos_msg)
 */
int cbm_rename(unsigned char device, const char *old_name, const char *new_name) {
    char cmd[41];
    unsigned char n;
    if (device == 0 || !old_name || !new_name || !*old_name || !*new_name) return -1;
    start(cmd, &n, DOS_R);
    if (put(cmd, &n, new_name) || put(cmd, &n, "=0:") || put(cmd, &n, old_name)) return -1;
    /* (the MEGA65's DOS renames to a name that exists, leaving two files of
     * that name: refused here, as CBM DOS does, with its 63) */
    switch (file_exists(device, new_name)) {
    case 1: strcpy(__cbm_dos_msg, "63,FILE EXISTS,00,00"); return -1;
    case 0: break;
    default: return -1;
    }
    return __cbm_dos_cmd(device, cmd, 0) == 0 ? 0 : -1;
}

/**
 * cbm_format - Format a disk: "N0:name,id" on the command channel; an empty
 * id clears the directory of a disk formatted before (the DOS's quick
 * "N0:name"). Every file on the disk is lost.
 *
 * Returns:
 *   0 on success, -1 on an error
 */
int cbm_format(unsigned char device, const char *name, const char *id) {
    char cmd[41];
    unsigned char n;
    if (device == 0 || !name || !id || !*name) return -1;
    start(cmd, &n, DOS_N);
    if (put(cmd, &n, name) || (*id && (put(cmd, &n, ",") || put(cmd, &n, id)))) return -1;
    return __cbm_dos_cmd(device, cmd, 0) == 0 ? 0 : -1;
}

