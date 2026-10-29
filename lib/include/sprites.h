/* sprites.h — the VIC-IV's 8 hardware sprites (lib/stdlib_asm/sprites.s45)
 *
 *   sprite_init();                              16-bit pointers: images anywhere
 *   dma_copy_far(0x40000UL, ship, 64);          an image in bank 4 (64-byte aligned)
 *   sprite_image(0, 0x40000UL);
 *   sprite_color(0, 1);
 *   sprite_pos(0, SPRITE_X0 + 100, SPRITE_Y0 + 80);
 *   sprite_show(0);
 *   ...
 *   sprite_done();                              the registers as they were
 *
 * Positions are sprite coordinates: the screen's top left is (SPRITE_X0,
 * SPRITE_Y0); a sprite pixel is two physical pixels wide unless sprite_h640(1)
 * (then x counts the 640-wide display's pixels, the left edge at
 * SPRITE_X0_H640). x and y can be up to 1023: bit 8 goes to $D010 / $D077,
 * bit 9 to $D05F / $D078.
 *
 * Images: 63 bytes (24 x 21, a bit a pixel; multicolour: 12 x 21 of two
 * bits), SPRITE_WIDE 8 bytes a row (64 pixels), SPRITE_16COLOR 8 bytes a row
 * of 16 pixels of 4 bits (colour c of sprite n is palette entry n * 16 + c;
 * 0 transparent). SPRITE_TALL sprites are sprite_height() rows.
 *
 * Every function is assembly with the ZP convention in either library; a
 * program links the ones it calls (about 1 KB for all). sprite_init and
 * sprite_done save and restore every register (of $D049, $D04B, $D04D,
 * $D04F and $D070 only the sprites' bits); SPRITE_TALL sprites share one
 * height ($D056: the hardware's).
 */
#ifndef _SPRITES_H
#define _SPRITES_H

#define SPRITE_COUNT     8

/* the screen's top left in sprite coordinates (a 40/80-column screen with
   its borders as the KERNAL sets them) */
#define SPRITE_X0        24
#define SPRITE_Y0        50
#define SPRITE_X0_H640   48

/* sprite_mode */
#define SPRITE_HIRES     0      /* one colour (sprite_color) */
#define SPRITE_MULTICOLOR 1     /* pairs of bits: transparent, sprite_multicolors' first, sprite_color, its second */
#define SPRITE_16COLOR   2      /* 4 bits a pixel, 16 wide (64 bits a row) */

/* sprite_size: flags or'd */
#define SPRITE_NORMAL    0
#define SPRITE_EXPAND_X  1      /* pixels twice as wide */
#define SPRITE_EXPAND_Y  2      /* twice as high */
#define SPRITE_WIDE      4      /* 8 bytes a row: 64 pixels */
#define SPRITE_TALL      8      /* sprite_height() rows instead of 21 */

/* Start: the registers saved; 16-bit sprite pointers in the library's table;
   every sprite off at (0, 0), hi-res, normal size, in front, image 0, not
   tiled, blended or in V400 rows (the palette bank as it was) */
__fastcall__ void sprite_init(void);
/* Every sprite off, the registers sprite_init saved put back */
__fastcall__ void sprite_done(void);

/* Sprite n's image: its data at the 28-bit address addr (a multiple of 64,
   below 4 MB: chip RAM) */
__fastcall__ void sprite_image(unsigned char n, unsigned long addr);

/* Position */
__fastcall__ void sprite_pos(unsigned char n, unsigned int x, unsigned int y);
__fastcall__ void sprite_move(unsigned char n, signed int dx, signed int dy);
__fastcall__ unsigned int sprite_x(unsigned char n);
__fastcall__ unsigned int sprite_y(unsigned char n);

/* On and off */
__fastcall__ void sprite_show(unsigned char n);
__fastcall__ void sprite_hide(unsigned char n);

/* Looks */
__fastcall__ void sprite_color(unsigned char n, unsigned char c);  /* SPRITE_16COLOR: the transparent colour */
__fastcall__ void sprite_multicolors(unsigned char c1, unsigned char c2);
__fastcall__ void sprite_mode(unsigned char n, unsigned char mode);
__fastcall__ void sprite_size(unsigned char n, unsigned char flags);
__fastcall__ void sprite_height(unsigned char rows);              /* every SPRITE_TALL sprite's */
__fastcall__ void sprite_behind(unsigned char n, unsigned char on); /* behind the foreground */
__fastcall__ void sprite_h640(unsigned char on);                  /* positions and pixels at 640 */
__fastcall__ void sprite_v400(unsigned char n, unsigned char on);  /* rows one physical pixel high */
__fastcall__ void sprite_tile(unsigned char n, unsigned char on);  /* repeated to the end of its line */

/* Colours: the palette bank (0-3) sprites take their colours from ($D070
   SPRPALSEL; sprite_init keeps the current one: the KERNAL's $D070 is $FF,
   bank 3 for everything). The text and graphics keep theirs; fill the
   sprites' with <palette.h> after mapping it at $D100 ($D070 bits 6-7, then
   back). A 16-colour sprite's second set of colours: entries n * 16 + c +
   128 (SPRBPMEN) */
__fastcall__ void sprite_palette_bank(unsigned char bank);
__fastcall__ void sprite_altcolors(unsigned char n, unsigned char on);

/* Alpha blending with what is behind the sprite (the hardware's; xemu does
   not show it): on for sprite n, and the level (0-255) for all */
__fastcall__ void sprite_alpha(unsigned char n, unsigned char on);
__fastcall__ void sprite_alpha_level(unsigned char level);

/* Collisions: a bit per sprite, since the last read (the hardware's: pixels
   touching another sprite's / the foreground's; reading clears them) */
__fastcall__ unsigned char sprite_collisions(void);
__fastcall__ unsigned char sprite_bg_collisions(void);
/* 1 when the boxes of sprites a and b overlap (their positions and sizes;
   shown or not) */
__fastcall__ unsigned char sprite_overlap(unsigned char a, unsigned char b);

#endif
