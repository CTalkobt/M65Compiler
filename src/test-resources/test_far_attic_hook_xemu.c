/* test_far_attic_hook_xemu.c — far_attic_hook on its own: far_attic_init
 * sets up attic RAM for far_attic_malloc only; after far_attic_hook,
 * far_malloc(.., BANK_ATTIC), far_size_long and far_free reach it too
 * (test_far_attic_xemu.c has it through far_attic_enable)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] far_attic_init: far_attic_malloc works, far_malloc(.., BANK_ATTIC)
 *       is still null (not hooked)
 *   [1] far_attic_hook: far_malloc(.., BANK_ATTIC) a block in attic RAM,
 *       its size, far_free gives it back
 *   [2] $AA sentinel
 */
#include <far_ptr.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char meta[FAR_ATTIC_META_SIZE(12)];

int main(void) {
    far_ptr_t a, b;
    unsigned long n;
    unsigned char ok;

    ok = far_attic_init(0x8000000UL, 0x80FFFFFUL, 12, meta, sizeof meta) == 0x100000UL;
    a = far_attic_malloc(100u);
    b = far_malloc(100u, BANK_ATTIC);
    ok = ok && !far_is_null(a) && far_is_null(b);
    far_attic_free(a);
    OUT[0] = ok;

    far_attic_hook();
    n = far_attic_available();
    b = far_malloc(5000u, BANK_ATTIC);
    ok = !far_is_null(b) && far_to_linear(b) >= 0x8000000UL && far_size_long(b) == 8192UL &&
         far_attic_available() == n - 8192UL;
    far_free(b);
    OUT[1] = ok && far_attic_available() == n;
    OUT[2] = 0xAA;
    return 0;
}
