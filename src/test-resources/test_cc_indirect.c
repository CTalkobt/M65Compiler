// Test: Indirect calls through function pointers
// Function pointers must use stack convention (callee unknown at compile time)

char *results = 0x4000;

// Stack-convention function
int stack_func(int a, int b) {
    return a * b;
}

// ZP-convention function
__fastcall__ int zp_func(int a, int b) {
    return a + b;
}

// Caller takes function pointer and invokes it
int call_via_ptr(int (*f)(int, int), int x, int y) {
    return f(x, y);  // Indirect call — must use stack convention
}

void main() {
    // Test 1: Call stack function via pointer (normal case)
    int r1 = call_via_ptr(stack_func, 6, 7);
    results[0] = (char)(r1 & 0xFF);      // Should be 42 (0x2A)
    results[1] = (char)((r1 >> 8) & 0xFF);

    // Test 2: Call ZP function via pointer
    // This works because the call is indirect (uses stack convention at call site),
    // but the ZP function was compiled with ZP params — ABI mismatch!
    // The result may be garbage because ZP params aren't set up.
    // This is expected to either fail at link time or produce undefined behavior.
    // We test that this is detected or handled safely.
    int (*fptr)(int, int) = zp_func;
    int r2 = call_via_ptr(fptr, 5, 3);
    results[2] = (char)(r2 & 0xFF);
    results[3] = (char)((r2 >> 8) & 0xFF);

    // Sentinel
    results[4] = 0xFF;

    __asm__("brk");
}
