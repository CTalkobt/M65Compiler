// Phase 96.1: Union with mixed field sizes test
// Tests unions where different fields have different sizes

#include <stdio.h>

union Mixed {
    unsigned char b;    // 1 byte
    int w;              // 2 bytes
    unsigned int dw;    // 4 bytes
};

// 8x8 union array with mixed field sizes
// Memory footprint: 4 bytes per element (largest = dw)
__striped union Mixed mixed[8][8];

void init_mixed_array(void) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            unsigned char idx = (y * 8) + x;
            mixed[y][x].dw = ((idx << 24) | ((idx+1) << 16) | ((idx+2) << 8) | (idx+3));
        }
    }
}

void test_byte_field(void) {
    printf("Byte field access: ");
    int sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sum += mixed[y][x].b;
        }
    }
    printf("sum=%d\n", sum);
}

void test_word_field(void) {
    printf("Word field access: ");
    int sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sum += mixed[y][x].w;
        }
    }
    printf("sum=%d\n", sum);
}

void test_dword_field(void) {
    printf("Dword field access: ");
    long sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sum += mixed[y][x].dw;
        }
    }
    printf("sum=%ld\n", sum);
}

void test_different_field_sizes(void) {
    printf("Different field size test:\n");
    // Initialize via dword, read as byte and word
    mixed[2][3].dw = 0x12345678;

    unsigned char b_val = mixed[2][3].b;
    int w_val = mixed[2][3].w;
    unsigned int dw_val = mixed[2][3].dw;

    printf("  Set dw=0x12345678\n");
    printf("  Read byte=0x%02x (expect 0x78)\n", b_val);
    printf("  Read word=0x%04x (expect 0x5678)\n", w_val);
    printf("  Read dword=0x%08x (expect 0x12345678)\n", dw_val);
}

void test_mixed_access_pattern(void) {
    printf("Mixed access pattern test:\n");
    int byte_sum = 0;
    int word_sum = 0;

    // Access different fields in same loop
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            byte_sum += mixed[y][x].b;
            word_sum += mixed[y][x].w;
        }
    }

    printf("  Byte sum (4x4)=%d\n", byte_sum);
    printf("  Word sum (4x4)=%d\n", word_sum);
}

int main(void) {
    printf("Phase 96.1: Union Mixed Field Sizes Test\n");
    printf("=========================================\n\n");

    init_mixed_array();

    test_byte_field();
    test_word_field();
    test_dword_field();
    test_different_field_sizes();
    test_mixed_access_pattern();

    printf("\nMixed field size test completed\n");
    return 0;
}
