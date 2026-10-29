/* tgi_drv.h — the TGI library's own declarations (not installed)
 *
 * A driver is a description (tgi_driver_t): the drivers are linked in and
 * all show the VIC-IV full-colour screen of tgi.c, so the description is all
 * that differs between them.
 */

#pragma once

#include <tgi/tgi-vectorfont.h>

#define TGI_API_VERSION 0x05

typedef struct {
    unsigned char sig[4];       /* 't' 'g' 'i' TGI_API_VERSION (ASCII) */
    const char *name;           /* tgi_load_driver's name */
    unsigned xres, yres;
    unsigned char colorcount;   /* 0: 256 */
    unsigned char pagecount;
    unsigned char fontwidth, fontheight;
    unsigned aspectratio;       /* 8.8 */
    unsigned char flags;
    unsigned char mode;         /* 0: 320x200, full colour; $80: 640x200, nibble colour */
} tgi_driver_t;

/* The drivers are tgi_drivers.c's tgi_static_stddrv and tgi_static_fcm2
 * (<tgi.h> declares them as void arrays, as cc65 does). */

/* tgi_pixel.s45: a line clipped to the screen (the cursor does not move) */
__fastcall__ void __tgi_clipline(int x1, int y1, int x2, int y2);

/* tgi.c: the installed vector font (cc65 keeps it to assembly: in C its
 * name would be the type's) */
extern const struct tgi_vectorfont *__tgi_vfont;
