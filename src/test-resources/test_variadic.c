// Test variadic functions — returns 0 on success
#include <stdarg.h>

// Sum N integers
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

// Find maximum of N integers
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

// Variadic with multiple named params (char* + int before ...)
int count_above(char *label, int threshold, int count, ...) {
    va_list ap;
    va_start(ap, count);
    int n = 0;
    int i;
    for (i = 0; i < count; i++) {
        int v = va_arg(ap, int);
        if (v > threshold) n++;
    }
    va_end(ap);
    return n;
}

// Zero variadic args — just uses the named params
int no_varargs(int a, ...) {
    return a * 2;
}

// Prototype-only variadic declaration (compile test)
int proto_variadic(int a, int b, ...);

int main(void) {
    int fail = 0;

    // Test 1: sum of 3 values
    if (sum(3, 10, 20, 30) != 60) fail++;

    // Test 2: sum of 1 value
    if (sum(1, 42) != 42) fail++;

    // Test 3: sum of 0 values
    if (sum(0) != 0) fail++;

    // Test 4: sum of 5 values
    if (sum(5, 1, 2, 3, 4, 5) != 15) fail++;

    // Test 5: max_of
    if (max_of(4, 5, 99, 3, 42) != 99) fail++;

    // Test 6: max_of single
    if (max_of(1, 77) != 77) fail++;

    // Test 7: count_above with multiple named params
    if (count_above("test", 50, 4, 10, 60, 30, 90) != 2) fail++;

    // Test 8: no variadic args consumed
    if (no_varargs(7) != 14) fail++;

    // Test 9: large values (> 255)
    if (sum(2, 300, 400) != 700) fail++;

    // Test 10: sum result used in expression
    int r = sum(2, 100, 200) + 5;
    if (r != 305) fail++;

    return fail;
}
