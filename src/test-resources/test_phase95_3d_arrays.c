// Phase 95.6: Field-striped 3D+ array test
// Tests field-striped struct arrays with 3+ dimensions

#include <stdio.h>

struct Pixel {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

// 3D field-striped array: 2 layers, 4 rows, 4 cols
// Outer dimension (2) is not striped, inner 2D (4x4) are striped
__striped struct Pixel layers[2][4][4];

void init_3d_array(void) {
    for (int layer = 0; layer < 2; layer++) {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                unsigned char idx = (layer * 16) + (y * 4) + x;
                layers[layer][y][x].r = (idx * 17) & 0xFF;
                layers[layer][y][x].g = (idx * 23) & 0xFF;
                layers[layer][y][x].b = (idx * 31) & 0xFF;
            }
        }
    }
}

void test_layer0_red(void) {
    printf("Layer 0 red values: ");
    int sum = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            sum += layers[0][y][x].r;
        }
    }
    printf("sum=%d\n", sum);
}

void test_layer1_green(void) {
    printf("Layer 1 green values: ");
    int sum = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            sum += layers[1][y][x].g;
        }
    }
    printf("sum=%d\n", sum);
}

void test_both_layers_mixed(void) {
    printf("Both layers mixed field access: ");
    int sum = 0;
    for (int layer = 0; layer < 2; layer++) {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                sum += layers[layer][y][x].r + layers[layer][y][x].g;
            }
        }
    }
    printf("sum=%d\n", sum);
}

int main(void) {
    printf("Phase 95.6: 3D field-striped array test\n");

    init_3d_array();

    test_layer0_red();
    test_layer1_green();
    test_both_layers_mixed();

    printf("All 3D tests completed\n");
    return 0;
}
