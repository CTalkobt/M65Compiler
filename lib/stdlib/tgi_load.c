/* tgi_load.c — <tgi.h>'s tgi_load_driver
 *
 * The drivers are linked in: tgi_load_driver finds one by its name (any
 * case, PETSCII or ASCII) instead of loading a file, and installs it (so it
 * links every driver).
 */
#include <tgi/tgi-error.h>
#include "tgi_drv.h"

__fastcall__ unsigned char tgi_install(const void *driver);
extern unsigned char tgi_error;
extern const void *tgi_drv;
extern const char tgi_stddrv[];
extern const tgi_driver_t tgi_static_stddrv;

/* the two-page driver: tgi_fcm2.c (its screens from tgi_heap.c's heap) */
extern const tgi_driver_t tgi_static_fcm2;
extern const char __tgi_fcm2_name[];

/* 640x200, 16 colours: tgi_ncm.s45 (with its drawing) */
extern const tgi_driver_t tgi_static_ncm640;
extern const char __tgi_ncm640_name[];
extern const tgi_driver_t tgi_static_ncm2;
extern const char __tgi_ncm2_name[];

/* A letter in one case and encoding: PETSCII's upper case and ASCII's lower
 * case as PETSCII's lower case (ASCII's upper case) */
static unsigned char fold(unsigned char c) {
    if (c >= 0x61 && c <= 0x7A) return (unsigned char)(c - 0x20);
    if (c >= 0xC1 && c <= 0xDA) return (unsigned char)(c - 0x80);
    return c;
}

static unsigned char same(const char *a, const char *b) {
    while (fold((unsigned char)*a) == fold((unsigned char)*b)) {
        if (!*a) return 1;
        a++;
        b++;
    }
    return 0;
}

void tgi_load_driver(const char *name) {
    if (tgi_drv) {
        tgi_error = TGI_ERR_INSTALLED;
        return;
    }
    if (name && same(name, tgi_stddrv))
        tgi_install(&tgi_static_stddrv);
    else if (name && same(name, __tgi_fcm2_name))
        tgi_install(&tgi_static_fcm2);
    else if (name && same(name, __tgi_ncm640_name))
        tgi_install(&tgi_static_ncm640);
    else if (name && same(name, __tgi_ncm2_name))
        tgi_install(&tgi_static_ncm2);
    else
        tgi_error = TGI_ERR_CANNOT_LOAD;
}
