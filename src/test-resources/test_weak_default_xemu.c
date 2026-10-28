/* test_weak_default_xemu.c — #pragma weak defaults called from their own
 * file and replaced by strong definitions from other files
 * (test_weak_default_b.c, test_weak_default_asm.s45): a call from this file
 * must reach the strong one, not a copy of the weak body (#357: it was
 * inlined at -O2) nor the weak one's static storage or __sac entry (a
 * same-file call in the stack convention stored the arguments there).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a small weak body (inlined before #357)
 *   [1] parameters: the strong one names them otherwise and has locals
 *   [2] constant arguments
 *   [3] the strong one in assembly (no __sac entry, no static storage)
 *   [4] the strong one (assembly) clobbers A/X/Y/Z and the register pool,
 *       the weak one nothing: a value live across the call survives
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

#pragma weak w_small
unsigned char w_small(void) { return 1; }                   /* strong: 2 */

#pragma weak w_params
unsigned int w_params(unsigned int a, unsigned char b) { return a + b + 1000; }   /* strong: a - b */

#pragma weak w_const
unsigned char w_const(unsigned char k) { return k; }        /* strong: k * 3 */

#pragma weak w_asm
unsigned int w_asm(unsigned int a) { return a + 1; }        /* strong (asm): a * 2 */

#pragma weak w_clobber
void w_clobber(void) { }                                    /* strong (asm): clobbers */

__attribute__((noinline)) unsigned int clobber_loop(unsigned int n, unsigned int b) {
    unsigned int s = 0, i;
    for (i = 0; i < n; i++) { s += b; w_clobber(); s += i; }
    return s;
}

int main(void) {
    OUT[0] = w_small() == 2;
    OUT[1] = w_params(300, 7) == 293;
    OUT[2] = (unsigned char)(w_const(5) + w_const(5)) == 30;
    OUT[3] = w_asm(10) + w_asm(11) == 42;
    OUT[4] = clobber_loop(10, 3) == 75;
    OUT[5] = 0xAA;
    return 0;
}
