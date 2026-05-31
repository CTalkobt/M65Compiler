// test_assert: validate assert() macro with integer conditions
// Returns 0 (A=$00) on success, non-zero on failure.

#include <assert.h>

int test_assertions(void) {
    // Test 1: Assert true condition passes
    assert(1);

    // Test 2: Assert non-zero passes
    assert(42);

    // Test 3: Assert non-zero expression passes
    assert(3 + 4);

    // Test 4: Assert pointer value passes
    int x = 5;
    assert(&x != 0);

    // Test 5: Multiple assertions
    assert(1 == 1);
    assert(2 > 1);
    assert(5 != 3);

    // Test 6: Assert in loop
    int i;
    for (i = 0; i < 5; i++) {
        assert(i >= 0);
    }

    // Test 7: Assert in conditional
    int value = 10;
    if (value > 5) {
        assert(value > 5);
    }

    // Test 8: Assert with comparison
    int a = 100;
    int b = 100;
    assert(a == b);

    // Test 9: Assert with arithmetic
    assert(2 + 2 == 4);
    assert(10 - 5 == 5);
    assert(3 * 4 == 12);

    return 0;
}

int main() {
    // Call function with assertions
    int result = test_assertions();

    // If we get here, all assertions passed
    if (result != 0) return 1;

    // Test 10: Assert after function call
    assert(result == 0);

    // Test 11: Multiple assertions in sequence
    int test_val = 50;
    assert(test_val > 0);
    assert(test_val < 100);
    assert(test_val == 50);

    // Test 12: Assert with bitwise operations
    int flags = 0xFF;
    assert(flags & 0x01);
    assert(flags | 0x00);

    // Test 13: Assert with logical operations
    assert(1 && 1);
    assert(1 || 0);

    // Test 14: Assert with complex expressions
    int x = 25;
    int y = 30;
    assert((x + y) == 55);
    assert((x < y) && (y > 0));

    // Test 15: Assert preserves local variables
    {
        int local = 42;
        assert(local == 42);
        local = 100;
        assert(local == 100);
    }

    // Test 16: Assert with array index
    int arr[3];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    assert(arr[0] == 10);
    assert(arr[1] == 20);
    assert(arr[2] == 30);

    // Test 17: Assert with struct
    struct point {
        int x;
        int y;
    };
    struct point p;
    p.x = 5;
    p.y = 10;
    assert(p.x == 5);
    assert(p.y == 10);

    // Test 18: Assert with char
    char c = 65;  // 'A'
    assert(c == 65);
    assert(c != 66);

    // Test 19: Assert in nested conditionals
    int n = 7;
    if (n > 5) {
        if (n < 10) {
            assert(n == 7);
        }
    }

    // Test 20: Assert with unary operations
    int neg = -5;
    assert(neg < 0);
    assert(-neg == 5);

    return 0; // All assertions passed
}
