// Compound literal mmemu test — results at $4000
struct Point {
    int x;
    int y;
};

int sum_point(struct Point *p) {
    return p->x + p->y;
}

volatile unsigned char *res = (unsigned char*)0x4000;

void main() {
    // Test 1: Struct compound literal as variable initializer
    struct Point p = (struct Point){10, 20};
    res[0] = (unsigned char)(p.x + p.y);  // 30 = 0x1E

    // Test 2: Scalar compound literal (int)
    int v = (int){42};
    res[1] = (unsigned char)v;  // 42 = 0x2A

    // Test 3: Scalar compound literal (char)
    char c = (char){7};
    res[2] = c;  // 7 = 0x07

    // Test 4: Struct compound literal passed as pointer argument
    int r = sum_point(&(struct Point){100, 200});
    res[3] = (unsigned char)r;  // 300 & 0xFF = 0x2C
    res[4] = (unsigned char)(r >> 8);  // 300 >> 8 = 0x01

    // Test 5: Struct compound literal with expressions
    int a = 5;
    int b = 15;
    struct Point p2 = (struct Point){a, b};
    res[5] = (unsigned char)(p2.x + p2.y);  // 20 = 0x14

    // Test 6: Compound literal with zero-init struct
    struct Point p3 = (struct Point){0, 0};
    res[6] = (unsigned char)(p3.x + p3.y);  // 0 = 0x00

    __asm__("brk");
}
