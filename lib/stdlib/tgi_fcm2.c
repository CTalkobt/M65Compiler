/* tgi_fcm2.c — <tgi.h>'s two-page driver, "mega65-fcm2.tgi"
 *
 * Two pages take most of banks 4 and 5: their screens (2000 bytes each) come
 * from malloc. Here, with the driver, so a program with the one-page driver
 * links no malloc (tgi_setup.s45's weak __tgi_heap / __tgi_unheap give nothing).
 * __fastcall__: the ZP convention in both libraries, as the weak ones.
 */
#include <stdlib.h>
#include "tgi_drv.h"

const char __tgi_fcm2_name[] = "mega65-fcm2.tgi";

extern const unsigned char __tgi_fcm[];     /* tgi_fcm.s45: its drawing */

const tgi_driver_t tgi_static_fcm2 = {
    { 0x74, 0x67, 0x69, TGI_API_VERSION }, __tgi_fcm2_name,
    320, 200, 0, 2, 8, 8, 0x00D4, 0, 0, __tgi_fcm
};

__fastcall__ void *__tgi_heap(unsigned int size) {
    return malloc(size);
}

__fastcall__ void __tgi_unheap(void *p) {
    free(p);
}
