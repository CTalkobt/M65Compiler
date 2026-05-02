// Test: short type (alias for int on 16-bit target)
// Validates: short, unsigned short, signed short, short in function params/returns,
//            sizeof(short), short pointers, short arrays.

volatile char *r = (char *)0x4000;

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
    short arr[3] = {100, 200, 300};
    short *p = &x;

    r[0] = z;              // 30 = 0x1E
    r[1] = neg + 10;       // 5
    r[2] = sizeof(short);  // 2
    r[3] = mul_short(3, 4); // 12
    r[4] = *p;             // 10 = 0x0A
    r[5] = arr[1];         // 200 = 0xC8
    r[6] = 0xAA;           // marker

}
