/* test_asm_calls_c_xemu.c — hand-written ZP-convention asm calling C
 * functions with arguments (test_asm_calls_c.s45, #753). In the stack
 * convention ln45 bridges the calls (zp_call -> stack_call), a tail call
 * ca45 made a jmp too
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a tail call (jsr + rts, a jmp): the int argument reaches take()
 *   [1] a call with work after it: two arguments, the int result back
 *   [2] a long argument and a long result
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned int got;
void take(unsigned int x) { got = x; }
unsigned int add2(unsigned int a, unsigned char b) { return a + b; }
unsigned long sum_long(unsigned long a, unsigned int b) { return a + b; }

void asm_tail(void);
unsigned int asm_call2(void);
unsigned long asm_call_long(void);

int main(void) {
    OUT[3] = 0;
    asm_tail();
    OUT[0] = got == 0x1234;
    OUT[1] = asm_call2() == 0x0102 + 7 + 1;
    OUT[2] = asm_call_long() == 0x11223346UL;
    OUT[3] = 0xAA;
    return 0;
}
