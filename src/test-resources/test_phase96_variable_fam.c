// Phase 96.2: Flexible Array Member (FAM) support test
// Tests structs with flexible array members in striped arrays

#include <stdio.h>

struct Buffer {
    int size;          // Fixed: 2 bytes - size of data
    unsigned char flags; // Fixed: 1 byte
    // Flexible array member: data[] follows in variable-size storage
    unsigned char data[];
};

// Note: FAM in striped arrays requires special handling
// For now, we test the fixed-size prefix
// FAM support is deferred to Phase 96.3

struct SimpleBuffer {
    int capacity;      // Fixed: 2 bytes
    int used;         // Fixed: 2 bytes
    unsigned char type; // Fixed: 1 byte
};

// 4x4 striped array of SimpleBuffer (fixed-size prefix only)
__striped struct SimpleBuffer buffers[4][4];

void init_buffers(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char idx = y * 4 + x;

            buffers[y][x].capacity = 256 + (idx * 10);
            buffers[y][x].used = idx * 5;
            buffers[y][x].type = (idx % 4);
        }
    }
}

void test_capacity_field(void) {
    printf("Capacity field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char idx = y * 4 + x;
            int expected = 256 + (idx * 10);
            int actual = buffers[y][x].capacity;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: capacity=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_used_field(void) {
    printf("Used field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char idx = y * 4 + x;
            int expected = idx * 5;
            int actual = buffers[y][x].used;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: used=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_type_field(void) {
    printf("Type field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char idx = y * 4 + x;
            unsigned char expected = (idx % 4);
            unsigned char actual = buffers[y][x].type;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: type=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_all_buffers(void) {
    printf("All buffer fields access test:\n");
    int total_errors = 0;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char idx = y * 4 + x;
            int expected_cap = 256 + (idx * 10);
            int expected_used = idx * 5;
            unsigned char expected_type = (idx % 4);

            int actual_cap = buffers[y][x].capacity;
            int actual_used = buffers[y][x].used;
            unsigned char actual_type = buffers[y][x].type;

            if (actual_cap != expected_cap ||
                actual_used != expected_used ||
                actual_type != expected_type) {
                total_errors++;
            }
        }
    }

    printf("  Checked all 16 buffers: %s (%d errors)\n",
           total_errors == 0 ? "PASS" : "FAIL", total_errors);
}

void test_fam_note(void) {
    printf("FAM support status:\n");
    printf("  - Fixed-size prefix striping: ENABLED\n");
    printf("  - Variable-size data access: Deferred to Phase 96.3\n");
    printf("  - Current support: Fixed-size fields only\n");
    printf("  Note: FAM data would be stored separately in Phase 96.3\n");
}

int main(void) {
    printf("Phase 96.2: Flexible Array Member Striped Array Test\n");
    printf("===================================================\n\n");

    init_buffers();

    test_capacity_field();
    printf("\n");
    test_used_field();
    printf("\n");
    test_type_field();
    printf("\n");
    test_all_buffers();
    printf("\n");
    test_fam_note();

    printf("\nFAM test completed\n");
    return 0;
}
