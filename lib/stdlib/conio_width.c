/* conio_width.c — conio_set_width / conio_get_width (<conio.h>)
 *
 * The width is the VIC-IV's H640 ($D031 bit 7). Writing $D031 resets the
 * VIC-IV's layout registers (a "hot" register): the screen's and colour
 * RAM's addresses are kept, the characters per row and the row stride set
 * for the new width (twice it with CHR16, as fcm_enable sets up), and the
 * screen is cleared.
 */
#include <conio.h>

#define REG(a) (*(volatile unsigned char *)(a))

int conio_set_width(int width) {
    unsigned char keep[6], i, v;
    if (width != 40 && width != 80) return 0;
    for (i = 0; i < 6; i++) keep[i] = REG(0xD060 + i);
    v = REG(0xD031);
    REG(0xD031) = width == 80 ? (v | 0x80) : (v & 0x7F);
    for (i = 0; i < 6; i++) REG(0xD060 + i) = keep[i];
    REG(0xD05E) = (unsigned char)width;
    REG(0xD058) = (REG(0xD054) & 1) ? (unsigned char)(2 * width) : (unsigned char)width;
    REG(0xD059) = 0;
    clrscr();
    return 1;
}

int conio_get_width(void) {
    return screenwidth();
}
