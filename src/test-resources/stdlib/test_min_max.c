// test_min_max: validate min() and max()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <math.h>

int main() {
    if (min(3, 5) != 3) return 1;
    if (min(5, 3) != 3) return 2;
    if (min(4, 4) != 4) return 3;
    if (min(-1, 1) != -1) return 4;
    if (min(-10, -5) != -10) return 5;

    if (max(3, 5) != 5) return 10;
    if (max(5, 3) != 5) return 11;
    if (max(4, 4) != 4) return 12;
    if (max(-1, 1) != 1) return 13;
    if (max(-10, -5) != -5) return 14;

    if (min(32767, -32768) != -32768) return 20;
    if (max(32767, -32768) != 32767) return 21;

    return 0;
}
