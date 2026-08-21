// Phase 96.1: Basic union striped array test
// Tests fundamental union member access in striped arrays

#include <stdio.h>

union Data {
    int i;
    unsigned char c;
};

// Basic 8x8 union striped array
__striped union Data values[8][8];

void init_union_array(void) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            unsigned char idx = (y * 8) + x;
            values[y][x].i = (idx * 17) & 0xFFFF;  // Initialized via int field
        }
    }
}

void test_int_field_access(void) {
    printf("Int field access: ");
    int sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sum += values[y][x].i;
        }
    }
    printf("sum=%d\n", sum);
}

void test_char_field_access(void) {
    printf("Char field access: ");
    int sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            sum += values[y][x].c;
        }
    }
    printf("sum=%d\n", sum);
}

void test_single_element(void) {
    printf("Single element test:\n");
    unsigned char idx = 3 * 8 + 5;
    int expected_i = (idx * 17) & 0xFFFF;
    unsigned char expected_c = (unsigned char)expected_i;

    printf("  [3,5] int=%d, char=%d\n", values[3][5].i, values[3][5].c);
    printf("  Expected int=%d, char=%d\n", expected_i, expected_c);
}

void test_overlay_consistency(void) {
    printf("Overlay consistency test:\n");
    // Set via int field, read via char field
    values[1][2].i = 0x4321;
    unsigned char lo = values[1][2].c;
    printf("  Set i=0x4321, char field=%d (0x%02x)\n", lo, lo);
    printf("  %s\n", lo == 0x21 ? "PASS" : "FAIL");
}

int main(void) {
    printf("Phase 96.1: Union Striped Array Test\n");
    printf("====================================\n\n");

    init_union_array();

    test_int_field_access();
    test_char_field_access();
    test_single_element();
    test_overlay_consistency();

    printf("\nBasic union test completed\n");
    return 0;
}
