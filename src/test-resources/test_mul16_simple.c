// Test mul.16 simulated opcode in isolation

volatile unsigned char *output = (unsigned char *)0x4000;

int main() {
    // Test 1: 1 * 2 should give 2
    unsigned char lo = 0, hi = 0;
    asm(
        "lda #1\n"
        "ldx #0\n"
        "mul.16 .AX, #2\n"  // 1 * 2 = 2
        "sta __result_lo\n"
        "stx __result_hi\n"
    );

    // Read the result
    asm(
        "lda __result_lo\n"
        "sta %0\n"
        : "=g"(lo)
    );
    asm(
        "lda __result_hi\n"
        "sta %0\n"
        : "=g"(hi)
    );

    output[0] = lo;   // Should be 2
    output[1] = hi;   // Should be 0

    // Test 2: 100 * 2 should give 200 (0xC8)
    asm(
        "lda #100\n"
        "ldx #0\n"
        "mul.16 .AX, #2\n"  // 100 * 2 = 200
        "sta __result_lo\n"
        "stx __result_hi\n"
    );

    asm("lda __result_lo\n" "sta %0\n" : "=g"(lo));
    asm("lda __result_hi\n" "sta %0\n" : "=g"(hi));

    output[2] = lo;   // Should be 0xC8 (200 low)
    output[3] = hi;   // Should be 0x00 (200 high)

    output[4] = 0xAA; // Test complete marker
    return 0;
}
