/**
 * Phase 90: Frame Pointer Optimization Tests
 *
 * Tests lazy FP initialization, leaf function detection, and smart FP recalculation
 * Expected savings: 10-25% code size for typical programs
 */

// Test 1: Leaf function - no locals, no calls
// Should NOT emit frame pointer setup
int leaf_double(int x) {
    return x * 2;
}

// Test 2: Leaf function with multiple operations
// Should NOT emit frame pointer setup
int leaf_triple_add(int a, int b, int c) {
    return a + b + c;
}

// Test 3: Function with locals - should setup FP once, not recalculate
int func_with_locals(int param) {
    int local1 = param + 1;
    int local2 = local1 * 2;
    return local2;
}

// Test 4: Leaf function returning zero
// Should NOT emit frame pointer setup
void leaf_void(void) {
    // Empty body - pure leaf
}

// Test 5: Function making a single zero-arg call
// Should setup FP only if has locals, not recalc after zero-arg call
void call_zero_arg_helper(void) {
    leaf_void();  // Zero-arg call - no FP recalc needed
}

// Test 6: Callback pattern - zero-arg calls to multiple functions
// Should NOT setup FP, NOT recalc after zero-arg calls
void event_handler(void) {
    leaf_void();
    leaf_void();
    leaf_void();
}

// Test 7: Function with locals and zero-arg calls
// Should setup FP once, NOT recalc after zero-arg calls
int func_with_locals_and_calls(void) {
    int result = 42;
    leaf_void();      // Zero-arg - no FP recalc
    leaf_void();      // Zero-arg - no FP recalc
    return result;
}

// Test 8: Function making calls with parameters
// Would need FP setup if it accesses stack params or locals
// Should recalc FP after such calls
int func_with_param_calls(int x) {
    int sum = x;
    sum += leaf_double(sum);  // Has parameter - may need recalc
    return sum;
}

// Test 9: Non-recursive function with conditional (still a leaf - no other calls)
// Should NOT emit frame pointer setup
int leaf_abs(int n) {
    if (n < 0) return -n;
    return n;
}

// Test 10: Empty function (ultimate leaf)
// Should emit nothing but return instruction
void empty_function(void) {
}

// Test 11: Multiple parameters, no locals (pure parameter access)
// This is tricky - technically could be a leaf if no local variables
int sum_five(int a, int b, int c, int d, int e) {
    return a + b + c + d + e;
}

// Test 12: Function with loop calling zero-arg function
// FP setup once, multiple zero-arg calls with no recalc
void loop_event_calls(int count) {
    for (int i = 0; i < count; i++) {
        leaf_void();  // Zero-arg in loop
    }
}

int main(void) {
    // Exercise test functions
    int r1 = leaf_double(5);
    int r2 = leaf_triple_add(1, 2, 3);
    int r3 = func_with_locals(10);

    leaf_void();
    call_zero_arg_helper();
    event_handler();

    int r4 = func_with_locals_and_calls();
    int r5 = func_with_param_calls(7);
    int r6 = leaf_abs(-5);
    int r7 = sum_five(1, 2, 3, 4, 5);

    empty_function();
    loop_event_calls(3);

    return r1 + r2 + r3 + r4 + r5 + r6 + r7;
}
