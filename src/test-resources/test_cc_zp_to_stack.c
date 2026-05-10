// Test: ZP-convention caller → Stack-convention callee (FORBIDDEN)
// This should FAIL at link time because ZP callers cannot pass stack params

// Stack-convention function (default)
int stack_add(int a, int b) {
    return a + b;
}

// ZP-convention caller trying to call stack-convention function
__fastcall__ int zp_caller(int x) {
    // This call should fail at link time
    // ZP caller has no mechanism to push stack parameters
    return stack_add(x, 10);
}

void main() {
    __asm__("brk");
}
