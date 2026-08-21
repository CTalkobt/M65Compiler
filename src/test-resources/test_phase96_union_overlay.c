// Phase 96.1: Union field overlay correctness test
// Verifies that fields properly overlay in memory

#include <stdio.h>

union Status {
    unsigned int flags;    // 4 bytes: contains all flags
    struct {
        unsigned char b0;  // Byte 0
        unsigned char b1;  // Byte 1
    } bytes;
};

union Color {
    unsigned int rgb;     // 4 bytes: RGB + padding
    struct {
        unsigned char r;  // Red
        unsigned char g;  // Green
        unsigned char b;  // Blue
        unsigned char a;  // Alpha (padding)
    } channels;
};

// 4x4 status array
__striped union Status status[4][4];

// 4x4 color array
__striped union Color colors[4][4];

void init_status_array(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            status[y][x].flags = 0xAABBCCDD;
        }
    }
}

void init_color_array(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            colors[y][x].rgb = 0xFF8040;  // R=0xFF, G=0x80, B=0x40, A=0x00
        }
    }
}

void test_status_overlay(void) {
    printf("Status overlay test:\n");
    printf("  flags=0xAABBCCDD should decompose as:\n");

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned int f = status[y][x].flags;
            unsigned char b0 = status[y][x].bytes.b0;
            unsigned char b1 = status[y][x].bytes.b1;

            printf("  [%d,%d] flags=0x%08x -> b0=0x%02x, b1=0x%02x\n", y, x, f, b0, b1);

            // Verify overlay: b0 should be low byte of flags
            unsigned char expected_b0 = f & 0xFF;
            if (b0 != expected_b0) {
                printf("    ERROR: Expected b0=0x%02x, got 0x%02x\n", expected_b0, b0);
            }

            // Verify overlay: b1 should be second byte of flags
            unsigned char expected_b1 = (f >> 8) & 0xFF;
            if (b1 != expected_b1) {
                printf("    ERROR: Expected b1=0x%02x, got 0x%02x\n", expected_b1, b1);
            }
        }
    }
}

void test_color_overlay(void) {
    printf("\nColor overlay test:\n");
    printf("  rgb=0xFF8040 should give R=0xFF, G=0x80, B=0x40\n");

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned int rgb = colors[y][x].rgb;
            unsigned char r = colors[y][x].channels.r;
            unsigned char g = colors[y][x].channels.g;
            unsigned char b = colors[y][x].channels.b;

            printf("  [%d,%d] rgb=0x%08x -> R=0x%02x, G=0x%02x, B=0x%02x\n", y, x, rgb, r, g, b);

            // Verify: R should be low byte
            unsigned char expected_r = rgb & 0xFF;
            if (r != expected_r) {
                printf("    ERROR: Expected R=0x%02x, got 0x%02x\n", expected_r, r);
            }

            // Verify: G should be second byte
            unsigned char expected_g = (rgb >> 8) & 0xFF;
            if (g != expected_g) {
                printf("    ERROR: Expected G=0x%02x, got 0x%02x\n", expected_g, g);
            }

            // Verify: B should be third byte
            unsigned char expected_b = (rgb >> 16) & 0xFF;
            if (b != expected_b) {
                printf("    ERROR: Expected B=0x%02x, got 0x%02x\n", expected_b, b);
            }
        }
    }
}

void test_write_via_field(void) {
    printf("\nWrite via field test:\n");

    // Write via composite field, read via int field
    status[1][1].bytes.b0 = 0x12;
    status[1][1].bytes.b1 = 0x34;

    unsigned int flags = status[1][1].flags;
    printf("  Wrote b0=0x12, b1=0x34\n");
    printf("  Read flags=0x%08x\n", flags);

    // Verify: should see b0 in low byte, b1 in second byte
    if ((flags & 0xFF) != 0x12) {
        printf("  ERROR: Low byte not preserved\n");
    }
    if (((flags >> 8) & 0xFF) != 0x34) {
        printf("  ERROR: Second byte not preserved\n");
    } else {
        printf("  PASS: Overlay preserved correctly\n");
    }
}

void test_consistency_across_array(void) {
    printf("\nConsistency across array test:\n");

    // Set different values via int field
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            colors[y][x].rgb = 0x00000000 + ((y * 4 + x) * 0x01010101);
        }
    }

    // Verify all elements are readable
    int errors = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char r = colors[y][x].channels.r;
            unsigned char g = colors[y][x].channels.g;
            unsigned char b = colors[y][x].channels.b;

            unsigned char expected = (y * 4 + x);
            if (r != expected || g != expected || b != expected) {
                errors++;
            }
        }
    }

    printf("  Checked all 16 elements: %s\n", errors == 0 ? "PASS" : "FAIL");
}

int main(void) {
    printf("Phase 96.1: Union Overlay Correctness Test\n");
    printf("==========================================\n\n");

    init_status_array();
    init_color_array();

    test_status_overlay();
    test_color_overlay();
    test_write_via_field();
    test_consistency_across_array();

    printf("\nOverlay test completed\n");
    return 0;
}
