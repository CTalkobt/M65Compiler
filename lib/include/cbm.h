/* cbm.h — Commodore KERNAL interface for cc45 / MEGA65
 *
 * Wrappers for the standard CBM KERNAL jump table. Each is a __jsr call
 * (cc45's call of an address with the CPU's registers): a ROM address goes
 * through __rom_call (lib/stdlib_asm/rom_call.s45), which switches to the
 * ROM's base page and page-1 stack for the routine and back. Write your
 * own the same way: __jsr(KERNAL_PLOT, .Carry = 1, .X -> row, .Y -> col).
 * Compatible with C64, C128, and MEGA65 KERNAL.
 *
 * Usage:
 *   #include <cbm.h>
 *   cbm_chrout('A');
 *   char ch = cbm_chrin();
 */

#pragma once

/* ===== KERNAL Jump Table Addresses ===== */

#define KERNAL_ACPTR    0xFF13  /* Get byte from serial bus */
#define KERNAL_CHKIN    0xFFC6  /* Open channel for input */
#define KERNAL_CHKOUT   0xFFC9  /* Open channel for output */
#define KERNAL_CHRIN    0xFFCF  /* Get character from input channel */
#define KERNAL_CHROUT   0xFFD2  /* Output character to channel */
#define KERNAL_CIOUT    0xFFA8  /* Send byte to serial bus */
#define KERNAL_CINT     0xFF81  /* Initialize screen editor */
#define KERNAL_CLALL    0xFFE7  /* Close all open files */
#define KERNAL_CLOSE    0xFFC3  /* Close a logical file */
#define KERNAL_CLRCHN   0xFFCC  /* Clear I/O channels to default */
#define KERNAL_GETIN    0xFFE4  /* Get character from keyboard queue */
#define KERNAL_IOBASE   0xFFF3  /* Get I/O base address */
#define KERNAL_IOINIT   0xFF84  /* Initialize I/O devices */
#define KERNAL_LISTEN   0xFFB1  /* Send LISTEN to serial bus */
#define KERNAL_LOAD     0xFFD5  /* Load from device */
#define KERNAL_MEMBOT   0xFF9C  /* Get/set bottom of memory */
#define KERNAL_MEMTOP   0xFF99  /* Get/set top of memory */
#define KERNAL_OPEN     0xFFC0  /* Open a logical file */
#define KERNAL_PLOT     0xFFF0  /* Get/set cursor position */
#define KERNAL_RAMTAS   0xFF87  /* Initialize RAM, tape buffer, screen */
#define KERNAL_RDTIM    0xFFDE  /* Read system clock */
#define KERNAL_READST   0xFFB7  /* Read I/O status word */
#define KERNAL_RESTOR   0xFF8A  /* Restore default I/O vectors */
#define KERNAL_SAVE     0xFFD8  /* Save to device */
#define KERNAL_SCNKEY   0xFF9F  /* Scan keyboard */
#define KERNAL_SCREEN   0xFFED  /* Get screen size */
#define KERNAL_SECOND   0xFF93  /* Send secondary address after LISTEN */
#define KERNAL_SETLFS   0xFFBA  /* Set logical/first/second address */
#define KERNAL_SETMSG   0xFF90  /* Set KERNAL message control */
#define KERNAL_SETNAM   0xFFBD  /* Set filename */
#define KERNAL_SETTIM   0xFFDB  /* Set system clock */
#define KERNAL_STOP     0xFFE1  /* Check for STOP key */
#define KERNAL_TALK     0xFFB4  /* Send TALK to serial bus */
#define KERNAL_TKSA     0xFF96  /* Send secondary address after TALK */
#define KERNAL_UDTIM    0xFFEA  /* Update system clock */
#define KERNAL_UNLSN    0xFFAE  /* Send UNLISTEN to serial bus */
#define KERNAL_UNTLK    0xFFAB  /* Send UNTALK to serial bus */
#define KERNAL_VECTOR   0xFF8D  /* Get/set KERNAL vectors */

/* ===== Character I/O ===== */

/* Output a character to the current output channel */
static void cbm_chrout(char c) { __jsr(KERNAL_CHROUT, .A = c); }

/* Read a character from the current input channel */
static int cbm_chrin(void) { return __jsr(KERNAL_CHRIN).A; }

/* Get a character from the keyboard queue (non-blocking, 0 if empty) */
static int cbm_getin(void) { return __jsr(KERNAL_GETIN).A; }

/* ===== I/O Status ===== */

/* Read the I/O status byte */
static int cbm_readst(void) { return __jsr(KERNAL_READST).A; }

/* ===== File I/O ===== */

/* Set logical file parameters: logical number, device, secondary address
 * KERNAL SETLFS: A=lfn, X=device, Y=sa
 */
static void cbm_setlfs(char lfn, char device, char sa) { __jsr(KERNAL_SETLFS, .A = lfn, .X = device, .Y = sa); }

/* Set filename for next OPEN/LOAD/SAVE
 * KERNAL SETNAM: A=length, X=lo(name), Y=hi(name)
 */
static void cbm_setnam(char *name, char len) { __jsr(KERNAL_SETNAM, .A = len, .XY = (unsigned int)name); }

/* The banks of the next LOAD / SAVE's data and of its file name (the C65
 * KERNAL's SETBNK: A = data bank, X = name bank; 0-5, chip RAM)
 */
static void cbm_setbnk(char data_bank, char name_bank) { __jsr(0xFF6B, .A = data_bank, .X = name_bank); }

/* Load a file from device to a linear address in chip RAM (banks 0-5:
 * $0000000-$005FFFF), skipping its two-byte header: name (in bank 0) of
 * len characters. Returns 0 on success, 1 on error. The next LOAD's banks
 * are 0 again after it.
 */
static int cbm_load_far(char *name, char len, char device, unsigned long lin) {
    int err;
    cbm_setbnk((char)(lin >> 16), 0);
    cbm_setnam(name, len);
    cbm_setlfs(0, device, 0);
    err = __jsr(KERNAL_LOAD, .A = 0, .XY = (unsigned int)lin).Carry;
    cbm_setbnk(0, 0);
    return err;
}

/* Open a logical file (call cbm_setlfs and cbm_setnam first)
 * Returns 0 on success, 1 on error (carry set)
 */
static int cbm_open(void) { return __jsr(KERNAL_OPEN).Carry; }

/* Close a logical file */
static void cbm_close(char lfn) { __jsr(KERNAL_CLOSE, .A = lfn); }

/* Set input channel. Returns 0 on success, 1 on error. */
static int cbm_chkin(char lfn) { return __jsr(KERNAL_CHKIN, .X = lfn).Carry; }

/* Set output channel. Returns 0 on success, 1 on error. */
static int cbm_chkout(char lfn) { return __jsr(KERNAL_CHKOUT, .X = lfn).Carry; }

/* Restore default I/O channels */
static void cbm_clrchn(void) { __jsr(KERNAL_CLRCHN); }

/* Close all open files */
static void cbm_clall(void) { __jsr(KERNAL_CLALL); }

/* ===== Keyboard ===== */

/* Check if STOP key is pressed (returns non-zero if pressed: STOP sets Z) */
static int cbm_stop(void) { return __jsr(KERNAL_STOP).Zero; }

/* ===== Screen ===== */

/* Set cursor position (col, row). 0-based.
 * KERNAL PLOT (CLC): X=row, Y=col
 */
static void cbm_gotoxy(char col, char row) { __jsr(KERNAL_PLOT, .Carry = 0, .X = row, .Y = col); }

/* Get cursor position. Returns col in low byte, row in high byte.
 * KERNAL PLOT (SEC) returns X=row, Y=col.
 */
static int cbm_wherexy(void) {
    unsigned char row, col;
    __jsr(KERNAL_PLOT, .Carry = 1, .X -> row, .Y -> col);
    return col | (row << 8);
}

/* Get screen size: KERNAL SCREEN's X in the low byte, Y in the high byte.
 * The MEGA65's gives the last column and row (79 and 24 on the 80x25
 * screen); the C64's gives the counts (40, 25).
 */
static int cbm_screen(void) { return __jsr(KERNAL_SCREEN).XY; }

/* ===== System Clock ===== */

/* Read the jiffy clock (1/60s ticks since power-on).
 * Returns low 16 bits (RDTIM returns A=hi, X=mid, Y=lo).
 */
static int cbm_rdtim(void) {
    unsigned char mid, lo;
    __jsr(KERNAL_RDTIM, .X -> mid, .Y -> lo);
    return lo | (mid << 8);
}

/* ===== Serial Bus (IEC) ===== */

/* Send LISTEN command to device */
static void cbm_listen(char device) { __jsr(KERNAL_LISTEN, .A = device); }

/* Send UNLISTEN */
static void cbm_unlisten(void) { __jsr(KERNAL_UNLSN); }

/* Send TALK command to device */
static void cbm_talk(char device) { __jsr(KERNAL_TALK, .A = device); }

/* Send UNTALK */
static void cbm_untalk(void) { __jsr(KERNAL_UNTLK); }

/* Send secondary address after LISTEN */
static void cbm_second(char sa) { __jsr(KERNAL_SECOND, .A = sa); }

/* Send secondary address after TALK */
static void cbm_tksa(char sa) { __jsr(KERNAL_TKSA, .A = sa); }

/* Send byte to serial bus */
static void cbm_ciout(char data) { __jsr(KERNAL_CIOUT, .A = data); }

/* Get byte from serial bus */
static int cbm_acptr(void) { return __jsr(KERNAL_ACPTR).A; }

/* ===== PETSCII Control Codes ===== */

/* ----------------------------------------------------------------------------
 * cc65's KERNAL functions (library: stdlib_asm/cbm_kernal.s45 and
 * stdlib/stdio_kernal.c; through __rom_call, so disk I/O works on the
 * relocated base page and 16-bit stack)
 * ------------------------------------------------------------------------- */

__fastcall__ unsigned char cbm_k_acptr(void);
__fastcall__ unsigned char cbm_k_basin(void);
__fastcall__ void cbm_k_bsout(unsigned char c);
__fastcall__ void cbm_k_ciout(unsigned char c);
__fastcall__ unsigned char cbm_k_ckout(unsigned char lfn);
__fastcall__ void cbm_k_clall(void);
__fastcall__ void cbm_k_clrch(void);
__fastcall__ unsigned char cbm_k_getin(void);
__fastcall__ unsigned int cbm_k_iobase(void);
__fastcall__ void cbm_k_listen(unsigned char dev);
/* LOAD (flag 0) or VERIFY (1) the file cbm_k_setlfs / cbm_k_setnam set up
 * to addr: the end address, or addr on an error (_oserror set) */
__fastcall__ unsigned int cbm_k_load(unsigned char flag, unsigned int addr);
/* SAVE start up to end (not included): 0, or the KERNAL's error */
__fastcall__ unsigned char cbm_k_save(unsigned int start, unsigned int end);
__fastcall__ void cbm_k_scnkey(void);
__fastcall__ void cbm_k_second(unsigned char addr);
__fastcall__ void cbm_k_settim(unsigned long timer);
__fastcall__ void cbm_k_talk(unsigned char dev);
__fastcall__ void cbm_k_tksa(unsigned char addr);
__fastcall__ void cbm_k_udtim(void);
__fastcall__ void cbm_k_unlsn(void);
__fastcall__ void cbm_k_untlk(void);
/* (also in <stdio.h>) */
unsigned char cbm_k_chkin(unsigned char fd);
unsigned char cbm_k_chkout(unsigned char fd);
unsigned char cbm_k_clrchn(void);
unsigned char cbm_k_readst(void);
unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary);
unsigned char cbm_k_setnam(const char *filename);
unsigned char cbm_k_open(void);
unsigned char cbm_k_close(unsigned char fd);
unsigned char cbm_k_chrin(void);
void cbm_k_chrout(unsigned char c);

/* The file name loaded from device to data (NULL: to the address in the
 * file); the bytes loaded, 0 on an error (_oserror set) */
__fastcall__ unsigned int cbm_load(const char *name, unsigned char device, void *data);
/* size bytes at data saved to device as name: 0, or the KERNAL's error
 * (also in _oserror) */
__fastcall__ unsigned char cbm_save(const char *name, unsigned char device, const void *data,
                                    unsigned int size);
/* The last KERNAL error of cbm_load / cbm_save / cbm_k_load (0: none) */
extern unsigned char _oserror;

#define TV_NTSC     0
#define TV_PAL      1
#define TV_OTHER    2
/* TV_NTSC or TV_PAL ($D06F bit 7) */
__fastcall__ unsigned char get_tv(void);
/* Until the raster starts the next frame */
__fastcall__ void waitvsync(void);

#define CH_RETURN       0x0D
#define CH_DELETE        0x14
#define CH_INSERT        0x94
#define CH_CLEAR         0x93
#define CH_HOME          0x13
#define CH_CURSOR_UP     0x91
#define CH_CURSOR_DOWN   0x11
#define CH_CURSOR_LEFT   0x9D
#define CH_CURSOR_RIGHT  0x1D
#define CH_REVERSE_ON    0x12
#define CH_REVERSE_OFF   0x92
#define CH_COLOR_BLACK   0x90
#define CH_COLOR_WHITE   0x05
#define CH_COLOR_RED     0x1C
#define CH_COLOR_CYAN    0x9F
#define CH_COLOR_PURPLE  0x9C
#define CH_COLOR_GREEN   0x1E
#define CH_COLOR_BLUE    0x1F
#define CH_COLOR_YELLOW  0x9E

/* ===== Device Numbers ===== */

#define DEV_KEYBOARD    0
#define DEV_TAPE        1
#define DEV_RS232       2
#define DEV_SCREEN      3
#define DEV_PRINTER     4
#define DEV_DISK_8      8
#define DEV_DISK_9      9
