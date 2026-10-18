/* test_cbmio_xemu.c — CBM file I/O, the KERNAL wrappers and the directory
 *
 * Run by test_xemu.sh with an empty D81 attached as drive 8 (xemu -8).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] cbm_open_ext / cbm_write / cbm_tell / cbm_close (300 bytes)
 *   [1] cbm_read / cbm_tell / cbm_eof / cbm_status, cbm_seek forward and
 *       back (reopening), past the end
 *   [2] cbm_drive_status: 0 after success, 62 for a missing file
 *   [3] cbm_k_setnam / setlfs / open / chkout / chrout / chkin / chrin /
 *       readst / clrchn / close directly
 *   [4] cbm_opendir / cbm_readdir / cbm_closedir: the files written, their
 *       types and sizes; cbm_dir_get_type / is_closed / is_locked /
 *       is_scratched / type_name
 *   [5] cbm_rel_strerror / cbm_rel_lasterror (an invalid handle)
 *   [6] $AA sentinel
 */
/* Standard C signedness: the cbm_* results are compared with < 0 */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

/* cc45 writes 'A' as $C1; the drive's directory listing uses the same bytes
 * for a name written from C */
static unsigned char data[300];
static unsigned char back[300];

static int t_write(void) {
    unsigned int i;
    int fd, n;
    for (i = 0; i < 300; i++) data[i] = (unsigned char)(i * 7 + 3);
    fd = cbm_open_ext(8, "cbmio,s,w", CBM_T_SEQ, 0xFF);
    if (fd < 0) return 0;
    n = cbm_write(fd, data, 300);
    if (n != 300 || cbm_tell(fd) != 300UL) { cbm_close(fd); return 0; }
    return cbm_close(fd) == 0;
}

static int t_read(void) {
    int fd, ok = 1;
    unsigned char b[10];
    fd = cbm_open_ext(8, "cbmio,s,r", CBM_T_SEQ, 0xFF);
    if (fd < 0) return 0;
    ok = cbm_read(fd, back, 100) == 100 && memcmp(back, data, 100) == 0 && cbm_tell(fd) == 100UL &&
         !cbm_eof(fd);
    /* forward: skip to 250 */
    ok = ok && cbm_seek(fd, 250) == 0 && cbm_tell(fd) == 250UL && cbm_read(fd, b, 10) == 10 &&
         memcmp(b, data + 250, 10) == 0;
    /* back: reopen and read to 5 */
    ok = ok && cbm_seek(fd, 5) == 0 && cbm_tell(fd) == 5UL && cbm_read(fd, b, 10) == 10 &&
         memcmp(b, data + 5, 10) == 0;
    /* the rest, then the end */
    ok = ok && cbm_read(fd, back, 300) == 285 && memcmp(back, data + 15, 285) == 0 && cbm_eof(fd) &&
         (cbm_status(fd) & 0x40) && cbm_read(fd, b, 1) == 0 && cbm_tell(fd) == 300UL;
    /* past the end: fails, at the end */
    ok = ok && cbm_seek(fd, 400) == -1 && cbm_tell(fd) == 300UL;
    cbm_close(fd);
    return ok;
}

static int t_status(void) {
    int ok = cbm_drive_status(8) == 0;
    int fd = cbm_open_ext(8, "nope,s,r", CBM_T_SEQ, 0xFF);
    ok = ok && cbm_drive_status(8) == 62;
    if (fd >= 0) cbm_close(fd);
    return ok && cbm_drive_status(8) < 20;
}

static int t_kernal(void) {
    unsigned char c1, c2, c3, st;
    int ok;
    cbm_k_setnam("kdirect,s,w");
    cbm_k_setlfs(5, 8, 5);
    if (cbm_k_open() != 0) return 0;
    ok = cbm_k_chkout(5) == 0;
    cbm_k_chrout('x');
    cbm_k_chrout('y');
    cbm_k_chrout('z');
    cbm_k_clrchn();
    cbm_k_close(5);
    cbm_k_setnam("kdirect,s,r");
    cbm_k_setlfs(5, 8, 5);
    if (cbm_k_open() != 0) return 0;
    ok = ok && cbm_k_chkin(5) == 0;
    c1 = cbm_k_chrin();
    c2 = cbm_k_chrin();
    c3 = cbm_k_chrin();
    st = cbm_k_readst();
    cbm_k_clrchn();
    cbm_k_close(5);
    return ok && c1 == 'x' && c2 == 'y' && c3 == 'z' && (st & 0x40);
}

static int t_dir(void) {
    char name[17];
    unsigned char type;
    unsigned int size;
    int found_cbmio = 0, found_k = 0, entries = 0, r;
    int d = cbm_opendir(8);
    if (d < 0) return 0;
    while ((r = cbm_readdir(d, name, &type, &size)) == 0) {
        entries++;
        if (strcmp(name, "cbmio") == 0)
            found_cbmio = cbm_dir_get_type(type) == (CBM_T_SEQ & 0x0F) && cbm_dir_is_closed(type) &&
                          !cbm_dir_is_locked(type) && !cbm_dir_is_scratched(type) && size == 2 &&
                          strcmp(cbm_dir_type_name(type), "SEQ") == 0;
        if (strcmp(name, "kdirect") == 0) found_k = type == CBM_T_SEQ && size == 1;
    }
    cbm_closedir(d);
    return r == 1 && entries >= 2 && found_cbmio && found_k &&
           cbm_dir_get_type(0xC2) == 2 && cbm_dir_is_locked(0xC2) && !cbm_dir_is_closed(0x02) &&
           strcmp(cbm_dir_type_name(CBM_T_PRG), "PRG") == 0 && strcmp(cbm_dir_type_name(CBM_T_REL), "REL") == 0;
}

static int t_rel(void) {
    return strcmp(cbm_rel_strerror(CBM_REL_ERR_NOTFOUND), "File not found") == 0 &&
           strcmp(cbm_rel_strerror(CBM_REL_ERR_NONE), "No error") == 0 &&
           cbm_rel_lasterror(-1) == CBM_REL_ERR_INVALID && cbm_rel_lasterror(7) == CBM_REL_ERR_INVALID;
}

int main(void) {
    OUT[0] = t_write();
    OUT[1] = t_read();
    OUT[2] = t_status();
    OUT[3] = t_kernal();
    OUT[4] = t_dir();
    OUT[5] = t_rel();
    OUT[6] = 0xAA;
    return 0;
}
