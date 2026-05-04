// Test mixed calling convention: zpCall function calling variadic (stack-push)
// Compiled with -fzpcall. Variadic callees use stack convention automatically.
// Results written to $4000+ for mmemu verification.

#include <stdarg.h>

char *results = (char *)0x4000;

// Variadic function — uses stack-push convention even in zpCallMode
int sum(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int total = 0;
    int i;
    for (i = 0; i < count; i++) {
        total = total + va_arg(ap, int);
    }
    va_end(ap);
    return total;
}

// Non-variadic zpCall helper
int double_it(int x) {
    return x + x;
}

// zpCall function that calls variadic — params must survive across the call
int call_variadic_with_live_params(int a, int b) {
    int s = sum(3, 10, 20, 30);  // variadic call clobbers ZP
    return a + b + s;             // a and b must be restored from frame
}

// zpCall function that mixes variadic and zpCall calls
int mixed_calls(int x) {
    int d = double_it(x);          // zpCall
    int s = sum(2, d, 100);        // variadic (stack-push)
    return s;
}

// Variadic with multiple named params
int max_of(int count, ...) {
    va_list ap;
    va_start(ap, count);
    int best = va_arg(ap, int);
    int i;
    for (i = 1; i < count; i++) {
        int v = va_arg(ap, int);
        if (v > best) best = v;
    }
    va_end(ap);
    return best;
}

void main() {
    // Test 1: direct variadic call from zpCall context
    // sum(3, 10, 20, 30) = 60 = $3C
    int r1 = sum(3, 10, 20, 30);
    results[0] = (char)(r1 & 0xFF);
    results[1] = (char)((r1 >> 8) & 0xFF);

    // Test 2: zpCall function that calls variadic with live params
    // call_variadic_with_live_params(5, 7) = 5 + 7 + 60 = 72 = $48
    int r2 = call_variadic_with_live_params(5, 7);
    results[2] = (char)(r2 & 0xFF);
    results[3] = (char)((r2 >> 8) & 0xFF);

    // Test 3: mixed zpCall + variadic calls
    // mixed_calls(25) = sum(2, double_it(25), 100) = sum(2, 50, 100) = 150 = $96
    int r3 = mixed_calls(25);
    results[4] = (char)(r3 & 0xFF);
    results[5] = (char)((r3 >> 8) & 0xFF);

    // Test 4: max_of variadic
    // max_of(4, 5, 99, 3, 42) = 99 = $63
    int r4 = max_of(4, 5, 99, 3, 42);
    results[6] = (char)(r4 & 0xFF);

    // Test 5: variadic result used in zpCall expression
    // double_it(sum(2, 15, 25)) = double_it(40) = 80 = $50
    int r5 = double_it(sum(2, 15, 25));
    results[7] = (char)(r5 & 0xFF);

    // Sentinel
    results[8] = 0xAA;

    __asm__("brk");
}
