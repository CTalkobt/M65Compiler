// Test: __func__, __FILE__, and __LINE__ predefined identifiers
// Validates that __func__ returns the enclosing function name as a string,
// and that __FILE__ and __LINE__ are populated correctly.

int check_func_name() {
    char *name = __func__;
    // "check_func_name" - verify first few chars
    if (name[0] != 99) return 1;   // 'c' = 99
    if (name[1] != 104) return 2;  // 'h' = 104
    if (name[2] != 101) return 3;  // 'e' = 101
    if (name[3] != 99) return 4;   // 'c' = 99
    if (name[4] != 107) return 5;  // 'k' = 107

    // Verify length by walking to null terminator
    int len = 0;
    while (name[len] != 0) {
        len = len + 1;
    }
    // "check_func_name" is 16 characters
    if (len != 16) return 6;

    return 0;
}

int verify_main() {
    char *name = __func__;
    // "verify_main" - first char should be 'v' = 118
    if (name[0] != 118) return 10;

    int len = 0;
    while (name[len] != 0) {
        len = len + 1;
    }
    // "verify_main" is 11 characters
    if (len != 11) return 11;

    return 0;
}

int test_file_macro() {
    char *file = __FILE__;
    // __FILE__ should be a non-empty string
    if (file[0] == 0) return 20;
    return 0;
}

int test_line_macro() {
    int line = __LINE__;
    // __LINE__ should be a positive number (this is around line 49)
    if (line < 1) return 30;
    return 0;
}

int main() {
    int result;

    result = check_func_name();
    if (result != 0) return result;

    result = verify_main();
    if (result != 0) return result;

    result = test_file_macro();
    if (result != 0) return result;

    result = test_line_macro();
    if (result != 0) return result;

    return 0;
}
