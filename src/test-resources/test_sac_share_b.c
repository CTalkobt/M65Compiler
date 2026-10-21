/* test_sac_share_b.c — the functions test_sac_share_xemu.c calls (in a file
 * of their own: not inlined) */
unsigned int id(unsigned int v) { return v; }
unsigned int sum_of(unsigned int *p) { return p[0] + p[1]; }
