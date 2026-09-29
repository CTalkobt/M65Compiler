// Test Phase 110.6: Bit Manipulation Strength Reduction
// Tests optimization of bit operations with special instructions

// =============================================================================
// SINGLE BIT OPERATIONS (TSB/TRB OPTIMIZATION)
// =============================================================================

// Test single bit extraction from LSB
int test_single_bit_lsb(int x) {
    // Pattern: x & 0x01 (test bit 0)
    // Optimized: TSB instruction
    return x & 0x01;
}

// Test single bit extraction from bit 4
int test_single_bit_4(int x) {
    // Pattern: x & 0x10 (test bit 4)
    // Optimized: TSB instruction
    return x & 0x10;
}

// Test single bit extraction from MSB
int test_single_bit_msb(int x) {
    // Pattern: x & 0x80 (test bit 7)
    // Optimized: TSB instruction
    return x & 0x80;
}

// Test single bit set operation
int test_single_bit_set(int x) {
    // Pattern: x | 0x02 (set bit 1)
    // Optimized: TSB set
    return x | 0x02;
}

// Test single bit clear operation
int test_single_bit_clear(int x) {
    // Pattern: x & ~0x04 (clear bit 2)
    // Optimized: TRB instruction
    return x & ~0x04;
}

// =============================================================================
// MULTI-BIT MASKS (IDENTITY OPTIMIZATION)
// =============================================================================

// Test identity AND with 0xFF
int test_identity_and_full(int x) {
    // Pattern: x & 0xFF (no-op for 8-bit)
    // Optimized: NOP / remove operation
    return x & 0xFF;
}

// Test identity AND within range
int test_identity_and_nibble(int x) {
    // Pattern: x & 0x0F (mask lower nibble)
    // Optimized: CMP/BIT check
    return x & 0x0F;
}

// Test 2-bit mask
int test_mask_2bit(int x) {
    // Pattern: x & 0x03 (mask lower 2 bits)
    // Optimized: BIT instruction
    return x & 0x03;
}

// =============================================================================
// XOR OPERATIONS
// =============================================================================

// Test XOR identity (no-op)
int test_xor_identity(int x) {
    // Pattern: x ^ 0x00 (no change)
    // Optimized: NOP / remove
    return x ^ 0x00;
}

// Test XOR bitwise NOT
int test_xor_not(int x) {
    // Pattern: x ^ 0xFF (bitwise NOT)
    // Optimized: EOR #0xFF
    return x ^ 0xFF;
}

// Test XOR toggle bits
int test_xor_toggle(int x) {
    // Pattern: x ^ 0x55 (toggle alternate bits)
    return x ^ 0x55;
}

// =============================================================================
// SHIFT OPERATIONS (STRENGTH REDUCTION)
// =============================================================================

// Test shift left by 1 (multiply by 2)
int test_shift_left_1(int x) {
    // Pattern: x << 1 (multiply by 2)
    // Optimized: ASL instruction (single byte)
    return x << 1;
}

// Test shift left by 2 (multiply by 4)
int test_shift_left_2(int x) {
    // Pattern: x << 2 (multiply by 4)
    // Optimized: ASL ASL (two bytes)
    return x << 2;
}

// Test shift left by 3 (multiply by 8)
int test_shift_left_3(int x) {
    // Pattern: x << 3 (multiply by 8)
    // Optimized: ASL ASL ASL (three bytes)
    return x << 3;
}

// Test shift right by 1 (divide by 2)
int test_shift_right_1(int x) {
    // Pattern: x >> 1 (divide by 2)
    // Optimized: LSR instruction (single byte)
    return x >> 1;
}

// Test shift right by 2 (divide by 4)
int test_shift_right_2(int x) {
    // Pattern: x >> 2 (divide by 4)
    // Optimized: LSR LSR (two bytes)
    return x >> 2;
}

// Test shift right by 4 (divide by 16)
int test_shift_right_4(int x) {
    // Pattern: x >> 4 (divide by 16)
    // Optimized: LSR LSR LSR LSR (four bytes) or swap nybbles + mask
    return x >> 4;
}

// =============================================================================
// BIT FIELD OPERATIONS
// =============================================================================

// Test extract low nibble
int test_extract_low_nibble(int x) {
    // Pattern: x & 0x0F (extract bits 0-3)
    return x & 0x0F;
}

// Test extract high nibble
int test_extract_high_nibble(int x) {
    // Pattern: (x >> 4) & 0x0F
    return (x >> 4) & 0x0F;
}

// Test bit range extraction
int test_extract_bits_2_5(int x) {
    // Pattern: (x >> 2) & 0x0F (extract bits 2-5)
    return (x >> 2) & 0x0F;
}

// Test bit range insertion
int test_insert_bits(int x, int bits) {
    // Pattern: (x & ~0x1C) | (bits << 2)
    // Clear bits 2-4, then set them from bits
    return (x & ~0x1C) | ((bits & 0x07) << 2);
}

// =============================================================================
// COMPLEX BIT PATTERNS
// =============================================================================

// Test multiple bit operations chained
int test_chain_operations(int x) {
    // Pattern: ((x >> 1) & 0x0F) | 0x80
    // Shift, mask, then set MSB
    return ((x >> 1) & 0x0F) | 0x80;
}

// Test bit swap (endianness conversion)
int test_swap_nibbles(int x) {
    // Pattern: ((x & 0x0F) << 4) | ((x >> 4) & 0x0F)
    // Swap low and high nibbles
    return ((x & 0x0F) << 4) | ((x >> 4) & 0x0F);
}

// Test bit reversal (simple 8-bit)
int test_reverse_bits_8(int x) {
    int result = 0;
    if (x & 0x01) result |= 0x80;
    if (x & 0x02) result |= 0x40;
    if (x & 0x04) result |= 0x20;
    if (x & 0x08) result |= 0x10;
    if (x & 0x10) result |= 0x08;
    if (x & 0x20) result |= 0x04;
    if (x & 0x40) result |= 0x02;
    if (x & 0x80) result |= 0x01;
    return result;
}

// Test population count (count set bits)
int test_popcount_manual(int x) {
    int count = 0;
    while (x) {
        if (x & 0x01) count++;
        x >>= 1;
    }
    return count;
}

// =============================================================================
// BOUNDARY CONDITIONS
// =============================================================================

// Test mask with all zeros
int test_mask_zeros(int x) {
    // Pattern: x & 0x00 (always produces 0)
    // Optimized: LDA #0
    return x & 0x00;
}

// Test OR with zeros
int test_or_zeros(int x) {
    // Pattern: x | 0x00 (no change, identity)
    // Optimized: NOP / remove
    return x | 0x00;
}

// Test OR with all ones
int test_or_ones(int x) {
    // Pattern: x | 0xFF (all bits set)
    // Optimized: LDA #0xFF
    return x | 0xFF;
}

// Test AND then OR (common pattern)
int test_mask_and_set(int x, int bits) {
    // Pattern: (x & ~mask) | bits
    // Clear some bits then set them to new value
    return (x & ~0x0C) | (bits & 0x0C);
}

// =============================================================================
// REAL-WORLD PATTERNS
// =============================================================================

// Flag extraction from packed byte
int extract_flags(int flags) {
    // Common pattern: extract multiple flags
    int carry = (flags >> 0) & 0x01;
    int zero = (flags >> 1) & 0x01;
    int negative = (flags >> 2) & 0x01;
    int overflow = (flags >> 3) & 0x01;
    return carry | (zero << 1) | (negative << 2) | (overflow << 3);
}

// Color component extraction (RGB from packed 24-bit)
int extract_red(int rgb) {
    // Pattern: (rgb >> 16) & 0xFF
    return (rgb >> 16) & 0xFF;
}

int extract_green(int rgb) {
    // Pattern: (rgb >> 8) & 0xFF
    return (rgb >> 8) & 0xFF;
}

int extract_blue(int rgb) {
    // Pattern: rgb & 0xFF
    return rgb & 0xFF;
}

// Hardware register manipulation (common in systems programming)
int set_irq_enable(int reg) {
    // Set bit 6 (IRQ enable)
    return reg | 0x40;
}

int clear_overflow_flag(int reg) {
    // Clear bit 3 (overflow)
    return reg & ~0x08;
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    int total = 0;

    // Single bit operations
    total += test_single_bit_lsb(0x55);     // 0x01
    total += test_single_bit_4(0xF0);       // 0x10
    total += test_single_bit_msb(0x80);     // 0x80
    total += test_single_bit_set(0x00);     // 0x02
    total += test_single_bit_clear(0xFF);   // 0xFB

    // Multi-bit masks
    total += test_identity_and_full(0xAB);  // 0xAB
    total += test_identity_and_nibble(0xF5);  // 0x05
    total += test_mask_2bit(0xFF);          // 0x03

    // XOR operations
    total += test_xor_identity(42);         // 42
    total += test_xor_not(0x00);            // 0xFF
    total += test_xor_toggle(0xAA);         // 0xFF

    // Shift operations
    total += test_shift_left_1(10);         // 20
    total += test_shift_left_2(5);          // 20
    total += test_shift_left_3(3);          // 24
    total += test_shift_right_1(20);        // 10
    total += test_shift_right_2(20);        // 5
    total += test_shift_right_4(0xA0);      // 10

    // Bit field operations
    total += test_extract_low_nibble(0xF5); // 5
    total += test_extract_high_nibble(0x5F); // 5
    total += test_extract_bits_2_5(0xFC);   // 15 (bits 2-5 = 1111)

    // Complex patterns
    total += test_chain_operations(0x20);   // (0x20>>1)&0x0F|0x80 = 0x90
    total += test_swap_nibbles(0x5A);       // 0xA5

    // Real-world patterns
    total += extract_red(0xFF0000);         // 0xFF
    total += extract_green(0x00FF00);       // 0xFF
    total += extract_blue(0x0000FF);        // 0xFF

    return total;  // Should be valid result
}
