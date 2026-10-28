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
 *
 * Both show a VIC-IV full-colour (FCM) screen: a byte a pixel, the byte the
 * palette entry. A page is 64000 bytes in bank 4 or 5 (far_malloc, taken by
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

/* The name of the standard tgi driver for a platform */
extern const char tgi_stddrv[];

/* The address of the static standard tgi driver for a platform */
extern const void tgi_static_stddrv[];

/* The two-page (double-buffering) driver */
extern const void tgi_static_fcm2[];

/* A vector font definition (cc45 needs the struct complete for pointers) */
typedef struct tgi_vectorfont tgi_vectorfont;
#include <tgi/tgi-vectorfont.h>

/* Load and install the given driver (by name: one of the drivers above). */
void tgi_load_driver(const char* name);

/* Uninstall, then unload the currently loaded driver. Will call tgi_done if
 * necessary. */
void tgi_unload(void);

/* Install an already loaded driver. Returns the error code (TGI_ERR_OK). */
unsigned char tgi_install(const void* driver);

/* Uninstall the currently loaded driver but do not unload it. Will call
 * tgi_done if necessary. */
void tgi_uninstall(void);

/* Initialize the already loaded graphics driver. */
void tgi_init(void);

/* End graphics mode, switch back to text mode. Will NOT uninstall or unload
 * the driver! */
void tgi_done(void);

/* Load a vector font into memory and return it. In case of errors, NULL is
 * returned and an error is set, which can be retrieved using tgi_geterror.
 * To use the font, it has to be installed using tgi_install_vectorfont. */
const tgi_vectorfont* tgi_load_vectorfont(const char* name);

/* Install a vector font for use (NULL: none). */
void tgi_install_vectorfont(const tgi_vectorfont* font);

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
void tgi_setviewpage(unsigned char page);

/* Set the drawable page. Will set an error if the page is not available. */
void tgi_setdrawpage(unsigned char page);

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
void tgi_arc(int x, int y, unsigned char rx, unsigned char ry,
             unsigned sa, unsigned ea);

/* Draw an ellipse pie slice with center at x/y and radii rx/ry using the
 * current drawing color. The pie slice covers the angle between sa and ea,
 * which must be in the range 0..360. */
void tgi_pieslice(int x, int y, unsigned char rx, unsigned char ry,
                  unsigned sa, unsigned ea);

/* Draw a bar (a filled rectangle) using the current color. */
void tgi_bar(int x1, int y1, int x2, int y2);

/* Set the direction for text output (TGI_TEXT_*). */
void tgi_settextdir(unsigned char dir);

/* Set the scaling for text output: 8.8 fixed point ($100 = 1, $200 = 2). */
void tgi_settextscale(unsigned width, unsigned height);

/* Set the style for text output: scales as tgi_settextscale, dir one of
 * TGI_TEXT_*, font one of TGI_FONT_*. */
void tgi_settextstyle(unsigned width, unsigned height,
                      unsigned char dir, unsigned char font);

/* Calculate the width of the text in pixels according to the current text
 * style. */
unsigned tgi_gettextwidth(const char* s);

/* Calculate the height of the text in pixels according to the current text
 * style. */
unsigned tgi_gettextheight(const char* s);

/* Output text at the current graphics cursor position. The graphics cursor
 * is moved to the end of the text. */
void tgi_outtext(const char* s);

/* Output text at the given cursor position. The graphics cursor is moved to
 * the end of the text. */
void tgi_outtextxy(int x, int y, const char* s);

/* Call the driver specific control function (TGI_IOCTL_*); unknown codes
 * set TGI_ERR_INV_FUNC. */
unsigned tgi_ioctl(unsigned char code, void* data);

/* Multiply two values, one being an 8.8 fixed point one, and return the
 * rounded and scaled result. */
int tgi_imulround(int rhs, int lhs);
