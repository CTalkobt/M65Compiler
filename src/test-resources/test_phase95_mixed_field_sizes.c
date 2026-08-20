// Phase 95.6: Field-striped struct test - mixed field sizes
// Tests field-striped arrays with different field byte sizes (1, 2, 4 bytes)

#include <stdio.h>

struct Vertex {
    unsigned char id;      // 1 byte
    int x, y;             // 2 bytes each (int on 6502 is 16-bit)
    unsigned int flags;   // 4 bytes
};

// Field-striped 4x4 vertex array with mixed field sizes
__striped struct Vertex verts[4][4] = {
    // Row 0
    {{0, 10, 10, 0x12345678}, {1, 20, 10, 0x87654321}, {2, 30, 10, 0xDEADBEEF}, {3, 40, 10, 0xCAFEBABE}},
    // Row 1
    {{4, 10, 20, 0x11111111}, {5, 20, 20, 0x22222222}, {6, 30, 20, 0x33333333}, {7, 40, 20, 0x44444444}},
    // Row 2
    {{8, 10, 30, 0x55555555}, {9, 20, 30, 0x66666666}, {10, 30, 30, 0x77777777}, {11, 40, 30, 0x88888888}},
    // Row 3
    {{12, 10, 40, 0x99999999}, {13, 20, 40, 0xAAAAAAAA}, {14, 30, 40, 0xBBBBBBBB}, {15, 40, 40, 0xCCCCCCCC}},
};

void test_id_field_access(void) {
    // Test accessing 1-byte id field
    printf("IDs: ");
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            printf("%d ", verts[y][x].id);
        }
    }
    printf("\n");
}

void test_x_field_access(void) {
    // Test accessing 2-byte x field
    int x_sum = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            x_sum += verts[y][x].x;
        }
    }
    printf("X sum: %d\n", x_sum);
}

void test_y_field_access(void) {
    // Test accessing 2-byte y field
    int y_sum = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            y_sum += verts[y][x].y;
        }
    }
    printf("Y sum: %d\n", y_sum);
}

void test_flags_field_access(void) {
    // Test accessing 4-byte flags field
    printf("First vertex flags: $%08X\n", verts[0][0].flags);
    printf("Last vertex flags: $%08X\n", verts[3][3].flags);
}

void test_sequential_field_access(void) {
    // Test accessing fields in sequence (id then x)
    for (int i = 0; i < 4; i++) {
        unsigned char id = verts[0][i].id;
        int x = verts[0][i].x;
        printf("Vert[0,%d]: id=%d, x=%d\n", i, id, x);
    }
}

int main(void) {
    printf("Phase 95.6: Mixed field sizes test\n");
    test_id_field_access();
    test_x_field_access();
    test_y_field_access();
    test_flags_field_access();
    test_sequential_field_access();
    printf("All tests completed\n");
    return 0;
}
