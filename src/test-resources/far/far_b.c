/* far_b.c — __far functions in a second automatic overlay */
#include <overlay.h>
#include "far.h"

__far int fb_mul(int a, int b) { return a * b; }

__far long fb_long(long x) { return x * 2 + 1; }

#pragma cc45 recurse
__far int fb_rec(int n) { return n <= 0 ? 0 : 1 + fa_rec(n - 1); }

__far unsigned char fb_group(void) { return __ovl_current; }

__far void fb_throw(void) { longjmp(jb, 1); }

__far __naked void fb_filler(void) {
    __asm__("rts");
    __asm__(".res 5000");
}
