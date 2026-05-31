// test_division: validate div() and ldiv() - integer division with remainder
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>

int main() {
    // Test 1: Basic div()
    div_t result;
    result = div(10, 3);
    if (result.quot != 3 || result.rem != 1) return 1;

    // Test 2: Exact division
    result = div(12, 3);
    if (result.quot != 4 || result.rem != 0) return 2;

    // Test 3: Small divisor
    result = div(5, 1);
    if (result.quot != 5 || result.rem != 0) return 3;

    // Test 4: Negative numerator
    result = div(-10, 3);
    if (result.quot != -3 || result.rem != -1) return 4;

    // Test 5: Negative denominator
    result = div(10, -3);
    if (result.quot != -3 || result.rem != 1) return 5;

    // Test 6: Both negative
    result = div(-10, -3);
    if (result.quot != 3 || result.rem != -1) return 6;

    // Test 7: Numerator is zero
    result = div(0, 5);
    if (result.quot != 0 || result.rem != 0) return 7;

    // Test 8: Large numbers
    result = div(1000, 7);
    if (result.quot != 142 || result.rem != 6) return 8;

    // Test 9: Numerator < divisor
    result = div(3, 10);
    if (result.quot != 0 || result.rem != 3) return 9;

    // Test 10: Numerator = divisor
    result = div(5, 5);
    if (result.quot != 1 || result.rem != 0) return 10;

    // Test 11: Basic ldiv()
    ldiv_t lresult;
    lresult = ldiv(10, 3);
    if (lresult.quot != 3 || lresult.rem != 1) return 11;

    // Test 12: ldiv() with exact division
    lresult = ldiv(100, 10);
    if (lresult.quot != 10 || lresult.rem != 0) return 12;

    // Test 13: ldiv() with negative numerator
    lresult = ldiv(-100, 7);
    if (lresult.quot != -14 || lresult.rem != -2) return 13;

    // Test 14: ldiv() with negative denominator
    lresult = ldiv(100, -7);
    if (lresult.quot != -14 || lresult.rem != 2) return 14;

    // Test 15: ldiv() both negative
    lresult = ldiv(-100, -7);
    if (lresult.quot != 14 || lresult.rem != -2) return 15;

    // Test 16: ldiv() zero numerator
    lresult = ldiv(0, 50);
    if (lresult.quot != 0 || lresult.rem != 0) return 16;

    // Test 17: ldiv() large numbers
    lresult = ldiv(9999, 13);
    if (lresult.quot != 769 || lresult.rem != 2) return 17;

    // Test 18: div() remainder is always same sign as dividend
    result = div(7, -2);
    if (result.rem != 1) return 18;

    // Test 19: Multiple sequential divisions
    result = div(20, 3);
    if (result.quot != 6 || result.rem != 2) return 19;
    result = div(result.quot, 2);
    if (result.quot != 3 || result.rem != 0) return 20;

    // Test 20: ldiv() same sign behavior
    lresult = ldiv(-15, 4);
    if (lresult.rem != -3) return 21;

    return 0; // All tests passed
}
