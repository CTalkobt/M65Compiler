// Phase 88 Benchmark: Compound Assignment Image Processing
// Tests: pixel/image manipulation with compound assignments
// Expected: 15-25% code reduction for pixel loops

#include <stdio.h>

#define WIDTH 64
#define HEIGHT 32

// Brightness adjustment using compound assignment
void adjust_brightness(unsigned char* pixels, int count, int delta) {
    for (int i = 0; i < count; i++) {
        pixels[i] += delta;
    }
}

// Contrast enhancement
void enhance_contrast(unsigned char* pixels, int count, int scale) {
    for (int i = 0; i < count; i++) {
        pixels[i] *= scale;
        pixels[i] -= (256 - pixels[i]) / 2;
    }
}

// Color channel mixing
void mix_channels(unsigned char* r, unsigned char* g, unsigned char* b, int count) {
    for (int i = 0; i < count; i++) {
        r[i] += g[i] / 2;
        g[i] += b[i] / 2;
        b[i] += r[i] / 2;
    }
}

// Convolution-style filtering with compound accumulation
void filter_3x3(unsigned char* src, unsigned char* dst, int width, int height) {
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int acc = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    acc += src[(y + dy) * width + (x + dx)];
                }
            }
            dst[y * width + x] = acc / 9;
        }
    }
}

// Gamma correction with compound operations
void apply_gamma(unsigned char* pixels, int count, int gamma_shift) {
    for (int i = 0; i < count; i++) {
        int val = pixels[i];
        val *= val;
        val >>= gamma_shift;
        pixels[i] = val;
    }
}

// Histogram equalization simulation
void histogram_equalize(unsigned char* pixels, int count) {
    unsigned char lookup[256];
    for (int i = 0; i < 256; i++) {
        lookup[i] = i;
    }

    for (int i = 0; i < count; i++) {
        pixels[i] = lookup[pixels[i]];
        lookup[pixels[i]] += 1;
    }
}

// Dithering pattern application
void apply_dither(unsigned char* pixels, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            pixels[idx] += (x + y) & 0x0F;
            pixels[idx] -= (x ^ y) & 0x0F;
        }
    }
}

int main() {
    static unsigned char pixels[WIDTH * HEIGHT];
    static unsigned char pixels2[WIDTH * HEIGHT];

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        pixels[i] = i & 0xFF;
        pixels2[i] = 128;
    }

    adjust_brightness(pixels, WIDTH * HEIGHT, 16);
    printf("brightness done\n");

    enhance_contrast(pixels2, WIDTH * HEIGHT, 2);
    printf("contrast done\n");

    unsigned char r[256], g[256], b[256];
    for (int i = 0; i < 256; i++) {
        r[i] = i;
        g[i] = 128;
        b[i] = 255 - i;
    }
    mix_channels(r, g, b, 256);
    printf("mix done\n");

    filter_3x3(pixels, pixels2, WIDTH, HEIGHT);
    printf("filter done\n");

    apply_gamma(pixels, WIDTH * HEIGHT, 2);
    printf("gamma done\n");

    apply_dither(pixels, WIDTH, HEIGHT);
    printf("dither done\n");

    return 0;
}
