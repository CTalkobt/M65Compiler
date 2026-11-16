/* test_far_index_mul_xemu.c — #585: a value that fits 16 bits times a byte
 * constant, as a 32-bit product (a far array of structs indexed by a 16-bit
 * index: the element's offset), is a call of __mul16y32 (A/X times Y, 24
 * bits), the value read before its widening.
 *
 * The base added to the product next is added in Q (clc; adcq <constant>)
 * when two or more of its bytes are not 0.
 *
 * Results at $C0C0: 1 = pass; [13] $AA.
 */
#include <stdint.h>

struct line { uint16_t start, y; uint8_t h, asc; uint16_t x; uint8_t extra, nsp, page, pad; };   /* 12 bytes */
#define LINES ((struct line __far *)0x40000UL)

uint16_t volatile vi;
uint8_t volatile vb;

uint16_t __attribute__((noinline)) get_y(uint16_t l) { return LINES[l].y; }
void __attribute__((noinline)) set_y(uint16_t l, uint16_t v) { LINES[l].y = v; }
uint8_t __attribute__((noinline)) get_h(uint8_t l) { return LINES[l].h; }
/* in a loop */
uint16_t __attribute__((noinline)) sum_y(uint16_t from, uint16_t n) {
    uint16_t l, s = 0;
    for (l = from; l < from + n; l++) s += LINES[l].y;
    return s;
}
/* the product itself */
uint32_t __attribute__((noinline)) times12(uint16_t l) { return (uint32_t)l * 12; }
uint32_t __attribute__((noinline)) times255(uint16_t l) { return (uint32_t)l * 255; }
uint32_t __attribute__((noinline)) byte200(uint8_t b) { return (uint32_t)b * 200; }
/* the widened value used again: kept */
uint32_t __attribute__((noinline)) twice(uint16_t l) { uint32_t w = l; return w * 10 + w; }
/* not a byte factor, and a value over 16 bits: the 32-bit multiply */
uint32_t __attribute__((noinline)) times300(uint16_t l) { return (uint32_t)l * 300; }
uint32_t __attribute__((noinline)) wide12(uint32_t l) { return l * 12; }

/* the product plus a constant: added in Q (bytes 1 and 2; all four with the
 * carry through them; the same constant twice; one byte: byte by byte) */
#define LINES2 ((struct line __far *)0x4D000UL)
uint16_t __attribute__((noinline)) get_y2(uint16_t l) { return LINES2[l].y; }
void __attribute__((noinline)) set_y2(uint16_t l, uint16_t v) { LINES2[l].y = v; }
uint32_t __attribute__((noinline)) plus4(uint16_t l) { return (uint32_t)l * 12 + 0x12F4F6F8UL; }
uint32_t __attribute__((noinline)) plus4b(uint8_t b) { return 0x12F4F6F8UL + (uint32_t)b * 200; }
uint32_t __attribute__((noinline)) plus1(uint16_t l) { return (uint32_t)l * 12 + 0x00FF0000UL; }
/* the 32-bit multiply's product */
uint32_t __attribute__((noinline)) wide300(uint16_t l) { return (uint32_t)l * 300 + 0x00FFFF01UL; }
uint32_t __attribute__((noinline)) widev(uint32_t a, uint32_t b) { return a * b + 0x80000001UL; }
/* the product used again: not added in Q */
uint32_t __attribute__((noinline)) again(uint16_t l) { uint32_t p = (uint32_t)l * 12; return (p + 0x00012345UL) ^ p; }

int main(void) {
    uint8_t *r = (uint8_t *)0xC0C0;
    uint16_t i;
    /* elements 0, 1, 300, 5461 (the last below 64 KB) and 5462, 6000 (past it) */
    static const uint16_t at[6] = {0, 1, 300, 5461, 5462, 6000};
    for (i = 0; i < 6; i++) set_y(at[i], 0x1100 + i);
    r[0] = get_y(0) == 0x1100 && get_y(1) == 0x1101 && get_y(300) == 0x1102;
    r[1] = get_y(5461) == 0x1103 && get_y(5462) == 0x1104 && get_y(6000) == 0x1105;
    /* where they are */
    r[2] = *(uint16_t __far *)(0x40000UL + 300UL * 12 + 2) == 0x1102 &&
           *(uint16_t __far *)(0x40000UL + 6000UL * 12 + 2) == 0x1105;
    LINES[200].h = 0x5A; LINES[255].h = 0xA5;
    r[3] = get_h(200) == 0x5A && get_h(255) == 0xA5;
    for (i = 5460; i < 5464; i++) set_y(i, i - 5459);
    r[4] = sum_y(5460, 4) == 10;
    vi = 65535;
    r[5] = times12(vi) == 786420UL && times12(0) == 0 && times12(1) == 12;
    r[6] = times255(vi) == 16711425UL && times255(256) == 65280UL;
    vb = 255;
    r[7] = byte200(vb) == 51000UL && byte200(2) == 400;
    r[8] = twice(vi) == 720885UL && twice(7) == 77;
    r[9] = times300(vi) == 19660500UL && wide12(100000UL) == 1200000UL && wide12(0x10000000UL) == 0xC0000000UL;
    for (i = 0; i < 6; i++) set_y2(at[i], 0x2200 + i);
    r[10] = get_y2(0) == 0x2200 && get_y2(300) == 0x2202 && get_y2(5462) == 0x2204 && get_y2(6000) == 0x2205 &&
            *(uint16_t __far *)(0x4D000UL + 6000UL * 12 + 2) == 0x2205 && get_y(6000) == 0x1105;
    vi = 65535; vb = 255;
    r[11] = plus4(vi) == 0x12F4F6F8UL + 786420UL && plus4(1) == 0x12F4F704UL && plus4(0) == 0x12F4F6F8UL &&
            plus4b(vb) == 0x12F4F6F8UL + 51000UL && plus1(vi) == 0x00FF0000UL + 786420UL;
    r[12] = wide300(vi) == 19660500UL + 0x00FFFF01UL && wide300(1) == 0x01000001UL + 44 &&
            widev(100000UL, 3) == 0x80000001UL + 300000UL && widev(0x10000UL, 0x8000UL) == 1 &&
            again(vi) == ((786420UL + 0x12345UL) ^ 786420UL) && again(0) == 0x12345UL;
    r[13] = 0xAA;
    return 0;
}
