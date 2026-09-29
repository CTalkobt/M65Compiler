#pragma cc45 no_zp_save
/* test_ctype_xemu.c — Validate ctype.h functions via xemu
 *
 * NOTE: cc45 character literals use ASCII, but MEGA65 ctype functions
 * use PETSCII ranges. Tests use values valid in both encodings where
 * possible, and PETSCII-specific values for alpha/case functions.
 *
 * Expected at $C030: 01 02 03 04 05 06 07 08 AA
 */
#include <ctype.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC030 + (i)))

void main() {
    /* Test 1: isdigit — same in ASCII and PETSCII */
    if (isdigit(0x30) && isdigit(0x39) && !isdigit(0x41)) RESULT(0) = 0x01;

    /* Test 2: isalpha — use PETSCII values: lowercase=$41-$5A, uppercase=$C1-$DA */
    if (isalpha(0x41) && isalpha(0xC1) && !isalpha(0x30)) RESULT(1) = 0x02;

    /* Test 3: isalnum — digit + PETSCII alpha */
    if (isalnum(0x41) && isalnum(0x35) && !isalnum(0x20)) RESULT(2) = 0x03;

    /* Test 4: isspace — same in ASCII and PETSCII */
    if (isspace(0x20) && isspace(0x0A) && !isspace(0x41)) RESULT(3) = 0x04;

    /* Test 5: isupper / islower — PETSCII: lower=$41-$5A, upper=$C1-$DA */
    if (!isupper(0x41) && isupper(0xC1) && islower(0x41) && !islower(0xC1)) RESULT(4) = 0x05;

    /* Test 6: toupper / tolower — PETSCII conversions */
    if (toupper(0x41) == 0xC1 && tolower(0xC1) == 0x41 && toupper(0x35) == 0x35) RESULT(5) = 0x06;

    /* Test 7: isxdigit — digits + PETSCII hex letters ($41-$46 = a-f in PETSCII) */
    if (isxdigit(0x30) && isxdigit(0x41) && !isxdigit(0x47)) RESULT(6) = 0x07;

    /* Test 8: isprint / iscntrl — same ranges */
    if (isprint(0x41) && !isprint(0x01) && iscntrl(0x01) && !iscntrl(0x41)) RESULT(7) = 0x08;

    RESULT(8) = 0xAA;
}
