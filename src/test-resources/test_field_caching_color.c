// Test: Field caching for color palette data structures
// Tests sequential and alias patterns in field access

#include <stdio.h>

// Color palette structure (aliasing pattern)
struct Palette {
    unsigned char* rgb;       // RGB color data pointer
    unsigned char* luma;      // Luminance pointer
    int colorCount;
};

// Sequential field access: rgb then colorCount
unsigned char get_color_intensity(struct Palette* pal, int index) {
    // Access rgb pointer, then colorCount
    // Optimizer should detect sequential pattern and cache rgb
    if (index < pal->colorCount) {
        return pal->rgb[index];
    }
    return 0;
}

// Aliasing pattern: rgb and luma accessed together
void palette_convert_to_bw(struct Palette* pal) {
    // Both rgb and luma accessed in same loop
    // Optimizer should detect aliasing and cache both
    for (int i = 0; i < pal->colorCount; i++) {
        unsigned char r = pal->rgb[i];
        pal->luma[i] = (r * 77) >> 8;  // Approximate luminance
    }
}

// Reuse pattern: same pointer accessed multiple times
void palette_find_closest(struct Palette* pal, unsigned char target) {
    int closest = 0;
    int minDiff = 255;

    // rgb pointer used multiple times in inner loop
    // Should be cached for lifetime of outer loop
    for (int i = 0; i < pal->colorCount; i++) {
        int diff = pal->rgb[i] - target;
        if (diff < 0) diff = -diff;
        if (diff < minDiff) {
            minDiff = diff;
            closest = i;
        }
    }
}

// Hot struct detection: multiple functions accessing same fields
int count_bright_colors(struct Palette* pal) {
    int count = 0;
    // rgb accessed in hot struct context
    for (int i = 0; i < pal->colorCount; i++) {
        if (pal->rgb[i] > 128) count++;
    }
    return count;
}

int main() {
    printf("Color palette field caching test\n");
    return 0;
}
