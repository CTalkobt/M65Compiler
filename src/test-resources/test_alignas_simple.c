// Simple test for _Alignas on local variable
int test_alignas_4byte(void) {
    char x;
    _Alignas(4) int y;
    return 0;
}
