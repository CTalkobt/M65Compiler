#include <stdbool.h>

// Helper to defeat constant folding — returns its argument at runtime
int opaque(int val) {
    return val;
}

int main() {
    // 1. Basic _Bool declaration and initialization
    _Bool a = 1;
    if (a != 1) return 1;

    _Bool b = 0;
    if (b != 0) return 2;

    // 2. Non-zero values normalize to 1
    _Bool c = 42;
    if (c != 1) return 3;

    _Bool d = 255;
    if (d != 1) return 4;

    // 3. stdbool.h macros
    bool e = true;
    if (e != 1) return 5;

    bool f = false;
    if (f != 0) return 6;

    // 4. Assignment normalization from int
    int x = 100;
    _Bool g = x;
    if (g != 1) return 7;

    x = 0;
    _Bool h = x;
    if (h != 0) return 8;

    // 5. sizeof(_Bool) == 1
    if (sizeof(_Bool) != 1) return 9;
    if (sizeof(bool) != 1) return 10;

    // 6. _Bool in expressions (promotes to int)
    _Bool t = 1;
    _Bool u = 1;
    int sum = t + u;
    if (sum != 2) return 11;

    // 7. Reassignment normalization
    _Bool v = 0;
    v = 7;
    if (v != 1) return 12;
    v = 0;
    if (v != 0) return 13;

    // 8. Bool from comparison result (already 0/1)
    _Bool cmp = (5 > 3);
    if (cmp != 1) return 14;
    cmp = (3 > 5);
    if (cmp != 0) return 15;

    // 9. Explicit cast to _Bool
    int big = 512;
    _Bool cb = (_Bool)big;
    if (cb != 1) return 16;
    cb = (_Bool)0;
    if (cb != 0) return 17;

    // 10. Bool in control flow
    _Bool flag = 1;
    int r = 0;
    if (flag) r = 1;
    if (r != 1) return 18;

    flag = 0;
    if (flag) return 19;

    // 11. Logical operators produce valid bool values
    _Bool land = (1 && 1);
    if (land != 1) return 20;
    _Bool lor = (0 || 0);
    if (lor != 0) return 21;
    _Bool lnot = !0;
    if (lnot != 1) return 22;

    // 12. Runtime normalization (defeats constant folding via function call)
    _Bool rb = opaque(42);
    if (rb != 1) return 23;

    rb = opaque(0);
    if (rb != 0) return 24;

    // 13. Runtime cast normalization
    int rv = opaque(999);
    _Bool rc = (_Bool)rv;
    if (rc != 1) return 25;

    rv = opaque(0);
    rc = (_Bool)rv;
    if (rc != 0) return 26;

    // 14. Runtime reassignment normalization
    _Bool rd = 0;
    rd = opaque(200);
    if (rd != 1) return 27;
    rd = opaque(0);
    if (rd != 0) return 28;

    return 0; // PASS
}
