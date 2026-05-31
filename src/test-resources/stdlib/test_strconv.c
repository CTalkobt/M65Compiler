// test_strconv: validate strtol, strtoul, ltoa
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>
#include <string.h>

int main() {
    char buf[32];
    long result;
    char *endptr;

    // Test strtol - base 10
    if (strtol("123", NULL, 10) != 123) return 1;
    if (strtol("-456", NULL, 10) != -456) return 2;
    if (strtol("0", NULL, 10) != 0) return 3;

    // Test strtol - base 16
    if (strtol("FF", NULL, 16) != 255) return 4;
    if (strtol("0x10", NULL, 16) != 16) return 5; // With 0x prefix

    // Test strtol - base 8
    if (strtol("10", NULL, 8) != 8) return 6;
    if (strtol("77", NULL, 8) != 63) return 7;

    // Test strtol - base 2
    if (strtol("101", NULL, 2) != 5) return 8;
    if (strtol("1111", NULL, 2) != 15) return 9;

    // Test strtol with endptr
    result = strtol("123abc", &endptr, 10);
    if (result != 123) return 10;
    if (strcmp(endptr, "abc") != 0) return 11;

    // Test strtol - leading whitespace
    if (strtol("  789", NULL, 10) != 789) return 12;

    // Test strtol - negative with base 16
    if (strtol("-FF", NULL, 16) != -255) return 13;

    // Test strtoul - unsigned long
    if (strtoul("123", NULL, 10) != 123) return 14;
    if (strtoul("456", NULL, 10) != 456) return 15;

    // Test strtoul with base auto-detection (base 0)
    if (strtol("0x20", NULL, 0) != 32) return 16;
    if (strtol("010", NULL, 0) != 8) return 17; // Octal with 0 prefix
    if (strtol("99", NULL, 0) != 99) return 18; // Decimal

    // Test ltoa - base 10
    ltoa(123, buf, 10);
    if (strcmp(buf, "123") != 0) return 19;

    // Test ltoa - negative base 10
    ltoa(-456, buf, 10);
    if (strcmp(buf, "-456") != 0) return 20;

    // Test ltoa - base 16
    ltoa(255, buf, 16);
    if (strcmp(buf, "ff") != 0) return 21;

    // Test ltoa - base 8
    ltoa(64, buf, 8);
    if (strcmp(buf, "100") != 0) return 22;

    // Test ltoa - base 2
    ltoa(5, buf, 2);
    if (strcmp(buf, "101") != 0) return 23;

    // Test ltoa - zero
    ltoa(0, buf, 10);
    if (strcmp(buf, "0") != 0) return 24;

    // Test ltoa - large number
    ltoa(123456, buf, 10);
    if (strcmp(buf, "123456") != 0) return 25;

    // Test ltoa - negative large number
    ltoa(-789012, buf, 10);
    if (strcmp(buf, "-789012") != 0) return 26;

    // Test round-trip: itoa -> strtol
    itoa(9999, buf, 10);
    if (strtol(buf, NULL, 10) != 9999) return 27;

    // Test strtol - empty string edge case
    if (strtol("", NULL, 10) != 0) return 28;

    return 0;
}
