// test_itoa: validate itoa()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>
#include <string.h>

int main() {
    char buf[20];

    itoa(0, buf, 10);
    if (strcmp(buf, "0") != 0) return 1;

    itoa(123, buf, 10);
    if (strcmp(buf, "123") != 0) return 2;

    itoa(255, buf, 16);
    if (strcmp(buf, "ff") != 0) return 3;

    itoa(10, buf, 2);
    if (strcmp(buf, "1010") != 0) return 4;

    return 0;
}
