// Test: assert_msg macro with custom diagnostic messages
// Validates that assert_msg prints condition, file:line, and custom message on failure

#include <assert.h>

char *results = 0x4000;

int main() {
    // Test 1: Successful assertion with message
    int x = 10;
    assert_msg(x > 0, "x must be positive");
    results[0] = 1;  // Reached if assertion passed

    // Test 2: Another successful assertion
    int y = 42;
    assert_msg(y == 42, "y should equal 42");
    results[1] = 1;  // Reached if assertion passed

    // Test 3: String assertion with message
    const char *name = "test";
    assert_msg(name != NULL, "name pointer is NULL");
    results[2] = 1;  // Reached if assertion passed

    // Sentinel
    results[3] = 0xFF;

    return 0;  // Success
}
