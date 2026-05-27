// Test: #pragma cc45 heap compiles and assembles correctly.
// Verifies the heap pragma is recognized and stdlib.h declarations work.
#pragma cc45 heap
#include <stdlib.h>

void main() {
    int *p = (int *)malloc(10);
    if (p != NULL) {
        *p = 42;
        free(p);
    }
    __asm__("brk");
}
