/* Comprehensive Error Message Test Suite
 * Tests all enhanced error suggestions
 * NOTE: These are intentionally invalid - for testing error messages only
 */

// ============================================================================
// PARSER ERRORS (Syntax)
// ============================================================================

// Test 1: Missing semicolon (Parser)
int test_missing_semicolon(void) {
    int x = 5
    return x;
}

// Test 2: Missing closing parenthesis (Parser)
int add(int a, int b {
    return a + b;
}

// Test 3: Missing closing brace (Parser)
void incomplete_func(void) {
    int x = 10;
    if (x > 5) {
        x++;
}

// ============================================================================
// SEMANTIC ERRORS (Type/Scope)
// ============================================================================

// Test 4: Unknown struct type (IRBuilder)
void test_unknown_struct(void) {
    struct UndefinedStruct x;
}

// Test 5: Compound literal with unknown struct (IRBuilder)
void test_compound_literal_unknown(void) {
    struct NonExistent val = (struct NonExistent){1};
}

// ============================================================================
// NOTE: Additional test cases can be uncommented individually
// ============================================================================

/*
// Test 6: Invalid function pointer parameter
void (*func_ptr)(int undefined_param);

// Test 7: Type mismatch in assignment
void type_mismatch_test(void) {
    int x;
    float f = x;
}

// Test 8: Const violation
void const_violation(void) {
    const int x = 5;
    x = 10;
}
*/
