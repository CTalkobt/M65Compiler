// test_strlen: validate strlen()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <string.h>

int main() {
    if (strlen("hello") != 5) return 1;
    if (strlen("") != 0) return 2;
    if (strlen("a") != 1) return 3;
    if (strlen("abcdefghij") != 10) return 4;
    return 0;
}
