/* test_section_xemu.c — sections (#70): #pragma section("NAME", address)
 * and __section("NAME") variables, placed by ln45 at the section's address
 * (screen RAM at $0800 in 80-column mode)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] an array in a section: at the address, written through the
 *       variable and read through a pointer; the next variable follows it
 *   [1] initial values copied there at startup (a string, an int table)
 *   [2] a file static and a function's static (= 0: copied too) in a
 *       section, in declaration order after the others; the static counts
 *       across calls
 *   [3] GCC's __attribute__((section("NAME"))): placed when NAME is
 *       declared, an ordinary variable when it is not
 *   [4] a struct in a section, through a pointer to it
 *   [5] $AA sentinel
 */
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define AT(a) ((volatile unsigned char *)(a))

#pragma section("SCREEN", 0x0800)
#pragma section("TABLES", $0C00)

__section("SCREEN") char screen[400];
__section("SCREEN") char row[80];
__section("TABLES") char msg[] = "HELLO";
__section("TABLES") const signed int squares[4] = { 0, 1, 4, -9 };
static __section("TABLES") unsigned char mark = 7;
char gcc_placed[2] __attribute__((section("TABLES")));
char gcc_ignored[2] __attribute__((section("ELSEWHERE")));

struct pt { signed int x, y; };
__section("SCREEN") struct pt where;

static unsigned char count(void) {
    static __section("TABLES") unsigned char n = 0;   /* (copied: section memory is not cleared) */
    return ++n;
}

static void move(struct pt *p, signed int dx) { p->x += dx; p->y -= dx; }

int main(void) {
    unsigned char ok, i;

    for (i = 0; i < 4; i++) screen[i] = i + 1;
    row[0] = 0x55;
    OUT[0] = (unsigned)screen == 0x0800 && *AT(0x0800) == 1 && *AT(0x0803) == 4 && (unsigned)row == 0x0800 + 400 &&
             *AT(0x0800 + 400) == 0x55 && sizeof screen == 400;

    ok = (unsigned)msg == 0x0C00 && strcmp((const char *)AT(0x0C00), "HELLO") == 0 && sizeof msg == 6;
    if ((unsigned)squares != 0x0C06 || squares[2] != 4 || squares[3] != -9 || *AT(0x0C06 + 6) != (unsigned char)-9) ok = 0;
    OUT[1] = ok;

    ok = (unsigned)&mark == 0x0C0E && mark == 7;
    mark = 9;
    if (*AT(0x0C0E) != 9) ok = 0;
    if (count() != 1 || count() != 2 || *AT(0x0C11) != 2) ok = 0;
    OUT[2] = ok;

    OUT[3] = (unsigned)gcc_placed == 0x0C0F && ((unsigned)gcc_ignored < 0x0800 || (unsigned)gcc_ignored > 0x0FFF);

    where.x = 10;
    where.y = 20;
    move(&where, 5);
    OUT[4] = (unsigned)&where == 0x0800 + 480 && where.x == 15 && where.y == 15 && *AT(0x0800 + 480) == 15;
    OUT[5] = 0xAA;
    return 0;
}
