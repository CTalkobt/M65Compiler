// Test case for MathLibraryOptimization
// Phase C4.2: Math function constant folding and specialization
//
// Expected behavior:
// - Constant math function calls should be pre-computed at compile time
// - Results replace function calls with literal values
// - Code size reduction for math-heavy computation
// - No behavioral change (results must be identical)

#include <math.h>

// Test 1: Constant sqrt calls
float test_sqrt_constants() {
    // These should be constant-folded at compile time
    float a = sqrtf(4.0f);      // = 2.0f
    float b = sqrtf(9.0f);      // = 3.0f
    float c = sqrtf(16.0f);     // = 4.0f
    return a + b + c;           // = 9.0f
}

// Test 2: Constant trig functions
float test_trig_constants() {
    // Pre-computed at compile time
    float s = sinf(0.0f);       // = 0.0f
    float c = cosf(0.0f);       // = 1.0f
    return s + c;               // = 1.0f
}

// Test 3: Constant fabs calls
float test_fabs_constants() {
    float a = fabsf(-5.0f);     // = 5.0f
    float b = fabsf(3.0f);      // = 3.0f
    return a + b;               // = 8.0f
}

// Test 4: Constant exp/log calls
float test_exp_log_constants() {
    // These might not fold perfectly due to floating point, but should attempt
    float a = expf(0.0f);       // = 1.0f
    float b = logf(1.0f);       // = 0.0f
    float c = log10f(1.0f);     // = 0.0f
    return a + b + c;           // = 1.0f
}

// Test 5: Mixed constant and variable (partial folding)
float test_mixed_constant_variable(float x) {
    float constant_part = sqrtf(4.0f);      // Can fold = 2.0f
    float variable_part = sqrtf(x);         // Cannot fold, depends on x
    return constant_part + variable_part;
}

// Test 6: Loop with constant math (hoisting opportunity)
float test_loop_constant_math() {
    float result = 0.0f;
    float constant_base = sqrtf(2.0f);      // Should fold once
    for (int i = 0; i < 10; i++) {
        result += constant_base;             // Should use folded value
    }
    return result;                           // = sqrt(2) * 10
}

// Test 7: Nested constant math calls
float test_nested_math() {
    // sqrtf(9) = 3, then abs(3) = 3
    float a = fabsf(sqrtf(9.0f));           // = 3.0f
    // sqrtf(16) = 4, then sin(4) ≈ -0.756...
    float b = sinf(sqrtf(16.0f));           // ≈ sin(4)
    return a;
}

// Test 8: Integer arguments to math functions
float test_int_to_float_math() {
    // Integer literals converted to float for math calls
    float a = sqrtf(25);        // 25 → 25.0f → sqrt = 5.0f
    float b = fabsf(-10);       // -10 → -10.0f → abs = 10.0f
    return a + b;               // = 15.0f
}

// Test 9: Math function with compile-time constants
#define PI_SQUARED 9.8696f
float test_macro_math() {
    float a = sqrtf(PI_SQUARED);  // Should fold
    return a;
}

// Test 10: Avoid false optimization (non-constant args)
float test_non_constant_sqrt(float x) {
    // Should NOT be folded - x is not constant
    return sqrtf(x);
}

int main() {
    // Run tests and verify results are reasonable
    float r1 = test_sqrt_constants();
    if (r1 != 9.0f) return -1;

    float r2 = test_trig_constants();
    if (r2 != 1.0f) return -2;

    float r3 = test_fabs_constants();
    if (r3 != 8.0f) return -3;

    float r4 = test_exp_log_constants();
    if (r4 != 1.0f) return -4;

    float r5 = test_mixed_constant_variable(4.0f);
    // = 2.0f (sqrt(4)) + 2.0f (sqrt(4)) = 4.0f
    if (r5 != 4.0f) return -5;

    float r6 = test_loop_constant_math();
    // sqrt(2) ≈ 1.414... * 10 ≈ 14.14...
    // Just verify it's positive and reasonable
    if (r6 <= 0.0f || r6 > 20.0f) return -6;

    float r7 = test_nested_math();
    if (r7 != 3.0f) return -7;

    float r8 = test_int_to_float_math();
    if (r8 != 15.0f) return -8;

    float r9 = test_macro_math();
    // sqrt(9.8696) ≈ 3.14... (approximately pi)
    if (r9 < 3.0f || r9 > 3.2f) return -9;

    // test_non_constant_sqrt just verifies it doesn't crash
    float r10 = test_non_constant_sqrt(16.0f);
    // sqrt(16) = 4.0f
    if (r10 != 4.0f) return -10;

    return 0;  // All tests passed
}
