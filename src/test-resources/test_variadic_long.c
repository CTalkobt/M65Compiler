#include <stdarg.h>

// Sum N longs
long sum_longs(int count, ...) {
    va_list ap;
    va_start(ap, count);
    long total = 0;
    int i;
    for (i = 0; i < count; i++) {
        total = total + va_arg(ap, long);
    }
    va_end(ap);
    return total;
}

int main(void) {
    long a = 100000L;
    long b = 200000L;
    long c = 300000L;

    // Test 1: Sum of constants
    if (sum_longs(3, 10L, 20L, 30L) != 60L) return 1;

    // Test 2: Sum of variables (potential folding candidate)
    if (sum_longs(2, a, b) != 300000L) return 2;

    // Test 3: Mixed small and large longs
    if (sum_longs(2, 5L, 400000L) != 400005L) return 3;

    // Test 4: Small values that look like ints but are typed long
    long x = 5L;
    long y = 10L;
    if (sum_longs(2, x, y) != 15L) return 4;

    return 0; // PASS
}
