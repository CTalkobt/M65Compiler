// Example: itoa — validate integer-to-string conversion
// Uses stdlib45.lib stdlib.h and string.h implementations.
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>
#include <string.h>

char buf[20];

int main() {
    // Decimal
    itoa(0, buf, 10);
    if (strcmp(buf, "0") != 0) return 1;

    itoa(1, buf, 10);
    if (strcmp(buf, "1") != 0) return 2;

    itoa(123, buf, 10);
    if (strcmp(buf, "123") != 0) return 3;

    itoa(65535, buf, 10);
    if (strcmp(buf, "65535") != 0) return 4;

    // Hex
    itoa(255, buf, 16);
    if (strcmp(buf, "ff") != 0) return 5;

    itoa(4096, buf, 16);
    if (strcmp(buf, "1000") != 0) return 6;

    // Binary
    itoa(10, buf, 2);
    if (strcmp(buf, "1010") != 0) return 7;

    // Single digit
    itoa(7, buf, 8);
    if (strcmp(buf, "7") != 0) return 8;

    return 0;
}
