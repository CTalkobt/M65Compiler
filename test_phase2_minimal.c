// Minimal test case for Phase 2 offset bug investigation
// Tests variable access before and after function call

int helper(int x) {
    return x + 1;
}

int test_offset() {
    int a = 10;        // Allocate variable 'a'
    int b = helper(a); // Call function (parameter access before)
    return b + a;      // Access 'a' AFTER call (offset bug should occur here)
}
