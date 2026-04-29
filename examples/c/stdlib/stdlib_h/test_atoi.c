// Example: atoi — validate string-to-integer conversion
// Uses stdlib45.lib stdlib.h implementation.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>

int main() {
    // Basic decimal conversion
    if (atoi("0") != 0) return 1;
    if (atoi("1") != 1) return 2;
    if (atoi("123") != 123) return 3;
    if (atoi("65535") != 65535) return 4;

    // Leading whitespace
    if (atoi("  42") != 42) return 5;

    // Leading plus sign
    if (atoi("+7") != 7) return 6;

    // Empty / non-numeric
    if (atoi("") != 0) return 7;
    if (atoi("abc") != 0) return 8;

    // Digits then non-digits
    if (atoi("99xyz") != 99) return 9;

    return 0;
}
