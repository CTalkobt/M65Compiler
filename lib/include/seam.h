/* seam.h — VIC-IV screens of two-byte cells: full-colour characters among
 * text (lib/stdlib/seam.c)
 *
 * In Super-Extended Attribute Mode (SEAM: CHR16, $D054 bit 0) a cell is
 * two bytes of screen RAM, a 13-bit character number (low byte first),
 * and two of colour RAM (byte 0 attributes, 0 here; byte 1 the colour).
 * With FCLRHI ($D054 bit 2) a number above $FF is a full-colour character:
 * 64 bytes at number x 64 in the first 384 KB of chip RAM, a byte a pixel
 * (an 8 x 8 picture, any of the 256 palette entries; pixel $00 shows the
 * screen's background $D021, $FF the cell's colour). Numbers up to $FF are
 * the character set's ($D068-$D06A) as in text mode, so text and pictures
 * mix on one screen. In 80 columns a full-colour pixel is as wide as a
 * text one, half as wide as it is tall.
 *
 * A seam_screen_t describes such a screen anywhere in chip RAM (with its
 * colour RAM at an offset from $FF80000: keep it away from the screen the
 * VIC-IV showed, $0000-$07CF at 80 x 25). seam_show makes the VIC-IV show
 * it, keeping what it showed (the registers it changes: $D054, $D058/$D059,
 * $D05E, $D060-$D065), and seam_hide puts that back; the character set,
 * the colours and the rest stay. The writing functions work while it is
 * shown or not. Characters 0-255 come from the screen codes the way the
 * text screen shows them (s"..." strings).
 *
 * Usage:
 *   #include <seam.h>
 *   static seam_screen_t s;
 *   seam_init(&s, 0x58000UL, 0x1000, 80, 25);   // bank 5, colour RAM from $1000
 *   seam_text(&s, 0, 0, s"score", 5, 1);         // white text
 *   seam_put(&s, 10, 5, 0x1000, 0);              // full-colour character $1000 ($40000)
 *   seam_show(&s);  ...  seam_hide(&s);
 */

#ifndef SEAM_H
#define SEAM_H

typedef struct {
    unsigned long screen;       /* screen RAM (linear): cols x rows x 2 bytes */
    unsigned int colour;        /* its colour RAM: an offset from $FF80000 */
    unsigned char cols, rows;
    unsigned char shown;        /* 1 between seam_show and seam_hide */
    unsigned char saved[10];    /* (the registers seam_show changed) */
} seam_screen_t;

/* Describe a screen of cols x rows cells and clear it (spaces, colour 1) */
void seam_init(seam_screen_t *s, unsigned long screen, unsigned int colour,
               unsigned char cols, unsigned char rows);

/* Fill it with spaces in colour */
void seam_clear(const seam_screen_t *s, unsigned char colour);

/* Show it (CHR16 and FCLRHI on); put back what was shown */
void seam_show(seam_screen_t *s);
void seam_hide(seam_screen_t *s);

/* A cell: character number ch, colour (0-15; a full-colour character's
 * $FF pixels) */
void seam_put(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned int ch, unsigned char colour);

/* n screen codes (one byte each, characters 0-255, in bank 0) from x, y
 * in colour */
void seam_text(const seam_screen_t *s, unsigned char x, unsigned char y, const void *codes,
               unsigned char n, unsigned char colour);

/* n cells from x, y from a one-byte screen's characters and colours (bank
 * 0: a row of a text screen's buffers) */
void seam_row(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned char n,
              const void *codes, const void *colours);

/* n cells from x, y (along the row and on: the screen's cells are one run)
 * from character numbers at a linear address (two bytes each, low byte
 * first: a picture's cells); their colours stay */
void seam_cells(const seam_screen_t *s, unsigned char x, unsigned char y, unsigned long numbers,
                unsigned int n);

/* the linear address of cell x, y's screen bytes; of its colour RAM bytes */
unsigned long seam_cell(const seam_screen_t *s, unsigned char x, unsigned char y);
unsigned long seam_colour(const seam_screen_t *s, unsigned char x, unsigned char y);

#endif /* SEAM_H */
