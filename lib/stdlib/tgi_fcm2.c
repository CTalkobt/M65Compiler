/* tgi_fcm2.c — <tgi.h>'s two-page 320x200 driver, "mega65-fcm2.tgi"
 *
 * Its drawing is tgi_fcm.s45's, its heap tgi_heap.c's (the screens do not
 * fit in banks 4 and 5 beside two pages).
 */
#include "tgi_drv.h"

const char __tgi_fcm2_name[] = "mega65-fcm2.tgi";

extern const unsigned char __tgi_fcm[];     /* tgi_fcm.s45: its drawing */
extern const unsigned char __tgi_heapm[];   /* tgi_heap.c: its heap */

const tgi_driver_t tgi_static_fcm2 = {
    { 0x74, 0x67, 0x69, TGI_API_VERSION }, __tgi_fcm2_name,
    320, 200, 0, 2, 8, 8, 0x00D4, 0, 0, __tgi_fcm, __tgi_heapm
};
