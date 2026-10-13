/* test_gc_sections_xemu.c -- ln45 drops the unused functions of a
 * sectioned library member (cc45 -ffunction-sections)
 *
 * test_xemu.sh compiles lib/stdlib/conio.c with -ffunction-sections into a
 * library of its own and links this program with it: of conio's functions
 * only gotoxy, wherex, wherey and what they call stay (cprintf and the
 * printf machinery it needs go, and so do the dropped functions' static
 * storage blocks and strings), and the program still works.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] gotoxy then wherex   [1] wherey   [2] $AA sentinel
 */
#include <conio.h>
#define OUT ((volatile unsigned char *)0xC0C0)
int main(void) {
    gotoxy(3, 5);
    OUT[0] = wherex() == 3;
    OUT[1] = wherey() == 5;
    OUT[2] = 0xAA;
    return 0;
}
