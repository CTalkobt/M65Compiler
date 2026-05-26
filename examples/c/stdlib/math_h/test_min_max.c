// Example: min/max — validate integer min/max functions
// Returns 0 (A=$00) on success, non-zero on failure.

#include <math.h>

int main() {
    // min: basic
    if (min(3, 5) != 3) return 1;
    if (min(5, 3) != 3) return 2;
    if (min(4, 4) != 4) return 3;

    // min: negative
    if (min(-1, 1) != -1) return 4;
    if (min(-10, -5) != -10) return 5;

    // min: zero
    if (min(0, 0) != 0) return 6;
    if (min(0, 1) != 0) return 7;
    if (min(-1, 0) != -1) return 8;

    // max: basic
    if (max(3, 5) != 5) return 10;
    if (max(5, 3) != 5) return 11;
    if (max(4, 4) != 4) return 12;

    // max: negative
    if (max(-1, 1) != 1) return 13;
    if (max(-10, -5) != -5) return 14;

    // max: zero
    if (max(0, 0) != 0) return 15;
    if (max(0, 1) != 1) return 16;
    if (max(-1, 0) != 0) return 17;

    // min/max: boundary values
    if (min(32767, -32768) != -32768) return 20;
    if (max(32767, -32768) != 32767) return 21;

    return 0;
}
