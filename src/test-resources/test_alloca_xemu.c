/* test_alloca_xemu.c — Validate local array stack allocation via xemu-mega65
 *
 * Tests compiler-generated stack frame allocation for local arrays.
 * Results written to $C000 using direct volatile casts.
 *
 * Expected at $C000: 01 02 03 AA
 *   01 = local array write/read in main
 *   02 = two arrays don't clobber each other
 *   03 = array fill pattern correct
 *   AA = completion marker
 */

#define RESULT(i) (*(volatile unsigned char *)(0xC000 + (i)))

void main() {
    /* Test 1: Local array — write and read back */
    unsigned char buf[8];
    buf[0] = 0x42;
    buf[7] = 0x99;
    if (buf[0] == 0x42 && buf[7] == 0x99)
        RESULT(0) = 0x01;

    /* Test 2: Two arrays don't clobber each other */
    unsigned char a[4];
    unsigned char b[4];
    a[0] = 0xAA; a[1] = 0xBB; a[2] = 0xCC; a[3] = 0xDD;
    b[0] = 0x11; b[1] = 0x22; b[2] = 0x33; b[3] = 0x44;
    if (a[0] == 0xAA && a[3] == 0xDD && b[0] == 0x11 && b[3] == 0x44)
        RESULT(1) = 0x02;

    /* Test 3: Fill pattern */
    unsigned char fill[16];
    int i;
    for (i = 0; i < 16; i++)
        fill[i] = (unsigned char)i;
    int ok = 1;
    for (i = 0; i < 16; i++)
        if (fill[i] != (unsigned char)i) ok = 0;
    if (ok) RESULT(2) = 0x03;

    RESULT(3) = 0xAA;
}
