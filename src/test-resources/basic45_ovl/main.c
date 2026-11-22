/* basic45_ovl/main.c — the resident part of test_basic45_ovl_xemu.bas
 * (#685: code overlays in a basic45 --link program, run in the program's
 * own area, ln45 --overlay-region=_mod_area+512).
 */
#include <overlay.h>
#include <string.h>

/* the area the program swaps its own modules through; the overlays run here */
char mod_area[512];

/* the program uses the area for something else: the overlay in it is gone */
void mod_wipe(void) {
    memset(mod_area, 0, sizeof mod_area);
    __ovl_current = 0;
}

unsigned char ovl_cur(void) {
    return __ovl_current;
}

/* 1: the overlays run in mod_area */
unsigned char region_ok(void) {
    return __ovl_region == mod_area;
}
