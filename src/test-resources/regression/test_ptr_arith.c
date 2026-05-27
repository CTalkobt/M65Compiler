// test_ptr_arith: pointer arithmetic and array indexing
// Returns 0 (A=$00) on success, non-zero on failure.

int main() {
    int arr[8];
    int *p;
    int i;

    // Array fill via index
    for (i = 0; i < 8; i++) {
        arr[i] = i * 10;
    }

    // Pointer access
    p = arr;
    if (*p != 0) return 1;

    // Pointer increment
    p = p + 1;
    if (*p != 10) return 2;

    p = p + 3;
    if (*p != 40) return 3;

    // Pointer subtraction
    int *q = &arr[7];
    int diff = q - arr;
    if (diff != 7) return 4;

    // Array via pointer with index
    p = arr;
    if (p[5] != 50) return 5;

    // Pointer to pointer dereference
    int val = 99;
    int *pp = &val;
    if (*pp != 99) return 6;
    *pp = 77;
    if (val != 77) return 7;

    // Char pointer arithmetic (stride 1)
    char buf[4];
    buf[0] = 'A';
    buf[1] = 'B';
    buf[2] = 'C';
    buf[3] = 0;
    char *cp = buf;
    if (*cp != 'A') return 8;
    cp = cp + 2;
    if (*cp != 'C') return 9;

    // Int array stride (stride 2)
    // &arr[3] - &arr[0] should be 3 elements
    if (&arr[3] - &arr[0] != 3) return 10;

    return 0;
}
