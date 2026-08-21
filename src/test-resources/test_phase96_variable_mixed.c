// Phase 96.2: Mixed fixed and variable field test
// Tests structs with both fixed-size and variable-size fields

#include <stdio.h>

struct Mixed {
    unsigned char id;      // Fixed: 1 byte
    int value;             // Fixed: 2 bytes
    unsigned char* extra;  // Pointer: 2 bytes (points to variable data)
    unsigned char flags;   // Fixed: 1 byte
};

// 4x4 striped array with mixed fields
__striped struct Mixed mixed[4][4];

// Extra data storage
unsigned char extra_data[256];
int extra_offset = 0;

void init_mixed_array(void) {
    int idx = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char elem_idx = y * 4 + x;

            // Fixed fields
            mixed[y][x].id = elem_idx;
            mixed[y][x].value = elem_idx * 100;
            mixed[y][x].flags = (elem_idx & 0x0F);

            // Pointer field
            mixed[y][x].extra = &extra_data[extra_offset];

            // Initialize extra data
            for (int i = 0; i < 3; i++) {
                extra_data[extra_offset + i] = elem_idx + i;
            }
            extra_offset += 3;

            idx++;
        }
    }
}

void test_id_field(void) {
    printf("ID field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char expected = y * 4 + x;
            unsigned char actual = mixed[y][x].id;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: id=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_value_field(void) {
    printf("Value field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char elem_idx = y * 4 + x;
            int expected = elem_idx * 100;
            int actual = mixed[y][x].value;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: value=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_flags_field(void) {
    printf("Flags field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char elem_idx = y * 4 + x;
            unsigned char expected = (elem_idx & 0x0F);
            unsigned char actual = mixed[y][x].flags;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: flags=0x%02x (expected 0x%02x)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_extra_field(void) {
    printf("Extra pointer field access test:\n");

    unsigned char* ptr1 = mixed[0][0].extra;
    unsigned char* ptr2 = mixed[1][0].extra;

    if (ptr1 != ptr2) {
        printf("  Pointers differ: ptr1=0x%04x, ptr2=0x%04x\n",
               (int)ptr1, (int)ptr2);
        printf("  PASS: Pointers are unique\n");
    } else {
        printf("  FAIL: Pointers should be different\n");
    }
}

void test_all_fields(void) {
    printf("All fields access test:\n");
    int total_errors = 0;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char elem_idx = y * 4 + x;
            unsigned char actual_id = mixed[y][x].id;
            int actual_value = mixed[y][x].value;
            unsigned char actual_flags = mixed[y][x].flags;
            unsigned char* actual_extra = mixed[y][x].extra;

            unsigned char expected_id = elem_idx;
            int expected_value = elem_idx * 100;
            unsigned char expected_flags = (elem_idx & 0x0F);

            if (actual_id != expected_id ||
                actual_value != expected_value ||
                actual_flags != expected_flags ||
                actual_extra == 0) {
                total_errors++;
            }
        }
    }

    printf("  Checked all 16 elements: %s (%d errors)\n",
           total_errors == 0 ? "PASS" : "FAIL", total_errors);
}

int main(void) {
    printf("Phase 96.2: Mixed Field Striped Array Test\n");
    printf("=========================================\n\n");

    init_mixed_array();

    test_id_field();
    printf("\n");
    test_value_field();
    printf("\n");
    test_flags_field();
    printf("\n");
    test_extra_field();
    printf("\n");
    test_all_fields();

    printf("\nMixed field test completed\n");
    return 0;
}
