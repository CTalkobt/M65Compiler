// Example: ftoa and heapfree (oscar64's names)
// ftoa writes a float as printf's %g does; heapfree is the heap's free
// bytes (cc65's _heapmemavail).
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char s[16];
    unsigned int before;
    void *p;

    ftoa(3.25, s);
    printf("3.25 -> %s\n", s);
    if (strcmp(s, "3.25") != 0) return 1;
    ftoa(-0.001, s);
    if (strcmp(s, "-0.001") != 0) return 2;
    ftoa(123456789.0, s);
    printf("123456789 -> %s\n", s);
    if (strcmp(s, "1.23457e+08") != 0) return 3;

    before = heapfree();
    p = malloc(100);
    printf("heap: %u free, %u after malloc(100)\n", before, heapfree());
    if (!p || heapfree() != before - 102) return 4;
    free(p);
    if (heapfree() != before) return 5;
    return 0;
}
