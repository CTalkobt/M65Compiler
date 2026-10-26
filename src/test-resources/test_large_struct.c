// Test: a 10-byte struct filled by memset, copied by memcpy and by assignment
// Returns 0 on success, non-zero on failure.

#include <string.h>

struct Large {
    int m1;
    int m2;
    int m3;
    int m4;
    int m5;
};

int main(void) {
    struct Large a;
    struct Large b;

    memset(&a, 65, 10);
    if (a.m1 != 0x4141 || a.m5 != 0x4141) return 1;

    memcpy(&b, &a, 10);
    if (b.m3 != 0x4141) return 2;

    b.m5 = 7;
    a = b;
    if (a.m5 != 7 || a.m1 != 0x4141) return 3;
    return 0;
}
