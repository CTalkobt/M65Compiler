/* test_isr_jsr_xemu.c — an __interrupt handler that uses __jsr leaves the
 * register bytes of a __jsr the interrupted code is in the middle of as
 * they were (the handler saves and restores them: __jsr_isr_save)
 *
 * The handler is run as an interrupt would run it (its RTI returns): the
 * return address and the flags pushed, then a jump to it.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the handler ran (its __jsr gave its own result)
 *   [1] ___jsr_a, ___jsr_x, ___jsr_q+3 and ___cbm_target+1 as they were
 *   [2] a __jsr after it works as before
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

extern void twice(void);   /* A = A * 2 */
asm("_twice:\n asl\n rts");

unsigned char in_handler, got_a, got_x, got_q3, got_t1;

unsigned long q;

__interrupt void handler(void) {
    /* its own values and results in the same bytes */
    in_handler = __jsr_raw(twice, .A = 21, .X = 0x34, .Q -> q).A;
}

int main(void) {
    __jsr_raw(twice, .A = 1);                      /* (the bytes linked) */
    /* a __jsr's bytes, as if it were between storing them and the call */
    asm(" .extern ___jsr_a, ___jsr_x, ___jsr_q, ___cbm_target\n lda #$A5\n sta ___jsr_a\n lda #$5A\n sta ___jsr_x\n lda #$C3\n sta ___jsr_q+3\n"
        " lda #$3C\n sta ___cbm_target+1");
    /* the interrupt: RTI returns to @back */
    asm(" lda #>@back\n pha\n lda #<@back\n pha\n php\n jmp _handler\n@back:");
    asm(" lda ___jsr_a\n sta _got_a\n lda ___jsr_x\n sta _got_x\n lda ___jsr_q+3\n sta _got_q3\n"
        " lda ___cbm_target+1\n sta _got_t1");
    OUT[0] = in_handler == 42;
    OUT[1] = got_a == 0xA5 && got_x == 0x5A && got_q3 == 0xC3 && got_t1 == 0x3C;
    OUT[2] = __jsr_raw(twice, .A = 50).A == 100;
    OUT[3] = 0xAA;
    return 0;
}
