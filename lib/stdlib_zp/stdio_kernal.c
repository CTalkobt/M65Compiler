/* stdio_kernal.c — KERNAL ROM wrapper functions for MEGA65
 *
 * Low-level interfaces to Commodore KERNAL file I/O routines
 * KERNAL routines reside at $FF5A-$FF63 in ROM
 *
 * Phase 1: KERNAL Integration
 */

#include <stdio.h>

/* ============================================================================
 * KERNAL ROM ADDRESSES
 * ============================================================================
 *
 * These are the standard Commodore KERNAL entry points.
 * On MEGA65, the KERNAL is accessible via JSRFAR with the appropriate bank.
 */

#define KERNAL_SETLFS   0xFF57  /* Set logical file, device, secondary addr */
#define KERNAL_SETNAM   0xFF5D  /* Set filename */
#define KERNAL_OPEN     0xFF5C  /* Open file */
#define KERNAL_CLOSE    0xFF60  /* Close file */
#define KERNAL_CHKIN    0xFF5A  /* Redirect input */
#define KERNAL_CHKOUT   0xFF5D  /* Redirect output */
#define KERNAL_CLRCHN   0xFF63  /* Clear input/output channels */
#define KERNAL_CHRIN    0xFF5F  /* Read character from file */
#define KERNAL_CHROUT   0xFF5D  /* Write character to file */
#define KERNAL_READST   0xFF5F  /* Read status byte */

/* Status byte bits */
#define STATUS_EOF      0x40    /* End of file */
#define STATUS_DEVICE   0x80    /* Device not present */
#define STATUS_NOTFND   0x04    /* File not found */
#define STATUS_FILEOPEN 0x08    /* File is open */
#define STATUS_ERROR    0x10    /* General error */
#define STATUS_WPROT    0x20    /* Write protected */

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/* These will be implemented in assembly via inline asm or separate .s45 file */
extern unsigned char _kernal_jsrfar(unsigned int addr, unsigned char a_reg,
                                    unsigned short x_reg, unsigned char y_reg);

/* ============================================================================
 * INTERNAL STATE
 * ============================================================================ */

static unsigned char _current_filename_len = 0;
static const char *_current_filename = NULL;

/* ============================================================================
 * HELPER: Pointer to address for SETNAM
 * ============================================================================ */

static void _prepare_filename(const char *filename, unsigned int max_len) {
    if (!filename) {
        _current_filename_len = 0;
        _current_filename = NULL;
        return;
    }

    /* Measure filename length, cap at max */
    unsigned int len = 0;
    while (filename[len] && len < max_len) {
        len++;
    }

    _current_filename = filename;
    _current_filename_len = (unsigned char)len;
}

/* ============================================================================
 * SETNAM - Set filename for next file operation
 * ============================================================================
 *
 * Entry: A/X = low/high address of filename string
 *        Y = length of filename
 * Exit:  Status in A (always succeeds)
 *
 * This routine tells KERNAL which filename to use.
 */

unsigned char cbm_k_setnam(const char *filename) {
    if (!filename) {
        return 0x00;
    }

    _prepare_filename(filename, FILENAME_MAX);

    /* TODO: Call KERNAL SETNAM
     * On standard 6502: JSR $FF5D
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * In C, we'd need:
     * 1. Get address of filename string
     * 2. Split into low byte (X) and high byte (A)
     * 3. Put length in Y
     * 4. Call KERNAL SETNAM
     */

    return 0;
}

/* ============================================================================
 * SETLFS - Set logical file, device, and secondary address
 * ============================================================================
 *
 * Entry: A = logical file number (3-14, 0-2 are reserved)
 *        X = device number (8=disk, 1=tape, 0=keyboard)
 *        Y = secondary address (0=read, 1=write, 2=append)
 *
 * This routine configures the file parameters.
 */

unsigned char cbm_k_setlfs(unsigned char fd, unsigned char device, unsigned char secondary) {
    /* Validate inputs */
    if (fd < 3 || fd > 14) {
        return 0x01;  /* Invalid file number */
    }

    /* TODO: Call KERNAL SETLFS
     * On standard 6502: JSR $FF57
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Registers:
     * A = fd
     * X = device
     * Y = secondary
     */

    return 0;
}

/* ============================================================================
 * OPEN - Open file using SETNAM and SETLFS parameters
 * ============================================================================
 *
 * Entry: None (uses SETNAM/SETLFS parameters)
 * Exit:  A = status byte, Carry = error flag
 *
 * This routine opens the file configured by the last SETNAM/SETLFS calls.
 */

unsigned char cbm_k_open(void) {
    /* TODO: Call KERNAL OPEN
     * On standard 6502: JSR $FF5C
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Returns status in A:
     * - 0 = success
     * - 1 = file not found
     * - 2 = file already open
     * - 3 = insufficient memory
     * etc.
     */

    return 0;
}

/* ============================================================================
 * CLOSE - Close file
 * ============================================================================
 *
 * Entry: A = logical file number
 * Exit:  Status in A
 *
 * Closes the specified file and frees the channel.
 */

unsigned char cbm_k_close(unsigned char fd) {
    if (fd < 3 || fd > 14) {
        return 0x01;  /* Invalid file number */
    }

    /* TODO: Call KERNAL CLOSE
     * On standard 6502: JSR $FF60
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Register A = fd
     */

    return 0;
}

/* ============================================================================
 * CHKIN - Redirect input from file
 * ============================================================================
 *
 * Entry: A = logical file number
 * Exit:  Status in A
 *
 * After calling CHKIN, CHRIN reads from the specified file.
 * Use CLRCHN to restore input to keyboard.
 */

unsigned char cbm_k_chkin(unsigned char fd) {
    if (fd < 3 || fd > 14) {
        return 0x01;  /* Invalid file number */
    }

    /* TODO: Call KERNAL CHKIN
     * On standard 6502: JSR $FF5A
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Register A = fd
     */

    return 0;
}

/* ============================================================================
 * CHKOUT - Redirect output to file
 * ============================================================================
 *
 * Entry: A = logical file number
 * Exit:  Status in A
 *
 * After calling CHKOUT, CHROUT writes to the specified file.
 * Use CLRCHN to restore output to screen.
 */

unsigned char cbm_k_chkout(unsigned char fd) {
    if (fd < 3 || fd > 14) {
        return 0x01;  /* Invalid file number */
    }

    /* TODO: Call KERNAL CHKOUT
     * On standard 6502: JSR $FF5D
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Register A = fd
     */

    return 0;
}

/* ============================================================================
 * CLRCHN - Clear input/output redirection
 * ============================================================================
 *
 * Entry: None
 * Exit:  Status in A
 *
 * Restores input to keyboard and output to screen.
 */

unsigned char cbm_k_clrchn(void) {
    /* TODO: Call KERNAL CLRCHN
     * On standard 6502: JSR $FF63
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * No arguments needed.
     */

    return 0;
}

/* ============================================================================
 * CHRIN - Read character from redirected input
 * ============================================================================
 *
 * Entry: None (uses redirected input from CHKIN)
 * Exit:  A = character read
 *        Status in STATUS register (0x90 = EOF)
 *
 * Reads one byte from the currently redirected input.
 */

unsigned char cbm_k_chrin(void) {
    /* TODO: Call KERNAL CHRIN
     * On standard 6502: JSR $FF5F
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Returns byte in A
     */

    return 0;
}

/* ============================================================================
 * CHROUT - Write character to redirected output
 * ============================================================================
 *
 * Entry: A = character to write
 * Exit:  A = character written
 *
 * Writes one byte to the currently redirected output.
 */

void cbm_k_chrout(unsigned char c) {
    /* TODO: Call KERNAL CHROUT
     * On standard 6502: JSR $FFD2 (always goes to screen)
     *                    or JMP $FFF0 for redirected output
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Register A = character
     */
}

/* ============================================================================
 * READST - Read status byte
 * ============================================================================
 *
 * Entry: None
 * Exit:  A = status byte
 *
 * Status byte bits:
 * - Bit 0: Not used
 * - Bit 1: Not used
 * - Bit 2: Not used
 * - Bit 3: File not found
 * - Bit 4: File already open
 * - Bit 5: Insufficient memory / Syntax error
 * - Bit 6: EOF (end of file)
 * - Bit 7: Device not present
 */

unsigned char cbm_k_readst(void) {
    /* TODO: Call KERNAL READST
     * On standard 6502: JSR $FF5F
     * On MEGA65: JSRFAR to KERNAL bank
     *
     * Returns status byte in A
     */

    return 0;
}

/* ============================================================================
 * ASSEMBLY HELPER - JSRFAR Implementation
 * ============================================================================
 *
 * For MEGA65, we need to use JSRFAR to call routines in other banks.
 * This helper handles the bank switching and JSR.
 *
 * To be implemented in: lib/src/stdio_kernal.s45
 */

/*
.segment "TEXT"

; void cbm_k_jsrfar(A=address_low, X=address_high, Y=bank, Z=return_bank)
; Calls JSR to address in specified bank, returns to current bank
cbm_k_jsrfar:
    ; Save return bank in $5F
    stz $5F

    ; Switch to target bank
    lda #$0F        ; Mask for bank register
    tsb $5F

    ; Call via JSR
    jsr ($00, X)    ; JSR to address in AX

    ; Switch back to previous bank
    lda #$0F
    trb $5F

    rts
*/

