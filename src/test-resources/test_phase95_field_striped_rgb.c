// Phase 95.6: Field-striped struct array test - RGB image access
// Tests basic field access in field-striped struct arrays

#include <stdio.h>

struct RGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// Field-striped 8x8 RGB image
__striped struct RGB image[8][8] = {
    // Row 0
    {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 0},
     {255, 0, 255}, {0, 255, 255}, {255, 255, 255}, {128, 128, 128}},
    // Row 1
    {{0, 0, 0}, {255, 0, 0}, {0, 255, 0}, {0, 0, 255},
     {255, 255, 0}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}},
    // Rows 2-7: repeat pattern
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

void test_red_field_access(void) {
    // Test accessing only the red field
    int red_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            red_sum += image[y][x].r;
        }
    }
    printf("Red sum: %d\n", red_sum);
}

void test_green_field_access(void) {
    // Test accessing only the green field
    int green_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            green_sum += image[y][x].g;
        }
    }
    printf("Green sum: %d\n", green_sum);
}

void test_blue_field_access(void) {
    // Test accessing only the blue field
    int blue_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            blue_sum += image[y][x].b;
        }
    }
    printf("Blue sum: %d\n", blue_sum);
}

void test_mixed_field_access(void) {
    // Test accessing multiple fields
    int brightness_sum = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int brightness = image[y][x].r + image[y][x].g + image[y][x].b;
            brightness_sum += brightness;
        }
    }
    printf("Brightness sum: %d\n", brightness_sum);
}

void test_single_pixel_access(void) {
    // Test accessing a single pixel's fields
    unsigned char r = image[3][4].r;
    unsigned char g = image[3][4].g;
    unsigned char b = image[3][4].b;
    printf("Pixel [3,4]: R=%d, G=%d, B=%d\n", r, g, b);
}

int main(void) {
    printf("Phase 95.6: Field-striped RGB test\n");
    test_red_field_access();
    test_green_field_access();
    test_blue_field_access();
    test_mixed_field_access();
    test_single_pixel_access();
    printf("All tests completed\n");
    return 0;
}
