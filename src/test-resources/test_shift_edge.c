// Test 32-bit shift edge cases and byte-shuffle optimizations

long shr(long v, int n) {
    // Variable-amount shift (loop path)
    if (n == 0) return v >> 0;
    if (n == 1) return v >> 1;
    if (n == 8) return v >> 8;
    if (n == 16) return v >> 16;
    if (n == 24) return v >> 24;
    if (n == 31) return v >> 31;
    return v >> n;
}

long shl(long v, int n) {
    if (n == 0) return v << 0;
    if (n == 1) return v << 1;
    if (n == 8) return v << 8;
    if (n == 16) return v << 16;
    if (n == 24) return v << 24;
    return v << n;
}

// 16-bit shift edge cases
int shr16(int v, int n) {
    if (n == 0) return v >> 0;
    if (n == 8) return v >> 8;
    return v >> n;
}

int main() {
    long v = 256;    // 0x00000100

    // --- 32-bit right shift ---

    // >> 0: identity
    if (shr(v, 0) != 256) return 1;

    // >> 1: 256 >> 1 = 128
    if (shr(v, 1) != 128) return 2;

    // >> 8: byte shuffle, 0x100 >> 8 = 1
    if (shr(v, 8) != 1) return 3;

    // >> 16: two-byte shuffle
    // Need value with byte 2 set: build via shift
    long big = 1;
    big = big << 16;   // 0x00010000
    if (shr(big, 16) != 1) return 4;

    // >> 24: three-byte shuffle
    long huge = 1;
    huge = huge << 24;  // 0x01000000
    if (shr(huge, 24) != 1) return 5;

    // >> 31: only sign/top bit survives
    long top = 1;
    top = top << 31;    // 0x80000000
    if (shr(top, 31) != 1) return 6;

    // --- 32-bit left shift ---

    // << 0: identity
    if (shl(1, 0) != 1) return 7;

    // << 1: double
    if (shl(1, 1) != 2) return 8;

    // << 8: byte shuffle up
    if (shl(1, 8) != 256) return 9;

    // << 16: two-byte shuffle
    long s16 = shl(1, 16);  // 0x00010000 = 65536
    if (shr(s16, 16) != 1) return 10;  // verify round-trip

    // << 24: three-byte shuffle
    long s24 = shl(1, 24);  // 0x01000000
    if (shr(s24, 24) != 1) return 11;  // verify round-trip

    // --- 16-bit shift edge cases ---

    // >> 0: identity
    if (shr16(256, 0) != 256) return 12;

    // >> 8: high byte to low byte
    if (shr16(256, 8) != 1) return 13;

    // General: 0x1234 >> 4 = 0x0123
    if (shr16(0x1234, 4) != 0x0123) return 14;

    return 0;
}
