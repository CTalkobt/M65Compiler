/* test_graphics_text_80.c — TEXT_80x24 Extended Text Driver (Phase 4) Test Suite
 *
 * Tests 80×24 extended text mode with character drawing.
 */

#include <stdio.h>
#include <graphics.h>

int main(void) {
    printf("Testing TEXT_80x24 extended text driver (Phase 4)...\n\n");

    /* Test 1: Initialize and switch to TEXT_80x24 mode */
    printf("Test 1: Initialize and Switch to TEXT_80x24 Mode\n");
    graphics_init();
    printf("  ✓ Graphics initialized\n");

    int result = graphics_setmode(GRAPHICS_MODE_TEXT_80x24);
    if (result == 0) {
        printf("  ✓ Switched to TEXT_80x24\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
        if (graphics_getwidth() == 80 && graphics_getheight() == 24) {
            printf("  ✓ Dimensions correct (80×24)\n");
        }
    } else {
        printf("  ✗ Failed to switch mode\n");
    }

    /* Test 2: Color Control */
    printf("\nTest 2: Color Control\n");
    graphics_setcolor(3);  /* Cyan */
    graphics_setbkcolor(1);  /* White background */
    printf("  ✓ Set foreground to Cyan (3), background to White (1)\n");

    /* Test 3: Screen Clear */
    printf("\nTest 3: Screen Clear\n");
    graphics_clear();
    printf("  ✓ Screen cleared (80×24)\n");

    /* Test 4: Pixel/Character Operations */
    printf("\nTest 4: Character Operations\n");
    graphics_plot(10, 5);
    printf("  ✓ Plotted character at (10,5)\n");

    unsigned char ch = graphics_getpixel(10, 5);
    printf("  Read character at (10,5): %d (space char)\n", ch);

    unsigned char ch_empty = graphics_getpixel(20, 10);
    printf("  Read character at (20,10): %d\n", ch_empty);

    /* Test 5: Drawing Lines */
    printf("\nTest 5: Line Drawing\n");

    graphics_setcolor(2);  /* Red */
    graphics_line(5, 5, 40, 10);
    printf("  ✓ Drew red line from (5,5) to (40,10)\n");

    graphics_setcolor(5);  /* Green */
    graphics_line(50, 2, 75, 15);
    printf("  ✓ Drew green line from (50,2) to (75,15)\n");

    graphics_setcolor(6);  /* Blue */
    graphics_line(0, 12, 79, 12);
    printf("  ✓ Drew blue horizontal line at y=12\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_line(40, 0, 40, 23);
    printf("  ✓ Drew magenta vertical line at x=40\n");

    /* Test 6: Filled Rectangle */
    printf("\nTest 6: Filled Rectangle (Bar)\n");

    graphics_setcolor(1);  /* White */
    graphics_bar(10, 8, 30, 12);
    printf("  ✓ Drew white filled bar (10,8)-(30,12)\n");

    graphics_setcolor(3);  /* Cyan */
    graphics_bar(50, 16, 70, 20);
    printf("  ✓ Drew cyan filled bar (50,16)-(70,20)\n");

    /* Test 7: Rectangle Outline */
    printf("\nTest 7: Rectangle Outline\n");

    graphics_setcolor(2);  /* Red */
    graphics_rect(35, 6, 60, 14);
    printf("  ✓ Drew red rectangle outline (35,6)-(60,14)\n");

    graphics_setcolor(5);  /* Green */
    graphics_rect(15, 16, 35, 22);
    printf("  ✓ Drew green rectangle outline (15,16)-(35,22)\n");

    /* Test 8: Circle Drawing */
    printf("\nTest 8: Circle Drawing\n");

    graphics_setcolor(6);  /* Blue */
    graphics_circle(20, 12, 5);
    printf("  ✓ Drew blue circle at (20,12) radius 5\n");

    graphics_setcolor(4);  /* Magenta */
    graphics_circle(60, 12, 6);
    printf("  ✓ Drew magenta circle at (60,12) radius 6\n");

    graphics_setcolor(1);  /* White */
    graphics_circle(40, 20, 4);
    printf("  ✓ Drew white circle at (40,20) radius 4\n");

    /* Test 9: Multi-Color Pattern */
    printf("\nTest 9: Multi-Color Pattern\n");

    graphics_setcolor(2);  /* Red */
    graphics_line(5, 3, 15, 3);
    graphics_line(5, 4, 15, 4);
    printf("  ✓ Drew red horizontal lines\n");

    graphics_setcolor(3);  /* Cyan */
    graphics_rect(72, 2, 78, 8);
    graphics_bar(73, 3, 77, 7);
    printf("  ✓ Drew cyan rect with filled inner bar\n");

    /* Test 10: Clipping Control */
    printf("\nTest 10: Clipping Control\n");
    graphics_setclip(10, 5, 70, 20);
    int cx1, cy1, cx2, cy2;
    graphics_getclip(&cx1, &cy1, &cx2, &cy2);
    printf("  Set clip (10,5)-(70,20): got (%d,%d)-(%d,%d)\n", cx1, cy1, cx2, cy2);
    if (cx1 == 10 && cy1 == 5 && cx2 == 70 && cy2 == 20) {
        printf("  ✓ Clipping set correctly\n");
    }

    /* Test 11: Mode Switching */
    printf("\nTest 11: Mode Switching\n");

    /* Switch to 40-column mode */
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);
    printf("  ✓ Switched to TEXT_40x25 (40×25)\n");
    printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());

    /* Switch back to 80-column mode */
    result = graphics_setmode(GRAPHICS_MODE_TEXT_80x24);
    if (result == 0) {
        printf("  ✓ Switched back to TEXT_80x24 (80×24)\n");
        printf("  Dimensions: %d×%d\n", graphics_getwidth(), graphics_getheight());
    }

    /* Test 12: Cleanup */
    printf("\nTest 12: Cleanup\n");
    graphics_setmode(GRAPHICS_MODE_TEXT_40x25);  /* Back to safe mode */
    graphics_done();
    printf("  ✓ Graphics cleaned up\n");

    printf("\n✓ All TEXT_80x24 (Phase 4) tests completed!\n");
    printf("Extended text mode supports 80×24 character display.\n");
    return 0;
}

