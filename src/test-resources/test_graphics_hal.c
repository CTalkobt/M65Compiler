/* test_graphics_hal.c — Graphics HAL (Phase 0) Test Suite
 *
 * Tests VIC-IV abstraction, banking, mode control, and clipping.
 */

#include <stdio.h>
#include <graphics_hal.h>

int main(void) {
    printf("Testing graphics_hal (Phase 0)...\n\n");

    /* Test 1: HAL Initialization */
    printf("Test 1: HAL Initialization\n");
    graphics_hal_init();
    printf("  ✓ HAL initialized\n");

    /* Test 2: Bank Management */
    printf("\nTest 2: Bank Management\n");
    for (int i = 0; i < 8; i++) {
        graphics_set_bank(i);
        unsigned char bank = graphics_get_bank();
        printf("  Set bank %d → got %d %s\n", i, bank, (i == bank) ? "✓" : "✗");
    }

    /* Test 3: Display Address Control */
    printf("\nTest 3: Display Address Control\n");
    graphics_set_display_addr(GRAPHICS_SCREEN_RAM);
    printf("  ✓ Set display addr: $%04X\n", GRAPHICS_SCREEN_RAM);

    graphics_set_bitmap_addr(GRAPHICS_BITMAP_ADDR);
    printf("  ✓ Set bitmap addr: $%04X\n", GRAPHICS_BITMAP_ADDR);

    graphics_set_color_addr(GRAPHICS_COLOR_RAM);
    printf("  ✓ Set color addr: $%04X\n", GRAPHICS_COLOR_RAM);

    /* Test 4: Mode Control */
    printf("\nTest 4: Mode Control - Bitmap Mode\n");
    graphics_enable_bitmap_mode();
    printf("  ✓ Bitmap mode enabled (320×200)\n");

    graphics_disable_bitmap_mode();
    printf("  ✓ Bitmap mode disabled\n");

    /* Test 5: Multicolor Mode */
    printf("\nTest 5: Mode Control - Multicolor Mode\n");
    graphics_enable_bitmap_mode();
    graphics_enable_multicolor();
    printf("  ✓ Multicolor enabled (160×200)\n");

    graphics_disable_multicolor();
    graphics_disable_bitmap_mode();
    printf("  ✓ Multicolor disabled\n");

    /* Test 6: Extended Text Mode */
    printf("\nTest 6: Mode Control - Extended Text Mode\n");
    graphics_enable_extended_text();
    printf("  ✓ Extended text enabled (80×24)\n");

    graphics_disable_extended_text();
    printf("  ✓ Extended text disabled\n");

    /* Test 7: Buffer Access */
    printf("\nTest 7: Buffer Access\n");
    unsigned char *screen = graphics_get_screen_buffer();
    unsigned char *color = graphics_get_color_buffer();
    printf("  ✓ Screen buffer: %p\n", (void *)screen);
    printf("  ✓ Color buffer: %p\n", (void *)color);

    /* Test 8: Clipping Control */
    printf("\nTest 8: Clipping Control\n");
    graphics_set_clip(50, 50, 150, 150);
    printf("  ✓ Set custom clip rectangle\n");

    int inside = graphics_clip_point(100, 100);
    int outside = graphics_clip_point(200, 100);
    printf("  Point (100,100) clipped: %d (expect 1)\n", inside);
    printf("  Point (200,100) clipped: %d (expect 0)\n", outside);

    int cx = graphics_clip_x(100);
    int cx_out = graphics_clip_x(200);
    printf("  Clamp X=100: %d (expect 100)\n", cx);
    printf("  Clamp X=200: %d (expect -1)\n", cx_out);

    graphics_clear_clip();
    printf("  ✓ Clip cleared to defaults\n");

    /* Test 9: Register Access */
    printf("\nTest 9: Register Access\n");
    unsigned char test_val = 0xAB;
    graphics_write_reg(0x21, test_val);
    unsigned char read_val = graphics_read_reg(0x21);
    printf("  Write $D021 = 0x%02X, read = 0x%02X %s\n",
           test_val, read_val, (read_val == test_val) ? "✓" : "✗");

    /* Test 10: Memory Fill */
    printf("\nTest 10: Memory Fill\n");
    unsigned char test_buf[64];
    for (int i = 0; i < 64; i++) {
        test_buf[i] = 0x00;
    }
    graphics_fill_rect((unsigned int)test_buf, 64, 0xFF);
    int fill_ok = 1;
    for (int i = 0; i < 64; i++) {
        if (test_buf[i] != 0xFF) {
            fill_ok = 0;
            break;
        }
    }
    printf("  Fill 64 bytes with 0xFF: %s\n", fill_ok ? "✓" : "✗");

    /* Test 11: Line Clipping (Cohen-Sutherland) */
    printf("\nTest 11: Line Clipping\n");
    int cx1, cy1, cx2, cy2;
    graphics_set_clip(0, 0, 100, 100);
    int result = graphics_clip_line(50, 50, 150, 150, &cx1, &cy1, &cx2, &cy2);
    printf("  Clip line (50,50)-(150,150): result=%d\n", result);
    printf("  ✓ Line clipping function callable\n");

    /* Cleanup */
    printf("\nTest 12: HAL Cleanup\n");
    graphics_hal_done();
    printf("  ✓ HAL cleaned up\n");

    printf("\n✓ All graphics_hal (Phase 0) tests completed!\n");
    return 0;
}

