/* tgi_drivers.c — <tgi.h>'s drivers and tgi_load_driver
 *
 * The drivers are linked in: tgi_load_driver finds one by its name (any
 * case, PETSCII or ASCII) instead of loading a file. Both are tgi.c's
 * 320x200 full-colour screen, with one page or two; the aspect ratio is the
 * C64's (cc65's c64-hi: a 4:3 display).
 */
#include <tgi/tgi-error.h>
#include "tgi_drv.h"

unsigned char tgi_install(const void *driver);
extern unsigned char tgi_error;
extern const void *tgi_drv;

const char tgi_stddrv[] = "mega65-fcm.tgi";
static const char fcm2_name[] = "mega65-fcm2.tgi";

const tgi_driver_t tgi_static_stddrv = {
    { 0x74, 0x67, 0x69, TGI_API_VERSION }, tgi_stddrv,
    320, 200, 0, 1, 8, 8, 0x00D4, 0
};

const tgi_driver_t tgi_static_fcm2 = {
    { 0x74, 0x67, 0x69, TGI_API_VERSION }, fcm2_name,
    320, 200, 0, 2, 8, 8, 0x00D4, 0
};

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
    else if (name && same(name, fcm2_name))
        tgi_install(&tgi_static_fcm2);
    else
        tgi_error = TGI_ERR_CANNOT_LOAD;
}
