// Force .fp instructions by initializing multiple locals
int test_fp_with_init(void) {
    int a = 10;
    int b = 20;
    int c = 30;
    char d = 5;
    return a + b + c + d;
}

int test_fp_multi_byte(void) {
    long x = 0x12345678;
    int y = 256;
    return x + y;
}
