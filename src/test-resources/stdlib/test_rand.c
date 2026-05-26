// test_rand: validate rand() returns values in range
// Returns 0 (A=$00) on success, non-zero on failure.

#include <stdlib.h>

int main() {
    int i;
    int val;

    // rand() should return 0-32767 (bit 15 clear)
    for (i = 0; i < 10; i++) {
        val = rand();
        if (val < 0) return 1;
        // On this target int is unsigned 16-bit, so val > 32767
        // means bit 15 is set
        if (val > 32767) return 2;
    }

    // Two successive calls should (almost certainly) differ
    // Not guaranteed, but hardware RNG makes collision extremely unlikely
    val = rand();
    if (val == rand() && val == rand()) return 3;

    return 0;
}
