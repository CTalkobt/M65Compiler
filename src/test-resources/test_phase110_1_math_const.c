// Test Phase 110.1: Math Library Constant Folding
// Tests constant folding of math library functions

#include <math.h>

// Test sqrt constant folding
float test_sqrt() {
    return sqrtf(4.0f);  // Should fold to 2.0f
}

// Test sin/cos constant folding
float test_trig() {
    float s = sinf(0.0f);   // Should fold to 0.0f
    float c = cosf(0.0f);   // Should fold to 1.0f
    return s + c;
}

// Test fabs constant folding
float test_fabs() {
    return fabsf(-3.14f);   // Should fold to 3.14f
}

// Test exp constant folding
float test_exp() {
    return expf(0.0f);      // Should fold to 1.0f
}

// Test log constant folding
float test_log() {
    return logf(1.0f);      // Should fold to 0.0f
}

// Mixed test with constant and variable args
float test_mixed(float x) {
    float const_sqrt = sqrtf(9.0f);  // Should fold to 3.0f
    float var_sqrt = sqrtf(x);       // Cannot fold
    return const_sqrt + var_sqrt;
}

// Test domain checks (sqrt of negative should not fold)
float test_domain() {
    // This should NOT be folded (would be NaN at runtime)
    // float bad = sqrtf(-1.0f);

    // These should fold normally
    float good = sqrtf(16.0f);  // 4.0f
    return good;
}

int main() {
    float result = 0.0f;

    result += test_sqrt();   // +2.0
    result += test_trig();   // +1.0 (0+1)
    result += test_fabs();   // +3.14
    result += test_exp();    // +1.0
    result += test_log();    // +0.0
    result += test_mixed(4.0f);  // 3.0 + 2.0 = 5.0
    result += test_domain(); // +4.0

    return (int)result;  // Should be ~15
}
