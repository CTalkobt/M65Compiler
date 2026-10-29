/* tgi_heap.c — the heap of <tgi.h>'s two-page drivers (tgi_static_fcm2,
 * tgi_static_ncm2)
 *
 * Two pages take most of banks 4 and 5: their screens (2000 bytes each) come
 * from malloc. A two-page driver's description
 * refers to __tgi_heapm (tgi_drv.h's heap), so a program links these (and
 * malloc) only with one; tgi_setup.s45's weak __tgi_heap / __tgi_unheap give
 * nothing. __fastcall__: the ZP convention in both libraries, as the weak
 * ones.
 */
#include <stdlib.h>
#include "tgi_drv.h"

const unsigned char __tgi_heapm[1] = { 0 };     /* (what the drivers refer to) */

__fastcall__ void *__tgi_heap(unsigned int size) {
    return malloc(size);
}

__fastcall__ void __tgi_unheap(void *p) {
    free(p);
}
