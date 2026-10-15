/* Computed goto and static allocation: cc45 exports the labels of a
 * &&label table (run__lbl_op_call...); ln45 took each as a new function, so
 * the calls after one were not run()'s: run() and helper() got the same
 * static storage (test_regression.sh checks the link map). */

static unsigned char prog[] = { 0, 1, 0, 1, 2 };
static unsigned int helper(unsigned int v) {
    unsigned int a = v * 3, b = a + 7, c = b ^ 0x55;
    return a + b + c;
}
static unsigned int run(const unsigned char *code) {
    static void *ops[] = { &&op_inc, &&op_call, &&op_end };
    unsigned char ip = 0;
    unsigned int acc = 1, keep = 1234;
    unsigned char hist[8] = { 9, 8, 7, 6, 5, 4, 3, 2 };   /* run's storage, live across helper() */
    goto *ops[code[ip++]];
op_inc:  acc++; goto *ops[code[ip++]];
op_call: acc = helper(acc) + keep; goto *ops[code[ip++]];
op_end:  for (ip = 0; ip < 8; ip++) if (hist[ip] != 9 - ip) return 0;
         return acc + keep;
}
int main(void) {
    unsigned int want = 1, k = 1234;
    unsigned char mark[6] = { 1, 2, 3, 4, 5, 6 };   /* main's storage: kept across run() */
    unsigned char i, ok = 1;
    want++; want = (want * 3) + (want * 3 + 7) + ((want * 3 + 7) ^ 0x55) + k;
    want++; want = (want * 3) + (want * 3 + 7) + ((want * 3 + 7) ^ 0x55) + k;
    want += k;
    unsigned char r = run(prog) == want;
    for (i = 0; i < 6; i++) if (mark[i] != i + 1) ok = 0;
    return r && ok ? 0 : 1;
}
