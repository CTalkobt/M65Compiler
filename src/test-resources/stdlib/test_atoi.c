// test_atoi: validate atoi()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>

int main() {
    if (atoi("0") != 0) return 1;
    if (atoi("1") != 1) return 2;
    if (atoi("123") != 123) return 3;
    if (atoi("  42") != 42) return 4;
    if (atoi("+7") != 7) return 5;
    if (atoi("") != 0) return 6;
    if (atoi("abc") != 0) return 7;
    if (atoi("99xyz") != 99) return 8;
    return 0;
}
