/* test_addr_cond_xemu.c — a global's address as a condition tests all its
 * bytes (the branch tested only the low one: an address $0800 was false;
 * #315's far_attic_init check of __ovl_attic_bank), and constant conditions
 *
 * Results at $C0C0 (01 = pass):
 *   [0] if ((unsigned int)page8), page8 at $0800: taken
 *   [1] if ((unsigned int)nul), nul at 0: not taken
 *   [2] !(unsigned int)page8 and (unsigned int)page8 != 0
 *   [3] while ((unsigned int)page8) runs; if (256) taken, if (0) not
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

asm("_page8 = $0800\n_nul = 0");
extern char page8[], nul[];

static unsigned char taken(void) { if ((unsigned int)page8) return 1; return 0; }
static unsigned char none(void) { if ((unsigned int)nul) return 1; return 0; }

int main(void) {
    unsigned char n = 0;
    OUT[0] = taken() == 1;
    OUT[1] = none() == 0;
    OUT[2] = !(unsigned int)page8 == 0 && ((unsigned int)page8 != 0);
    while ((unsigned int)page8) { if (++n == 3) break; }
    OUT[3] = n == 3;
    if (256) OUT[3] &= 1; else OUT[3] = 0;
    if (0) OUT[3] = 0;
    OUT[4] = 0xAA;
    return 0;
}
