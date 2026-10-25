/* test_jsr_xemu.c — __jsr / __jsr_raw / __jmp: calls of an address with
 * the CPU's registers
 *
 * Results at $C0C0 (01 = pass):
 *   [0] KERNAL PLOT (set: .Carry = 0, .X row, .Y column) then CHROUT
 *       (.A): the characters in screen RAM where the cursor was put
 *   [1] PLOT read (.Carry = 1, .X -> row, .Y -> column): where CHROUT left it
 *   [2] a routine of the program (__jsr_raw): .A in and out, values that
 *       are not constants, the carry it returns
 *   [3] .AX, .XY, .Q in and out; .Z
 *   [4] the flags after the call (.Zero, .Negative, .Overflow); .Carry in
 *       from a variable; a result in an expression and a condition
 *   [5] __jmp: the routine returns to the caller's caller
 *   [6] a constant expression as the address (KERNAL GETIN)
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define SCREEN ((volatile unsigned char *)0x0800)
#define PLOT 0xFFF0
#define CHROUT 0xFFD2

extern void twice(void);      /* A = A * 2, carry set */
extern void swap_ax(void);    /* A <-> X */
extern void bump_q(void);     /* each of A X Y Z + 1 */
extern void flags_of(void);   /* lda the value in A again: N and Z from it; V set */
extern void landing(void);    /* stores A in landed, returns */
extern volatile unsigned char landed;
extern void z_plus_x(void);   /* A = Z + X */

asm("_twice:\n asl\n sec\n rts\n"
    "_swap_ax:\n pha\n txa\n plx\n rts\n"
    "_bump_q:\n inc\n inx\n iny\n inz\n rts\n"
    "_flags_of:\n bit _v_set\n ora #0\n rts\n"
    "_v_set: .byte $40\n"
    "_landing:\n sta _landed\n rts\n"
    "_landed: .byte 0\n_tmp: .byte 0\n"
    "_z_plus_x:\n tza\n clc\n stx _tmp\n adc _tmp\n rts\n");

static void go(unsigned char v) {
    __jmp(landing, .A = v);     /* landing's rts returns to main */
}

int main(void) {
    unsigned char row = 3, col = 7, r, x, y, n, ok;
    unsigned int w;
    unsigned long q;
    unsigned char c;

    /* [0] the cursor to row 3, column 7, then "AB" */
    __jsr(PLOT, .Carry = 0, .X = row, .Y = col);
    __jsr(CHROUT, .A = 'a');
    c = 'b';
    __jsr(CHROUT, .A = c);
    OUT[0] = SCREEN[3 * 80 + 7] == 1 && SCREEN[3 * 80 + 8] == 2;

    /* [1] the cursor read back */
    __jsr(PLOT, .Carry = 1, .X -> x, .Y -> y);
    OUT[1] = x == 3 && y == 9;

    /* [2] the program's own routine */
    r = __jsr_raw(twice, .A = 21).A;
    n = 100;
    ok = r == 42 && __jsr_raw(twice, .A = n).A == 200 && __jsr_raw(twice, .A = n).Carry == 1;
    OUT[2] = ok;

    /* [3] 16 and 32 bits, Z */
    w = __jsr_raw(swap_ax, .AX = 0x1234).AX;
    ok = w == 0x3412;
    __jsr_raw(swap_ax, .A = 5, .X = 9, .AX -> w);
    ok = ok && w == 0x0509;
    q = __jsr_raw(bump_q, .Q = 0x01020304UL).Q;
    ok = ok && q == 0x02030405UL;
    __jsr_raw(bump_q, .XY = 0x7F10, .A = 0, .Z = 0, .XY -> w, .Z -> c);
    ok = ok && w == 0x8011 && c == 1;
    ok = ok && __jsr_raw(z_plus_x, .Z = 30, .X = 12).A == 42;
    OUT[3] = ok;

    /* [4] flags */
    ok = __jsr_raw(flags_of, .A = 0).Zero == 1 && __jsr_raw(flags_of, .A = 0x80).Negative == 1 &&
         __jsr_raw(flags_of, .A = 1).Zero == 0 && __jsr_raw(flags_of, .A = 1).Overflow == 1;
    c = 1;
    __jsr_raw(swap_ax, .Carry = c, .Carry -> r);
    ok = ok && r == 1;
    c = 0;
    __jsr_raw(swap_ax, .Carry = c, .Carry -> r);
    ok = ok && r == 0;
    if (__jsr_raw(twice, .A = 4).A + 1 != 9) ok = 0;
    if (!__jsr_raw(twice, .A = 4).Carry) ok = 0;
    OUT[4] = ok;

    /* [5] __jmp */
    go(0x5A);
    OUT[5] = landed == 0x5A;

    /* [6] a constant expression as the address (a ROM routine: GETIN, 0 = no key) */
    OUT[6] = __jsr(0xFF00 + 0xE4).A == 0;
    OUT[7] = 0xAA;
    return 0;
}
