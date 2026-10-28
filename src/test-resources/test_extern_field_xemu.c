/* test_extern_field_xemu.c — fields of an extern struct and elements of an
 * extern array at constant offsets are absolute addresses (#356: lda
 * _pl+86, sta _pl+86, as for a struct defined in the file); the other file
 * (test_extern_field_b.c) defines them
 *
 * Results at $C0C0 (01 = pass):
 *   [0] fields read: a byte, the next one, an int
 *   [1] fields written, seen by the other file
 *   [2] read-modify-write (++, +=) of a field
 *   [3] an extern array's elements at constant indices
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
struct p { unsigned char a[86]; unsigned char x, y; unsigned int hp; };
extern struct p pl;
extern unsigned char tab[40];
unsigned char get_x(void);
unsigned int get_hp(void);

int main(void) {
    OUT[0] = pl.x == 7 && pl.y == 9 && pl.hp == 1000 && pl.a[85] == 0;
    pl.x = 42;
    pl.hp = 0x1234;
    OUT[1] = get_x() == 42 && get_hp() == 0x1234;
    pl.y++;
    pl.hp += 0x100;
    pl.a[3] = 5;
    pl.a[3] += 2;
    OUT[2] = pl.y == 10 && pl.hp == 0x1334 && pl.a[3] == 7;
    tab[39] = tab[3] + 10;
    OUT[3] = tab[39] == 13 && tab[1] == 1;
    OUT[4] = 0xAA;
    return 0;
}
