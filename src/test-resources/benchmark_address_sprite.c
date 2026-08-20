// Phase 89 Benchmark: Sprite and Table Address Calculations
// Tests: base + index * size patterns
// Expected: 30-40% code reduction for address calculations

#include <stdio.h>

#define SPRITE_SIZE 64
#define SPRITE_COUNT 16
#define SPRITE_DATA_SIZE (SPRITE_SIZE * SPRITE_COUNT)

#define PALETTE_SIZE 256
#define PALETTE_ENTRY_SIZE 3

static unsigned char sprite_data[SPRITE_DATA_SIZE];
static unsigned char palette[PALETTE_SIZE * PALETTE_ENTRY_SIZE];

// Load sprite from source
void load_sprite(unsigned char sprite_id, const unsigned char* src) {
    unsigned char* dst = sprite_data + (sprite_id * SPRITE_SIZE);
    for (int i = 0; i < SPRITE_SIZE; i++) {
        dst[i] = src[i];
    }
}

// Get pointer to sprite data
unsigned char* get_sprite_ptr(unsigned char sprite_id) {
    return sprite_data + (sprite_id * SPRITE_SIZE);
}

// Set sprite pixel
void set_sprite_pixel(unsigned char sprite_id, unsigned char offset, unsigned char value) {
    sprite_data[sprite_id * SPRITE_SIZE + offset] = value;
}

// Copy between sprites
void copy_sprite(unsigned char src_id, unsigned char dst_id) {
    unsigned char* src = sprite_data + (src_id * SPRITE_SIZE);
    unsigned char* dst = sprite_data + (dst_id * SPRITE_SIZE);
    for (int i = 0; i < SPRITE_SIZE; i++) {
        dst[i] = src[i];
    }
}

// Clear a sprite
void clear_sprite(unsigned char sprite_id, unsigned char value) {
    unsigned char* spr = sprite_data + (sprite_id * SPRITE_SIZE);
    for (int i = 0; i < SPRITE_SIZE; i++) {
        spr[i] = value;
    }
}

// Set palette entry (RGB: 3 bytes per color)
void set_palette_entry(unsigned char color_id, unsigned char r, unsigned char g, unsigned char b) {
    unsigned char* entry = palette + (color_id * PALETTE_ENTRY_SIZE);
    entry[0] = r;
    entry[1] = g;
    entry[2] = b;
}

// Get palette entry
void get_palette_entry(unsigned char color_id, unsigned char* r, unsigned char* g, unsigned char* b) {
    unsigned char* entry = palette + (color_id * PALETTE_ENTRY_SIZE);
    *r = entry[0];
    *g = entry[1];
    *b = entry[2];
}

// Fill palette with gradient
void fill_palette_gradient(unsigned char start_id, unsigned char count) {
    for (unsigned char i = 0; i < count; i++) {
        unsigned char* entry = palette + ((start_id + i) * PALETTE_ENTRY_SIZE);
        entry[0] = i;          // R
        entry[1] = 255 - i;    // G
        entry[2] = i / 2;      // B
    }
}

// Blend palette entries
void blend_palette_entry(unsigned char dst_id, unsigned char src_id, unsigned char alpha) {
    unsigned char* dst = palette + (dst_id * PALETTE_ENTRY_SIZE);
    unsigned char* src = palette + (src_id * PALETTE_ENTRY_SIZE);
    dst[0] = ((dst[0] * (255 - alpha)) + (src[0] * alpha)) / 256;
    dst[1] = ((dst[1] * (255 - alpha)) + (src[1] * alpha)) / 256;
    dst[2] = ((dst[2] * (255 - alpha)) + (src[2] * alpha)) / 256;
}

// Accumulate palette (for averaging)
void accumulate_palette(unsigned char dst_id, unsigned char src_id, unsigned char weight) {
    unsigned char* dst = palette + (dst_id * PALETTE_ENTRY_SIZE);
    unsigned char* src = palette + (src_id * PALETTE_ENTRY_SIZE);
    dst[0] += (src[0] * weight) / 256;
    dst[1] += (src[1] * weight) / 256;
    dst[2] += (src[2] * weight) / 256;
}

// Sprite animation: rotate all sprites
void rotate_all_sprites() {
    unsigned char tmp[SPRITE_SIZE];

    // Save first sprite
    for (int i = 0; i < SPRITE_SIZE; i++) {
        tmp[i] = sprite_data[i];
    }

    // Rotate
    for (int id = 0; id < SPRITE_COUNT - 1; id++) {
        unsigned char* src = sprite_data + ((id + 1) * SPRITE_SIZE);
        unsigned char* dst = sprite_data + (id * SPRITE_SIZE);
        for (int i = 0; i < SPRITE_SIZE; i++) {
            dst[i] = src[i];
        }
    }

    // Wrap around
    unsigned char* last = sprite_data + ((SPRITE_COUNT - 1) * SPRITE_SIZE);
    for (int i = 0; i < SPRITE_SIZE; i++) {
        last[i] = tmp[i];
    }
}

// Complex sprite operation: scale sprite down by 2
void scale_sprite_down(unsigned char src_id, unsigned char dst_id) {
    unsigned char* src = sprite_data + (src_id * SPRITE_SIZE);
    unsigned char* dst = sprite_data + (dst_id * SPRITE_SIZE);

    // Simple 2x2 -> 1 averaging
    int dst_offset = 0;
    for (int y = 0; y < 8; y += 2) {
        for (int x = 0; x < 8; x += 2) {
            int src_idx = y * 8 + x;
            unsigned char avg = (src[src_idx] + src[src_idx + 1] +
                               src[src_idx + 8] + src[src_idx + 9]) / 4;
            dst[dst_offset++] = avg;
        }
    }
}

int main() {
    // Initialize sprite data
    for (int i = 0; i < SPRITE_DATA_SIZE; i++) {
        sprite_data[i] = 0;
    }

    // Initialize palette (black)
    for (int i = 0; i < PALETTE_SIZE * PALETTE_ENTRY_SIZE; i++) {
        palette[i] = 0;
    }

    // Test sprite operations
    unsigned char test_sprite[SPRITE_SIZE];
    for (int i = 0; i < SPRITE_SIZE; i++) {
        test_sprite[i] = i & 0xFF;
    }

    load_sprite(0, test_sprite);
    load_sprite(1, test_sprite);

    copy_sprite(0, 2);
    copy_sprite(1, 3);

    clear_sprite(4, 0);
    clear_sprite(5, 255);

    // Test palette operations
    set_palette_entry(0, 255, 0, 0);      // Red
    set_palette_entry(1, 0, 255, 0);      // Green
    set_palette_entry(2, 0, 0, 255);      // Blue
    set_palette_entry(3, 255, 255, 0);    // Yellow

    fill_palette_gradient(4, 32);
    blend_palette_entry(10, 11, 128);
    accumulate_palette(15, 16, 64);

    rotate_all_sprites();
    scale_sprite_down(0, 8);

    printf("Sprite and palette operations complete\n");
    return 0;
}
