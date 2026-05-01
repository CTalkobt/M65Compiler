// Test: #pragma crt heap compiles and assembles correctly.
// Verifies the heap pragma is recognized and stdlib.h declarations work.
#pragma crt heap
#include <stdlib.h>

void main() {
    int *p = (int *)malloc(10);
    if (p != NULL) {
        *p = 42;
        free(p);
    }
    __asm__("brk");
}
