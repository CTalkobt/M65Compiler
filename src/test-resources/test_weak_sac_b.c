/* test_weak_sac_b.c — the strong wf of test_weak_sac_xemu.c */
int wf(int x) { int m[2]; m[0] = x * 10; m[1] = x; return m[0] + m[1]; }   /* x * 11 */
