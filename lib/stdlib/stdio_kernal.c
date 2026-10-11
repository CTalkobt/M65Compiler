/* stdio_kernal.c — KERNAL file I/O wrappers for cc45 / MEGA65
 *
 * The low-level layer under stdio_cbm.c (fopen, fgetc, fputc, ...):
 * thin functions around the <cbm.h> KERNAL calls, which bracket each ROM
 * call with __bp_rom/__bp_prog. Status results are 0 on success.
 */

#include <cbm.h>

/* SETNAM keeps a pointer to the name: it must stay valid until OPEN */
unsigned char cbm_k_setnam(const char *filename) {
    unsigned char len = 0;
    if (filename) while (filename[len]) len++;
    cbm_setnam((char *)filename, len);
    return 0;
}

unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary) {
    cbm_setlfs(fd, device, secondary);
    return 0;
}

/* 0 on success, else the I/O status (or 1 when OPEN reported an error) */
unsigned char cbm_k_open(void) {
    if (cbm_open()) {
        unsigned char st = (unsigned char)cbm_readst();
        return st ? st : 1;
    }
    return 0;
}

unsigned char cbm_k_close(unsigned char fd) {
    cbm_close(fd);
    return 0;
}

unsigned char cbm_k_chkin(unsigned char fd) {
    return (unsigned char)cbm_chkin(fd);
}

unsigned char cbm_k_chkout(unsigned char fd) {
    return (unsigned char)cbm_chkout(fd);
}

unsigned char cbm_k_clrchn(void) {
    cbm_clrchn();
    return 0;
}

unsigned char cbm_k_chrin(void) {
    return (unsigned char)cbm_chrin();
}

void cbm_k_chrout(unsigned char c) {
    cbm_chrout(c);
}

unsigned char cbm_k_readst(void) {
    return (unsigned char)cbm_readst();
}
