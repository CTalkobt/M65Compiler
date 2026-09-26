// Test Phase 110.7: DMA Operation Template
// Tests recognition and optimization of memory transfer patterns

#include <string.h>

// =============================================================================
// MEMCPY PATTERNS (COPY OPERATIONS)
// =============================================================================

// Standard memcpy with constant length (DMA worthy - 64 bytes)
int test_memcpy_64_bytes(void) {
    unsigned char src[64];
    unsigned char dst[64];
    int i;

    // Initialize source
    for (i = 0; i < 64; i++) {
        src[i] = i;
    }

    // Copy 64 bytes (DMA beneficial: 64 > 20 threshold)
    memcpy(dst, src, 64);

    // Verify
    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// memcpy with smaller size (marginal DMA benefit - 32 bytes)
int test_memcpy_32_bytes(void) {
    unsigned char src[32];
    unsigned char dst[32];
    int i;

    for (i = 0; i < 32; i++) {
        src[i] = i * 2;
    }

    memcpy(dst, src, 32);

    int errors = 0;
    for (i = 0; i < 32; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// memcpy with small size (DMA NOT beneficial - 8 bytes)
int test_memcpy_8_bytes(void) {
    unsigned char src[8];
    unsigned char dst[8];
    int i;

    for (i = 0; i < 8; i++) {
        src[i] = 42 + i;
    }

    memcpy(dst, src, 8);

    int errors = 0;
    for (i = 0; i < 8; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// =============================================================================
// MEMSET PATTERNS (FILL OPERATIONS)
// =============================================================================

// Standard memset with constant length (DMA worthy - 64 bytes)
int test_memset_64_bytes(void) {
    unsigned char buf[64];
    int i;

    // Fill 64 bytes with 0xFF (DMA beneficial)
    memset(buf, 0xFF, 64);

    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (buf[i] != 0xFF) errors++;
    }
    return errors;
}

// memset with medium size (32 bytes)
int test_memset_32_bytes(void) {
    unsigned char buf[32];
    int i;

    memset(buf, 0x55, 32);

    int errors = 0;
    for (i = 0; i < 32; i++) {
        if (buf[i] != 0x55) errors++;
    }
    return errors;
}

// memset with small size (not DMA worthy - 8 bytes)
int test_memset_8_bytes(void) {
    unsigned char buf[8];
    int i;

    memset(buf, 0xAA, 8);

    int errors = 0;
    for (i = 0; i < 8; i++) {
        if (buf[i] != 0xAA) errors++;
    }
    return errors;
}

// =============================================================================
// MEMMOVE PATTERNS (OVERLAPPING COPY)
// =============================================================================

// memmove with constant length (DMA worthy - 40 bytes)
int test_memmove_40_bytes(void) {
    unsigned char buf[80];
    int i;

    // Initialize first 40 bytes
    for (i = 0; i < 40; i++) {
        buf[i] = i;
    }

    // Move to overlapping position (shift by 16 bytes)
    memmove(buf + 16, buf, 40);

    // Result: buf[16..55] contains 0..39
    int errors = 0;
    for (i = 0; i < 40; i++) {
        if (buf[16 + i] != i) errors++;
    }
    return errors;
}

// =============================================================================
// INLINE COPY LOOPS (MEMCPY EQUIVALENT)
// =============================================================================

// Inline loop copying 64 bytes (equivalent to memcpy)
int test_inline_copy_64(void) {
    unsigned char src[64];
    unsigned char dst[64];
    int i;

    for (i = 0; i < 64; i++) {
        src[i] = i;
    }

    // Inline copy loop - should be recognized as DMA candidate
    for (i = 0; i < 64; i++) {
        dst[i] = src[i];
    }

    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// Inline loop with unrolled copies (2 items per iteration - 32 iterations)
int test_inline_copy_unrolled_64(void) {
    unsigned char src[64];
    unsigned char dst[64];
    int i;

    for (i = 0; i < 64; i++) {
        src[i] = i;
    }

    // Unrolled loop - should still recognize total transfer as 64 bytes
    for (i = 0; i < 64; i += 2) {
        dst[i] = src[i];
        dst[i + 1] = src[i + 1];
    }

    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// =============================================================================
// INLINE FILL LOOPS (MEMSET EQUIVALENT)
// =============================================================================

// Inline loop filling 64 bytes (equivalent to memset)
int test_inline_fill_64(void) {
    unsigned char buf[64];
    int i;

    // Inline fill loop - should be recognized as DMA fill candidate
    for (i = 0; i < 64; i++) {
        buf[i] = 0xCC;
    }

    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (buf[i] != 0xCC) errors++;
    }
    return errors;
}

// Inline loop with unrolled fills (4 items per iteration - 16 iterations)
int test_inline_fill_unrolled_64(void) {
    unsigned char buf[64];
    int i;

    for (i = 0; i < 64; i += 4) {
        buf[i] = 0x11;
        buf[i + 1] = 0x11;
        buf[i + 2] = 0x11;
        buf[i + 3] = 0x11;
    }

    int errors = 0;
    for (i = 0; i < 64; i++) {
        if (buf[i] != 0x11) errors++;
    }
    return errors;
}

// =============================================================================
// STRUCT INITIALIZATION (COPY PATTERN)
// =============================================================================

struct DataBlock {
    unsigned char header[8];
    unsigned char payload[56];
    unsigned int checksum;
};

// Copying entire struct (64 bytes)
int test_struct_copy(void) {
    struct DataBlock src;
    struct DataBlock dst;
    int i;

    // Initialize source struct
    for (i = 0; i < 8; i++) {
        src.header[i] = i;
    }
    for (i = 0; i < 56; i++) {
        src.payload[i] = i + 100;
    }
    src.checksum = 0x12345678;

    // Copy struct (memcpy internally - 64 bytes)
    memcpy(&dst, &src, sizeof(struct DataBlock));

    int errors = 0;
    for (i = 0; i < 8; i++) {
        if (dst.header[i] != src.header[i]) errors++;
    }
    for (i = 0; i < 56; i++) {
        if (dst.payload[i] != src.payload[i]) errors++;
    }
    if (dst.checksum != src.checksum) errors++;

    return errors;
}

// =============================================================================
// ARRAY OPERATIONS
// =============================================================================

// Copy array of integers (16 ints = 32 bytes on 16-bit systems)
int test_copy_int_array_16(void) {
    int src[16];
    int dst[16];
    int i;

    for (i = 0; i < 16; i++) {
        src[i] = i * 10;
    }

    memcpy(dst, src, 16 * sizeof(int));

    int errors = 0;
    for (i = 0; i < 16; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// Clear array of integers (32 ints = 64 bytes)
int test_clear_int_array_32(void) {
    int buf[32];
    int i;

    // Initialize
    for (i = 0; i < 32; i++) {
        buf[i] = 0x12345678;
    }

    // Clear with memset
    memset(buf, 0, 32 * sizeof(int));

    int errors = 0;
    for (i = 0; i < 32; i++) {
        if (buf[i] != 0) errors++;
    }
    return errors;
}

// =============================================================================
// REAL-WORLD PATTERNS
// =============================================================================

// Screen memory copy (1000 bytes - very DMA beneficial)
int test_screen_copy_1000(void) {
    unsigned char screen[1000];
    unsigned char backup[1000];
    int i;

    for (i = 0; i < 1000; i++) {
        screen[i] = i & 0xFF;
    }

    // Large screen copy - definitely use DMA
    memcpy(backup, screen, 1000);

    int errors = 0;
    for (i = 0; i < 1000; i++) {
        if (backup[i] != screen[i]) errors++;
    }
    return errors;
}

// Sprite data initialization (128 bytes per sprite)
struct Sprite {
    unsigned char data[128];
    unsigned int x, y;
};

int test_sprite_init_4_sprites(void) {
    struct Sprite sprites[4];
    unsigned char pattern[128];
    int i, j;

    for (i = 0; i < 128; i++) {
        pattern[i] = (i * 2) & 0xFF;
    }

    // Initialize each sprite's pattern (4 × 128 = 512 bytes total)
    for (i = 0; i < 4; i++) {
        memcpy(sprites[i].data, pattern, 128);
        sprites[i].x = i * 40;
        sprites[i].y = i * 30;
    }

    int errors = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 128; j++) {
            if (sprites[i].data[j] != pattern[j]) errors++;
        }
    }
    return errors;
}

// =============================================================================
// BOUNDARY CONDITIONS
// =============================================================================

// Just below DMA threshold (19 bytes)
int test_below_threshold_19(void) {
    unsigned char src[19];
    unsigned char dst[19];
    int i;

    for (i = 0; i < 19; i++) {
        src[i] = i;
    }

    memcpy(dst, src, 19);

    int errors = 0;
    for (i = 0; i < 19; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// Just at DMA threshold (20 bytes)
int test_at_threshold_20(void) {
    unsigned char src[20];
    unsigned char dst[20];
    int i;

    for (i = 0; i < 20; i++) {
        src[i] = i;
    }

    memcpy(dst, src, 20);

    int errors = 0;
    for (i = 0; i < 20; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// Just above DMA threshold (21 bytes)
int test_above_threshold_21(void) {
    unsigned char src[21];
    unsigned char dst[21];
    int i;

    for (i = 0; i < 21; i++) {
        src[i] = i;
    }

    memcpy(dst, src, 21);

    int errors = 0;
    for (i = 0; i < 21; i++) {
        if (dst[i] != src[i]) errors++;
    }
    return errors;
}

// =============================================================================
// MAIN TEST DRIVER
// =============================================================================

int main(void) {
    int total_errors = 0;

    // memcpy patterns
    total_errors += test_memcpy_64_bytes();
    total_errors += test_memcpy_32_bytes();
    total_errors += test_memcpy_8_bytes();

    // memset patterns
    total_errors += test_memset_64_bytes();
    total_errors += test_memset_32_bytes();
    total_errors += test_memset_8_bytes();

    // memmove patterns
    total_errors += test_memmove_40_bytes();

    // Inline loops
    total_errors += test_inline_copy_64();
    total_errors += test_inline_copy_unrolled_64();
    total_errors += test_inline_fill_64();
    total_errors += test_inline_fill_unrolled_64();

    // Struct and array operations
    total_errors += test_struct_copy();
    total_errors += test_copy_int_array_16();
    total_errors += test_clear_int_array_32();

    // Real-world patterns
    total_errors += test_screen_copy_1000();
    total_errors += test_sprite_init_4_sprites();

    // Boundary conditions
    total_errors += test_below_threshold_19();
    total_errors += test_at_threshold_20();
    total_errors += test_above_threshold_21();

    return total_errors;  // Should be 0
}
