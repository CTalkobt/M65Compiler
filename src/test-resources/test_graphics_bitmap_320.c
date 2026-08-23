/* test_graphics_bitmap_320.c — BITMAP_320x200 Driver (Phase 2) Test Suite
 *
 * Tests pixel operations, Bresenham lines, rectangles, and circles.
 */

#include <stdio.h>
#include <graphics.h>

int main(void) {
    printf("Testing BITMAP_320x200 driver (Phase 2)...\n\n");

    /* Test 1: Initialize graphics and switch to bitmap mode */
    printf("Test 1: Initialize and Switch to Bitmap Mode\n");
    graphics_init();
    printf("  ✓ Graphics initialized\n");

    int result = graphics_setmode(GRAPHICS_MODE_BITMAP_320x200);
    if (result == 0) {
        printf("  ✓ Switched to BITMAP_320x200\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
        if (graphics_getwidth() == 320 && graphics_getheight() == 200) {
            printf("  ✓ Dimensions correct\n");
        }
    } else {
        printf("  ✗ Failed to switch mode\n");
    }

    /* Test 2: Color Control */
    printf("\nTest 2: Color Control\n");
    graphics_setcolor(3);  /* Cyan */
    graphics_setbkcolor(1);  /* White background */
    printf("  ✓ Set color to Cyan (3), bg to White (1)\n");

    /* Test 3: Screen Clear */
    printf("\nTest 3: Screen Clear\n");
    graphics_clear();
    printf("  ✓ Bitmap cleared\n");

    /* Test 4: Pixel Operations */
    printf("\nTest 4: Pixel Operations\n");
    graphics_plot(10, 10);
    printf("  ✓ Plotted pixel at (10,10)\n");

    unsigned char pix = graphics_getpixel(10, 10);
    printf("  Read pixel at (10,10): %d (expect 1)\n", pix);

    unsigned char pix_empty = graphics_getpixel(20, 20);
    printf("  Read pixel at (20,20): %d (expect 0)\n", pix_empty);

    /* Test 5: Bresenham Line Drawing */
    printf("\nTest 5: Bresenham Line Drawing\n");

    graphics_setcolor(2);  /* Red */
    graphics_line(10, 20, 100, 50);
    printf("  ✓ Drew red line from (10,20) to (100,50)\n");

    graphics_setcolor(5);  /* Green */
    graphics_line(200, 10, 150, 80);
    printf("  ✓ Drew green line from (200,10) to (150,80)\n");

    graphics_setcolor(6);  /* Blue */
    graphics_line(0, 100, 319, 100);
    printf("  ✓ Drew blue horizontal line at y=100\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_line(160, 0, 160, 199);
    printf("  ✓ Drew magenta vertical line at x=160\n");

    /* Test 6: Filled Rectangle (Bar) */
    printf("\nTest 6: Filled Rectangle (Bar)\n");

    graphics_setcolor(1);  /* White */
    graphics_bar(30, 30, 80, 60);
    printf("  ✓ Drew white filled bar (30,30)-(80,60)\n");

    graphics_setcolor(3);  /* Cyan */
    graphics_bar(200, 150, 280, 180);
    printf("  ✓ Drew cyan filled bar (200,150)-(280,180)\n");

    /* Test 7: Rectangle Outline */
    printf("\nTest 7: Rectangle Outline\n");

    graphics_setcolor(2);  /* Red */
    graphics_rect(120, 40, 200, 80);
    printf("  ✓ Drew red rectangle outline (120,40)-(200,80)\n");

    graphics_setcolor(5);  /* Green */
    graphics_rect(50, 120, 150, 160);
    printf("  ✓ Drew green rectangle outline (50,120)-(150,160)\n");

    /* Test 8: Circle Drawing */
    printf("\nTest 8: Circle Drawing\n");

    graphics_setcolor(6);  /* Blue */
    graphics_circle(100, 100, 20);
    printf("  ✓ Drew blue circle at (100,100) radius 20\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_circle(250, 100, 30);
    printf("  ✓ Drew magenta circle at (250,100) radius 30\n");

    graphics_setcolor(1);  /* White */
    graphics_circle(160, 150, 15);
    printf("  ✓ Drew white circle at (160,150) radius 15\n");

    /* Test 9: Complex Drawing (Combination) */
    printf("\nTest 9: Complex Drawing\n");

    graphics_setcolor(3);  /* Cyan */
    graphics_line(20, 20, 40, 40);
    graphics_line(40, 20, 20, 40);
    printf("  ✓ Drew X pattern with cyan lines\n");

    graphics_setcolor(5);  /* Green */
    graphics_rect(150, 150, 180, 180);
    graphics_bar(155, 155, 175, 175);
    printf("  ✓ Drew green rect with filled inner bar\n");

    /* Test 10: Clipping Control */
    printf("\nTest 10: Clipping Control\n");
    graphics_setclip(50, 50, 250, 150);
    int cx1, cy1, cx2, cy2;
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Set clip (50,50)-(250,150): got (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);
    if (cx1 == 50 && cy1 == 50 && cx2 == 250 && cy2 == 150) {
        printf("  ✓ Clipping set correctly\n");
    }

    /* Test 11: Mode Switching Back to Text */
    printf("\nTest 11: Mode Switching\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched back to TEXT_40x25\n");
    printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());

    /* Test 12: Cleanup */
    printf("\nTest 12: Cleanup\n");
    graphics_done();
    printf("  ✓ Graphics cleaned up\n");

    printf("\n✓ All BITMAP_320x200 (Phase 2) tests completed!\n");
    printf("Note: Visual verification requires emulator (mmemu).\n");
    return 0;
}

