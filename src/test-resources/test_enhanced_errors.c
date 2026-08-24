/* test_enhanced_errors.c — Enhanced Error Message Examples
 *
 * Demonstrates improved error reporting with context and suggestions.
 *
 * NOTE: These errors are intentional for testing the error system.
 * Each section shows what the enhanced error message would display.
 */

/* ============================================================================
 * Error 1: Undefined Variable (with typo suggestion)
 * ============================================================================
 *
 * Enhanced error output would be:
 *
 * test.c:20:5: error: undefined identifier 'x'
 *  19 | int main(void) {
 *  20 |     prntf("hello");
 *     |     ^^^^^
 *
 * Suggestions:
 *   • Did you mean 'printf'?
 */

// int main(void) {
//     prntf("hello");  // Error: undefined identifier 'prntf'
//     return 0;
// }

/* ============================================================================
 * Error 2: Type Mismatch (with context and suggestion)
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:35:12: error: type mismatch in assignment
 *  34 | int x = 10;
 *  35 | float f = x;  // implicit conversion
 *  36 | return 0;
 *     |         ^
 *
 * Suggestions:
 *   • Type mismatch: expected 'float' but got 'int'
 *   • Use explicit cast: (float)x
 */

// int test_type_mismatch(void) {
//     int x = 10;
//     float f = x;  // Error: type mismatch
//     return 0;
// }

/* ============================================================================
 * Error 3: Missing Semicolon (detected by error context)
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:55:8: error: expected ';' after statement
 *  54 | int x = 5
 *  55 | int y = 10;
 *     |       ^^
 *
 * Suggestions:
 *   • Did you forget a semicolon at end of line 54?
 */

// int test_missing_semicolon(void) {
//     int x = 5   // Error: missing semicolon
//     int y = 10;
//     return 0;
// }

/* ============================================================================
 * Error 4: Assignment vs Comparison (common mistake)
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:75:9: error: assignment in condition
 *  74 | if (x = 5) {  // Assignment, not comparison
 *  75 |     return 0;
 *     |   ^^^^^^
 *
 * Suggestions:
 *   • Did you mean == instead of =?
 */

// int test_assignment_vs_comparison(void) {
//     int x;
//     if (x = 5) {  // Error: assignment in condition
//         return 0;
//     }
//     return 1;
// }

/* ============================================================================
 * Error 5: Undefined Function with Suggestions
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:95:5: error: undefined function 'strcpy2'
 *  94 | char buf[10];
 *  95 | strcpy2(buf, src);  // undefined function
 *  96 |
 *     | ^^^^^^^^
 *
 * Suggestions:
 *   • Did you mean 'strcpy'?
 *   • Did you mean 'strncpy'?
 */

// int test_undefined_function(void) {
//     char buf[10];
//     strcpy2(buf, "test");  // Error: undefined function
//     return 0;
// }

/* ============================================================================
 * Error 6: Type Error with Related Locations
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:115:12: error: argument type mismatch
 *  114 | void takes_int(int x) { }
 *  115 | takes_int("string");
 *     |           ^^^^^^^^
 *
 * Suggestions:
 *   • Type mismatch: expected 'int' but got 'const char *'
 *
 * Related locations:
 *   test.c:114: parameter declared here
 */

// void takes_int(int x) { }
//
// int test_type_error(void) {
//     takes_int("string");  // Error: type mismatch
//     return 0;
// }

/* ============================================================================
 * Error 7: Unreachable Code Detection
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:140:5: warning: unreachable code
 *  139 | return 0;
 *  140 | x = 5;  // unreachable
 *  141 |
 *     | ^
 *
 * Suggestions:
 *   • This code is unreachable
 */

// int test_unreachable(void) {
//     return 0;
//     x = 5;  // Warning: unreachable code
// }

/* ============================================================================
 * Error 8: Missing Return Statement
 * ============================================================================
 *
 * Enhanced error output:
 *
 * test.c:160:1: warning: control reaches end of non-void function
 *  158 | int get_value(void) {
 *  159 |     if (rand() > 0) return 42;
 *  160 | }
 *     | ^
 *
 * Suggestions:
 *   • Function 'get_value' should have a return statement
 */

// int get_value(void) {
//     if (rand() > 0) return 42;
//     // Error: missing return
// }

/**
 * DEMONSTRATION: How to use enhanced diagnostics in code
 */
int main(void) {
    // This is a valid test file showing the error message system.
    // The commented sections above show what enhanced error messages
    // would look like for common programming mistakes.

    // The enhanced diagnostic system provides:
    // 1. Source context (shows surrounding lines)
    // 2. Error position (column with caret indicator)
    // 3. Suggestions (helpful hints for fixing the error)
    // 4. Related locations (where symbols were defined)
    // 5. Color coding (different colors for different severity levels)

    // Example of valid code:
    int x = 42;
    printf("Value: %d\n", x);

    return 0;
}
