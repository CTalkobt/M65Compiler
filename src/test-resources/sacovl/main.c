/* sacovl/main.c — the SAC overlay shares the static-allocation blocks of
 * functions in code overlays too (Issue #302)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] o1_f keeps a local array across a call overlay o1 -> resident
 *       r_mid -> overlay o2 (o2_g fills its own locals): o1_f's array is
 *       intact (the two blocks must not overlap)
 *   [1] o1_h and o2_k, never active together, each compute from their
 *       locals (their blocks may share storage: test_xemu.sh checks they do)
 *   [2] the chain again after [1] ran (no stale values)
 *   [3] o2_g called directly from resident code
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int o1_f(unsigned char x);
unsigned int o1_h(unsigned char x);
unsigned int o2_g(unsigned char y);
unsigned int o2_k(unsigned char y);

/* resident: called from overlay o1, calls into overlay o2 */
unsigned int r_mid(unsigned char y) { return o2_g(y) + 1; }

int main(void) {
    /* o1_f(3): its 16 bytes of 3 = 48, plus r_mid(7) = o2_g(7) + 1 */
    unsigned int g7 = 7u * 16u + 5u;            /* o2_g(7): 16 bytes of 7, + 5 */
    OUT[0] = o1_f(3) == 48 + g7 + 1;
    OUT[1] = o1_h(2) == 2u * 12u && o2_k(9) == 9u * 12u + 1u;
    OUT[2] = o1_f(4) == 64 + g7 + 1;
    OUT[3] = o2_g(7) == g7;
    OUT[4] = 0xAA;
    return 0;
}
