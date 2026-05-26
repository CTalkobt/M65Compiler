// test_gcd_lcm: validate gcd() and lcm()
// Returns 0 (A=$00) on success, non-zero on failure.

#include <math.h>

int main() {
    if (gcd(12, 8) != 4) return 1;
    if (gcd(7, 13) != 1) return 2;
    if (gcd(5, 0) != 5) return 3;
    if (gcd(0, 5) != 5) return 4;
    if (gcd(0, 0) != 0) return 5;
    if (gcd(-12, 8) != 4) return 6;
    if (gcd(15, 5) != 5) return 7;

    if (lcm(4, 6) != 12) return 10;
    if (lcm(3, 7) != 21) return 11;
    if (lcm(0, 5) != 0) return 12;
    if (lcm(6, 6) != 6) return 13;
    if (lcm(8, 15) != 120) return 14;

    return 0;
}
