// Example: gcd/lcm — validate integer gcd/lcm functions
// Returns 0 (A=$00) on success, non-zero on failure.

#include <math.h>

int main() {
    // gcd: basic
    if (gcd(12, 8) != 4) return 1;
    if (gcd(8, 12) != 4) return 2;
    if (gcd(7, 13) != 1) return 3;
    if (gcd(100, 75) != 25) return 4;

    // gcd: identity / zero
    if (gcd(5, 0) != 5) return 5;
    if (gcd(0, 5) != 5) return 6;
    if (gcd(0, 0) != 0) return 7;

    // gcd: equal values
    if (gcd(6, 6) != 6) return 8;

    // gcd: one is multiple of other
    if (gcd(15, 5) != 5) return 9;
    if (gcd(5, 15) != 5) return 10;

    // gcd: negative inputs
    if (gcd(-12, 8) != 4) return 11;
    if (gcd(12, -8) != 4) return 12;
    if (gcd(-12, -8) != 4) return 13;

    // lcm: basic
    if (lcm(4, 6) != 12) return 20;
    if (lcm(6, 4) != 12) return 21;
    if (lcm(3, 7) != 21) return 22;

    // lcm: zero
    if (lcm(0, 5) != 0) return 23;
    if (lcm(5, 0) != 0) return 24;
    if (lcm(0, 0) != 0) return 25;

    // lcm: equal values
    if (lcm(6, 6) != 6) return 26;

    // lcm: one divides the other
    if (lcm(3, 9) != 9) return 27;
    if (lcm(9, 3) != 9) return 28;

    // lcm: coprime
    if (lcm(8, 15) != 120) return 29;

    return 0;
}
