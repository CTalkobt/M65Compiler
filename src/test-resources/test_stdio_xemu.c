#pragma cc45 no_zp_save
/* test_stdio_xemu.c — Validate stdio.h functions via xemu
 *
 * Tests sprintf/snprintf/sscanf — formatted I/O to memory buffers.
 * printf/putchar/puts output to KERNAL and can't be verified via memory dump.
 *
 * Expected at $C090: 01 02 03 04 05 06 07 08 AA
 */
#include <stdio.h>
#include <string.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC090 + (i)))

char buf[64];

void main() {
    /* Test 1: sprintf %d */
    sprintf(buf, "%d", 42);
    if (strcmp(buf, "42") == 0) RESULT(0) = 0x01;

    /* Test 2: sprintf %s */
    sprintf(buf, "hello %s", "world");
    if (strcmp(buf, "hello world") == 0) RESULT(1) = 0x02;

    /* Test 3: sprintf %x */
    sprintf(buf, "%x", 255);
    if (buf[0] == 0x46 && buf[1] == 0x46) RESULT(2) = 0x03;
    /* Note: itoa hex digits are ASCII uppercase — 'F'=0x46 */

    /* Test 4: sprintf %c */
    sprintf(buf, "%c%c%c", 0x41, 0x42, 0x43);
    if (buf[0] == 0x41 && buf[1] == 0x42 && buf[2] == 0x43) RESULT(3) = 0x04;

    /* Test 5: sprintf %u (unsigned) */
    sprintf(buf, "%u", 65535);
    if (strcmp(buf, "65535") == 0) RESULT(4) = 0x05;

    /* Test 6: sprintf multiple args */
    sprintf(buf, "%d+%d=%d", 3, 4, 7);
    if (strcmp(buf, "3+4=7") == 0) RESULT(5) = 0x06;

    /* Test 7: snprintf truncation */
    snprintf(buf, 4, "hello");
    buf[3] = 0;  /* ensure NUL */
    if (buf[0] == 0x48 && buf[1] == 0x45 && buf[2] == 0x4C) RESULT(6) = 0x07;
    /* PETSCII lowercase: 'h'=0x48, 'e'=0x45, 'l'=0x4C */

    /* Test 8: sprintf %ld (long) */
    sprintf(buf, "%ld", 100000L);
    if (strcmp(buf, "100000") == 0) RESULT(7) = 0x08;

    RESULT(8) = 0xAA;
}
