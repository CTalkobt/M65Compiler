// Extended Phase 110.1: Math Library Constant Folding Tests
// Comprehensive coverage of math function constants and special values

#include <math.h>

// =============================================================================
// TRIGONOMETRIC SPECIAL VALUES
// =============================================================================

// sin() special values
float test_sin_zero() {
    return sinf(0.0f);  // Should fold to 0.0
}

float test_sin_pi_2() {
    return sinf(M_PI / 2.0f);  // Should fold to 1.0
}

float test_sin_pi() {
    return sinf((float)M_PI);  // Should fold to ~0.0 (very small)
}

float test_sin_3pi_2() {
    return sinf(3.0f * M_PI / 2.0f);  // Should fold to -1.0
}

// cos() special values
float test_cos_zero() {
    return cosf(0.0f);  // Should fold to 1.0
}

float test_cos_pi_2() {
    return cosf(M_PI / 2.0f);  // Should fold to ~0.0
}

float test_cos_pi() {
    return cosf((float)M_PI);  // Should fold to -1.0
}

float test_cos_3pi_2() {
    return cosf(3.0f * M_PI / 2.0f);  // Should fold to ~0.0
}

float test_cos_2pi() {
    return cosf(2.0f * M_PI);  // Should fold to 1.0
}

// tan() special values
float test_tan_zero() {
    return tanf(0.0f);  // Should fold to 0.0
}

float test_tan_pi_4() {
    return tanf(M_PI / 4.0f);  // Should fold to 1.0
}

float test_tan_pi() {
    return tanf((float)M_PI);  // Should fold to ~0.0
}

// =============================================================================
// EXPONENTIAL AND LOGARITHMIC SPECIAL VALUES
// =============================================================================

// exp() special values
float test_exp_zero() {
    return expf(0.0f);  // Should fold to 1.0
}

float test_exp_one() {
    return expf(1.0f);  // Should fold to e ≈ 2.71828
}

float test_exp_negative_one() {
    return expf(-1.0f);  // Should fold to 1/e ≈ 0.36788
}

// log() special values
float test_log_one() {
    return logf(1.0f);  // Should fold to 0.0
}

float test_log_e() {
    return logf((float)M_E);  // Should fold to 1.0
}

// log10() special values
float test_log10_one() {
    return log10f(1.0f);  // Should fold to 0.0
}

float test_log10_ten() {
    return log10f(10.0f);  // Should fold to 1.0
}

float test_log10_hundred() {
    return log10f(100.0f);  // Should fold to 2.0
}

float test_log10_thousand() {
    return log10f(1000.0f);  // Should fold to 3.0
}

// =============================================================================
// SQUARE ROOT SPECIAL VALUES
// =============================================================================

float test_sqrt_zero() {
    return sqrtf(0.0f);  // Should fold to 0.0
}

float test_sqrt_one() {
    return sqrtf(1.0f);  // Should fold to 1.0
}

float test_sqrt_four() {
    return sqrtf(4.0f);  // Should fold to 2.0
}

float test_sqrt_nine() {
    return sqrtf(9.0f);  // Should fold to 3.0
}

float test_sqrt_sixteen() {
    return sqrtf(16.0f);  // Should fold to 4.0
}

float test_sqrt_twentyfive() {
    return sqrtf(25.0f);  // Should fold to 5.0
}

float test_sqrt_hundred() {
    return sqrtf(100.0f);  // Should fold to 10.0
}

// Perfect squares
float test_sqrt_144() {
    return sqrtf(144.0f);  // Should fold to 12.0
}

// Non-perfect squares (should still fold)
float test_sqrt_2() {
    return sqrtf(2.0f);  // Should fold to ~1.41421
}

float test_sqrt_0_5() {
    return sqrtf(0.5f);  // Should fold to ~0.70711
}

// =============================================================================
// ABSOLUTE VALUE SPECIAL VALUES
// =============================================================================

float test_fabs_zero() {
    return fabsf(0.0f);  // Should fold to 0.0
}

float test_fabs_positive() {
    return fabsf(3.14159f);  // Should fold to 3.14159
}

float test_fabs_negative() {
    return fabsf(-2.71828f);  // Should fold to 2.71828
}

float test_fabs_small_positive() {
    return fabsf(0.00001f);  // Should fold to 0.00001
}

float test_fabs_small_negative() {
    return fabsf(-0.00001f);  // Should fold to 0.00001
}

// =============================================================================
// COMBINED OPERATIONS
// =============================================================================

// Chain of constant folding operations
float test_nested_sqrt() {
    // sqrt(sqrt(16)) = sqrt(4) = 2
    return sqrtf(sqrtf(16.0f));
}

// Multiple constant folds in sequence
float test_multi_trig() {
    // sin(0) + cos(0) + sin(π/2)
    return sinf(0.0f) + cosf(0.0f) + sinf(M_PI / 2.0f);  // 0 + 1 + 1 = 2
}

// Mixing different constant folding types
float test_mixed_constants() {
    float a = sqrtf(4.0f);      // 2.0
    float b = logf(1.0f);       // 0.0
    float c = expf(0.0f);       // 1.0
    float d = cosf(0.0f);       // 1.0
    return a + b + c + d;       // 2 + 0 + 1 + 1 = 4
}

// =============================================================================
// ERROR CASES (should NOT fold)
// =============================================================================

float test_sqrt_negative(void) {
    // Domain error: sqrt(-1) is undefined
    // Should NOT fold
    // Compiler should keep as function call
    float x = -1.0f;
    return sqrtf(x);
}

float test_log_zero(void) {
    // Domain error: log(0) is -∞
    // Should NOT fold
    float x = 0.0f;
    return logf(x);
}

float test_log_negative(void) {
    // Domain error: log(-1) is undefined
    // Should NOT fold
    float x = -1.0f;
    return logf(x);
}

// =============================================================================
// VARIABLE ARGUMENTS (should NOT fold)
// =============================================================================

float test_sin_variable(float x) {
    return sinf(x);  // Should NOT fold (argument not constant)
}

float test_sqrt_variable(float x) {
    return sqrtf(x);  // Should NOT fold (argument not constant)
}

float test_exp_variable(float x) {
    return expf(x);  // Should NOT fold (argument not constant)
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    float total = 0.0f;

    // Trig tests
    total += test_sin_zero();      // 0
    total += test_sin_pi_2();      // 1
    total += test_cos_zero();      // 1
    total += test_cos_pi();        // -1
    total += test_tan_zero();      // 0
    total += test_tan_pi_4();      // 1

    // Exp/Log tests
    total += test_exp_zero();      // 1
    total += test_log_one();       // 0
    total += test_log10_ten();     // 1
    total += test_log10_hundred(); // 2

    // Sqrt tests
    total += test_sqrt_zero();     // 0
    total += test_sqrt_one();      // 1
    total += test_sqrt_four();     // 2
    total += test_sqrt_nine();     // 3
    total += test_sqrt_sixteen();  // 4

    // Abs tests
    total += test_fabs_zero();     // 0
    total += test_fabs_positive(); // 3.14159
    total += test_fabs_negative(); // 2.71828

    // Combined tests
    total += test_multi_trig();    // 2
    total += test_mixed_constants(); // 4

    // Variable argument tests (cannot fold)
    total += test_sqrt_variable(2.0f);  // ~1.41421
    total += test_sin_variable(0.0f);   // 0

    // Expected total: ~48 (if all constants folded correctly)
    return (int)total;
}
