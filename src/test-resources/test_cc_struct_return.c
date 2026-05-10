// Test: Struct returns across calling conventions
// Runtime assertions validate struct return correctness

#include <assert.h>

char *results = 0x4000;

struct Point {
    char x;
    char y;
};

struct Rect {
    int left;
    int top;
};

// Stack-convention struct return
struct Point stack_get_point(char px, char py) {
    struct Point p;
    p.x = px;
    p.y = py;
    return p;
}

// ZP-convention struct return
__fastcall__ struct Rect zp_get_rect(int l, int t) {
    struct Rect r;
    r.left = l;
    r.top = t;
    return r;
}

// Stack caller invoking both conventions
void main() {
    // Test 1: Stack-convention struct return
    struct Point p = stack_get_point(10, 20);
    assert(p.x == 10);
    assert(p.y == 20);
    results[0] = p.x;  // Should be 10
    results[1] = p.y;  // Should be 20

    // Test 2: ZP-convention struct return (called from stack code)
    struct Rect r = zp_get_rect(256, 512);
    assert(r.left == 256);
    assert(r.top == 512);
    results[2] = (char)(r.left & 0xFF);    // Should be 0 (lo byte of 256)
    results[3] = (char)((r.left >> 8) & 0xFF);  // Should be 1 (hi byte of 256)
    results[4] = (char)(r.top & 0xFF);     // Should be 0 (lo byte of 512)
    results[5] = (char)((r.top >> 8) & 0xFF);   // Should be 2 (hi byte of 512)

    // Sentinel
    results[6] = 0xFF;

    return 0;  // Success
}
