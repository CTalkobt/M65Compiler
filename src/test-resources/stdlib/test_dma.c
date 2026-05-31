// test_dma: validate DMA operations (dma_copy, dma_fill)
// Returns 0 (A=$00) on success, non-zero on failure.

#include <dma.h>
#include <string.h>

int main() {
    char src[16];
    char dst[16];
    int i;

    // Initialize source buffer
    for (i = 0; i < 16; i++) {
        src[i] = i;
        dst[i] = 0;
    }

    // Test 1: dma_copy - basic copy
    dma_copy(dst, src, 8);
    for (i = 0; i < 8; i++) {
        if (dst[i] != src[i]) return 1;
    }
    // Verify unfilled part is still zero
    if (dst[8] != 0) return 2;

    // Test 2: dma_copy - full buffer
    for (i = 0; i < 16; i++) {
        dst[i] = 0;
    }
    dma_copy(dst, src, 16);
    for (i = 0; i < 16; i++) {
        if (dst[i] != src[i]) return 3;
    }

    // Test 3: dma_copy - zero length (should do nothing)
    char preserve[4] = {10, 20, 30, 40};
    dma_copy(preserve, src, 0);
    if (preserve[0] != 10 || preserve[1] != 20) return 4;

    // Test 4: dma_fill - fill with zero
    char fill_buf[8];
    dma_fill(fill_buf, 8, 0);
    for (i = 0; i < 8; i++) {
        if (fill_buf[i] != 0) return 5;
    }

    // Test 5: dma_fill - fill with pattern
    dma_fill(fill_buf, 8, 0xFF);
    for (i = 0; i < 8; i++) {
        if (fill_buf[i] != 0xFF) return 6;
    }

    // Test 6: dma_fill - partial fill
    for (i = 0; i < 8; i++) {
        fill_buf[i] = 0;
    }
    dma_fill(fill_buf, 4, 0xAA);
    if (fill_buf[0] != 0xAA || fill_buf[1] != 0xAA) return 7;
    if (fill_buf[2] != 0xAA || fill_buf[3] != 0xAA) return 8;
    if (fill_buf[4] != 0 || fill_buf[5] != 0) return 9;

    // Test 7: dma_fill - single byte
    fill_buf[2] = 0;
    dma_fill(fill_buf + 2, 1, 0x42);
    if (fill_buf[2] != 0x42) return 10;
    if (fill_buf[1] != 0 || fill_buf[3] != 0) return 11;

    // Test 8: dma_copy - overlapping regions (forward)
    char overlap[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    dma_copy(overlap + 2, overlap, 4);
    // After copy: overlap[2..5] = overlap[0..3] = {1,2,3,4}
    if (overlap[0] != 1 || overlap[1] != 2) return 12;
    if (overlap[2] != 1 || overlap[3] != 2) return 13;
    if (overlap[4] != 3 || overlap[5] != 4) return 14;

    // Test 9: dma_fill - large fill pattern
    char large[32];
    dma_fill(large, 32, 0x55);
    for (i = 0; i < 32; i++) {
        if (large[i] != 0x55) return 15;
    }

    // Test 10: dma_copy - different patterns
    char pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    char pattern_dst[4];
    dma_copy(pattern_dst, pattern, 4);
    if (pattern_dst[0] != 0xDE || pattern_dst[1] != 0xAD) return 16;
    if (pattern_dst[2] != 0xBE || pattern_dst[3] != 0xEF) return 17;

    // Test 11: Sequential DMA operations
    char seq1[4];
    char seq2[4];
    dma_fill(seq1, 4, 0x11);
    dma_fill(seq2, 4, 0x22);
    if (seq1[0] != 0x11 || seq1[3] != 0x11) return 18;
    if (seq2[0] != 0x22 || seq2[3] != 0x22) return 19;

    // Test 12: DMA copy then fill
    char chain[8];
    dma_copy(chain, pattern, 4);
    if (chain[0] != 0xDE) return 20;
    dma_fill(chain + 4, 4, 0x99);
    if (chain[4] != 0x99 || chain[7] != 0x99) return 21;

    return 0; // All tests passed
}
