// Phase 95.6: Backward compatibility test
// Verifies that Phase 95.4-95.5 don't break Phase 94 (non-field-striped) struct arrays

#include <stdio.h>

struct RGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// Phase 94: Regular striped array (whole-element striping, NOT field-striped)
__striped struct RGB phase94_pixels[8][8] = {
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
};

// Regular non-striped array (Phase 92-94 baseline, no striping)
struct RGB regular_pixels[8][8] = {
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
};

void test_phase94_array(void) {
    // Test Phase 94 (non-field-striped) whole-element striped array
    printf("Phase 94 array: ");
    int r_sum = 0, g_sum = 0, b_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            r_sum += phase94_pixels[y][x].r;
            g_sum += phase94_pixels[y][x].g;
            b_sum += phase94_pixels[y][x].b;
        }
    }
    printf("R=%d, G=%d, B=%d\n", r_sum, g_sum, b_sum);
}

void test_regular_array(void) {
    // Test regular non-striped array
    printf("Regular array: ");
    int r_sum = 0, g_sum = 0, b_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            r_sum += regular_pixels[y][x].r;
            g_sum += regular_pixels[y][x].g;
            b_sum += regular_pixels[y][x].b;
        }
    }
    printf("R=%d, G=%d, B=%d\n", r_sum, g_sum, b_sum);
}

void verify_data_integrity(void) {
    // Verify that Phase 94 and regular arrays have same data
    printf("Data integrity check: ");
    int match = 1;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (phase94_pixels[y][x].r != regular_pixels[y][x].r ||
                phase94_pixels[y][x].g != regular_pixels[y][x].g ||
                phase94_pixels[y][x].b != regular_pixels[y][x].b) {
                match = 0;
                break;
            }
        }
        if (!match) break;
    }
    printf("%s\n", match ? "PASS" : "FAIL");
}

int main(void) {
    printf("Phase 95.6: Phase 94 backward compatibility test\n");

    test_phase94_array();
    test_regular_array();
    verify_data_integrity();

    printf("Compatibility test completed\n");
    return 0;
}
