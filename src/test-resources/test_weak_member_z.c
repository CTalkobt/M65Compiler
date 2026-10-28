/* test_weak_member_z.c — the other library member (#358): z(), reached
 * only from the unused y() unless the program calls it, and a strong w() */
unsigned char big[2000] = { 1 };
unsigned char z(void) { return big[0]; }
unsigned char w(void) { return big[0] + 4; }
