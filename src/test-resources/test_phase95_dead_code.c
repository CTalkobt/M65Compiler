// Phase 95.6: Field-striped struct test - dead code elimination
// Tests that Phase 95.5 optimizer correctly eliminates unused field access code

#include <stdio.h>

struct Data {
    unsigned char r;    // Field 0: Used
    unsigned char g;    // Field 1: Unused in function A
    unsigned char b;    // Field 2: Unused in both
};

__striped struct Data pixels[16][16];

// Initialize for testing
void init_pixels(void) {
    int idx = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            pixels[y][x].r = (idx * 17) & 0xFF;
            pixels[y][x].g = (idx * 23) & 0xFF;
            pixels[y][x].b = (idx * 31) & 0xFF;
            idx++;
        }
    }
}

// Function A: Only uses red field
// Phase 95.5 should eliminate g and b field offset calculations
int sum_red_field(void) {
    int sum = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            sum += pixels[y][x].r;  // Only red used
        }
    }
    return sum;
}

// Function B: Only uses green field
// Phase 95.5 should eliminate r and b field offset calculations
int sum_green_field(void) {
    int sum = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            sum += pixels[y][x].g;  // Only green used
        }
    }
    return sum;
}

// Function C: Uses all fields
// Phase 95.5 should NOT eliminate any field calculations
int sum_all_fields(void) {
    int sum = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            sum += pixels[y][x].r + pixels[y][x].g + pixels[y][x].b;
        }
    }
    return sum;
}

// Function D: Uses r and g, but not b
// Phase 95.5 should eliminate b field offset calculations
int sum_r_and_g(void) {
    int sum = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            sum += pixels[y][x].r + pixels[y][x].g;
        }
    }
    return sum;
}

int main(void) {
    printf("Phase 95.6: Dead code elimination test\n");

    init_pixels();

    int r_sum = sum_red_field();
    int g_sum = sum_green_field();
    int all_sum = sum_all_fields();
    int rg_sum = sum_r_and_g();

    printf("Red sum: %d\n", r_sum);
    printf("Green sum: %d\n", g_sum);
    printf("All sum: %d\n", all_sum);
    printf("R+G sum: %d\n", rg_sum);
    printf("Verification: R+G=%d, All-B=%d\n", rg_sum, all_sum - (sum_red_field() + sum_green_field()));

    return 0;
}
