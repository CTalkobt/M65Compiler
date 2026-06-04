/* memset_screen — Fill screen with each character 0-255, repeating.
 *
 * Demonstrates memset with direct screen RAM writes.
 */

#include <string.h>
#include <mega65.h>

#define SCREEN  ((char *)0x0800)
#define SCRSIZE (40 * 25)

int main() {
    unsigned char ch;

    VREG_BORDER = COLOR_BLUE;
    VREG_BG0 = COLOR_BLACK;
    __asm__("sei"); 

    while (1) {
//    VREG_BG0 = COLOR_BLACK;
        for (ch = 0; ; ch++) {
//    VREG_BORDER = ch;
            memset(SCREEN, ch, SCRSIZE);
            if (ch >= 255) break;
        }
// VREG_BG0=COLOR_RED;
    }

    return 0;
}
