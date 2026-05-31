// test_io: validate I/O functions (puts, putchar, basic output validation)
// Returns 0 (A=$00) on success, non-zero on failure.
// Note: Full I/O testing requires redirected stdout; this validates function calls.

#include <stdio.h>
#include <string.h>

int main() {
    // Test 1: puts() function exists and accepts string
    puts("Test output 1");

    // Test 2: puts() with short string
    puts("hi");

    // Test 3: puts() with longer string
    puts("This is a longer test string");

    // Test 4: putchar() function exists and accepts character
    putchar('A');
    putchar('\n');

    // Test 5: Multiple putchar calls
    putchar('a');
    putchar('b');
    putchar('c');
    putchar('\n');

    // Test 6: putchar with newline
    putchar('\n');

    // Test 7: printf() basic format string
    printf("Test printf\n");

    // Test 8: printf() with integer
    printf("Number: %d\n", 42);

    // Test 9: printf() with character
    printf("Char: %c\n", 'X');

    // Test 10: printf() with string
    printf("String: %s\n", "hello");

    // Test 11: puts() with empty string behavior
    puts("");

    // Test 12: Multiple printf calls
    printf("First: %d\n", 1);
    printf("Second: %d\n", 2);

    // Test 13: printf() with hex format
    printf("Hex: %x\n", 255);

    // Test 14: printf() multiple formats
    printf("Combo: %d %c %s\n", 99, 'Z', "test");

    // Test 15: putchar with special characters
    putchar('\t');
    putchar('\n');

    // Test 16: sprintf() for string formatting
    char buffer[32];
    sprintf(buffer, "Value: %d", 123);
    if (strcmp(buffer, "Value: 123") != 0) return 1;

    // Test 17: sprintf() with multiple fields
    sprintf(buffer, "%d + %d = %d", 2, 3, 5);
    if (strcmp(buffer, "2 + 3 = 5") != 0) return 2;

    // Test 18: sprintf() with string
    sprintf(buffer, "Prefix: %s", "test");
    if (strcmp(buffer, "Prefix: test") != 0) return 3;

    // Test 19: sprintf() with character
    sprintf(buffer, "%c%c%c", 'a', 'b', 'c');
    if (strcmp(buffer, "abc") != 0) return 4;

    // Test 20: sprintf() with formatting
    sprintf(buffer, "0x%x", 255);
    if (strcmp(buffer, "0xff") != 0) return 5;

    return 0; // All I/O function calls succeeded
}
