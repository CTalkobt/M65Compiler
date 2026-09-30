#pragma cc45 no_zp_save
/* test_math_xemu.c — Validate math.h functions via xemu
 *
 * Tests hardware divider via modulo operator, min/max, and itoa.
 * NOTE: itoa can only be called once per test (second call hangs
 * after other stdlib calls — ZP state interaction issue).
 *
 * Expected at $C070: 01 02 03 04 AA
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

    /* Test 4: itoa division chain — 12345 to string */
    itoa(12345, buf, 10);
    if (buf[0] == '1' && buf[4] == '5') RESULT(3) = 0x04;

    RESULT(4) = 0xAA;
}
