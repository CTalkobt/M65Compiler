// Test assert() macro with NDEBUG — assertions disabled
// Compile with -DNDEBUG to disable assertions
// Even false assertions should be no-ops, so program proceeds

#define NDEBUG
#include <assert.h>

void main() {
    // These would normally fail, but NDEBUG disables them
    assert(1 == 0);      // Would fail, but disabled by NDEBUG
    assert(5 < 3);       // Would fail, but disabled by NDEBUG
    assert(0 == 1);      // Would fail, but disabled by NDEBUG

    // Program should reach this even though assertions are false
    __asm__("brk");
}
