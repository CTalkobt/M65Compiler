/* cbm.h — Commodore KERNAL interface for cc45 / MEGA65
 *
 * Provides inline wrappers for the standard CBM KERNAL jump table.
 * All functions use __asm__ to call KERNAL routines directly.
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
static void cbm_chrout(char c) {
    __asm__("lda.sp @_p_c");
    __asm__("jsr $FFD2");
}

/* Read a character from the current input channel */
static int cbm_chrin(void) {
    __asm__("jsr $FFCF");
    __asm__("ldx #$00");
    return;
}

/* Get a character from the keyboard queue (non-blocking, 0 if empty) */
static int cbm_getin(void) {
    __asm__("jsr $FFE4");
    __asm__("ldx #$00");
    return;
}

/* ===== I/O Status ===== */

/* Read the I/O status byte */
static int cbm_readst(void) {
    __asm__("jsr $FFB7");
    __asm__("ldx #$00");
    return;
}

/* ===== File I/O ===== */

/* Set logical file parameters: logical number, device, secondary address
 * KERNAL SETLFS: A=lfn, X=device, Y=sa
 */
static void cbm_setlfs(char lfn, char device, char sa) {
    __asm__("lda.sp @_p_sa");
    __asm__("tay");
    __asm__("lda.sp @_p_device");
    __asm__("tax");
    __asm__("lda.sp @_p_lfn");
    __asm__("jsr $FFBA");
}

/* Set filename for next OPEN/LOAD/SAVE
 * KERNAL SETNAM: A=length, X=lo(name), Y=hi(name)
 */
static void cbm_setnam(char *name, char len) {
    __asm__("ldxy @_p_name, sp");  /* X=lo, Y=hi */
    __asm__("lda.sp @_p_len");
    __asm__("jsr $FFBD");
}

/* Open a logical file (call cbm_setlfs and cbm_setnam first)
 * Returns 0 on success, 1 on error (carry set)
 */
static int cbm_open(void) {
    __asm__("jsr $FFC0");
    __asm__("lda #$00");
    __asm__("tax");
    __asm__("adc #$00");  /* A = carry (0=ok, 1=error) */
    return;
}

/* Close a logical file */
static void cbm_close(char lfn) {
    __asm__("lda.sp @_p_lfn");
    __asm__("jsr $FFC3");
}

/* Set input channel. Returns 0 on success, 1 on error. */
static int cbm_chkin(char lfn) {
    __asm__("lda.sp @_p_lfn");
    __asm__("tax");
    __asm__("jsr $FFC6");
    __asm__("lda #$00");
    __asm__("tax");
    __asm__("adc #$00");
    return;
}

/* Set output channel. Returns 0 on success, 1 on error. */
static int cbm_chkout(char lfn) {
    __asm__("lda.sp @_p_lfn");
    __asm__("tax");
    __asm__("jsr $FFC9");
    __asm__("lda #$00");
    __asm__("tax");
    __asm__("adc #$00");
    return;
}

/* Restore default I/O channels */
static void cbm_clrchn(void) {
    __asm__("jsr $FFCC");
}

/* Close all open files */
static void cbm_clall(void) {
    __asm__("jsr $FFE7");
}

/* ===== Keyboard ===== */

/* Check if STOP key is pressed (returns non-zero if pressed) */
static int cbm_stop(void) {
    __asm__("jsr $FFE1");
    /* Z=1 if STOP pressed. Preserve Z across LDX. */
    __asm__("php");
    __asm__("ldx #$00");
    __asm__("plp");
    __asm__("bne *+3");   /* Z=0: not pressed, skip INX */
    __asm__("inx");       /* Z=1: pressed, X=1 */
    __asm__("txa");       /* A=X (0 or 1) */
    return;
}

/* ===== Screen ===== */

/* Set cursor position (col, row). 0-based.
 * KERNAL PLOT (CLC): X=row, Y=col
 */
static void cbm_gotoxy(char col, char row) {
    __asm__("clc");
    __asm__("lda.sp @_p_row");
    __asm__("tax");
    __asm__("lda.sp @_p_col");
    __asm__("tay");
    __asm__("jsr $FFF0");
}

/* Get cursor position. Returns col in low byte, row in high byte. */
static int cbm_wherexy(void) {
    __asm__("sec");
    __asm__("jsr $FFF0");
    /* X=row, Y=col → return col(lo) | row(hi) */
    __asm__("tya");      /* A = col */
    /* X already = row */
    return;
}

/* Get screen size. Returns cols in low byte, rows in high byte.
 * KERNAL SCREEN returns X=cols, Y=rows.
 */
static int cbm_screen(void) {
    __asm__("jsr $FFED");
    /* X=cols, Y=rows → need A=cols, X=rows */
    __asm__("txa");      /* A = cols (save) */
    __asm__("taz");      /* Z = cols */
    __asm__("tya");      /* A = rows */
    __asm__("tax");      /* X = rows */
    __asm__("tza");      /* A = cols */
    return;
}

/* ===== System Clock ===== */

/* Read the jiffy clock (1/60s ticks since power-on).
 * Returns low 16 bits (Y=lo, X=mid from KERNAL).
 */
static int cbm_rdtim(void) {
    __asm__("jsr $FFDE");
    /* RDTIM returns A=hi, X=mid, Y=lo (24-bit clock) */
    __asm__("tya");      /* A = lo byte */
    /* X already = mid byte */
    return;
}

/* ===== Serial Bus (IEC) ===== */

/* Send LISTEN command to device */
static void cbm_listen(char device) {
    __asm__("lda.sp @_p_device");
    __asm__("jsr $FFB1");
}

/* Send UNLISTEN */
static void cbm_unlisten(void) {
    __asm__("jsr $FFAE");
}

/* Send TALK command to device */
static void cbm_talk(char device) {
    __asm__("lda.sp @_p_device");
    __asm__("jsr $FFB4");
}

/* Send UNTALK */
static void cbm_untalk(void) {
    __asm__("jsr $FFAB");
}

/* Send secondary address after LISTEN */
static void cbm_second(char sa) {
    __asm__("lda.sp @_p_sa");
    __asm__("jsr $FF93");
}

/* Send secondary address after TALK */
static void cbm_tksa(char sa) {
    __asm__("lda.sp @_p_sa");
    __asm__("jsr $FF96");
}

/* Send byte to serial bus */
static void cbm_ciout(char data) {
    __asm__("lda.sp @_p_data");
    __asm__("jsr $FFA8");
}

/* Get byte from serial bus */
static int cbm_acptr(void) {
    __asm__("jsr $FF13");
    __asm__("ldx #$00");
    return;
}

/* ===== PETSCII Control Codes ===== */

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
