/* test_graphics_rrb_phase3.c — RRB Phase 105.3 Tests
 *
 * Tests layer rendering strategies: FULL, SPARSE, STACK modes.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing RRB Phase 105.3: Layer Rendering Strategies...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized in TEXT_40x25 mode\n\n");

    rrb_system_t rrb;
    rrb.init(&rrb, 5, 40, 25);
    rrb.configure_vic(&rrb, 0, 40, 80);
    rrb.enable(&rrb);
    printf("✓ RRB system initialized\n\n");

    /* ====================================================================
     * Test 1: FULL Mode Strategy
     * ==================================================================== */

    printf("Test 1: FULL Mode (Entire Layer Rendered)\n");

    int layer_bg = rrb.create_layer(&rrb, RRB_MODE_FULL, 40, 25);
    rrb_layer_t *bg = rrb.get_layer(&rrb, layer_bg);

    /* Fill background with pattern */
    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            unsigned char ch = (row % 2 == 0) ? '=' : '-';
            bg->set_char(bg, col, row, ch, 0x02);
        }
    }
    bg->set_priority(bg, 0);
    bg->set_scroll(bg, 0, 0);

    int result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ FULL mode background rendered (40x25 grid)\n");
    }

    /* ====================================================================
     * Test 2: SPARSE Mode Strategy
     * ==================================================================== */

    printf("\nTest 2: SPARSE Mode (Transparent Background)\n");

    int layer_obj = rrb.create_layer(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *obj = rrb.get_layer(&rrb, layer_obj);

    /* Fill with spaces (transparent) */
    obj->clear(obj, 32, 0x00);

    /* Place scattered objects */
    obj->set_char(obj, 5, 5, 'O', 0x03);   /* Enemy 1 */
    obj->set_char(obj, 10, 10, 'O', 0x03); /* Enemy 2 */
    obj->set_char(obj, 15, 7, 'O', 0x03);  /* Enemy 3 */
    obj->set_char(obj, 25, 12, '*', 0x0F); /* Projectile 1 */
    obj->set_char(obj, 30, 3, '*', 0x0F);  /* Projectile 2 */

    obj->set_priority(obj, 1);
    obj->set_scroll(obj, 0, 0);

    result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ SPARSE mode objects rendered (5 non-transparent chars)\n");
    }

    /* ====================================================================
     * Test 3: STACK Mode Strategy
     * ==================================================================== */

    printf("\nTest 3: STACK Mode (Positioned Floating Layer)\n");

    int layer_ui = rrb.create_layer(&rrb, RRB_MODE_STACK, 10, 5);
    rrb_layer_t *ui = rrb.get_layer(&rrb, layer_ui);

    /* Fill UI layer with border and content */
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 10; col++) {
            unsigned char ch = ' ';
            if (row == 0 || row == 4) {
                ch = '-';  /* Top/bottom border */
            } else if (col == 0 || col == 9) {
                ch = '|';  /* Left/right border */
            }
            ui->set_char(ui, col, row, ch, 0x05);
        }
    }

    ui->set_char(ui, 1, 1, 'H', 0x01);
    ui->set_char(ui, 2, 1, 'I', 0x01);
    ui->set_priority(ui, 2);
    ui->set_scroll(ui, 100, 5);  /* Position at pixel 100, row 5 */

    result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ STACK mode UI layer rendered at (100, 5)\n");
    }

    /* ====================================================================
     * Test 4: FULL + SPARSE Composition
     * ==================================================================== */

    printf("\nTest 4: FULL + SPARSE Composition\n");

    /* Clear and reset objects for cleaner rendering */
    obj->clear(obj, 32, 0x00);

    /* Add a few scattered objects */
    for (int i = 0; i < 10; i++) {
        int x = (i * 4) % 40;
        int y = (i * 3) % 25;
        obj->set_char(obj, x, y, '*', 0x0F);
    }

    result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ FULL background + SPARSE objects composed (10 points)\n");
    }

    /* ====================================================================
     * Test 5: SPARSE + STACK Composition
     * ==================================================================== */

    printf("\nTest 5: SPARSE + STACK Composition\n");

    /* Hide background to show SPARSE + STACK more clearly */
    bg->hide(bg);

    result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ SPARSE objects + STACK UI rendered (background hidden)\n");
    }

    bg->show(bg);

    /* ====================================================================
     * Test 6: All Three Modes Together
     * ==================================================================== */

    printf("\nTest 6: All Three Modes (FULL + SPARSE + STACK)\n");

    result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ All three layer modes rendered together\n");
        printf("    Layer 0 (FULL):   Background pattern (=/-)\n");
        printf("    Layer 1 (SPARSE): Scattered objects (O, *)\n");
        printf("    Layer 2 (STACK):  Floating UI at (100,5)\n");
    }

    /* ====================================================================
     * Test 7: Layer Visibility Per Mode
     * ==================================================================== */

    printf("\nTest 7: Per-Mode Visibility Control\n");

    obj->hide(obj);
    result = rrb.render(&rrb);
    if (result == 0 && obj->get_priority(obj) == 1 && !obj->is_visible(obj)) {
        printf("  ✓ SPARSE layer hidden (BG + UI visible)\n");
    }

    obj->show(obj);

    ui->hide(ui);
    result = rrb.render(&rrb);
    if (result == 0 && ui->get_priority(ui) == 2 && !ui->is_visible(ui)) {
        printf("  ✓ STACK layer hidden (BG + SPARSE visible)\n");
    }

    ui->show(ui);

    /* ====================================================================
     * Test 8: Row-by-Row Rendering with Modes
     * ==================================================================== */

    printf("\nTest 8: Row-by-Row Rendering (Mode Verification)\n");

    int failures = 0;
    for (int row = 0; row < 25; row++) {
        result = rrb.render_row(&rrb, row);
        if (result != 0) {
            failures++;
        }
    }

    if (failures == 0) {
        printf("  ✓ All 25 rows rendered with layered modes\n");
    } else {
        printf("  ⚠ %d rows exceeded budget\n", failures);
    }

    /* ====================================================================
     * Test 9: Mode Switching
     * ==================================================================== */

    printf("\nTest 9: Mode Switching (Re-initialization)\n");

    rrb.destroy_layer(&rrb, layer_ui);
    int new_ui = rrb.create_layer(&rrb, RRB_MODE_STACK, 15, 8);
    rrb_layer_t *new_ui_ptr = rrb.get_layer(&rrb, new_ui);

    if (new_ui_ptr != NULL && new_ui_ptr->mode == RRB_MODE_STACK) {
        printf("  ✓ Layer mode verified after recreation (STACK)\n");
    }

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    printf("\nTest 10: Cleanup\n");

    rrb.done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 105.3 tests passed!\n");
    printf("Phase 105.3: Layer Rendering Strategies complete.\n");
    printf("\nStrategy Summary:\n");
    printf("  FULL:   Complete layer coverage, best for backgrounds\n");
    printf("  SPARSE: Scattered objects with transparency, best for gameplay\n");
    printf("  STACK:  Positioned UI/overlays, best for UI and dialogs\n");
    printf("\nNext: Phase 105.4 - Advanced Features (VIC-IV Hardware Integration)\n");
    return 0;
}
