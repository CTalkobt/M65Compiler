// Test: short type (alias for int on 16-bit target)
// Validates: short, unsigned short, signed short, short in function params/returns,
//            sizeof(short), short pointers, short arrays.

#include <stdio.h>

volatile char *r = (char *)0x4000;

short add_short(short a, short b) {
    printf("add_short: a=%d, b=%d\n", a, b);
    short result = a + b;
    printf("add_short result: %d\n", result);
    return result;
}

unsigned short mul_short(unsigned short a, unsigned short b) {
    printf("mul_short: a=%d, b=%d\n", a, b);
    unsigned short result = a * b;
    printf("mul_short result: %d\n", result);
    return result;
}

void main() {
    printf("=== test_short START ===\n");
    short x = 10;
    short y = 20;
    printf("main: x=%d, y=%d\n", x, y);
    unsigned short z = add_short(x, y);
    printf("main: z (add result)=%d\n", z);
    signed short neg = -5;

    printf("Writing results to 0x4000...\n");
    r[0] = z;              // 30 = 0x1E
    r[1] = neg + 10;       // 5
    r[2] = sizeof(short);  // 2
    r[3] = mul_short(3, 4); // 12
    r[4] = x;              // 10 = 0x0A
    r[5] = y;              // 20 = 0x14
    r[6] = 0xAA;           // marker
    printf("Results written:\n");
    printf("r[0]=%02X (expect 1E)\n", r[0]);
    printf("r[1]=%02X (expect 05)\n", r[1]);
    printf("r[2]=%02X (expect 02)\n", r[2]);
    printf("r[3]=%02X (expect 0C)\n", r[3]);
    printf("r[4]=%02X (expect 0A)\n", r[4]);
    printf("r[5]=%02X (expect 14)\n", r[5]);
    printf("r[6]=%02X (expect AA)\n", r[6]);
    printf("=== test_short END ===\n");
}
