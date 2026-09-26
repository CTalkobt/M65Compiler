// Phase 96.2: Pointer field support in striped arrays
// Tests that pointer fields work correctly with fixed-prefix striping

#include <stdio.h>

struct DataPtr {
    int width;          // Fixed: 2 bytes
    int height;         // Fixed: 2 bytes
    unsigned char* data; // Pointer: 2 bytes address, variable data
};

// 8x8 striped array with pointer field
__striped struct DataPtr arrays[8][8];

// Storage for pointed-to data
unsigned char data_storage[4096];
int data_offset = 0;

void init_array(void) {
    int idx = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            unsigned char size = (y * 8 + x) + 1;

            // Set fixed fields
            arrays[y][x].width = size;
            arrays[y][x].height = size;

            // Allocate data
            arrays[y][x].data = &data_storage[data_offset];

            // Initialize data
            for (int i = 0; i < size; i++) {
                data_storage[data_offset + i] = (unsigned char)(idx + i);
            }

            data_offset += size;
            idx += size;
        }
    }
}

void test_fixed_fields(void) {
    printf("Fixed field access test:\n");
    int errors = 0;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char expected = (y * 8 + x) + 1;

            if (arrays[y][x].width != expected) {
                printf("  ERROR [%d,%d]: width=%d (expected %d)\n",
                       y, x, arrays[y][x].width, expected);
                errors++;
            }

            if (arrays[y][x].height != expected) {
                printf("  ERROR [%d,%d]: height=%d (expected %d)\n",
                       y, x, arrays[y][x].height, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_pointer_field(void) {
    printf("Pointer field access test:\n");

    unsigned char* ptr = arrays[2][3].data;
    if (ptr != 0) {
        printf("  Pointer at [2,3]: 0x%04x\n", (int)ptr);

        // Check that pointer is within storage
        int offset = (int)ptr - (int)data_storage;
        printf("  Offset from base: %d bytes\n", offset);

        if (offset >= 0 && offset < 4096) {
            printf("  PASS: Pointer is valid\n");
        } else {
            printf("  FAIL: Pointer is out of bounds\n");
        }
    }
}

void test_pointer_data(void) {
    printf("Pointed-to data access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char size = (y * 8 + x) + 1;
            unsigned char* ptr = arrays[y][x].data;

            // Check first byte of data
            unsigned char expected_first = y * 8 + x;
            if (*ptr != expected_first) {
                printf("  ERROR [%d,%d]: data[0]=%d (expected %d)\n",
                       y, x, *ptr, expected_first);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_mixed_access(void) {
    printf("Mixed fixed/variable access test:\n");

    // Access fixed field
    int w = arrays[3][2].width;

    // Access pointer field
    unsigned char* p = arrays[3][2].data;

    printf("  width=%d, pointer=0x%04x\n", w, (int)p);

    if (w > 0 && p != 0) {
        printf("  PASS: Both fields accessible\n");
    } else {
        printf("  FAIL: One or both fields invalid\n");
    }
}

int main(void) {
    printf("Phase 96.2: Pointer Field Striped Array Test\n");
    printf("============================================\n\n");

    init_array();

    test_fixed_fields();
    printf("\n");
    test_pointer_field();
    printf("\n");
    test_pointer_data();
    printf("\n");
    test_mixed_access();

    printf("\nPointer field test completed\n");
    return 0;
}
