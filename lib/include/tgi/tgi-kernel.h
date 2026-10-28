/* tgi/tgi-kernel.h — the TGI kernel's variables (as cc65's)
 *
 * For programs that read them directly; change them only through the
 * tgi_* functions.
 */

#pragma once

#include <tgi.h>

extern const void*   tgi_drv;           /* Pointer to driver */
extern unsigned char tgi_error;         /* Last error code */
extern unsigned char tgi_gmode;         /* Flag: Graphics mode active */
extern signed int    tgi_curx;          /* Current drawing cursor X */
extern signed int    tgi_cury;          /* Current drawing cursor Y */
extern unsigned char tgi_color;         /* Current drawing color */
extern unsigned char tgi_font;          /* Current font type */
extern unsigned char tgi_textdir;       /* Current text direction */
extern unsigned      tgi_xres;          /* X resolution of the current mode */
extern unsigned      tgi_yres;          /* Y resolution of the current mode */
extern unsigned      tgi_xmax;          /* tgi_xres - 1 */
extern unsigned      tgi_ymax;          /* tgi_yres - 1 */
extern unsigned char tgi_colorcount;    /* Number of available colors (0: 256) */
extern unsigned char tgi_pagecount;     /* Number of available screens */
extern unsigned char tgi_fontwidth;     /* System font width in pixels */
extern unsigned char tgi_fontheight;    /* System font height in pixels */
extern unsigned      tgi_aspectratio;   /* Aspect ratio as fixed point 8.8 */
extern unsigned char tgi_flags;         /* TGI driver flags */
extern unsigned      tgi_textscalew[2]; /* Vector/bitmap font scale 8.8 */
extern unsigned      tgi_textscaleh[2]; /* Vector/bitmap font scale 8.8 */
extern unsigned      tgi_charwidth;     /* Width of scaled bitmap font 8.8 */
extern unsigned      tgi_charheight;    /* Height of scaled bitmap font 8.8 */
