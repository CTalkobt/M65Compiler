/* tgi.h — Tiny Graphics Interface (cc65's TGI) for the MEGA65
 *
 * cc65's TGI API (https://cc65.github.io/doc/tgi.html), so programs written
 * for it port with their calls unchanged. The functions in assembly
 * (lib/stdlib_asm/tgi_pixel.s45) are __fastcall__: the ZP convention in both
 * libraries, called directly from stack-convention code too. The drivers are linked in (static,
 * no loadable modules): tgi_load_driver finds one by name, tgi_install takes
 * one's address.
 *
 *   tgi_stddrv / tgi_static_stddrv  "mega65-fcm.tgi"   320x200, 256 colours, 1 page
 *   tgi_static_fcm2                 "mega65-fcm2.tgi"  320x200, 256 colours, 2 pages
 *   tgi_static_ncm640               "mega65-ncm640.tgi" 640x200, 16 colours, 1 page
 *   tgi_static_ncm2                 "mega65-ncm2.tgi"  640x200, 16 colours, 2 pages
 *   tgi_static_hires                "mega65-hires.tgi" 320x200, 2 colours a cell (#645)
 *   tgi_static_mcm                  "mega65-mcm.tgi"   160x200, 4 colours a cell
 *   tgi_static_text40               "mega65-text-40x25.tgi" 80x50: block characters
 *   tgi_static_text80               "mega65-text-80x50.tgi" 160x100, the same
 *
 * The 320x200 ones show a VIC-IV full-colour (FCM) screen: a byte a pixel,
 * the byte the palette entry; the 640x200 one nibble-colour characters (NCM):
 * 4 bits a pixel. A page is 64000 bytes in bank 4 or 5 (far_malloc, taken by
 * tgi_install, given back by tgi_uninstall: far_malloc, overlays and conio's
 * full-colour mode keep the rest); so is its screen (2000 bytes).
 * The two-page driver takes most of both banks: its screens come from
 * malloc.
 *
 * Colours are palette indices 0-255 (tgi_getcolorcount returns 0: 256).
 * The palette at tgi_install is the default: entries 0-15 are the C64's
 * colours, so the TGI_COLOR_* values below are cc65's C64 ones.
 * tgi_setpalette(p) shows, at index i, the colour the default palette had at
 * p[i] (256 entries); tgi_done puts the default palette back.
 *
 * Coordinates are signed; everything is clipped to the screen. Bitmap text
 * is the lower/upper case half of the VIC's current font (8x8, PETSCII
 * strings: both cases show as written), the cursor at the top
 * left of the text (TGI_TEXT_VERTICAL: the bottom left, text going up),
 * magnified by whole numbers (the 8.8 scale rounded). Vector fonts are
 * cc65's .vfn files (<tgi/tgi-vectorfont.h>), loaded with fopen.
 */

#pragma once

#include <tgi/tgi-error.h>

/* Font constants for use with tgi_settextstyle */
#define TGI_FONT_BITMAP         0
#define TGI_FONT_VECTOR         1

/* Direction constants for use with tgi_settextstyle */
#define TGI_TEXT_HORIZONTAL     0
#define TGI_TEXT_VERTICAL       1

/* Colours of the default palette (the C64's) */
#define TGI_COLOR_BLACK         0
#define TGI_COLOR_WHITE         1
#define TGI_COLOR_RED           2
#define TGI_COLOR_CYAN          3
#define TGI_COLOR_PURPLE        4
#define TGI_COLOR_GREEN         5
#define TGI_COLOR_BLUE          6
#define TGI_COLOR_YELLOW        7
#define TGI_COLOR_ORANGE        8
#define TGI_COLOR_BROWN         9
#define TGI_COLOR_LIGHTRED      10
#define TGI_COLOR_GRAY1         11
#define TGI_COLOR_GRAY2         12
#define TGI_COLOR_LIGHTGREEN    13
#define TGI_COLOR_LIGHTBLUE     14
#define TGI_COLOR_GRAY3         15

/* tgi_ioctl codes of the MEGA65 drivers */
#define TGI_IOCTL_DRAWPAGE_ADDR 0x80    /* *(unsigned long *)data := the draw page's address */
#define TGI_IOCTL_VIEWPAGE_ADDR 0x81    /* *(unsigned long *)data := the view page's address */
#define TGI_IOCTL_LAYOUT        0x82    /* *(tgi_layout_t *)data := the draw page's layout */

/* A page's layout, for drawing on it directly (DMA, far pointers). Every
 * driver's page is character columns of col_bytes (1600) bytes, one after
 * another; in a column a pixel row is row_bytes (8) bytes after the one above.
 * A column is col_px pixels across: 8 a byte each (FCM, bpp 8) or 16 a nibble
 * each (NCM, bpp 4; the left pixel of a byte its low nibble). page is the
 * current draw page (two-page drivers: after tgi_setdrawpage too). */
typedef struct {
    unsigned long page;                 /* the draw page (as TGI_IOCTL_DRAWPAGE_ADDR) */
    unsigned int  col_bytes;            /* bytes a character column: 1600 */
    unsigned char row_bytes;            /* bytes from a pixel row to the next in a column: 8 */
    unsigned char col_px;               /* pixels a character column: 8 (FCM) / 16 (NCM) */
    unsigned char bpp;                  /* bits a pixel: 8 (FCM) / 4 (NCM) */
} tgi_layout_t;

/* The byte of pixel (x, y) (0 <= x <= tgi_getmaxx(), 0 <= y <= tgi_getmaxy())
 * of a page with layout *l, and the pixel's nibble in it (NCM; 0 the low one,
 * always 0 with FCM) */
#define TGI_PIXEL_ADDR(l, x, y) ((l)->page + (unsigned long)((unsigned)(x) / (l)->col_px) * (l)->col_bytes + \
    (unsigned)(y) * (l)->row_bytes + ((unsigned)(x) % (l)->col_px) * (l)->bpp / 8)
#define TGI_PIXEL_NIBBLE(l, x)  ((l)->bpp == 4 ? (unsigned char)((unsigned)(x) & 1) : 0)

/* Character column c's offset in a page, c * 1600, for c = 0..39 (both kinds of
 * driver have 40 columns): tgi_col_lo[c] | tgi_col_hi[c] << 8 */
extern const unsigned char tgi_col_lo[40];
extern const unsigned char tgi_col_hi[40];

/* The name of the standard tgi driver for a platform */
extern const char tgi_stddrv[];

/* The address of the static standard tgi driver for a platform */
extern const void tgi_static_stddrv[];

/* The two-page (double-buffering) driver */
extern const void tgi_static_fcm2[];

/* 640x200 in 16 colours, one page: the VIC-IV's nibble-colour characters at
 * H640, a page as one of the 320x200 driver's (64000 bytes); colour c is
 * palette entry c for c = 1..14. Colour 0 shows the background colour
 * register $D021 (tgi_init sets it to 0) and colour 15 the characters' colour
 * RAM byte ($0F): they are palette entries 0 and 15 only while those keep
 * these values (a program changing $D021 changes colour 0). */
extern const void tgi_static_ncm640[];

/* The same with two pages (double buffering): the screens from malloc */
extern const void tgi_static_ncm2[];

/* The VIC-II's bitmap modes on the VIC-IV (stdlib_asm/tgi_bmp.s45,
 * stdlib/tgi_bmpsetup.c; #645): an 8000-byte bitmap and 1000 colour bytes in a
 * 16 KB far_malloc block of bank 4 or 5, the screen 40x25 while shown.
 * hires: 320x200, a cell of 8x8 pixels has two colours, its foreground (the
 * colour last set in it) and the background (0): a pixel set in colour c is
 * on and makes c the cell's foreground; in 0, off. mcm: 160x200, a cell of
 * 4x8 pixels has the background (0, $D021) and three colours: a pixel set
 * in c takes the one that is c, else one no pixel of the cell uses, else
 * the first (the cell's others in it change). 16 colours (the palette's
 * first 16). */
extern const void tgi_static_hires[];
extern const void tgi_static_mcm[];

/* The KERNAL's text screen, 40x25 or 80x50 (set by the screen editor:
 * ESC 4, ESC 5), as TGI pixels: a pixel is a quarter of a character cell
 * (80x50, 160x100), the four quarters one of the 16 PETSCII block
 * characters; tgi_outtext writes characters (with the bitmap font: a
 * character a cell, 2x2 pixels, at the cell of the cursor), so text and
 * pixels mix. A pixel's colour is its cell's (the colour RAM): set in c,
 * the quarter on and the cell c; in 0, off. tgi_done puts the screen's size
 * and character set back (stdlib_asm/tgi_txt.s45; #645). */
extern const void tgi_static_text40[];
extern const void tgi_static_text80[];

/* The character set (any driver: the text screen's): the upper-case /
 * graphics one or the lower / upper-case one, CHR$(142) / CHR$(14) to the
 * screen editor; the block characters are in both. 1: lower case now */
unsigned char tgi_toggle_case(void);

/* The rectangle (x1, y1)-(x2, y2) copied to (dx, dy) on the draw page, any
 * driver: a pixel at a time (tgi_getpixel / tgi_setpixel; overlapping
 * copies right; off the screen 0 is read and nothing written) */
void tgi_copyrect(int x1, int y1, int x2, int y2, int dx, int dy);

/* A vector font definition (cc45 needs the struct complete for pointers) */
typedef struct tgi_vectorfont tgi_vectorfont;
#include <tgi/tgi-vectorfont.h>

/* Load and install the given driver (by name: one of the drivers above). */
void tgi_load_driver(const char* name);

/* Uninstall, then unload the currently loaded driver. Will call tgi_done if
 * necessary. */
void tgi_unload(void);

/* Install an already loaded driver. Returns the error code (TGI_ERR_OK). */
__fastcall__ unsigned char tgi_install(const void* driver);

/* Uninstall the currently loaded driver but do not unload it. Will call
 * tgi_done if necessary. */
__fastcall__ void tgi_uninstall(void);

/* Initialize the already loaded graphics driver. */
__fastcall__ void tgi_init(void);

/* End graphics mode, switch back to text mode. Will NOT uninstall or unload
 * the driver! */
__fastcall__ void tgi_done(void);

/* Load a vector font into memory and return it. In case of errors, NULL is
 * returned and an error is set, which can be retrieved using tgi_geterror.
 * To use the font, it has to be installed using tgi_install_vectorfont. */
const tgi_vectorfont* tgi_load_vectorfont(const char* name);

/* Install a vector font for use (NULL: none). */
__fastcall__ void tgi_install_vectorfont(const tgi_vectorfont* font);

/* Free a vector font that was previously loaded into memory. */
void tgi_free_vectorfont(const tgi_vectorfont* font);

/* Return the error code for the last operation. This will also clear the
 * error. */
unsigned char tgi_geterror(void);

/* Get an error message describing the error in code. */
const char* tgi_geterrormsg(unsigned char code);

/* Clear the drawpage (to colour 0). */
void tgi_clear(void);

/* Returns the number of screen pages available. */
unsigned tgi_getpagecount(void);

/* Set the visible page. Will set an error if the page is not available. */
__fastcall__ void tgi_setviewpage(unsigned char page);

/* Set the drawable page. Will set an error if the page is not available. */
__fastcall__ void tgi_setdrawpage(unsigned char page);

/* Get the number of available colors. Zero means 256 colors. */
unsigned char tgi_getcolorcount(void);

/* Return the maximum supported color number. */
unsigned char tgi_getmaxcolor(void);

/* Set the current drawing color (palette index). */
void tgi_setcolor(unsigned char color_index);

/* Return the current drawing color (palette index). */
unsigned char tgi_getcolor(void);

/* Set the palette: 256 entries, each a colour of the default palette. */
void tgi_setpalette(const unsigned char* palette);

/* Return the current palette. */
const unsigned char* tgi_getpalette(void);

/* Return the default palette. */
const unsigned char* tgi_getdefpalette(void);

/* Return the resolution in X direction. */
unsigned tgi_getxres(void);

/* Return the maximum x coordinate (tgi_getxres() - 1). */
unsigned tgi_getmaxx(void);

/* Return the resolution in Y direction. */
unsigned tgi_getyres(void);

/* Return the maximum y coordinate (tgi_getyres() - 1). */
unsigned tgi_getmaxy(void);

/* Returns the aspect ratio for the loaded driver (8.8 fixed point). */
unsigned tgi_getaspectratio(void);

/* Set a new aspect ratio for the loaded driver (8.8 fixed point). */
void tgi_setaspectratio(unsigned aspectratio);

/* Get the color value of a pixel (0 outside the screen). */
__fastcall__ unsigned char tgi_getpixel(int x, int y);

/* Plot a pixel in the current drawing color. */
__fastcall__ void tgi_setpixel(int x, int y);

/* Set the graphics cursor to the given position. */
void tgi_gotoxy(int x, int y);

/* Draw a line in the current drawing color. The graphics cursor will be set
 * to x2/y2 by this call. */
__fastcall__ void tgi_line(int x1, int y1, int x2, int y2);

/* Draw a line in the current drawing color from the graphics cursor to the
 * new end point. The graphics cursor will be updated to x2/y2. */
__fastcall__ void tgi_lineto(int x2, int y2);

/* Draw a circle in the current drawing color. */
__fastcall__ void tgi_circle(int x, int y, unsigned char radius);

/* Draw a full ellipse with center at x/y and radii rx/ry using the current
 * drawing color. */
__fastcall__ void tgi_ellipse(int x, int y, unsigned char rx, unsigned char ry);

/* Draw an ellipse arc with center at x/y and radii rx/ry using the current
 * drawing color. The arc covers the angle between sa and ea (startangle and
 * endangle), which must be in the range 0..360. */
__fastcall__ void tgi_arc(int x, int y, unsigned char rx, unsigned char ry,
                          unsigned sa, unsigned ea);

/* Draw an ellipse pie slice with center at x/y and radii rx/ry using the
 * current drawing color. The pie slice covers the angle between sa and ea,
 * which must be in the range 0..360. */
__fastcall__ void tgi_pieslice(int x, int y, unsigned char rx, unsigned char ry,
                               unsigned sa, unsigned ea);

/* Draw a bar (a filled rectangle) using the current color, or the fill
 * pattern tgi_setpattern / tgi_setpattern_mono set. */
__fastcall__ void tgi_bar(int x1, int y1, int x2, int y2);

/* Fill patterns for tgi_bar (a cc45 extension; lib/stdlib_asm/tgi_pattern.s45,
 * linked only by a program that sets one). A pattern is 8x8 pixels anchored
 * to the screen (pixel (x, y) takes the pattern's (x & 7, y & 7)), so bars
 * side by side meet seamlessly; it is copied by DMA, a character column at a
 * time. tgi_setpattern: 64 colours, row by row (the 640x200 driver uses
 * their low nibbles); pixels of colour transparent (0-255) are left as they
 * are (TGI_TRANSPARENT: none). tgi_setpattern_mono: 8 bytes, a row each,
 * bit 7 the left pixel: set bits fg, clear ones bg or, for TGI_TRANSPARENT,
 * left as they are. tgi_setsolid: the current colour again. In 640x200 a
 * transparent pattern is drawn a pixel at a time. */
#define TGI_TRANSPARENT (-1)
__fastcall__ void tgi_setpattern(const unsigned char *pattern, int transparent);
__fastcall__ void tgi_setpattern_mono(const unsigned char *bits, unsigned char fg, int bg);
__fastcall__ void tgi_setsolid(void);

/* Set the direction for text output (TGI_TEXT_*). */
__fastcall__ void tgi_settextdir(unsigned char dir);

/* Set the scaling for text output: 8.8 fixed point ($100 = 1, $200 = 2). */
__fastcall__ void tgi_settextscale(unsigned width, unsigned height);

/* Set the style for text output: scales as tgi_settextscale, dir one of
 * TGI_TEXT_*, font one of TGI_FONT_*. */
__fastcall__ void tgi_settextstyle(unsigned width, unsigned height,
                      unsigned char dir, unsigned char font);

/* Calculate the width of the text in pixels according to the current text
 * style. */
__fastcall__ unsigned tgi_gettextwidth(const char* s);

/* Calculate the height of the text in pixels according to the current text
 * style. */
__fastcall__ unsigned tgi_gettextheight(const char* s);

/* Output text at the current graphics cursor position. The graphics cursor
 * is moved to the end of the text. */
__fastcall__ void tgi_outtext(const char* s);

/* Output text at the given cursor position. The graphics cursor is moved to
 * the end of the text. */
__fastcall__ void tgi_outtextxy(int x, int y, const char* s);

/* Call the driver specific control function (TGI_IOCTL_*); unknown codes
 * set TGI_ERR_INV_FUNC. */
unsigned tgi_ioctl(unsigned char code, void* data);

/* Multiply two values, one being an 8.8 fixed point one, and return the
 * rounded and scaled result. */
__fastcall__ int tgi_imulround(int rhs, int lhs);
