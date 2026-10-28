/* test_weak_member_xemu.c — a strong definition replaces a weak one only
 * when its library member is reached from live code (#358)
 *
 * Without USE_Z the member with the strong w() is pulled in only by the
 * unused y() (dropped): x() keeps the weak default (4) and the member (with
 * its 2000-byte array) is dropped. With USE_Z the program calls z(), so the
 * member is live and its strong w() wins (5).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] x() is 4 (5 with USE_Z)
 *   [1] with USE_Z: z() is 1
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
unsigned char x(void);
unsigned char z(void);

int main(void) {
#ifdef USE_Z
    OUT[0] = x() == 5;
    OUT[1] = z() == 1;
#else
    OUT[0] = x() == 4;
    OUT[1] = 1;
#endif
    OUT[2] = 0xAA;
    return 0;
}
