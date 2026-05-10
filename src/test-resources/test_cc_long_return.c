// Test: 32-bit (long) return values across calling conventions
// Both conventions return longs in AXYZ, so this should work seamlessly

char *results = 0x4000;

// Stack-convention long return
long stack_get_long(int a, int b) {
    return ((long)a << 16) | (unsigned)b;
}

// ZP-convention long return
__fastcall__ long zp_get_long(int a, int b) {
    return ((long)a << 16) | (unsigned)b;
}

// Stack caller invoking both
void main() {
    // Test 1: Stack-convention long return
    long r1 = stack_get_long(0x1234, 0x5678);
    results[0] = (char)(r1 & 0xFF);           // 0x78
    results[1] = (char)((r1 >> 8) & 0xFF);    // 0x56
    results[2] = (char)((r1 >> 16) & 0xFF);   // 0x34
    results[3] = (char)((r1 >> 24) & 0xFF);   // 0x12

    // Test 2: ZP-convention long return
    long r2 = zp_get_long(0xABCD, 0xEF01);
    results[4] = (char)(r2 & 0xFF);           // 0x01
    results[5] = (char)((r2 >> 8) & 0xFF);    // 0xEF
    results[6] = (char)((r2 >> 16) & 0xFF);   // 0xCD
    results[7] = (char)((r2 >> 24) & 0xFF);   // 0xAB

    // Sentinel
    results[8] = 0xFF;

    __asm__("brk");
}
