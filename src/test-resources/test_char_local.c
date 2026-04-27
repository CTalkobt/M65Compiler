// Test: 8-bit local variable access via lda.sp/sta.sp
// Validates char-sized stack variable load/store.

char add_chars(char a, char b) {
    return a + b;
}

int main() {
    char x = 10;
    char y = 20;
    char z = x + y;

    if (z != 30) return 1;

    // Char assignment from expression
    char w = z - x;
    if (w != 20) return 2;

    // Char passed to and returned from function
    char r = add_chars(x, y);
    if (r != 30) return 3;

    // Char in array context
    char arr[4];
    arr[0] = x;
    arr[1] = y;
    arr[2] = arr[0] + arr[1];
    arr[3] = 0;

    if (arr[2] != 30) return 4;

    return 0;
}
