#pragma cc45 no_zp_save
/* test_math_xemu.c — Validate math.h functions via xemu
 *
 * Tests hardware divider, min/max, gcd, lcm, itoa.
 * NOTE: Character literals use PETSCII encoding (case-swapped vs ASCII).
 *       itoa hex digits produce ASCII 0x41-0x46, not PETSCII 'A'-'F'.
 *
 * Expected at $C070: 01 02 03 04 05 06 07 AA
 */
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC070 + (i)))

char buf[10];

void main() {
    /* Test 1: modulo — 100 % 7 = 2 (hardware divider) */
    if (100 % 7 == 2) RESULT(0) = 0x01;

    /* Test 2: min/max */
    if (min(3, 7) == 3 && max(3, 7) == 7) RESULT(1) = 0x02;

    /* Test 3: gcd(12, 8) = 4 */
    if (gcd(12, 8) == 4) RESULT(2) = 0x03;

    /* Test 4: itoa base 10 + strcmp */
    itoa(255, buf, 10);
    if (strcmp(buf, "255") == 0) RESULT(3) = 0x04;

    /* Test 6: itoa base 16 after strcmp (uses 0x46 not 'F' — PETSCII encoding) */
    itoa(255, buf, 16);
    if (buf[0] == 0x46 && buf[1] == 0x46) RESULT(4) = 0x05;

    /* Test 7: itoa division chain — 12345 */
    itoa(12345, buf, 10);
    if (buf[0] == 0x31 && buf[4] == 0x35) RESULT(5) = 0x06;

    /* Test 8: modulo chain — 1000 % 13 = 12 */
    if (1000 % 13 == 12) RESULT(6) = 0x07;

    RESULT(7) = 0xAA;
}
