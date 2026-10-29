/* tgi_drivers.c — <tgi.h>'s standard driver, "mega65-fcm.tgi"
 *
 * The drivers are linked in (no files): tgi.c's (and tgi_setup.s45's) 320x200 full-colour screen,
 * with one page (here) or two (tgi_fcm2.c); the aspect ratio is the C64's
 * (cc65's c64-hi: a 4:3 display). tgi_load_driver (tgi_load.c) finds one by
 * name; a member of its own, so a program installing a driver itself links
 * only that one.
 */
#include <tgi/tgi-error.h>
#include "tgi_drv.h"

const char tgi_stddrv[] = "mega65-fcm.tgi";

const tgi_driver_t tgi_static_stddrv = {
    { 0x74, 0x67, 0x69, TGI_API_VERSION }, tgi_stddrv,
    320, 200, 0, 1, 8, 8, 0x00D4, 0
};
