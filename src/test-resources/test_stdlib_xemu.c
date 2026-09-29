#pragma cc45 no_zp_save
/* test_stdlib_xemu.c — Validate stdlib.h functions via xemu
 *
 * Expected at $C040: 01 02 03 04 05 06 07 AA
 */
#include <stdlib.h>
#include <string.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC040 + (i)))

char strbuf[20];

void main() {
    /* Test 1: abs */
    if (abs(-5) == 5 && abs(3) == 3 && abs(0) == 0) RESULT(0) = 0x01;

    /* Test 2: atoi */
    if (atoi("123") == 123 && atoi("-42") == -42) RESULT(1) = 0x02;

    /* Test 3: itoa base 10 */
    itoa(255, strbuf, 10);
    if (strcmp(strbuf, "255") == 0) RESULT(2) = 0x03;

    /* Test 4: itoa base 16 */
    itoa(255, strbuf, 16);
    if (strcmp(strbuf, "FF") == 0 || strcmp(strbuf, "ff") == 0) RESULT(3) = 0x04;

    /* Test 5: labs */
    if (labs(-100000L) == 100000L) RESULT(4) = 0x05;

    /* Test 6: atol */
    if (atol("50000") == 50000L) RESULT(5) = 0x06;

    /* Test 7: ltoa base 10 */
    ltoa(100000L, strbuf, 10);
    if (strcmp(strbuf, "100000") == 0) RESULT(6) = 0x07;

    RESULT(7) = 0xAA;
}
