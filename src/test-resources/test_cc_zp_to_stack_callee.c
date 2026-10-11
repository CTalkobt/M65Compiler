// Stack-convention callees for test_cc_zp_to_stack.c (built with -fno-zpcall)

int stack_add(int a, int b) {
    return a + b;
}

long stack_mix(char c, long l, int i) {
    return l - c + i;
}
