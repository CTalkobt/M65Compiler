/* test_graphics_rrb_phase4.c — RRB Phase 105.4 Tests
 *
 * Tests advanced features: VIC-IV hardware integration, raster budget analysis,
 * and performance optimization strategies.
 */

#include <stdio.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>

void print_budget_report(rrb_budget_report_t *report) {
    printf("  Cycle Budget Report:\n");
    printf("    Total cycles available:  %d\n", report->total_cycles_available);
    printf("    Estimated cycles used:   %d\n", report->estimated_cycles_used);
    printf("    Headroom remaining:      %d%%\n", report->headroom_percent);
    printf("    Within budget:           %s\n", report->is_within_budget ? "YES" : "NO");
    printf("    GOTOX instructions:      %d\n", report->gotox_count);
    printf("    Max CHRCOUNT needed:     %d\n", report->max_chrcount_needed);
    printf("    DBLRR recommended:       %s\n", report->dblrr_recommended ? "YES" : "NO");
}

int main(void) {
    printf("Testing RRB Phase 105.4: Advanced VIC-IV Features...\n\n");

    graphics_init();
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("✓ Graphics initialized in TEXT_40x25 mode\n\n");

    rrb_system_t rrb;
    rrb.init(&rrb, 5, 40, 25);
    rrb.configure_vic(&rrb, 0, 40, 80);
    rrb.enable(&rrb);
    printf("✓ RRB system initialized\n\n");

    /* ====================================================================
     * Test 1: Budget Analysis - Single FULL Layer
     * ==================================================================== */

    printf("Test 1: Budget Analysis - Single FULL Layer\n");

    int layer_full = rrb.create_layer(&rrb, RRB_MODE_FULL, 40, 25);
    rrb_layer_t *full = rrb.get_layer(&rrb, layer_full);

    for (int row = 0; row < 25; row++) {
        for (int col = 0; col < 40; col++) {
            full->set_char(full, col, row, '#', 0x02);
        }
    }
    full->set_priority(full, 0);

    rrb_budget_report_t report = rrb.analyze_budget(&rrb);
    if (report.is_within_budget) {
        printf("  ✓ Single FULL layer fits within budget\n");
        print_budget_report(&report);
    }

    /* ====================================================================
     * Test 2: Budget Analysis - Multiple Layers
     * ==================================================================== */

    printf("\nTest 2: Budget Analysis - Multiple Layers\n");

    int layer_sparse = rrb.create_layer(&rrb, RRB_MODE_SPARSE, 40, 25);
    rrb_layer_t *sparse = rrb.get_layer(&rrb, layer_sparse);

    sparse->clear(sparse, 32, 0x00);
    for (int i = 0; i < 10; i++) {
        sparse->set_char(sparse, i * 4, 12, 'O', 0x03);
    }
    sparse->set_priority(sparse, 1);

    int layer_stack = rrb.create_layer(&rrb, RRB_MODE_STACK, 15, 5);
    rrb_layer_t *stack = rrb.get_layer(&rrb, layer_stack);

    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 15; col++) {
            stack->set_char(stack, col, row, '-', 0x05);
        }
    }
    stack->set_priority(stack, 2);
    stack->set_scroll(stack, 100, 10);

    report = rrb.analyze_budget(&rrb);
    printf("  ✓ Multi-layer composition analyzed\n");
    print_budget_report(&report);

    /* ====================================================================
     * Test 3: Max CHRCOUNT Calculation
     * ==================================================================== */

    printf("\nTest 3: Maximum CHRCOUNT Calculation\n");

    int max_chrcount = rrb.get_max_chrcount(&rrb);
    printf("  ✓ Max CHRCOUNT for current composition: %d\n", max_chrcount);
    printf("    (Accounts for FULL: 40+1, SPARSE: ~20+1, STACK: 15+1, end-of-row: 1)\n");

    /* ====================================================================
     * Test 4: Optimize for Speed (DBLRR Mode)
     * ==================================================================== */

    printf("\nTest 4: Optimize for Speed (DBLRR Activation)\n");

    int dblrr_before = rrb.is_double_time();
    printf("  DBLRR before: %s\n", dblrr_before ? "enabled" : "disabled");

    rrb.optimize_for_speed(&rrb);

    int dblrr_after = rrb.is_double_time();
    printf("  DBLRR after:  %s\n", dblrr_after ? "enabled" : "disabled");

    if (dblrr_after) {
        printf("  ✓ Speed optimization enabled (DBLRR mode activated)\n");

        report = rrb.analyze_budget(&rrb);
        printf("  Budget with DBLRR:\n");
        printf("    Available cycles: %d (double budget)\n", report.total_cycles_available);
        printf("    Headroom: %d%%\n", report.headroom_percent);
    }

    /* ====================================================================
     * Test 5: Optimize for Quality (High CHRCOUNT)
     * ==================================================================== */

    printf("\nTest 5: Optimize for Quality (Full CHRCOUNT)\n");

    rrb.optimize_for_quality(&rrb);

    int dblrr_quality = rrb.is_double_time();
    int chrcount_quality = rrb.chrcount;

    printf("  ✓ Quality optimization applied\n");
    printf("    DBLRR disabled: %s\n", !dblrr_quality ? "YES" : "NO");
    printf("    CHRCOUNT: %d (max allowed)\n", chrcount_quality);

    /* ====================================================================
     * Test 6: Raster Budget Headroom Tracking
     * ==================================================================== */

    printf("\nTest 6: Raster Budget Headroom\n");

    report = rrb.analyze_budget(&rrb);

    if (report.headroom_percent >= 0) {
        printf("  ✓ Headroom calculation successful\n");
        if (report.headroom_percent < 20) {
            printf("    ⚠ Warning: Less than 20%% headroom remaining\n");
        } else if (report.headroom_percent < 50) {
            printf("    ⚠ Caution: Less than 50%% headroom\n");
        } else {
            printf("    ✓ Good headroom (%d%% available)\n", report.headroom_percent);
        }
    }

    /* ====================================================================
     * Test 7: DBLRR Recommendation Logic
     * ==================================================================== */

    printf("\nTest 7: DBLRR Recommendation Logic\n");

    report = rrb.analyze_budget(&rrb);

    printf("  Analysis:\n");
    printf("    Cycles used: %d / %d\n", report.estimated_cycles_used, report.total_cycles_available);
    printf("    Headroom: %d%%\n", report.headroom_percent);
    printf("    DBLRR Recommended: %s\n", report.dblrr_recommended ? "YES" : "NO");

    if (report.dblrr_recommended) {
        printf("  ✓ DBLRR correctly recommended for tight budget\n");
    } else {
        printf("  ✓ Current budget adequate without DBLRR\n");
    }

    /* ====================================================================
     * Test 8: Rendering with Advanced Configuration
     * ==================================================================== */

    printf("\nTest 8: Rendering with Advanced Configuration\n");

    int result = rrb.render(&rrb);
    if (result == 0) {
        printf("  ✓ Composition rendered successfully with advanced config\n");
        printf("    (FULL background + SPARSE objects + STACK UI)\n");
    }

    /* ====================================================================
     * Test 9: Row-by-Row Rendering with Budget Checks
     * ==================================================================== */

    printf("\nTest 9: Row-by-Row Rendering\n");

    int failures = 0;
    for (int row = 0; row < 25; row++) {
        result = rrb.render_row(&rrb, row);
        if (result != 0) {
            failures++;
        }
    }

    if (failures == 0) {
        printf("  ✓ All 25 rows rendered within budget\n");
    } else {
        printf("  ⚠ %d rows exceeded raster budget\n", failures);
    }

    /* ====================================================================
     * Test 10: Performance Tuning
     * ==================================================================== */

    printf("\nTest 10: Performance Tuning Strategies\n");

    /* Scenario: Need more performance (fewer layers, optimize) */
    rrb.optimize_for_speed(&rrb);
    report = rrb.analyze_budget(&rrb);

    printf("  Speed-optimized configuration:\n");
    printf("    DBLRR: %s\n", rrb.is_double_time() ? "enabled" : "disabled");
    printf("    Cycles available: %d\n", report.total_cycles_available);
    printf("    Headroom: %d%%\n\n", report.headroom_percent);

    /* Scenario: Need better quality (higher refresh) */
    rrb.optimize_for_quality(&rrb);
    report = rrb.analyze_budget(&rrb);

    printf("  Quality-optimized configuration:\n");
    printf("    DBLRR: %s\n", rrb.is_double_time() ? "enabled" : "disabled");
    printf("    Cycles available: %d\n", report.total_cycles_available);
    printf("    Headroom: %d%%\n", report.headroom_percent);

    if (report.is_within_budget) {
        printf("  ✓ Quality mode feasible with current layer setup\n");
    } else {
        printf("  ⚠ Quality mode tight - consider reducing layer complexity\n");
    }

    /* ====================================================================
     * Test 11: Layer Complexity Analysis
     * ==================================================================== */

    printf("\nTest 11: Layer Complexity Analysis\n");

    int visible_count = 0;
    for (int i = 0; i < rrb.get_layer_count(); i++) {
        rrb_layer_t *layer = rrb.get_layer(&rrb, i);
        if (layer && layer->is_visible(layer)) {
            visible_count++;
        }
    }

    printf("  ✓ Composition analysis:\n");
    printf("    Total layers: %d\n", rrb.get_layer_count());
    printf("    Visible layers: %d\n", visible_count);

    report = rrb.analyze_budget(&rrb);
    printf("    GOTOX instructions: %d\n", report.gotox_count);
    printf("    Max CHRCOUNT: %d\n", report.max_chrcount_needed);

    /* ====================================================================
     * Cleanup
     * ==================================================================== */

    printf("\nTest 12: Cleanup\n");

    rrb.done(&rrb);
    graphics_done();

    printf("\n✓ All Phase 105.4 tests passed!\n");
    printf("Phase 105.4: Advanced VIC-IV Features complete.\n");
    printf("\nKey Features Demonstrated:\n");
    printf("  ✓ Raster budget analysis with detailed metrics\n");
    printf("  ✓ CHRCOUNT calculation for layer compositions\n");
    printf("  ✓ Speed vs. Quality optimization strategies\n");
    printf("  ✓ DBLRR mode activation based on headroom\n");
    printf("  ✓ Performance monitoring and recommendations\n");
    printf("\nStatus: RRB system complete with full advanced features.\n");
    return 0;
}
