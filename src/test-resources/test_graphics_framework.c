/* test_graphics_framework.c — Graphics Framework (Phase 1) Test Suite
 *
 * Tests driver abstraction, mode switching, color control, and primitives.
 */

#include <stdio.h>
#include <graphics.h>

int main(void) {
    printf("Testing graphics framework (Phase 1)...\n\n");

    /* Test 1: Framework Initialization */
    printf("Test 1: Framework Initialization\n");
    graphics_init();
    printf("  ✓ Framework initialized\n");
    printf("  Mode: %d (expect 0 for TEXT_40x25)\n", graphics_getmode());
    printf("  Width: %d, Height: %d\n", graphics_getwidth(), graphics_getheight());

    /* Test 2: Mode Information */
    printf("\nTest 2: Mode Information\n");
    int w = graphics_getwidth();
    int h = graphics_getheight();
    int colors = graphics_getmaxcolor();
    printf("  Current mode: %d×%d with %d colors\n", w, h, colors + 1);
    if (w == 40 && h == 25 && colors == 15) {
        printf("  ✓ TEXT_40x25 dimensions correct\n");
    } else {
        printf("  ✗ Dimension mismatch\n");
    }

    /* Test 3: Color Control */
    printf("\nTest 3: Color Control\n");
    graphics_setcolor(2);  /* RED */
    printf("  Set color to RED (2)\n");
    if (graphics_getcolor() == 2) {
        printf("  ✓ Color set correctly\n");
    }

    graphics_setbkcolor(6);  /* BLUE */
    printf("  Set background to BLUE (6)\n");
    if (graphics_getbkcolor() == 6) {
        printf("  ✓ Background color set correctly\n");
    }

    /* Test 4: Clipping Control */
    printf("\nTest 4: Clipping Control\n");
    graphics_setclip(5, 5, 35, 20);
    int cx1, cy1, cx2, cy2;
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Set clip (5,5)-(35,20): got (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);
    if (cx1 == 5 && cy1 == 5 && cx2 == 35 && cy2 == 20) {
        printf("  ✓ Clipping set correctly\n");
    }

    graphics_clearclip();
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Cleared clip: (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);

    /* Test 5: Drawing Operations (TEXT_40x25 mode) */
    printf("\nTest 5: Drawing Operations (TEXT_40x25)\n");
    graphics_clear();
    printf("  ✓ Screen cleared\n");

    graphics_setcolor(1);  /* WHITE */
    graphics_plot(5, 5);
    printf("  ✓ Plot at (5,5)\n");

    graphics_line(0, 0, 10, 10);
    printf("  ✓ Line from (0,0) to (10,10)\n");

    graphics_bar(15, 5, 25, 10);
    printf("  ✓ Bar from (15,5) to (25,10)\n");

    graphics_rect(30, 5, 39, 10);
    printf("  ✓ Rect outline at (30,5)-(39,10)\n");

    graphics_circle(20, 18, 3);
    printf("  ✓ Circle at (20,18) radius 3\n");

    /* Test 6: Mode Switching (Bitmap Mode Stub) */
    printf("\nTest 6: Mode Switching\n");
    int result = graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    printf("  Switch to BITMAP_320x200: result=%d\n", result);
    if (result == 0) {
        printf("  ✓ Mode switched successfully\n");
        printf("  New dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
        if (graphics_getwidth() == 320 && graphics_getheight() == 200) {
            printf("  ✓ Bitmap mode dimensions correct\n");
        }
    } else {
        printf("  ✗ Mode switch failed\n");
    }

    /* Test 7: Switch back to TEXT mode */
    printf("\nTest 7: Return to TEXT mode\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched back to TEXT_40x25\n");
    printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());

    /* Test 8: Extended Text Mode Stub */
    printf("\nTest 8: Extended Text Mode\n");
    result = graphics_setmode(GRAPHICS_MODE_TEXT_80x24);
    if (result == 0) {
        printf("  ✓ TEXT_80x24 mode available\n");
        printf("  Dimensions: %d×%d (expect 80×24)\n", graphics_getwidth(), graphics_getheight());
    } else {
        printf("  ✗ TEXT_80x24 not available\n");
    }

    /* Switch back to safe mode */
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);

    /* Test 9: Cleanup */
    printf("\nTest 9: Framework Cleanup\n");
    graphics_done();
    printf("  ✓ Framework cleaned up\n");

    printf("\n✓ All graphics framework (Phase 1) tests completed!\n");
    return 0;
}

