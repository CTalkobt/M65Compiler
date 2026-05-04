// Comprehensive regression test for ZP calling convention (-fzpcall)
// Tests: nested calls, address-of, long return, recursion, mixed types
// Results written to $4000+ for mmemu verification

char *results = (char *)0x4000;

// --- Test 1: Nested call save/restore ---
int inner_clobber(int a, int b) {
    return a * b;
}

int nested_test(int x, int y, int z) {
    int product = inner_clobber(x, y);
    return product + z;
}

// --- Test 2: Deep nesting (3 levels) ---
int level3(int a) {
    return a + 1;
}

int level2(int a, int b) {
    int r = level3(a);
    return r + b;
}

int level1(int a, int b, int c) {
    int r = level2(a, b);
    return r + c;
}

// --- Test 3: Address-of on ZP param ---
void write_through_ptr(int *p, int val) {
    *p = val;
}

int addr_of_param(int x) {
    write_through_ptr(&x, 77);
    return x;
}

// --- Test 4: Address-of with read-back ---
int read_through_ptr(int *p) {
    return *p;
}

int addr_of_read(int x) {
    int val = read_through_ptr(&x);
    return val;
}

// --- Test 5: Long return in AXYZ ---
long add_long(long a, long b) {
    return a + b;
}

// --- Test 6: Recursion (factorial) ---
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// --- Test 7: Mutual recursion ---
int is_odd(int n);

int is_even(int n) {
    if (n == 0) return 1;
    return is_odd(n - 1);
}

int is_odd(int n) {
    if (n == 0) return 0;
    return is_even(n - 1);
}

// --- Test 8: Many params (max ZP usage) ---
int sum4(int a, int b, int c, int d) {
    return a + b + c + d;
}

// --- Test 9: Char params (1-byte) ---
char char_add(char a, char b, char c) {
    return a + b + c;
}

void main() {
    // Test 1: nested_test(3, 4, 100) = 3*4 + 100 = 112 = $70
    int r1 = nested_test(3, 4, 100);
    results[0] = (char)(r1 & 0xFF);
    results[1] = (char)((r1 >> 8) & 0xFF);

    // Test 2: level1(1, 2, 3) = ((1+1)+2)+3 = 7
    int r2 = level1(1, 2, 3);
    results[2] = (char)r2;

    // Test 3: addr_of_param(99) = 77
    int r3 = addr_of_param(99);
    results[3] = (char)(r3 & 0xFF);
    results[4] = (char)((r3 >> 8) & 0xFF);

    // Test 4: addr_of_read(55) = 55
    int r4 = addr_of_read(55);
    results[5] = (char)(r4 & 0xFF);
    results[6] = (char)((r4 >> 8) & 0xFF);

    // Test 5: add_long(100000, 200000) = 300000 = $000493E0
    long l1 = add_long(100000L, 200000L);
    results[7] = (char)l1;
    results[8] = (char)(l1 >> 8);
    results[9] = (char)(l1 >> 16);
    results[10] = (char)(l1 >> 24);

    // Test 6: factorial(5) = 120 = $78
    int r6 = factorial(5);
    results[11] = (char)(r6 & 0xFF);
    results[12] = (char)((r6 >> 8) & 0xFF);

    // Test 7: is_even(4) = 1, is_even(3) = 0
    results[13] = (char)is_even(4);
    results[14] = (char)is_even(3);

    // Test 8: sum4(10, 20, 30, 40) = 100 = $64
    int r8 = sum4(10, 20, 30, 40);
    results[15] = (char)r8;

    // Test 9: char_add(10, 20, 30) = 60 = $3C
    results[16] = char_add(10, 20, 30);

    // Sentinel
    results[17] = 0xAA;

    __asm__("brk");
}
