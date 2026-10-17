/* test_zp_globals_b_xemu.c — the second module of test_zp_globals_xemu.c */
__zp int zother = 5;
__zp unsigned char ztab[4] = { 10, 20, 30, 40 };
static __zp unsigned char zs = 1;   /* the other module's zs is another variable */

void set_via(int *p, int v) { *p = v; }
int bump_other(void) { zs++; return ++zother; }
