/* test_constparam_xemu.c — constant-parameter specialization across modules
 * (linked with test_constparam_lib_xemu.c)
 *
 * A static-allocation function whose parameter gets the same constant from
 * every call in its own file may keep that value in pre-initialized storage
 * and let its callers skip the store. That is only safe when no other code
 * can enter it: here f() is exported and called with 9 from this module
 * first, then through callf() (which does f(1)) from its own module.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] f(9) from another module
 *   [1] callf(): f(1) after f(9) (read 9 when the store was skipped)
 *   [2] a static function's constant parameter (still specialized)
 *   [3] address-taken static function called directly and through a pointer
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

int f(int a);
int callf(void);
int use_static(void);
int use_ptr(void);

int main(void) {
    OUT[0] = f(9) == 10;
    OUT[1] = callf() == 2;
    OUT[2] = use_static() == 49;
    OUT[3] = use_ptr() == 1;
    OUT[4] = 0xAA;
    return 0;
}
