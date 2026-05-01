// Test: malloc/free/calloc/realloc declarations compile.
// Verifies stdlib.h with size_t and heap function prototypes.
#include <stdlib.h>
#include <stddef.h>

void main() {
    // malloc
    int *p = (int *)malloc(16);
    if (p != NULL) {
        // realloc
        int *p2 = (int *)realloc(p, 32);
        if (p2 != NULL) {
            free(p2);
        } else {
            free(p);
        }
    }

    // calloc
    int *arr = (int *)calloc(4, sizeof(int));
    if (arr != NULL) {
        free(arr);
    }

    __asm__("brk");
}
