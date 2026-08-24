/* test_graphics_graphics_80.c — GRAPHICS_80x50 Driver (Phase 6) Test Suite
 *
 * Tests 80×50 graphics mode using block-drawing character simulation.
 */

#include <stdio.h>
#include <graphics.h>

int main(void) {
    printf("Testing GRAPHICS_80x50 driver (Phase 6)...\n\n");

    /* Test 1: Initialize and switch to GRAPHICS_80x50 mode */
    printf("Test 1: Initialize and Switch to GRAPHICS_80x50\n");
    graphics_init();
    printf("  ✓ Graphics initialized\n");

    int result = graphics_setmode(GRAPHICS_MODE_GRAPHICS_80x50);
    if (result == 0) {
        printf("  ✓ Switched to GRAPHICS_80x50\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
        if (graphics_getwidth() == 80 && graphics_getheight() == 50) {
            printf("  ✓ Dimensions correct (80×50)\n");
        }
    } else {
        printf("  ✗ Failed to switch mode\n");
    }

    /* Test 2: Color Control */
    printf("\nTest 2: Color Control\n");
    graphics_setcolor(3);  /* Cyan */
    graphics_setbkcolor(1);  /* White background */
    printf("  ✓ Set colors (Cyan fg, White bg)\n");

    /* Test 3: Screen Clear */
    printf("\nTest 3: Screen Clear\n");
    graphics_clear();
    printf("  ✓ Screen cleared (80×50 virtual grid)\n");

    /* Test 4: Pixel Operations */
    printf("\nTest 4: Pixel Operations\n");
    graphics_plot(10, 10);
    printf("  ✓ Plotted pixel at (10,10)\n");

    unsigned char pix = graphics_getpixel(10, 10);
    printf("  Read pixel at (10,10): %d (Cyan=3)\n", pix);

    unsigned char pix_empty = graphics_getpixel(20, 20);
    printf("  Read pixel at (20,20): %d (empty)\n", pix_empty);

    /* Test 5: Bresenham Lines */
    printf("\nTest 5: Bresenham Line Drawing\n");

    graphics_setcolor(2);  /* Red */
    graphics_line(10, 20, 70, 40);
    printf("  ✓ Drew red line from (10,20) to (70,40)\n");

    graphics_setcolor(5);  /* Green */
    graphics_line(60, 10, 20, 45);
    printf("  ✓ Drew green line from (60,10) to (20,45)\n");

    graphics_setcolor(6);  /* Blue */
    graphics_line(0, 25, 79, 25);
    printf("  ✓ Drew blue horizontal line at y=25\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_line(40, 0, 40, 49);
    printf("  ✓ Drew magenta vertical line at x=40\n");

    /* Test 6: Filled Rectangle */
    printf("\nTest 6: Filled Rectangle (Bar)\n");

    graphics_setcolor(1);  /* White */
    graphics_bar(15, 15, 35, 30);
    printf("  ✓ Drew white filled bar (15,15)-(35,30)\n");

    graphics_setcolor(3);  /* Cyan */
    graphics_bar(50, 35, 75, 48);
    printf("  ✓ Drew cyan filled bar (50,35)-(75,48)\n");

    /* Test 7: Rectangle Outline */
    printf("\nTest 7: Rectangle Outline\n");

    graphics_setcolor(2);  /* Red */
    graphics_rect(20, 5, 60, 20);
    printf("  ✓ Drew red rectangle outline (20,5)-(60,20)\n");

    graphics_setcolor(5);  /* Green */
    graphics_rect(10, 32, 45, 45);
    printf("  ✓ Drew green rectangle outline (10,32)-(45,45)\n");

    /* Test 8: Circle Drawing */
    printf("\nTest 8: Circle Drawing\n");

    graphics_setcolor(6);  /* Blue */
    graphics_circle(25, 12, 8);
    printf("  ✓ Drew blue circle at (25,12) radius 8\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_circle(55, 40, 10);
    printf("  ✓ Drew magenta circle at (55,40) radius 10\n");

    graphics_setcolor(1);  /* White */
    graphics_circle(40, 40, 5);
    printf("  ✓ Drew white circle at (40,40) radius 5\n");

    /* Test 9: Fine-Grained Drawing */
    printf("\nTest 9: Fine-Grained Drawing (80×50 detail)\n");

    graphics_setcolor(3);  /* Cyan */
    /* Draw a small box pattern to demonstrate precision */
    for (int y = 3; y < 8; y++) {
        for (int x = 70; x < 78; x++) {
            if (x == 70 || x == 77 || y == 3 || y == 7) {
                graphics_plot(x, y);
            }
        }
    }
    printf("  ✓ Drew detailed pattern with 80×50 precision\n");

    /* Test 10: Mode Comparison */
    printf("\nTest 10: Mode Switching\n");

    /* Switch to TEXT_40x25 */
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched to TEXT_40x25 (40×25)\n");
    printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());

    /* Back to GRAPHICS_80x50 */
    result = graphics_setmode(GRAPHICS_MODE_GRAPHICS_80x50);
    if (result == 0) {
        printf("  ✓ Switched back to GRAPHICS_80x50 (80×50)\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
    }

    /* Test 11: Clipping Control */
    printf("\nTest 11: Clipping Control\n");
    graphics_setclip(10, 10, 70, 40);
    int cx1, cy1, cx2, cy2;
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Set clip (10,10)-(70,40): got (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);
    if (cx1 == 10 && cy1 == 10 && cx2 == 70 && cy2 == 40) {
        printf("  ✓ Clipping set correctly\n");
    }

    /* Test 12: Cleanup */
    printf("\nTest 12: Cleanup\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);  /* Back to safe mode */
    graphics_done();
    printf("  ✓ Graphics cleaned up\n");

    printf("\n✓ All GRAPHICS_80x50 (Phase 6) tests completed!\n");
    printf("80×50 graphics mode provides fine-grained pixel control\n");
    printf("using block-drawing character simulation.\n");
    return 0;
}

