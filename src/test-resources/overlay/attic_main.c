/* attic_main.c — code overlays kept in attic RAM (#315): test_xemu.sh links
 * it with attic_x.c and attic_y.c, --overlay-store=0x8000000-0x803FFFF
 * --overlay-staging=4 (the file is loaded into bank 4, then copied up)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] both overlays run, swapped in by DMA from attic RAM
 *   [1] far_attic_enable over the store's 256 KB leaves the overlays out:
 *       the first 4 KB granule (they are smaller) is not handed out
 *   [2] all the attic RAM it took, written with $FF: the overlays still
 *       come back right
 *   [3] $AA sentinel
 */
#include <far_ptr.h>
#include <far_ops.h>
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char ax(unsigned char v);
unsigned char ay(unsigned char v);

int main(void) {
    far_ptr_t p;
    unsigned char ok = 1, k;

    OUT[0] = ax(1) == 11 && ay(1) == 21 && ax(2) == 12;
    OUT[1] = far_attic_enable(0x8000000UL, 0x803FFFFUL) == 0x3F000UL;
    for (k = 0; k < 63; k++) {
        p = far_malloc(4096u, BANK_ATTIC);
        if (far_is_null(p) || far_to_linear(p) < 0x8001000UL) ok = 0;
        else far_memset(p, 0xFF, 4096u);
    }
    OUT[2] = ok && ay(3) == 23 && ax(3) == 13 && ay(4) == 24;
    OUT[3] = 0xAA;
    return 0;
}
