/* stdio_remove.c — C's remove() and rename() (#763): cbm_delete and
 * cbm_rename on the disk drive fopen uses (CBM_DEVICE_DISK, unit 8)
 *
 * 0 on success; else -1 and errno: ENOENT (no such file: 62 FILE NOT FOUND,
 * or nothing scratched), EEXIST (rename to a name that exists: 63), EACCES
 * (26 WRITE PROTECT ON), EINVAL (no name, or one too long for the command),
 * EIO (any other drive error, or no drive)
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>
#include <errno.h>

int __cbm_dos_cmd(unsigned char device, const char *cmd, unsigned char *count);
extern char __cbm_dos_msg[];

static int dos_errno(void) {
    int st;
    if (__cbm_dos_msg[0] < '0' || __cbm_dos_msg[0] > '9') return EIO;
    st = (__cbm_dos_msg[0] - '0') * 10 + (__cbm_dos_msg[1] - '0');
    switch (st) {
    case 1: case 62: return ENOENT;     /* (01: nothing scratched) */
    case 63: return EEXIST;
    case 26: return EACCES;
    default: return EIO;
    }
}

static unsigned char name_ok(const char *s) {
    unsigned char n = 0;
    if (!s || !*s) return 0;
    while (s[n]) if (++n > 38) return 0;   /* (the command and the name: 40) */
    return 1;
}

int remove(const char *filename) {
    if (!name_ok(filename)) { errno = EINVAL; return -1; }
    __cbm_dos_msg[0] = 0;
    if (cbm_delete(CBM_DEVICE_DISK, filename) == 0) return 0;
    errno = dos_errno();
    return -1;
}

int rename(const char *oldname, const char *newname) {
    if (!name_ok(oldname) || !name_ok(newname)) { errno = EINVAL; return -1; }
    __cbm_dos_msg[0] = 0;
    if (cbm_rename(CBM_DEVICE_DISK, oldname, newname) == 0) return 0;
    errno = dos_errno();
    return -1;
}
