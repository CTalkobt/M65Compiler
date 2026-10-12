/* test_init_features_xemu.c — a program's own init_features() replaces
 * crt0's weak default and runs before main
 *
 * (crt0's call used to be bound to its own weak rts when crt0 was
 * assembled; references to .weak symbols are now relocated by name)
 *
 * Results at $C0C0: [0] 01 = init_features ran before main, [1] $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char seen;

void init_features(void) { seen = 0x5A; }

int main(void) {
    OUT[0] = seen == 0x5A;
    OUT[1] = 0xAA;
    return 0;
}
