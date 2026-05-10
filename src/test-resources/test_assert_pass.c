// Test assert() macro — assertion passes
// Should compile and execute without halting

#include <assert.h>

void main() {
    // Basic assertions that should pass
    assert(1 == 1);
    assert(5 > 3);
    assert(0 == 0);
    assert(10 >= 10);

    // Expressions with variables
    int x = 42;
    assert(x == 42);
    assert(x > 0);
    assert(x < 100);

    // If we reach here, all assertions passed
    __asm__("brk");
}
