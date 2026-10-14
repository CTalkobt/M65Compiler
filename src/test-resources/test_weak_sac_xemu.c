/* test_weak_sac_xemu.c — a weak function with static storage overridden by
 * a strong one in another module (test_weak_sac_b.c), while the weak one's
 * object stays linked (other code of it is used): its storage symbols are
 * weak too, so the two do not clash.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the strong definition is called
 *   [1] the weak module's other function works
 *   [2] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

#pragma weak wf
int wf(int x) { int k[3]; k[0] = x; k[1] = 1; k[2] = 2; return k[0] + k[1] + k[2]; }   /* x + 3 */

__attribute__((noinline)) int keep(int x) { int t = x * 2; return t + 1; }

int main(void) {
    OUT[0] = wf(10) == 110;     /* the strong one: x * 11 */
    OUT[1] = keep(4) == 9;
    OUT[2] = 0xAA;
    return 0;
}
