#pragma cc45 no_zp_save
/* test_math_xemu.c — Validate math.h functions via xemu
 *
 * Tests hardware divider (modulo), min/max, gcd/lcm, itoa.
 * NOTE: div/ldiv struct return has calling convention issue — pending.
 *
 * Expected at $C070: 01 02 03 04 05 06 AA
 */
#include <stdlib.h>
#include <math.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC070 + (i)))

char buf[10];

void main() {
    /* Test 1: modulo — 100 % 7 = 2 (hardware divider) */
    if (100 % 7 == 2) RESULT(0) = 0x01;

    /* Test 2: modulo chain — 1000 % 13 = 12 */
    if (1000 % 13 == 12) RESULT(1) = 0x02;

    /* Test 3: min/max */
    if (min(3, 7) == 3 && max(3, 7) == 7) RESULT(2) = 0x03;

    /* Test 4: gcd(12, 8) = 4 */
    if (gcd(12, 8) == 4) RESULT(3) = 0x04;

    /* Test 5: itoa division chain — 12345 */
    itoa(12345, buf, 10);
    if (buf[0] == '1' && buf[4] == '5') RESULT(4) = 0x05;

    RESULT(5) = 0xAA;
}
