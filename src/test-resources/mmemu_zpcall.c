// Test ZP calling convention (-fzpcall)
// Results written to $4000-$400F for mmemu verification
// Runtime assertions validate expected behavior

#include <assert.h>

char *results = 0x4000;

// Test 1: basic param passing
char add(char a, char b) {
    return a + b;
}

// Test 2: modify through pointer (address-of spill)
void set_via_ptr(int *p, int val) {
    *p = val;
}

// Test 3: address-of param — param must be spilled to frame
int addr_of_test(int x) {
    set_via_ptr(&x, 42);
    return x;  // must return 42, not the original value
}

// Test 4: nested calls — caller-save/restore
int inner(int a) {
    return a + 10;
}

int outer(int a, int b) {
    int r = inner(a);
    return r + b;  // b must survive the call to inner()
}

void main() {
    // Test 1: add(3, 7) = 10
    char r1 = add(3, 7);
    assert(r1 == 10);
    results[0] = r1;

    // Test 2: addr_of_test(99) should return 42 (not 99)
    int r2 = addr_of_test(99);
    assert(r2 == 42);
    results[1] = (char)(r2 & 0xFF);
    results[2] = (char)((r2 >> 8) & 0xFF);

    // Test 3: outer(5, 20) = 15 + 20 = 35
    int r3 = outer(5, 20);
    assert(r3 == 35);
    results[3] = (char)(r3 & 0xFF);

    // Sentinel
    results[4] = 0xFF;

    return 0;  // Success
}
