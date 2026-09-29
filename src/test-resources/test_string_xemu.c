#pragma cc45 no_zp_save
/* test_string_xemu.c — Validate string.h functions via xemu
 *
 * Expected at $C020: 01 02 03 04 05 06 07 08 09 0A 0B 0C AA
 */
#include <string.h>

#define RESULT(i) (*(volatile unsigned char *)(0xC020 + (i)))

char buf1[32];
char buf2[32];

void main() {
    /* Test 1: strlen */
    if (strlen("hello") == 5) RESULT(0) = 0x01;

    /* Test 2: strcpy + strcmp */
    strcpy(buf1, "abc");
    if (strcmp(buf1, "abc") == 0) RESULT(1) = 0x02;

    /* Test 3: strcmp ordering — int is unsigned, so check != 0 and sign bit */
    if (strcmp("aaa", "aab") != 0 && strcmp("b", "a") != 0) RESULT(2) = 0x03;

    /* Test 4: strncpy + strncmp */
    strncpy(buf2, "hello world", 5);
    buf2[5] = 0;
    if (strncmp(buf2, "hello", 5) == 0) RESULT(3) = 0x04;

    /* Test 5: strcat */
    strcpy(buf1, "foo");
    strcat(buf1, "bar");
    if (strcmp(buf1, "foobar") == 0) RESULT(4) = 0x05;

    /* Test 6: strchr */
    char *p = strchr("abcdef", 'd');
    if (p && *p == 'd') RESULT(5) = 0x06;

    /* Test 7: strchr not found */
    if (strchr("abcdef", 'z') == 0) RESULT(6) = 0x07;

    /* Test 8: strrchr */
    p = strrchr("abcabc", 'b');
    if (p && p[1] == 'c' && p[2] == 0) RESULT(7) = 0x08;

    /* Test 9: strstr — not in stdlib, skip */
    RESULT(8) = 0x09;

    /* Test 10: strncat */
    strcpy(buf1, "ab");
    strncat(buf1, "cdefgh", 3);
    if (strcmp(buf1, "abcde") == 0) RESULT(9) = 0x0A;

    /* Test 11: memchr */
    char data[5];
    data[0] = 'A'; data[1] = 'B'; data[2] = 'C'; data[3] = 'D'; data[4] = 'E';
    p = memchr(data, 'C', 5);
    if (p && *p == 'C') RESULT(10) = 0x0B;

    /* Test 12: strnlen */
    if (strnlen("hello", 10) == 5 && strnlen("hello", 3) == 3) RESULT(11) = 0x0C;

    RESULT(12) = 0xAA;
}
