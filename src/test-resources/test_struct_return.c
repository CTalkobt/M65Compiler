// Test: Return struct by value
// Validates the hidden-pointer ABI for struct return.
//
// Expected at $C010: 01 02 03 04 0A 14 AA

#define RESULT(i) (*(volatile unsigned char *)(0xC010 + (i)))

struct Point {
    int x;
    int y;
};

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;
}

int sum_point(struct Point *p) {
    return p->x + p->y;
}

void main() {
    // Test 1: Basic struct return and member access
    struct Point p1 = make_point(1, 2);
    RESULT(0) = p1.x;  // 1
    RESULT(1) = p1.y;  // 2

    // Test 2: Different values
    struct Point p2 = make_point(3, 4);
    RESULT(2) = p2.x;  // 3
    RESULT(3) = p2.y;  // 4

    // Test 3: Use returned struct in computation
    struct Point p3 = make_point(10, 20);
    RESULT(4) = p3.x;  // 10 = 0x0A
    RESULT(5) = p3.y;  // 20 = 0x14

    // Success marker
    RESULT(6) = 0xAA;
}
