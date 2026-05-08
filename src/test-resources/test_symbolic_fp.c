// Test symbolic .fp references for local variables
int test_symbolic_fp(void) {
    int x = 42;
    int y = 100;
    char z = 5;

    // These should use symbolic .fp references like:
    // lda.fp _l_x (instead of lda.fp 0)
    // lda.fp _l_y+1 (instead of lda.fp 3)
    // etc.

    return x + y + z;
}

// Test with arrays to verify offset calculation
int test_array_fp(void) {
    int arr[4];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    return arr[0] + arr[1];
}
