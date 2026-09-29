#pragma cc45 no_zp_save
/* test_mem_ops_xemu.c — Validate stdlib memset/memcpy/memcmp/memmove via xemu
 *
 * Tests the hand-written 45GS02 assembly implementations in c45.lib.
 * Uses global variables throughout (local arrays/vars emit pseudo-ops
 * not yet supported by the assembler).
 * Results written to $C000 using direct volatile stores.
 *
 * Expected at $C000: 01 02 03 04 05 06 07 08 09 0A AA
 *   01 = memset fills buffer correctly
 *   02 = memset partial fill preserves rest
 *   03 = memcpy basic copy
 *   04 = memcpy zero-length is safe
 *   05 = memcmp equal buffers return 0
 *   06 = memcmp detects difference
 *   07 = memcmp returns correct sign
 *   08 = memmove non-overlapping copy
 *   09 = memmove overlapping forward (src < dst)
 *   0A = memmove overlapping backward (src > dst)
 *   AA = completion marker
 */

#include <string.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC000 + (i)))

unsigned char buf[32];
unsigned char src[16];
unsigned char dst[16];
int g_i;
int g_ok;

void main() {
    /* Test 1: memset fills entire buffer */
    memset(buf, 0x55, 16);
    g_ok = 1;
    for (g_i = 0; g_i < 16; g_i++)
        if (buf[g_i] != 0x55) g_ok = 0;
    if (g_ok) RESULT(0) = 0x01;

    /* Test 2: memset partial fill — first 8 zeroed, rest preserved */
    memset(buf, 0xBB, 16);
    memset(buf, 0, 8);
    g_ok = 1;
    for (g_i = 0; g_i < 8; g_i++)
        if (buf[g_i] != 0) g_ok = 0;
    for (g_i = 8; g_i < 16; g_i++)
        if (buf[g_i] != 0xBB) g_ok = 0;
    if (g_ok) RESULT(1) = 0x02;

    /* Test 3: memcpy basic copy */
    for (g_i = 0; g_i < 8; g_i++) src[g_i] = (unsigned char)(g_i + 0x10);
    memset(dst, 0, 16);
    memcpy(dst, src, 8);
    g_ok = 1;
    for (g_i = 0; g_i < 8; g_i++)
        if (dst[g_i] != (unsigned char)(g_i + 0x10)) g_ok = 0;
    for (g_i = 8; g_i < 16; g_i++)
        if (dst[g_i] != 0) g_ok = 0;
    if (g_ok) RESULT(2) = 0x03;

    /* Test 4: memcpy zero-length should not crash or modify */
    dst[0] = 0xEE;
    memcpy(dst, src, 0);
    if (dst[0] == 0xEE) RESULT(3) = 0x04;

    /* Test 5: memcmp — equal buffers */
    memset(src, 0x42, 8);
    memset(dst, 0x42, 8);
    if (memcmp(src, dst, 8) == 0) RESULT(4) = 0x05;

    /* Test 6: memcmp — detects difference */
    dst[4] = 0x41;
    if (memcmp(src, dst, 8) != 0) RESULT(5) = 0x06;

    /* Test 7: memcmp — correct sign (0x42 > 0x41, should be positive) */
    if (memcmp(src, dst, 8) > 0) RESULT(6) = 0x07;

    /* Test 8: memmove — non-overlapping (same as memcpy) */
    for (g_i = 0; g_i < 8; g_i++) src[g_i] = (unsigned char)(g_i + 0x20);
    memset(dst, 0, 16);
    memmove(dst, src, 8);
    g_ok = 1;
    for (g_i = 0; g_i < 8; g_i++)
        if (dst[g_i] != (unsigned char)(g_i + 0x20)) g_ok = 0;
    if (g_ok) RESULT(7) = 0x08;

    /* Test 9: memmove overlapping forward (src < dst, must copy backward)
     * buf: [A0 A1 A2 A3 A4 A5 A6 A7 ...]
     * memmove(buf+2, buf, 6) -> [A0 A1 A0 A1 A2 A3 A4 A5 ...] */
    for (g_i = 0; g_i < 8; g_i++) buf[g_i] = (unsigned char)(0xA0 + g_i);
    memmove(buf + 2, buf, 6);
    if (buf[0] == 0xA0 && buf[1] == 0xA1 &&
        buf[2] == 0xA0 && buf[3] == 0xA1 &&
        buf[4] == 0xA2 && buf[5] == 0xA3 &&
        buf[6] == 0xA4 && buf[7] == 0xA5)
        RESULT(8) = 0x09;

    /* Test 10: memmove overlapping backward (src > dst, must copy forward)
     * buf: [B0 B1 B2 B3 B4 B5 B6 B7 ...]
     * memmove(buf, buf+2, 6) -> [B2 B3 B4 B5 B6 B7 B6 B7 ...] */
    for (g_i = 0; g_i < 8; g_i++) buf[g_i] = (unsigned char)(0xB0 + g_i);
    memmove(buf, buf + 2, 6);
    if (buf[0] == 0xB2 && buf[1] == 0xB3 &&
        buf[2] == 0xB4 && buf[3] == 0xB5 &&
        buf[4] == 0xB6 && buf[5] == 0xB7)
        RESULT(9) = 0x0A;

    RESULT(10) = 0xAA;
}
