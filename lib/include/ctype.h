/* ctype.h — PETSCII-aware character classification for cc45 / MEGA65 */

#pragma once

/*
 * PETSCII character ranges:
 *   Digits    '0'-'9' = $30-$39
 *   Lowercase 'a'-'z' = $41-$5A  (PETSCII default — unshifted)
 *   Uppercase 'A'-'Z' = $61-$7A  (what cc45 makes of 'A' in C source)
 *                     and $C1-$DA  (shifted keys from the keyboard)
 *   Space     $20
 *   CR        $0D  (PETSCII carriage return / Enter)
 *   Tab       $09  (not standard PETSCII, included for compatibility)
 *
 * toupper gives $C1-$DA; tolower takes both uppercase ranges to $41-$5A.
 *
 * All functions are inlined for performance — no function call overhead for
 * character classification operations, all use simple range checks and bit ops.
 */

/* Inlined digit test */
static inline int isdigit(int c) {
    return (c >= 0x30 && c < 0x3A) ? 1 : 0;
}

/* Inlined alphabetic test: lowercase $41-$5A or uppercase $C1-$DA */
static inline int isalpha(int c) {
    return ((c >= 0x41 && c < 0x5B) || (c >= 0x61 && c < 0x7B) || (c >= 0xC1 && c < 0xDB)) ? 1 : 0;
}

/* Inlined alphanumeric test */
static inline int isalnum(int c) {
    return isdigit(c) || isalpha(c);
}

/* Space or $09-$0D (TAB, LF, VT, FF, CR) */
static inline int isspace(int c) {
    return (c == 0x20 || (c >= 0x09 && c <= 0x0D)) ? 1 : 0;
}

/* $20-$7E or $A0-$FF */
static inline int isprint(int c) {
    return ((c >= 0x20 && c < 0x7F) || c >= 0xA0) ? 1 : 0;
}

/* Inlined uppercase test: A-Z in PETSCII = $C1-$DA */
static inline int isupper(int c) {
    return ((c >= 0x61 && c < 0x7B) || (c >= 0xC1 && c < 0xDB)) ? 1 : 0;
}

/* Inlined lowercase test: a-z in PETSCII = $41-$5A */
static inline int islower(int c) {
    return (c >= 0x41 && c < 0x5B) ? 1 : 0;
}

/* Inlined hex digit test: 0-9, a-f, A-F */
static inline int isxdigit(int c) {
    return (isdigit(c) || (c >= 0x41 && c <= 0x46) || (c >= 0x61 && c <= 0x66) || (c >= 0xC1 && c <= 0xC6)) ? 1 : 0;
}

/* Inlined blank test: space or tab */
static inline int isblank(int c) {
    return (c == 0x20 || c == 0x09) ? 1 : 0;
}

/* $00-$1F or $80-$9F */
static inline int iscntrl(int c) {
    return (c < 0x20 || (c >= 0x80 && c < 0xA0)) ? 1 : 0;
}

/* Inlined isgraph test: printable excluding space */
static inline int isgraph(int c) {
    /* Reject space */
    if (c == 0x20) return 0;
    /* Check $21-$7E (printable) */
    if (c >= 0x21 && c < 0x7F) return 1;
    /* Reject non-breaking space $A0 */
    if (c == 0xA0) return 0;
    /* Check $A1-$FF (graphic/uppercase letters) */
    if (c >= 0xA1) return 1;
    return 0;
}

/* Inlined ispunct test: printable non-alphanumeric excluding space */
static inline int ispunct(int c) {
    /* Reject control chars ($00-$1F) */
    if (c < 0x20) return 0;
    /* Reject space */
    if (c == 0x20) return 0;
    /* Reject digits $30-$39 */
    if (c >= 0x30 && c < 0x3A) return 0;
    /* Reject lowercase $41-$5A */
    if (c >= 0x41 && c < 0x5B) return 0;
    /* Reject uppercase $61-$7A ('A' in a cc45 literal) and $C1-$DA */
    if (c >= 0x61 && c < 0x7B) return 0;
    if (c >= 0xC1 && c < 0xDB) return 0;
    /* Everything else: $21-$2F, $3A-$40, $5B-$C0, $DB-$FF */
    return 1;
}

/* Inlined uppercase conversion: lowercase $41-$5A → uppercase $C1-$DA (set bit 7) */
static inline int toupper(int c) {
    if (c >= 0x41 && c < 0x5B) {
        return c | 0x80;
    }
    return c;
}

/* Inlined lowercase conversion: uppercase $61-$7A ('A' in a cc45 literal)
 * or $C1-$DA (shifted keys) → lowercase $41-$5A */
static inline int tolower(int c) {
    if (c >= 0x61 && c < 0x7B) {
        return c - 0x20;
    }
    if (c >= 0xC1 && c < 0xDB) {
        return c & 0x7F;
    }
    return c;
}
