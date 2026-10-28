/* test_extern_field_b.c — the other file of test_extern_field_xemu.c (#356):
 * the struct and array the main file reaches as externs */
struct p { unsigned char a[86]; unsigned char x, y; unsigned int hp; };
struct p pl = { { 0 }, 7, 9, 1000 };
unsigned char tab[40] = { 0, 1, 2, 3 };
unsigned char get_x(void) { return pl.x; }
unsigned int get_hp(void) { return pl.hp; }
