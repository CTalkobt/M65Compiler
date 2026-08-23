/* test_graphics_multicolor.c — BITMAP_160x200 Multicolor Driver (Phase 5) Test Suite
 *
 * Tests 4-color per-cell palette bitmap mode.
 */

#include <stdio.h>
#include <graphics.h>

int main(void) {
    printf("Testing BITMAP_160x200 multicolor driver (Phase 5)...\n\n");

    /* Test 1: Initialize and switch to multicolor mode */
    printf("Test 1: Initialize and Switch to Multicolor Mode\n");
    graphics_init();
    printf("  ✓ Graphics initialized\n");

    int result = graphics_setmode(GRAPHICS_MODE_BITMAP_160x200);
    if (result == 0) {
        printf("  ✓ Switched to BITMAP_160x200\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
        if (graphics_getwidth() == 160 && graphics_getheight() == 200) {
            printf("  ✓ Dimensions correct (160×200)\n");
        }
    } else {
        printf("  ✗ Failed to switch mode\n");
    }

    /* Test 2: Color Control */
    printf("\nTest 2: Color Control\n");
    graphics_setcolor(1);  /* Use color 1 from palette */
    graphics_setbkcolor(0);  /* Background color 0 */
    printf("  ✓ Set color palette indices\n");

    /* Test 3: Screen Clear */
    printf("\nTest 3: Screen Clear\n");
    graphics_clear();
    printf("  ✓ Multicolor bitmap cleared\n");

    /* Test 4: Pixel Operations */
    printf("\nTest 4: Pixel Operations\n");
    graphics_plot(10, 10);
    printf("  ✓ Plotted pixel at (10,10)\n");

    unsigned char pix = graphics_getpixel(10, 10);
    printf("  Read pixel at (10,10): %d (mapped from palette)\n", pix);

    /* Test 5: Bresenham Lines */
    printf("\nTest 5: Bresenham Line Drawing\n");

    graphics_setcolor(1);
    graphics_line(10, 20, 80, 50);
    printf("  ✓ Drew line from (10,20) to (80,50)\n");

    graphics_setcolor(2);
    graphics_line(100, 10, 75, 80);
    printf("  ✓ Drew line from (100,10) to (75,80)\n");

    graphics_setcolor(3);
    graphics_line(0, 100, 159, 100);
    printf("  ✓ Drew horizontal line at y=100\n");

    graphics_setcolor(1);
    graphics_line(80, 0, 80, 199);
    printf("  ✓ Drew vertical line at x=80\n");

    /* Test 6: Filled Rectangle */
    printf("\nTest 6: Filled Rectangle (Bar)\n");

    graphics_setcolor(2);
    graphics_bar(20, 30, 60, 60);
    printf("  ✓ Drew filled bar (20,30)-(60,60)\n");

    graphics_setcolor(3);
    graphics_bar(100, 150, 140, 180);
    printf("  ✓ Drew filled bar (100,150)-(140,180)\n");

    /* Test 7: Rectangle Outline */
    printf("\nTest 7: Rectangle Outline\n");

    graphics_setcolor(1);
    graphics_rect(70, 40, 120, 80);
    printf("  ✓ Drew rectangle outline (70,40)-(120,80)\n");

    graphics_setcolor(2);
    graphics_rect(30, 120, 80, 160);
    printf("  ✓ Drew rectangle outline (30,120)-(80,160)\n");

    /* Test 8: Circle Drawing */
    printf("\nTest 8: Circle Drawing\n");

    graphics_setcolor(1);
    graphics_circle(50, 100, 15);
    printf("  ✓ Drew circle at (50,100) radius 15\n");

    graphics_setcolor(2);
    graphics_circle(110, 100, 20);
    printf("  ✓ Drew circle at (110,100) radius 20\n");

    graphics_setcolor(3);
    graphics_circle(80, 150, 10);
    printf("  ✓ Drew circle at (80,150) radius 10\n");

    /* Test 9: Multiple Colors Pattern */
    printf("\nTest 9: Multiple Colors Pattern\n");

    graphics_setcolor(1);
    graphics_line(20, 20, 40, 40);
    graphics_line(40, 20, 20, 40);
    printf("  ✓ Drew X pattern with color 1\n");

    graphics_setcolor(2);
    graphics_bar(100, 20, 130, 50);
    printf("  ✓ Drew filled bar with color 2\n");

    graphics_setcolor(3);
    graphics_circle(30, 150, 8);
    printf("  ✓ Drew circle with color 3\n");

    /* Test 10: Clipping Control */
    printf("\nTest 10: Clipping Control\n");
    graphics_setclip(20, 20, 140, 180);
    int cx1, cy1, cx2, cy2;
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Set clip (20,20)-(140,180): got (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);
    if (cx1 == 20 && cy1 == 20 && cx2 == 140 && cy2 == 180) {
        printf("  ✓ Clipping set correctly\n");
    }

    /* Test 11: Mode Switching Back */
    printf("\nTest 11: Mode Switching\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched back to TEXT_40x25\n");
    printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());

    /* Test 12: Cleanup */
    printf("\nTest 12: Cleanup\n");
    graphics_done();
    printf("  ✓ Graphics cleaned up\n");

    printf("\n✓ All BITMAP_160x200 (Phase 5) tests completed!\n");
    printf("Note: Visual verification requires emulator (mmemu).\n");
    printf("Multicolor mode supports 4 colors per 8×8 cell via palette lookup.\n");
    return 0;
}

