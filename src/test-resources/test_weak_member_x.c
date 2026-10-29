/* test_weak_member_x.c — a library member (#358): x() is used and calls
 * w(), whose weak default is here; y() is never used and refers to z(),
 * the other member's */
unsigned char z(void);
#pragma weak w
unsigned char w(void) { return 4; }
unsigned char x(void) { return w(); }
unsigned char y(void) { return z(); }
