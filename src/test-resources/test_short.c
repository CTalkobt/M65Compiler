// Test: short type (alias for int on 16-bit target)
// Validates: short, unsigned short, signed short, short in function params/returns,
//            sizeof(short).
//
// Expected at $C010: 1E 05 02 0C 0A 14 AA

#define RESULT(i) (*(volatile unsigned char *)(0xC010 + (i)))

short add_short(short a, short b) {
    return a + b;
}

unsigned short mul_short(unsigned short a, unsigned short b) {
    return a * b;
}

void main() {
    short x = 10;
    short y = 20;
    unsigned short z = add_short(x, y);
    signed short neg = -5;

    RESULT(0) = z;              // 30 = 0x1E
    RESULT(1) = neg + 10;       // 5
    RESULT(2) = sizeof(short);  // 2
    RESULT(3) = mul_short(3, 4); // 12 = 0x0C
    RESULT(4) = x;              // 10 = 0x0A
    RESULT(5) = y;              // 20 = 0x14
    RESULT(6) = 0xAA;           // marker
}
