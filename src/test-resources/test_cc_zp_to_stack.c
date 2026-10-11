// Test: ZP-convention caller -> stack-convention callee in another module
// Built with -fzpcall; its callees (test_cc_zp_to_stack_callee.c) with
// -fno-zpcall. The caller passes its arguments in the ZP block, so ln45
// must bridge each call: with --no-thunks it reports "calling convention
// mismatch", by default it generates a thunk that pushes them for the callee.
//
// Results at $4000: stack_add(1000, 234) = $04D2,
// stack_mix(5, $12345678, $100) = $12345773, then $FF.

int stack_add(int a, int b);
long stack_mix(char c, long l, int i);

volatile unsigned char *results = (volatile unsigned char *)0x4000;

int main(void) {
    int r = stack_add(1000, 234);
    long m = stack_mix(5, 0x12345678L, 0x100);
    results[0] = r;
    results[1] = r >> 8;
    results[2] = m;
    results[3] = m >> 8;
    results[4] = m >> 16;
    results[5] = m >> 24;
    results[6] = 0xFF;
    return 0;
}
