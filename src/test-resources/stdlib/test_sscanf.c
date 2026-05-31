// test_sscanf: validate sscanf() - string parsing and formatting
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdio.h>

int main() {
    int n;
    int val;
    int a;
    int b;
    char ch;
    int count;
    short sh;
    char buffer[32];

    // Test 1: Parse single integer
    n = 0;
    n = sscanf("42", "%d", &n);
    if (n != 1 || n != 42) return 1;

    // Test 2: Parse integer with spaces
    val = 0;
    sscanf("  100  ", "%d", &val);
    if (val != 100) return 2;

    // Test 3: Parse negative integer
    val = 0;
    sscanf("-50", "%d", &val);
    if (val != -50) return 3;

    // Test 4: Parse zero
    val = 0;
    sscanf("0", "%d", &val);
    if (val != 0) return 4;

    // Test 5: Parse string
    sscanf("hello", "%s", buffer);
    if (buffer[0] != 'h' || buffer[4] != 'o') return 5;

    // Test 6: Parse string with max width
    sscanf("hello world", "%5s", buffer);
    if (buffer[0] != 'h' || buffer[4] != 'o' || buffer[5] != '\0') return 6;

    // Test 7: Parse multiple integers
    a = 0;
    b = 0;
    sscanf("10 20", "%d %d", &a, &b);
    if (a != 10 || b != 20) return 7;

    // Test 8: Parse integer and string
    a = 0;
    sscanf("99 test", "%d %s", &a, buffer);
    if (a != 99 || buffer[0] != 't') return 8;

    // Test 9: Parse character
    ch = 0;
    sscanf("X", "%c", &ch);
    if (ch != 'X') return 9;

    // Test 10: Parse with literal characters
    a = 0;
    sscanf("value:42", "value:%d", &a);
    if (a != 42) return 10;

    // Test 11: Parse comma-separated
    a = 0;
    b = 0;
    sscanf("5,10", "%d,%d", &a, &b);
    if (a != 5 || b != 10) return 11;

    // Test 12: Parse with tabs
    a = 0;
    b = 0;
    sscanf("15\t25", "%d %d", &a, &b);
    if (a != 15 || b != 25) return 12;

    // Test 13: Parse hex format
    a = 0;
    sscanf("FF", "%x", &a);
    if (a != 255) return 13;

    // Test 14: Parse octal format
    a = 0;
    sscanf("77", "%o", &a);
    if (a != 63) return 14;

    // Test 15: Partial parse
    a = 0;
    count = sscanf("42 extra", "%d", &a);
    if (count != 1 || a != 42) return 15;

    // Test 16: Failed parse returns 0
    count = sscanf("abc", "%d", &a);
    if (count != 0) return 16;

    // Test 17: Parse short with h format
    sh = 0;
    sscanf("300", "%hd", &sh);
    if (sh != 300) return 17;

    // Test 18: Parse with precision (ignore for integers)
    a = 0;
    sscanf("123", "%5d", &a);
    if (a != 123) return 18;

    // Test 19: Multiple spaces between values
    a = 0;
    b = 0;
    sscanf("10    20", "%d %d", &a, &b);
    if (a != 10 || b != 20) return 19;

    // Test 20: Empty parse with format
    a = 0;
    sscanf("", "%d", &a);
    if (a != 0 && a != 20) return 20;  // May or may not modify a

    return 0; // All tests passed
}
