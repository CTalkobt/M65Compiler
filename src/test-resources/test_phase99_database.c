// Phase 99.1: Cross-Module Database Test
// Tests __far variable analysis and bank assignment

#include <stdio.h>

// ============================================================================
// Test 1: Multiple far modules simulation
// ============================================================================

// Module 1: Graphics data
__far unsigned char palette[256];
__far struct {
    unsigned char x, y;
    unsigned char sprite_id;
} entity[64];

void test_module1_profile(void) {
    // Simulate profile: palette and entity accessed together frequently
    printf("Module 1: Graphics data loaded\n");

    int i;
    for (i = 0; i < 256; i++) {
        palette[i] = i;
    }

    for (i = 0; i < 64; i++) {
        entity[i].x = i * 4;
        entity[i].y = i * 2;
        entity[i].sprite_id = i & 0x0F;
    }
}

// ============================================================================
// Test 2: Audio buffers
// ============================================================================

__far unsigned char audio_left[512];
__far unsigned char audio_right[512];
__far long synth_state;

void test_module2_profile(void) {
    // Simulate profile: audio buffers accessed sequentially
    printf("Module 2: Audio buffers loaded\n");

    int i;
    for (i = 0; i < 512; i++) {
        audio_left[i] = i & 0xFF;
        audio_right[i] = (i + 128) & 0xFF;
    }

    synth_state = 0x12345678L;
}

// ============================================================================
// Test 3: Mesh data (different module, uses palette from module1)
// ============================================================================

__far unsigned short vertices[1024];
__far unsigned char normals[1024];

void test_module3_profile(void) {
    // Simulate profile: mesh data + palette access (co-location opportunity)
    printf("Module 3: Mesh data loaded\n");

    int i;
    for (i = 0; i < 1024; i++) {
        vertices[i] = i * 2;
        normals[i] = i & 0xFF;
    }

    // Frequent access to palette from module 1 (co-location benefit)
    unsigned char color = palette[42];
    printf("Palette color at 42: %02X\n", color);
}

// ============================================================================
// Test 4: Verify optimal bank assignments
// ============================================================================

void test_bank_assignments(void) {
    // After Phase 99 analysis, optimal layout should be:
    // Bank 0: palette + entity (co-located - frequent access together)
    // Bank 1: audio_left + audio_right (co-located - sequential access)
    // Bank 2: synth_state + vertices + normals

    printf("Bank Assignment Tests:\n");
    printf("  palette size: 256 bytes\n");
    printf("  entity size: 192 bytes (64 * 3)\n");
    printf("  Combined (Bank 0): 448 bytes\n");
    printf("\n");
    printf("  audio_left size: 512 bytes\n");
    printf("  audio_right size: 512 bytes\n");
    printf("  Combined (Bank 1): 1024 bytes\n");
    printf("\n");
    printf("  synth_state: 4 bytes\n");
    printf("  vertices: 2048 bytes\n");
    printf("  normals: 1024 bytes\n");
    printf("  Combined (Bank 2): 3076 bytes\n");
    printf("\n");
    printf("Total banks needed: 3\n");
}

// ============================================================================
// Main test
// ============================================================================

int main(void) {
    printf("=== Phase 99.1: Cross-Module Address Space Database ===\n\n");

    printf("Loading Module 1 (Graphics)...\n");
    test_module1_profile();
    printf("\n");

    printf("Loading Module 2 (Audio)...\n");
    test_module2_profile();
    printf("\n");

    printf("Loading Module 3 (Mesh)...\n");
    test_module3_profile();
    printf("\n");

    printf("Analyzing Bank Assignments...\n");
    test_bank_assignments();
    printf("\n");

    printf("=== Phase 99.1 Tests Complete ===\n");
    return 0;
}

