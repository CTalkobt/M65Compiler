// Phase 96.2: Comprehensive variable-size field test
// Tests complex struct with multiple fixed and variable fields

#include <stdio.h>

struct Image {
    unsigned char width;   // Fixed: 1 byte
    unsigned char height;  // Fixed: 1 byte
    int format;           // Fixed: 2 bytes
    unsigned char* pixels; // Pointer: 2 bytes (variable data)
    int metadata;         // Fixed: 2 bytes
};

// 4x4 striped array with mixed fixed/variable fields
__striped struct Image images[4][4];

// Pixel data storage
unsigned char pixel_data[4096];
int pixel_offset = 0;

void init_images(void) {
    int image_idx = 0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char w = 8 + (y * 2);
            unsigned char h = 8 + (x * 2);
            int size = w * h;

            // Fixed fields
            images[y][x].width = w;
            images[y][x].height = h;
            images[y][x].format = (image_idx & 0xFF);
            images[y][x].metadata = (y * 4 + x) * 1000;

            // Pointer field
            images[y][x].pixels = &pixel_data[pixel_offset];

            // Initialize pixel data
            for (int i = 0; i < size && pixel_offset < 4096; i++) {
                pixel_data[pixel_offset + i] = (unsigned char)(image_idx + i);
            }
            pixel_offset += size;
            image_idx++;
        }
    }
}

void test_dimension_fields(void) {
    printf("Dimension field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char expected_w = 8 + (y * 2);
            unsigned char expected_h = 8 + (x * 2);
            unsigned char actual_w = images[y][x].width;
            unsigned char actual_h = images[y][x].height;

            if (actual_w != expected_w) {
                printf("  ERROR [%d,%d]: width=%d (expected %d)\n",
                       y, x, actual_w, expected_w);
                errors++;
            }

            if (actual_h != expected_h) {
                printf("  ERROR [%d,%d]: height=%d (expected %d)\n",
                       y, x, actual_h, expected_h);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_format_field(void) {
    printf("Format field access test:\n");
    int format_sum = 0;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            format_sum += images[y][x].format;
        }
    }

    printf("  Format sum: %d\n", format_sum);
    printf("  %s\n", format_sum > 0 ? "PASS" : "FAIL");
}

void test_metadata_field(void) {
    printf("Metadata field access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            int expected = (y * 4 + x) * 1000;
            int actual = images[y][x].metadata;

            if (actual != expected) {
                printf("  ERROR [%d,%d]: metadata=%d (expected %d)\n",
                       y, x, actual, expected);
                errors++;
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_pixel_pointers(void) {
    printf("Pixel pointer access test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char* ptr = images[y][x].pixels;

            if (ptr == 0) {
                printf("  ERROR [%d,%d]: null pointer\n", y, x);
                errors++;
            } else {
                // Verify pointer is within storage range
                int offset = (int)ptr - (int)pixel_data;
                if (offset < 0 || offset >= 4096) {
                    printf("  ERROR [%d,%d]: pointer out of bounds (offset=%d)\n",
                           y, x, offset);
                    errors++;
                }
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_pixel_data_integrity(void) {
    printf("Pixel data integrity test:\n");
    int errors = 0;

    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            unsigned char* ptr = images[y][x].pixels;
            unsigned char w = images[y][x].width;

            if (ptr != 0 && w > 0) {
                // Check first pixel
                unsigned char expected = (y * 4 + x);
                unsigned char actual = ptr[0];

                if (actual != expected) {
                    printf("  ERROR [%d,%d]: first pixel=0x%02x (expected 0x%02x)\n",
                           y, x, actual, expected);
                    errors++;
                }
            }
        }
    }

    printf("  %s (%d errors)\n", errors == 0 ? "PASS" : "FAIL", errors);
}

void test_mixed_field_access(void) {
    printf("Mixed field access pattern test:\n");

    int y = 1, x = 2;
    unsigned char w = images[y][x].width;
    unsigned char h = images[y][x].height;
    int fmt = images[y][x].format;
    unsigned char* pix = images[y][x].pixels;
    int meta = images[y][x].metadata;

    printf("  Image[%d,%d]:\n", y, x);
    printf("    width=%d, height=%d\n", w, h);
    printf("    format=%d, metadata=%d\n", fmt, meta);
    printf("    pixels=0x%04x\n", (int)pix);

    if (w > 0 && h > 0 && pix != 0) {
        printf("  PASS: All fields accessible\n");
    } else {
        printf("  FAIL: One or more fields invalid\n");
    }
}

void test_array_iteration(void) {
    printf("Full array iteration test:\n");
    int total_pixels = 0;
    int total_errors = 0;

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            unsigned char w = images[y][x].width;
            unsigned char h = images[y][x].height;
            unsigned char* pix = images[y][x].pixels;

            if (w > 0 && h > 0 && pix != 0) {
                total_pixels += (w * h);
            } else {
                total_errors++;
            }
        }
    }

    printf("  Total pixels: %d\n", total_pixels);
    printf("  Errors: %d\n", total_errors);
    printf("  %s\n", total_errors == 0 ? "PASS" : "FAIL");
}

int main(void) {
    printf("Phase 96.2: Comprehensive Variable-Size Field Test\n");
    printf("==================================================\n\n");

    init_images();

    test_dimension_fields();
    printf("\n");
    test_format_field();
    printf("\n");
    test_metadata_field();
    printf("\n");
    test_pixel_pointers();
    printf("\n");
    test_pixel_data_integrity();
    printf("\n");
    test_mixed_field_access();
    printf("\n");
    test_array_iteration();

    printf("\nComprehensive variable-size field test completed\n");
    return 0;
}
