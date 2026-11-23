/* test_cbm_xemu.c — the <cbm.h> KERNAL wrappers (__jsr calls through
 * __rom_call) at run time
 *
 * Results at $C0C0 (01 = pass):
 *   [0] cbm_gotoxy + cbm_chrout: the characters in screen RAM there
 *   [1] cbm_wherexy: the cursor after them
 *   [2] cbm_screen: the last column and row of the 80x25 screen (79, 24:
 *       the MEGA65's KERNAL SCREEN)
 *   [3] cbm_stop / cbm_getin: no STOP, no key
 *   [4] a file on the screen (device 3): setlfs, setnam, open, chkout,
 *       chrout to it, clrchn, close; the character where the cursor was
 *   [5] cbm_rdtim: the time of day as 0xHHMMSSTT, packed BCD (the MEGA65
 *       KERNAL's RDTIM: Y hours, X minutes, A seconds, Z tenths, #742)
 *   [6] $AA sentinel
 */
#include <cbm.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define SCREEN ((volatile unsigned char *)0x0800)

int main(void) {
    unsigned int pos;
    unsigned char ok;

    cbm_gotoxy(5, 2);                         /* column 5, row 2 */
    cbm_chrout('a');
    cbm_chrout('b');
    OUT[0] = SCREEN[2 * 80 + 5] == 1 && SCREEN[2 * 80 + 6] == 2;
    pos = cbm_wherexy();
    OUT[1] = pos == (7 | (2 << 8));
    OUT[2] = cbm_screen() == (79 | (24 << 8));
    OUT[3] = cbm_stop() == 0 && cbm_getin() == 0;

    ok = 1;
    cbm_gotoxy(10, 4);
    cbm_setlfs(1, 3, 0);
    cbm_setnam("", 0);
    if (cbm_open() != 0) ok = 0;
    if (cbm_chkout(1) != 0) ok = 0;
    cbm_chrout('z');
    cbm_clrchn();
    cbm_close(1);
    OUT[4] = ok && SCREEN[4 * 80 + 10] == 26;

    {
        unsigned long t = cbm_rdtim();
        unsigned char h = t >> 24, m = t >> 16, s = t >> 8, d = t;
        OUT[5] = (h & 15) <= 9 && h <= 0x23 && (m & 15) <= 9 && m <= 0x59 && (s & 15) <= 9 && s <= 0x59 && d <= 9;
    }

    OUT[6] = 0xAA;
    return 0;
}
