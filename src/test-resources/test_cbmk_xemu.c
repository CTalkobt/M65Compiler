/* test_cbmk_xemu.c — the cbm_k_ MEGA65 KERNAL wrappers test_cc65_more_xemu.c does not reach
 * (test_xemu.sh gives it an empty D81 as drive 8):
 * Each is checked against the MEGA65 KERNAL itself (what its routines do):
 *   [0] cbm_k_settim: the time of day in SETTIM's registers (Q: A seconds,
 *       X minutes, Y hours, Z tenths, packed BCD), read back by RDTIM
 *   [1] cbm_k_open / cbm_k_ckout / cbm_k_bsout / cbm_k_clrch: a SEQ file
 *       written through the output channel, then read back through
 *       cbm_k_chkin / cbm_k_basin
 *   [2] the serial bus by hand: cbm_k_listen / cbm_k_second / cbm_k_ciout /
 *       cbm_k_unlsn write a file (the OPEN, the data and the CLOSE secondary
 *       addresses), cbm_k_talk / cbm_k_tksa / cbm_k_acptr / cbm_k_untlk read
 *       it back
 *   [3] cbm_k_save, then cbm_k_load (LOAD to another address) gives the bytes
 *   [4] cbm_k_clall closes every file (a CHKIN on one that was open fails
 *       afterwards)
 *   [5] cbm_k_udtim: the STOP key sampled into $91 ($7F held, $FF not; the
 *       MEGA65's UDTIM has no jiffy clock)
 *   [6] cbm_k_scnkey: the STOP column selected ($DC00 = $7F), Shift+C=
 *       switching the character set and back (the MEGA65's SCNKEY does not
 *       fill the keyboard buffer: keys wait at $D610)
 * $AA at [7] when it got to the end. (Names in lower case: cc45 strings are
 * PETSCII, and the drive reads ",s,w" there; upper case is the shifted set.)
 */
#include <cbm.h>
#include <string.h>

static unsigned char *const result = (unsigned char *)0xC0C0;
#define REG(a) (*(volatile unsigned char *)(a))
static volatile unsigned char *zp91;

static const char msg[] = "HELLO";
static unsigned char buf[16];
static unsigned char block[8] = { 1, 2, 3, 4, 250, 251, 252, 253 };
static unsigned char back[8];

static int settim_rdtim(void) {
    unsigned long t;
    /* 12:34:56.7 in SETTIM's registers: Z tenths, Y hours, X minutes, A seconds */
    cbm_k_settim(0x07123456UL);
    t = __jsr(KERNAL_RDTIM).Q;          /* read back by the KERNAL's RDTIM */
    /* the same hours and minutes; a second may have gone by */
    return (t & 0x00FFFF00UL) == 0x00123400UL &&
           (t & 0xFF) >= 0x56 && (t & 0xFF) <= 0x57 && (t >> 24) <= 0x09;
}

static int channel_file(void) {
    unsigned char i, ok = 1;
    cbm_k_setlfs(2, 8, 2);
    cbm_k_setnam("@0:chan,s,w");
    if (cbm_k_open()) return 0;
    if (cbm_k_ckout(2)) { cbm_k_close(2); return 0; }
    for (i = 0; i < 5; i++) cbm_k_bsout(msg[i]);
    cbm_k_clrch();
    cbm_k_close(2);

    cbm_k_setlfs(2, 8, 2);
    cbm_k_setnam("chan,s,r");
    if (cbm_k_open()) return 0;
    if (cbm_k_chkin(2)) { cbm_k_close(2); return 0; }
    for (i = 0; i < 5; i++) buf[i] = cbm_k_basin();
    cbm_k_clrch();
    cbm_k_close(2);
    for (i = 0; i < 5; i++) ok = ok && buf[i] == (unsigned char)msg[i];
    return ok;
}

static void send_name(const char *s) {
    while (*s) cbm_k_ciout((unsigned char)*s++);
}

static int serial_bus(void) {
    unsigned char i, ok = 1;
    /* OPEN 3 on channel 3: LISTEN, SECOND $F3, the name, UNLISTEN */
    cbm_k_listen(8);
    cbm_k_second(0xF3);
    send_name("@0:bus,s,w");
    cbm_k_unlsn();
    /* the data: LISTEN, SECOND $63 */
    cbm_k_listen(8);
    cbm_k_second(0x63);
    for (i = 0; i < 5; i++) cbm_k_ciout((unsigned char)msg[i]);
    cbm_k_unlsn();
    /* CLOSE: LISTEN, SECOND $E3 */
    cbm_k_listen(8);
    cbm_k_second(0xE3);
    cbm_k_unlsn();

    cbm_k_listen(8);
    cbm_k_second(0xF3);
    send_name("bus,s,r");
    cbm_k_unlsn();
    cbm_k_talk(8);
    cbm_k_tksa(0x63);
    for (i = 0; i < 5; i++) buf[i] = cbm_k_acptr();
    cbm_k_untlk();
    cbm_k_listen(8);
    cbm_k_second(0xE3);
    cbm_k_unlsn();
    for (i = 0; i < 5; i++) ok = ok && buf[i] == (unsigned char)msg[i];
    return ok;
}

static int save_load(void) {
    unsigned int end;
    cbm_k_setlfs(1, 8, 0);
    cbm_k_setnam("@0:ksave");
    if (cbm_k_save((unsigned int)block, (unsigned int)block + sizeof block)) return 0;
    memset(back, 0, sizeof back);
    cbm_k_setlfs(1, 8, 0);
    cbm_k_setnam("ksave");
    end = cbm_k_load(0, (unsigned int)back);
    return end == (unsigned int)back + sizeof back && memcmp(back, block, sizeof back) == 0;
}

static int clall_closes(void) {
    unsigned char e;
    cbm_k_setlfs(4, 8, 4);
    cbm_k_setnam("chan,s,r");
    if (cbm_k_open()) return 0;
    cbm_k_clall();
    e = cbm_k_chkin(4);              /* file 4 is no longer open: an error */
    cbm_k_clrch();
    return e != 0;
}

/* (a key held through the keyboard's virtual keys, $D615/$D616: C65 scan
 * codes, row * 8 + column; $7F lets go) */
#define KEY_SCAN_LSHIFT 0x0F
#define KEY_SCAN_CBM    0x3D
#define KEY_SCAN_STOP   0x3F
static void settle(void) {
    volatile unsigned int i;
    for (i = 0; i < 30000u; i++) ;
}
static void hold(unsigned char k1, unsigned char k2) {
    REG(0xD615) = k1;
    REG(0xD616) = k2;
    settle();
}

/* The MEGA65's UDTIM keeps no jiffy clock: it samples the STOP key's
 * column (CIA 1) into $91 for the KERNAL's STOP, $7F while it is held */
static int udtim_stop(void) {
    unsigned char up, down;
    zp91 = (volatile unsigned char *)0x0091;    /* (the ROM's base page) */
    *zp91 = 0;
    cbm_k_udtim();
    up = *zp91;
    hold(KEY_SCAN_STOP, 0x7F);
    cbm_k_udtim();
    down = *zp91;
    hold(0x7F, 0x7F);
    cbm_k_udtim();
    return up == 0xFF && down == 0x7F && *zp91 == 0xFF;
}

/* The MEGA65's SCNKEY does not fill the keyboard buffer (keys wait at
 * $D610): it acts on the modifier keys (Shift+C= switches the character
 * set, $D069 bit 3) and selects the STOP key's column ($DC00 = $7F) */
static int scnkey_mods(void) {
    unsigned char i, c0, c1, c2;
    REG(0xDC00) = 0;
    cbm_k_scnkey();
    if (REG(0xDC00) != 0x7F) return 0;
    for (i = 0; i < 20; i++) cbm_k_scnkey();    /* (its debounce count run out) */
    c0 = REG(0xD069);
    hold(KEY_SCAN_LSHIFT, KEY_SCAN_CBM);
    cbm_k_scnkey();
    c1 = REG(0xD069);
    hold(0x7F, 0x7F);
    for (i = 0; i < 20; i++) cbm_k_scnkey();
    hold(KEY_SCAN_LSHIFT, KEY_SCAN_CBM);
    cbm_k_scnkey();                             /* again: back */
    c2 = REG(0xD069);
    hold(0x7F, 0x7F);
    return c1 == (c0 ^ 0x08) && c2 == c0;
}

int main(void) {
    result[0] = settim_rdtim();
    result[1] = channel_file();
    result[2] = serial_bus();
    result[3] = save_load();
    result[4] = clall_closes();
    /* (the KERNAL's interrupt calls UDTIM and SCNKEY too: off meanwhile) */
    __asm__("sei");
    result[5] = udtim_stop();
    result[6] = scnkey_mods();
    __asm__("cli");
    result[7] = 0xAA;
    return 0;
}
