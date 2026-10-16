/* main.c — __far functions: calls in every direction
 * results at $C0C0: 01 per check, $AA after the last */
#include <far_ptr.h>
#include <far_ops.h>
#include "far.h"

int counter;

int resident_inc(int x) { return x + 1; }

static int neg(int x) { return -x; }

/* a far function in the file with main */
__far static int local_far(int x) { return x + 100; }

int main(void) {
    volatile unsigned char *r = (volatile unsigned char *)0xC0C0;
    int (*p)(int);
    unsigned int w;

    r[0] = fa_add(2, 3) == 5 && counter == 1;               /* resident -> far */
    r[1] = fa_sq_inc(4) == 16 + 5;                          /* far -> far, far -> resident */
    r[2] = fb_mul(6, 7) == 42 && fb_long(100000L) == 200001L;
    r[3] = fa_rec(9) == 9 && fb_rec(4) == 4;                /* between two overlays */
    p = fa_sq_inc;                                          /* pointers to and from far code */
    r[4] = p(3) == 9 + 4 && fa_apply(neg, 5) == (int)(-5 + 1) && fa_apply(local_far, 1) == 102;
    w = fa_where();
    r[5] = fa_group() != fb_group() && fa_group() != 0 && w >= 0x6000 && w < 0x8000 &&
           local_far(1) == 101;
    /* far_malloc leaves the overlays alone: bank 5 handed out to the last
       byte and overwritten, the far code still runs */
    {
        unsigned int got = 0, n = 0;
        far_ptr_t p5;
        while (n < 30 && !far_is_null(p5 = far_malloc(2048, BANK5))) {
            far_memset(p5, 0xEA, 2048);
            got += 2048 / 256;
            n++;
        }
        r[6] = n > 0 && fa_add(1, 1) == 2 && fb_mul(3, 3) == 9 && fa_rec(5) == 5;
    }
    r[7] = 0xAA;
    return 0;
}
