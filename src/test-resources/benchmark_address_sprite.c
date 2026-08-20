// Phase 89.4.4: Benchmark Sprite Offset Addressing
// Pattern: base + index * size (common for sprite/data table lookup)
// Expected: 53-75% code reduction vs naive arithmetic

struct Sprite {
    unsigned char data[32];
};

unsigned char *sprite_ptr(struct Sprite *sprites, int index) {
    return (unsigned char *)&sprites[index];
}

void load_sprite_line(struct Sprite *sprites, int sprite_id, int line, unsigned char *out) {
    unsigned char *sprite = sprite_ptr(sprites, sprite_id);
    out[0] = sprite[line * 32];
}

void sprite_copy(struct Sprite *src, int src_id, struct Sprite *dst, int dst_id) {
    unsigned char *src_data = (unsigned char *)&src[src_id];
    unsigned char *dst_data = (unsigned char *)&dst[dst_id];
    
    for (int i = 0; i < 32; i++) {
        dst_data[i] = src_data[i];
    }
}

int main(void) {
    struct Sprite sprites[64];
    unsigned char buffer[256];
    
    // Test 1: Simple pointer calculation (base + index * 32)
    struct Sprite *s0 = &sprites[0];
    struct Sprite *s1 = &sprites[1];
    struct Sprite *s5 = &sprites[5];
    
    // Test 2: In function
    unsigned char *sp = sprite_ptr(sprites, 10);
    sp[0] = 0xFF;
    
    // Test 3: In loop
    for (int i = 0; i < 64; i++) {
        unsigned char *sprite_data = (unsigned char *)&sprites[i];
        sprite_data[0] = i;
    }
    
    // Test 4: Multiple offset calculations
    for (int sprite = 0; sprite < 8; sprite++) {
        for (int byte = 0; byte < 32; byte++) {
            ((unsigned char *)&sprites[sprite])[byte] = sprite + byte;
        }
    }
    
    // Test 5: Different sizes
    struct {
        unsigned char data[8];
    } small_table[16];
    
    for (int i = 0; i < 16; i++) {
        small_table[i].data[0] = i * 8;
    }
    
    return 0;
}
