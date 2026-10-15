/* test_lib_regress_xemu.c — library assembly damaged by a conversion
 * (values off by 13) and struct compound-assignment operators
 *
 * Results at $C0C0 (01 = pass):
 *   [0] isblank: tab and space are blank, $16 is not
 *   [1] atoi skips tabs and newlines before the number
 *   [2] key_pressed selects the key's keyboard column: SPACE column 4 ($EF),
 *       RUN/STOP column 7 ($7F), A column 2 ($FB)
 *   [3] a += b on a 4-byte struct calls its operator+= (it was a 32-bit
 *       integer add: x's carry went into y); a 6-byte struct's += and -=
 *   [4] $AA sentinel
 */
#include <ctype.h>
#include <stdlib.h>
#include <mega65.h>
#define OUT ((volatile unsigned char *)0xC0C0)

struct V2 {
    signed int x, y;
    void operator+=(struct V2 o) { this->x = this->x + o.x; this->y = this->y + o.y; }
};
struct V3 {
    signed int x, y, z;
    void operator+=(struct V3 o) { this->x = this->x + o.x; this->y = this->y + o.y; this->z = this->z + o.z; }
    void operator-=(struct V3 o) { this->x = this->x - o.x; this->y = this->y - o.y; this->z = this->z - o.z; }
};
struct V2 p, v;
struct V3 a, b;

int main(void) {
    unsigned char c1, c2, c3;
    OUT[0] = isblank('\t') && isblank(' ') && !isblank(0x16) && !isblank('a');
    OUT[1] = atoi("\t\n 42") == 42 && atoi("7") == 7;

    __asm__("sei");
    key_pressed(KEY_SPACE); c1 = cia1->pra;
    key_pressed(KEY_STOP);  c2 = cia1->pra;
    key_pressed(KEY_A);     c3 = cia1->pra;
    cia1->pra = 0xFF;
    __asm__("cli");
    OUT[2] = c1 == 0xEF && c2 == 0x7F && c3 == 0xFB;

    p.x = 102; p.y = 200; v.x = -2; v.y = 0;
    p += v;
    a.x = 100; a.y = 200; a.z = 300; b.x = -2; b.y = 0; b.z = 5;
    a += b;
    OUT[3] = p.x == 100 && p.y == 200 && a.x == 98 && a.y == 200 && a.z == 305;
    a -= b;
    OUT[3] = OUT[3] && a.x == 100 && a.z == 300;

    OUT[4] = 0xAA;
    return 0;
}
