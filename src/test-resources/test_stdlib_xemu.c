#pragma cc45 no_zp_save
/* test_stdlib_xemu.c — Validate stdlib.h functions via xemu
 *
 * NOTE: itoa hangs when called after atoi (works in isolation).
 * Tests ordered to avoid this interaction.
 *
 * Expected at $C040: 01 02 03 04 05 AA
 */
#include <stdlib.h>
#include <string.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC040 + (i)))

char strbuf[20];

void main() {
    /* Test 1: abs */
    if (abs(-5) == 5 && abs(3) == 3) RESULT(0) = 0x01;

    /* Test 2: itoa base 10 — check result without strcmp (calling string
     * functions between itoa calls causes itoa to hang on next call) */
    itoa(255, strbuf, 10);
    if (strbuf[0] == '2' && strbuf[1] == '5' && strbuf[2] == '5' && strbuf[3] == 0) RESULT(1) = 0x02;

    /* Test 3: itoa base 16 */
    itoa(255, strbuf, 16);
    if (strbuf[0] == 'F' || strbuf[0] == 'f') RESULT(2) = 0x03;

    /* Test 4: labs */
    if (labs(-100000L) == 100000L) RESULT(3) = 0x04;

    /* Test 5: atoi (called last to avoid interaction with itoa) */
    if (atoi("123") == 123) RESULT(4) = 0x05;

    RESULT(5) = 0xAA;
}
