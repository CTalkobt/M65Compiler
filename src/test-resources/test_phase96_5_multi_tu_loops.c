// Phase 96.5.5: Multi-TU Test Program - Loop-Nested Cross-Module Caching
// Tests field caching in nested loops across translation units

#include <stdio.h>

// Sprite data structure with pointer field
typedef struct {
    int data;          // Pointer to sprite data
    int width;
    int height;
} Sprite;

// Large sprite array
Sprite sprites[16];

// TU1: sprite_util.c - Sprite utility functions
int sprite_get_size(int sprite_id) {
    // Access sprite.width and sprite.height in loops
    // Cross-module function accessing pointer field

    int total_size = 0;

    for (int y = 0; y < sprites[sprite_id].height; y++) {
        for (int x = 0; x < sprites[sprite_id].width; x++) {
            // This inner loop accesses the same field repeatedly
            // Perfect candidate for loop-level caching
            total_size += sprites[sprite_id].data;
        }
    }

    return total_size;
}

// TU2: sprite_render.c - Sprite rendering functions
extern int sprite_get_size(int sprite_id);

void render_sprites(void) {
    // Iterate through sprites and render each
    // Each sprite access goes through cross-module function

    for (int sprite_id = 0; sprite_id < 16; sprite_id++) {
        int size = sprite_get_size(sprite_id);

        printf("Sprite %d size: %d, data: %d\n",
               sprite_id, size, sprites[sprite_id].data);
    }
}

// TU3: main.c - Program entry
int main(void) {
    // Initialize sprite array
    for (int i = 0; i < 16; i++) {
        sprites[i].data = 256 + i;
        sprites[i].width = 8;
        sprites[i].height = 8;
    }

    // Render all sprites
    render_sprites();

    printf("Loop-nested cross-module caching test completed\n");

    return 0;
}
