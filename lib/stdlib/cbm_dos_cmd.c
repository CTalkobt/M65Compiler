/* cbm_dos_cmd.c — a DOS command on the drive's command channel (#762)
 *
 * int __cbm_dos_cmd(unsigned char device, const char *cmd, unsigned char *count);
 *   Sends cmd ("S:NAME", "R:NEW=OLD", "N:NAME,ID", ...) and a CR on logical
 *   file 15, the command channel cbm_drive_status keeps open (opening a
 *   second file with secondary address 15 and closing it would make the
 *   drive close every file it has open), then reads the drive's answer,
 *   "NN,MESSAGE,TT,SS". Returns NN (0 OK, 1 FILES SCRATCHED, >= 20 an
 *   error), -1 when the channel can't be opened or the answer is not a
 *   status. *count (if not NULL) gets TT: the number of files scratched.
 *   The answer is kept in __cbm_dos_msg (its text, up to 40 characters).
 */

/* Written for standard C signedness (int results compared with < 0) */
#pragma cc45 signed_default

#include <stdio.h>

extern unsigned char cbm_k_chkin(unsigned char fd);
extern unsigned char cbm_k_chkout(unsigned char fd);
extern unsigned char cbm_k_clrchn(void);
extern unsigned char cbm_k_chrin(void);
extern void cbm_k_chrout(unsigned char c);
extern unsigned char cbm_k_readst(void);

char __cbm_dos_msg[41];

/* the number at s (two digits), else -1 */
static int two_digits(const char *s) {
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') return -1;
    return (s[0] - '0') * 10 + (s[1] - '0');
}

int __cbm_dos_cmd(unsigned char device, const char *cmd, unsigned char *count) {
    unsigned char n = 0, c, commas = 0;
    int st, tt = 0;
    /* (opens the channel; reads, and so clears, the status left before) */
    if (cbm_drive_status(device) < 0) return -1;
    if (cbm_k_chkout(15) != 0) {
        cbm_k_clrchn();
        return -1;
    }
    while (*cmd) cbm_k_chrout((unsigned char)*cmd++);
    cbm_k_chrout(13);
    cbm_k_clrchn();
    if (cbm_k_chkin(15) != 0) {
        cbm_k_clrchn();
        return -1;
    }
    for (;;) {
        c = cbm_k_chrin();
        if (c == 13) break;
        if (n < sizeof(__cbm_dos_msg) - 1) __cbm_dos_msg[n++] = (char)c;
        if (cbm_k_readst() & 0x40) break;   /* (EOI: the last character) */
    }
    cbm_k_clrchn();
    __cbm_dos_msg[n] = 0;
    st = two_digits(__cbm_dos_msg);
    /* TT, after the second comma */
    for (c = 0; c < n && commas < 2; c++)
        if (__cbm_dos_msg[c] == ',') commas++;
    if (commas == 2) tt = two_digits(__cbm_dos_msg + c);
    if (count) *count = tt < 0 ? 0 : (unsigned char)tt;
    return st;
}
