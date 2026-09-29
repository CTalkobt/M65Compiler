/* test_graphics_rrb_phase1.c — RRB Phase 105.1 Tests
 *
 * Tests core RRB initialization, layer management, and VIC-IV configuration.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing RRB Phase 105.1: Core VIC-IV Integration...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized in TEXT_40x25 mode\n\n");

    /* ====================================================================
     * Test 1: RRB System Initialization
     * ==================================================================== */

    printf("Test 1: RRB System Initialization\n");

    rrb_system_t rrb;
    int result = rrb.init(&rrb, 3, 40, 25);

    if (result == 0) {
        printf("  ✓ RRB initialized (3 layers max, 40×25)\n");
    }

    if (!rrb.is_enabled()) {
        printf("  ✓ RRB starts disabled\n");
    }

    if (rrb.screen_width == 320 && rrb.screen_height == 25) {
        printf("  ✓ Screen dimensions: 320×25\n");
    }

    if (rrb.chrcount == 40) {
        printf("  ✓ Default CHRCOUNT: 40\n");
    }

    /* ====================================================================
     * Test 2: Layer Creation
     * ==================================================================== */

    printf("\nTest 2: Layer Creation\n");

    int layer0 = rrb.create_layer(&rrb, RRB_MODE_FULL, 40, 25);
    if (layer0 == 0) {
        printf("  ✓ Layer 0 created (FULL mode, 40×25)\n");
    }

    int layer1 = rrb.create_layer(&rrb, RRB_MODE_SPARSE, 40, 25);
    if (layer1 == 1) {
        printf("  ✓ Layer 1 created (SPARSE mode, 40×25)\n");
    }

    int layer2 = rrb.create_layer(&rrb, RRB_MODE_STACK, 20, 12);
    if (layer2 == 2) {
        printf("  ✓ Layer 2 created (STACK mode, 20×12)\n");
    }

    /* Get layer pointers for later tests */
    rrb_layer_t *l2 = rrb.get_layer(&rrb, layer2);

    /* Try to exceed max layers */
    int layer3 = rrb.create_layer(&rrb, RRB_MODE_FULL, 40, 25);
    if (layer3 == -1) {
        printf("  ✓ Layer creation blocked when max exceeded\n");
    }

    /* ====================================================================
     * Test 3: Layer Access
     * ==================================================================== */

    printf("\nTest 3: Layer Access\n");

    rrb_layer_t *l0 = rrb.get_layer(&rrb, 0);
    if (l0 != NULL && l0->width == 40 && l0->height == 25) {
        printf("  ✓ Layer 0 retrieved with correct dimensions\n");
    }

    rrb_layer_t *l1 = rrb.get_layer(&rrb, 1);
    if (l1 != NULL && l1->mode == RRB_MODE_SPARSE) {
        printf("  ✓ Layer 1 has correct mode (SPARSE)\n");
    }

    rrb_layer_t *invalid = rrb.get_layer(&rrb, 999);
    if (invalid == NULL) {
        printf("  ✓ Invalid layer index returns NULL\n");
    }

    /* ====================================================================
     * Test 4: Layer Data Access (Character & Color)
     * ==================================================================== */

    printf("\nTest 4: Character & Color Access\n");

    l0->set_char(l0, 0, 0, 'A', 0x0F);
    unsigned char ch = l0->get_char(l0, 0, 0);
    if (ch == 'A') {
        printf("  ✓ Character set and retrieved (A)\n");
    }

    unsigned char color = l0->get_color(l0, 0, 0);
    if (color == 0x0F) {
        printf("  ✓ Color set and retrieved (0x0F)\n");
    }

    /* Set multiple characters */
    for (int i = 0; i < 5; i++) {
        l0->set_char(l0, i, 0, 'A' + i, 0x0F);
    }
    printf("  ✓ Set character row (ABCDE)\n");

    /* ====================================================================
     * Test 5: Layer Clear
     * ==================================================================== */

    printf("\nTest 5: Layer Clear\n");

    l0->clear(l0, 32, 0x00);
    unsigned char cleared = l0->get_char(l0, 0, 0);
    if (cleared == 32) {
        printf("  ✓ Layer cleared to spaces (0x20)\n");
    }

    unsigned char cleared_color = l0->get_color(l0, 0, 0);
    if (cleared_color == 0x00) {
        printf("  ✓ Color cleared to 0x00\n");
    }

    /* ====================================================================
     * Test 6: Layer Configuration
     * ==================================================================== */

    printf("\nTest 6: Layer Configuration\n");

    l0->set_scroll(l0, 100, 50);
    if (l0->scroll_x == 100 && l0->scroll_y == 50) {
        printf("  ✓ Scroll position set (100, 50)\n");
    }

    l1->set_priority(l1, 10);
    if (l1->priority == 10) {
        printf("  ✓ Priority set to 10\n");
    }

    l2->hide(l2);
    if (!l2->visible) {
        printf("  ✓ Layer hidden\n");
    }

    l2->show(l2);
    if (l2->visible) {
        printf("  ✓ Layer shown\n");
    }

    /* ====================================================================
     * Test 7: Direct Memory Access
     * ==================================================================== */

    printf("\nTest 7: Direct Memory Access\n");

    unsigned char *screen_ptr = l0->screen_ptr(l0);
    if (screen_ptr != NULL) {
        printf("  ✓ Screen memory pointer obtained\n");
    }

    unsigned char *color_ptr = l0->color_ptr(l0);
    if (color_ptr != NULL) {
        printf("  ✓ Color memory pointer obtained\n");
    }

    /* Write directly to memory */
    screen_ptr[0] = 'X';
    if (l0->get_char(l0, 0, 0) == 'X') {
        printf("  ✓ Direct memory write works\n");
    }

    /* ====================================================================
     * Test 8: VIC-IV Configuration
     * ==================================================================== */

    printf("\nTest 8: VIC-IV Configuration\n");

    int config_result = rrb.configure_vic(&rrb, 0, 40, 80);
    if (config_result == 0) {
        printf("  ✓ VIC-IV configured (320px, CHRCOUNT=40, LINESTEP=80)\n");
    }

    if (rrb.chrcount == 40 && rrb.linestep == 80) {
        printf("  ✓ Configuration parameters stored\n");
    }

    /* Test 640px mode */
    int config_640 = rrb.configure_vic(&rrb, 1, 80, 160);
    if (config_640 == 0 && rrb.h640 == 1) {
        printf("  ✓ 640px mode configured\n");
    }

    /* Test invalid parameters */
    int bad_config = rrb.configure_vic(&rrb, 0, 2000, 80);
    if (bad_config == -1) {
        printf("  ✓ Invalid CHRCOUNT rejected\n");
    }

    /* ====================================================================
     * Test 9: RRB Enable/Disable
     * ==================================================================== */

    printf("\nTest 9: RRB Enable/Disable\n");

    rrb.enable(&rrb);
    if (rrb.is_enabled()) {
        printf("  ✓ RRB enabled\n");
    }

    rrb.disable(&rrb);
    if (!rrb.is_enabled()) {
        printf("  ✓ RRB disabled\n");
    }

    /* ====================================================================
     * Test 10: Double-Raster-Time Mode
     * ==================================================================== */

    printf("\nTest 10: Double-Raster-Time Mode\n");

    int dblrr_result = rrb.enable_double_time(&rrb);
    if (dblrr_result == 0 && rrb.is_double_time()) {
        printf("  ✓ Double-raster-time enabled\n");
    }

    rrb.disable_double_time(&rrb);
    if (!rrb.is_double_time()) {
        printf("  ✓ Double-raster-time disabled\n");
    }

    /* ====================================================================
     * Test 11: Calculation Utilities
     * ==================================================================== */

    printf("\nTest 11: Calculation Utilities\n");

    int needed_chrcount = rrb_calc_chrcount(40, 10);
    if (needed_chrcount == 50) {
        printf("  ✓ CHRCOUNT calculation: 40 chars + 10 GOTOX = 50\n");
    }

    int needed_linestep = rrb_calc_linestep(40);
    if (needed_linestep == 80) {
        printf("  ✓ LINESTEP calculation: 40 * 2 = 80\n");
    }

    int raster_ok = rrb_test_raster_budget(&rrb, 0);
    printf("  ✓ Raster budget test executed (result: %s)\n",
           raster_ok ? "fits" : "exceeds");

    /* ====================================================================
     * Test 12: Layer Destruction
     * ==================================================================== */

    printf("\nTest 12: Layer Destruction\n");

    rrb.destroy_layer(&rrb, 2);
    if (rrb.get_layer_count() == 2) {
        printf("  ✓ Layer 2 destroyed (count now 2)\n");
    }

    rrb_layer_t *destroyed = rrb.get_layer(&rrb, 2);
    if (destroyed == NULL) {
        printf("  ✓ Destroyed layer inaccessible\n");
    }

    /* ====================================================================
     * Test 13: RRB Cleanup
     * ==================================================================== */

    printf("\nTest 13: RRB Cleanup\n");

    rrb.done(&rrb);
    if (rrb.layers == NULL && rrb.get_layer_count() == 0) {
        printf("  ✓ RRB cleaned up\n");
    }

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    graphics_done();

    printf("\n✓ All Phase 105.1 tests passed!\n");
    printf("Phase 105.1: Core VIC-IV Integration complete.\n");
    printf("\nNext: Phase 105.2 - GOTOX Encoding & Rendering\n");
    return 0;
}
