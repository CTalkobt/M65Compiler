// test_printf: validate sprintf and printf formatting
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdio.h>
#include <string.h>

int main() {
    char buf[128];

    // Test 1: Basic integer format %d
    sprintf(buf, "%d", 123);
    if (strcmp(buf, "123") != 0) return 1;

    // Test 2: Negative integer
    sprintf(buf, "%d", -456);
    if (strcmp(buf, "-456") != 0) return 2;

    // Test 3: Zero
    sprintf(buf, "%d", 0);
    if (strcmp(buf, "0") != 0) return 3;

    // Test 4: Hex format %x
    sprintf(buf, "%x", 255);
    if (strcmp(buf, "ff") != 0) return 4;

    // Test 5: Hex format uppercase %X
    sprintf(buf, "%X", 255);
    if (strcmp(buf, "FF") != 0) return 5;

    // Test 6: Octal format %o
    sprintf(buf, "%o", 64);
    if (strcmp(buf, "100") != 0) return 6;

    // Test 7: String format %s
    sprintf(buf, "%s", "hello");
    if (strcmp(buf, "hello") != 0) return 7;

    // Test 8: Character format %c
    sprintf(buf, "%c", 'A');
    if (strcmp(buf, "A") != 0) return 8;

    // Test 9: Multiple formats
    sprintf(buf, "num=%d str=%s", 42, "test");
    if (strcmp(buf, "num=42 str=test") != 0) return 9;

    // Test 10: Width specifier
    sprintf(buf, "%5d", 42);
    if (strcmp(buf, "   42") != 0) return 10;

    // Test 11: Zero-padding
    sprintf(buf, "%05d", 42);
    if (strcmp(buf, "00042") != 0) return 11;

    // Test 12: Left-align
    sprintf(buf, "%-5d", 42);
    if (strcmp(buf, "42   ") != 0) return 12;

    // Test 13: Plus sign for positive
    sprintf(buf, "%+d", 42);
    if (strcmp(buf, "+42") != 0) return 13;

    // Test 14: Space for positive
    sprintf(buf, "% d", 42);
    if (strcmp(buf, " 42") != 0) return 14;

    // Test 15: Complex format mix
    sprintf(buf, "%d: %s (%x)", 1, "test", 255);
    if (strcmp(buf, "1: test (ff)") != 0) return 15;

    // Test 16: Literal percent
    sprintf(buf, "50%%");
    if (strcmp(buf, "50%") != 0) return 16;

    // Test 17: Empty format
    sprintf(buf, "");
    if (strcmp(buf, "") != 0) return 17;

    // Test 18: Only text
    sprintf(buf, "hello world");
    if (strcmp(buf, "hello world") != 0) return 18;

    // Test 19: Large number
    sprintf(buf, "%d", 999999);
    if (strcmp(buf, "999999") != 0) return 19;

    // Test 20: Negative hex (should be unsigned)
    sprintf(buf, "%x", -1);
    if (buf[0] == '\0') return 20; // Should produce some output

    return 0;
}
