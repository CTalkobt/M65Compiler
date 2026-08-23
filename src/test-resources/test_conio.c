/* test_conio.c — Test conio console I/O functions
 *
 * Tests MEGA65 text mode console I/O (conio.h)
 */

#include <stdio.h>
#include <conio.h>
#include <string.h>

int main(void) {
    printf("Testing conio.h functions...\n\n");

    /* Test 1: Screen dimensions */
    printf("Screen Dimensions:\n");
    printf("  Width: %d columns\n", screenwidth());
    printf("  Height: %d rows\n", screenheight());

    /* Test 2: Color constants */
    printf("\nColor Constants:\n");
    printf("  BLACK=%d, WHITE=%d, RED=%d, CYAN=%d\n", BLACK, WHITE, RED, CYAN);
    printf("  MAGENTA=%d, GREEN=%d, BLUE=%d, YELLOW=%d\n", MAGENTA, GREEN, BLUE, YELLOW);

    /* Test 3: Cursor positioning */
    printf("\nCursor Tests:\n");
    gotoxy(5, 5);
    printf("  Cursor at: (%d, %d)\n", wherex(), wherey());
    gotoxy(10, 10);
    printf("  Cursor at: (%d, %d)\n", wherex(), wherey());
    gotoxy(0, 0);

    /* Test 4: Text attributes */
    printf("\nText Attributes:\n");
    textcolor(RED);
    printf("  Red text\n");
    textcolor(WHITE);
    textbackground(BLUE);
    printf("  White text on blue\n");
    textbackground(BLACK);

    /* Test 5: Character output */
    printf("\nCharacter Output:\n");
    gotoxy(0, 12);
    cputs("Direct screen output via cputs()");
    gotoxy(0, 13);
    cputch(0, 13, 'D');
    cputch(1, 13, 'i');
    cputch(2, 13, 'r');
    cputch(3, 13, 'e');
    cputch(4, 13, 'c');
    cputch(5, 13, 't');
    cputch(6, 13, ' ');
    cputch(7, 13, 'c');
    cputch(8, 13, 'h');
    cputch(9, 13, 'a');
    cputch(10, 13, 'r');
    cputch(11, 13, 's');

    /* Test 6: Formatted output */
    printf("\nFormatted Output:\n");
    gotoxy(0, 15);
    cprintf("Integer: %d, Hex: %X, String: %s", 42, 255, "MEGA65");

    /* Test 7: Screen buffer reading */
    printf("\nScreen Buffer Reading:\n");
    gotoxy(0, 17);
    int ch = getch_xy(0, 17);
    int attr = getattr_xy(0, 17);
    printf("Char at (0,17): %c (code %d)\n", ch, ch);
    printf("Attr at (0,17): %02X\n", attr);

    /* Test 8: String positioning */
    printf("\nString Positioning:\n");
    cputsxy(10, 19, "Positioned string");
    cputsxy(10, 20, "Another row");

    printf("\n\n✓ All conio tests completed!\n");
    printf("Note: Actual screen output requires VIC-IV integration.\n");

    return 0;
}

