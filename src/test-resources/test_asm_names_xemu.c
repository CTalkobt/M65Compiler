/* test_asm_names_xemu.c — the names inline assembly reaches C objects by
 * (cc45.md, "Inline assembly: names"): a global (_name), a parameter of the
 * ZP convention (@_p_val, its ZP slot), a parameter of a static-allocation
 * function (_func__param_val), a register local (@_l_i). Replaces mmemu's
 * test_inline_asm.c, which used the stack-relative forms (ldax _p_x, sp)
 * the docs now warn against.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] @_p_val: an int parameter stored to a global from asm, then 0
 *   [1] @_p_f: a char parameter
 *   [2] _func__param_val: the SAC parameter's static copy (ZP build: the
 *       asm reads it after the entry has stored it)
 *   [3] @_l_i: a register local written from asm, read in C
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

volatile int g_value;
volatile char g_flag;

__attribute__((noinline)) void store_to_global(int val) {
    __asm__("lda @_p_val");
    __asm__("ldx @_p_val+1");
    __asm__("sta _g_value");
    __asm__("stx _g_value+1");
}

__attribute__((noinline)) void set_flag(char f) {
    __asm__("lda @_p_f");
    __asm__("sta _g_flag");
}

__attribute__((noinline)) int doubled(int val) {
    int r;
    (void)val;   /* (read by the asm only) */
    __asm__("lda _doubled__param_val");
    __asm__("asl");
    __asm__("sta _g_value");
    __asm__("lda _doubled__param_val+1");
    __asm__("rol");
    __asm__("sta _g_value+1");
    r = g_value;
    return r;
}

__attribute__((noinline)) unsigned char reg_local(void) {
    register unsigned char i = 1;
    __asm__("lda #$5A");
    __asm__("sta @_l_i");
    return i;
}

int main(void) {
    store_to_global(0xAB);
    OUT[0] = g_value == 0xAB;
    store_to_global(0);
    if (g_value != 0) OUT[0] = 0;
    set_flag(0x42);
    OUT[1] = g_flag == 0x42;
    OUT[2] = doubled(0x123) == 0x246;
    OUT[3] = reg_local() == 0x5A;
    OUT[4] = 0xAA;
    return 0;
}
