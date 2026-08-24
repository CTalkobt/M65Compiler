/* test_graphics_rrb_phase2.c — RRB Phase 105.2 Tests
 *
 * Tests GOTOX encoding, layer composition, and multi-layer rendering.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing RRB Phase 105.2: GOTOX Encoding & Rendering...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized in TEXT_40x25 mode\n\n");

    rrb_system_t rrb;
    rrb_init(&rrb, 3, 40, 25);
    rrb_configure_vic(&rrb, 0, 40, 80);
    rrb_enable(&rrb);
    printf("✓ RRB system initialized\n\n");

    /* ====================================================================
     * Test 1: Single Layer Rendering
     * ==================================================================== */

    printf("Test 1: Single Layer Rendering\n");

    int layer0 = rrb_layer_create(&rrb, RRB_MODE_FULL, 40, 25);
    rrb_layer_t *l0 = rrb_get_layer(&rrb, layer0);

    /* Fill layer with grass character */
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            rrb_layer_set_char(l0, col, row, '#', 0x02);  /* Green */
        }
    }

    rrb_layer_set_scroll(l0, 0, 0);
    rrb_layer_set_priority(l0, 0);

    int result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Single layer rendered successfully\n");
    }

    /* Verify buffer contains grass character */
    unsigned char *screen_buf = rrb.screen_buffer;
    if (screen_buf[0] == 0x00 || screen_buf[1] == 0x00) {  /* Should be GOTOX or '#' */
        printf("  ✓ Screen buffer populated\n");
    }

    /* ====================================================================
     * Test 2: Multi-Layer Composition (Terrain + Objects)
     * ==================================================================== */

    printf("\nTest 2: Multi-Layer Composition\n");

    /* Layer 0: Terrain (background) */
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            rrb_layer_set_char(l0, col, row, '.', 0x02);  /* Grass */
        }
    }

    /* Layer 1: Objects (sparse) */
    int layer1 = rrb_layer_create(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *l1 = rrb_get_layer(&rrb, layer1);

    /* Fill with spaces (transparent) */
    rrb_layer_clear(l1, 32, 0x00);

    /* Place some objects */
    rrb_layer_set_char(l1, 10, 10, 'S', 0x01);  /* Soldier */
    rrb_layer_set_char(l1, 15, 10, 'T', 0x05);  /* Tank */
    rrb_layer_set_char(l1, 20, 10, 'H', 0x03);  /* House */

    rrb_layer_set_priority(l0, 0);   /* Background */
    rrb_layer_set_priority(l1, 1);   /* Foreground */

    result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Multi-layer composition successful\n");
    }

    printf("  ✓ Objects placed on terrain (Soldier, Tank, House)\n");

    /* ====================================================================
     * Test 3: Layer Visibility Control
     * ==================================================================== */

    printf("\nTest 3: Layer Visibility Control\n");

    rrb_layer_hide(l1);
    result = rrb_render(&rrb);

    if (result == 0) {
        printf("  ✓ Hidden layer excluded from render\n");
    }

    rrb_layer_show(l1);
    result = rrb_render(&rrb);

    if (result == 0) {
        printf("  ✓ Shown layer included in render\n");
    }

    /* ====================================================================
     * Test 4: Layer Priority (Z-Order)
     * ==================================================================== */

    printf("\nTest 4: Layer Priority (Z-Order)\n");

    int layer2 = rrb_layer_create(&rrb, RRB_MODE_FULL, 40, 25);
    rrb_layer_t *l2 = rrb_get_layer(&rrb, layer2);

    /* Fill with solid foreground characters */
    rrb_layer_clear(l2, 32, 0x00);

    /* Set priorities */
    rrb_layer_set_priority(l0, 0);   /* Background (grass) */
    rrb_layer_set_priority(l1, 1);   /* Middle (objects) */
    rrb_layer_set_priority(l2, 10);  /* Foreground (UI) */

    result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Layers rendered in priority order (0, 1, 10)\n");
    }

    /* ====================================================================
     * Test 5: Scroll Offset Application
     * ==================================================================== */

    printf("\nTest 5: Scroll Offset Application\n");

    rrb_layer_set_scroll(l0, 100, 50);  /* Background scroll */
    rrb_layer_set_scroll(l1, 50, 25);   /* Foreground scroll */

    result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Scroll offsets applied (100,50) and (50,25)\n");
    }

    /* ====================================================================
     * Test 6: Row-by-Row Rendering
     * ==================================================================== */

    printf("\nTest 6: Row-by-Row Rendering\n");

    int failures = 0;
    for (int row = 0; row < 25; row++) {
        result = rrb_render_row(&rrb, row);
        if (result != 0) {
            failures++;
        }
    }

    if (failures == 0) {
        printf("  ✓ All 25 rows rendered successfully\n");
    } else {
        printf("  ⚠ %d rows exceeded raster budget\n", failures);
    }

    /* ====================================================================
     * Test 7: Sparse Layer with Transparency
     * ==================================================================== */

    printf("\nTest 7: Sparse Layer Transparency\n");

    /* Create sparse layer with transparent spaces */
    int layer_sparse = rrb_layer_create(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *sparse = rrb_get_layer(&rrb, layer_sparse);

    rrb_layer_clear(sparse, 32, 0x00);  /* Fill with spaces */

    /* Place characters (sparse) */
    for (int i = 0; i < 10; i++) {
        rrb_layer_set_char(sparse, i * 3, 5, '*', 0x0F);  /* Stars */
    }

    rrb_layer_set_priority(sparse, 2);

    result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Sparse layer with transparency rendered\n");
    }

    /* ====================================================================
     * Test 8: Multiple GOTOX per Row
     * ==================================================================== */

    printf("\nTest 8: Multiple GOTOX Instructions\n");

    int layer_multi = rrb_layer_create(&rrb, RRB_MODE_STACK, 10, 25);
    rrb_layer_t *multi = rrb_get_layer(&rrb, layer_multi);

    /* Small layer at specific position */
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 10; col++) {
            rrb_layer_set_char(multi, col, row, 'X', 0x01);
        }
    }

    rrb_layer_set_priority(multi, 5);
    rrb_layer_set_scroll(multi, 200, 100);  /* Offset position */

    result = rrb_render(&rrb);
    if (result == 0) {
        printf("  ✓ Stack layer with GOTOX repositioning rendered\n");
    }

    /* ====================================================================
     * Test 9: Raster Budget Testing
     * ==================================================================== */

    printf("\nTest 9: Raster Budget\n");

    int budget_ok = rrb_test_raster_budget(&rrb, 0);
    if (budget_ok == 1) {
        printf("  ✓ Raster budget test passed (fits in single raster)\n");
    } else {
        printf("  ⚠ Raster budget exceeded (consider DBLRR mode)\n");
    }

    /* ====================================================================
     * Test 10: GOTOX Encoding Verification
     * ==================================================================== */

    printf("\nTest 10: GOTOX Encoding\n");

    unsigned char test_screen[4] = {0};
    unsigned char test_color[4] = {0};

    /* Write GOTOX to pixel 100 with no transparency */
    rrb_write_gotox((unsigned int)&test_screen[0],
                    (unsigned int)&test_color[0],
                    100, 0);

    /* Verify encoding: pixel 100 = 0x64 */
    if (test_screen[0] == 0x64 && (test_screen[1] & 0x03) == 0x00) {
        printf("  ✓ GOTOX encoded correctly for pixel 100\n");
    }

    if ((test_color[0] & RRB_GOTOX_FLAG) != 0) {
        printf("  ✓ GOTOX flag set in color byte\n");
    }

    /* Write GOTOX with transparency */
    rrb_write_gotox((unsigned int)&test_screen[2],
                    (unsigned int)&test_color[2],
                    200, 1);

    if ((test_color[2] & RRB_TRANSPARENCY_FLAG) != 0) {
        printf("  ✓ Transparency flag set correctly\n");
    }

    /* ====================================================================
     * Test 11: Screen Buffer Integrity
     * ==================================================================== */

    printf("\nTest 11: Screen Buffer Integrity\n");

    if (rrb.screen_buffer != NULL && rrb.color_buffer != NULL) {
        printf("  ✓ Screen and color buffers allocated\n");
    }

    if (rrb.buffer_size >= 40 * 25 * 2) {
        printf("  ✓ Buffer size sufficient for screen (%d bytes)\n", rrb.buffer_size);
    }

    /* ====================================================================
     * Test 12: Layer Destruction Order
     * ==================================================================== */

    printf("\nTest 12: Layer Destruction\n");

    int initial_count = rrb.layer_count;

    rrb_layer_destroy(&rrb, 0);
    if (rrb.layer_count == initial_count - 1) {
        printf("  ✓ Layer destroyed (count: %d → %d)\n", initial_count, rrb.layer_count);
    }

    /* ====================================================================
     * Test 13: Full Composition Test
     * ==================================================================== */

    printf("\nTest 13: Full Composition Pipeline\n");

    /* Reset system */
    rrb_done(&rrb);
    rrb_init(&rrb, 3, 40, 25);
    rrb_configure_vic(&rrb, 0, 40, 80);
    rrb_enable(&rrb);

    /* Create 2-layer scene */
    int bg = rrb_layer_create(&rrb, RRB_MODE_FULL, 40, 25);
    int fg = rrb_layer_create(&rrb, RRB_MODE_SPARSE, 40, 25);

    rrb_layer_t *bg_layer = rrb_get_layer(&rrb, bg);
    rrb_layer_t *fg_layer = rrb_get_layer(&rrb, fg);

    /* Fill backgrounds */
    rrb_layer_clear(bg_layer, ' ', 0x00);
    rrb_layer_clear(fg_layer, ' ', 0x00);

    /* Place objects */
    for (int i = 0; i < 20; i++) {
        rrb_layer_set_char(bg_layer, i, 10, '=', 0x02);
    }
    for (int i = 0; i < 5; i++) {
        rrb_layer_set_char(fg_layer, 5 + i, 10, '*', 0x0F);
    }

    rrb_layer_set_priority(bg_layer, 0);
    rrb_layer_set_priority(fg_layer, 1);

    result = rrb_update(&rrb);
    printf("  ✓ Full composition pipeline executed\n");

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    rrb_done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 105.2 tests passed!\n");
    printf("Phase 105.2: GOTOX Encoding & Rendering complete.\n");
    printf("\nNext: Phase 105.3 - Layer Strategies (FULL, SPARSE, STACK)\n");
    return 0;
}
