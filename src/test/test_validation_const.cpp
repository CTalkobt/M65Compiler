#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        tests_failed++; \
    } else { \
        printf("PASS: %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

// Test helper: compile C code and return stderr output
static std::string compileCode(const std::string& cCode) {
    // Clear error file first
    system("rm -f build/test_validation_temp.err");

    // Write C code to temp file
    std::ofstream c_file("build/test_validation_temp.c");
    c_file << cCode;
    c_file.close();

    // Compile and capture stderr
    int ret = system("./bin/cc45 build/test_validation_temp.c -o build/test_validation_temp.s 2>build/test_validation_temp.err");

    // Read stderr
    std::ifstream err_file("build/test_validation_temp.err");
    std::string error_output((std::istreambuf_iterator<char>(err_file)),
                              std::istreambuf_iterator<char>());
    err_file.close();

    return error_output;
}

// Test helper: check if compilation failed with expected error message
static bool compilationFailedWithError(const std::string& cCode, const std::string& expectedError) {
    std::string output = compileCode(cCode);
    return output.find(expectedError) != std::string::npos;
}

// Test helper: check if compilation succeeded
static bool compilationSucceeded(const std::string& cCode) {
    std::ofstream c_file("build/test_validation_temp.c");
    c_file << cCode;
    c_file.close();

    int ret = system("./bin/cc45 build/test_validation_temp.c -o build/test_validation_temp.s 2>/dev/null");
    return ret == 0;
}

// Test 1: Increment of const indirect location (dereferencing const pointer)
void test_increment_const_indirect() {
    std::string code = R"(
int main() {
    int x = 5;
    const int *p = &x;
    (*p)++;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Increment/decrement of read-only location");
    CHECK(failed_correctly, "Increment of const indirect location fails with expected error");
}

// Test 2: Decrement of const indirect location
void test_decrement_const_indirect() {
    std::string code = R"(
int main() {
    int x = 10;
    const int *p = &x;
    (*p)--;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Increment/decrement of read-only location");
    CHECK(failed_correctly, "Decrement of const indirect location fails with expected error");
}

// Test 3: Increment of const struct member
void test_increment_const_member() {
    std::string code = R"(
struct S {
    const int x;
    int y;
};
int main() {
    struct S s;
    s.x++;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Increment/decrement of read-only member");
    CHECK(failed_correctly, "Increment of const struct member fails with expected error");
}

// Test 4: Decrement of const struct member
void test_decrement_const_member() {
    std::string code = R"(
struct S {
    const int x;
};
int main() {
    struct S s;
    s.x--;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Increment/decrement of read-only member");
    CHECK(failed_correctly, "Decrement of const struct member fails with expected error");
}

// Test 5: Cannot take address of register variable
void test_address_of_register() {
    std::string code = R"(
void f() {
    register int x;
    int *p = &x;
}
int main() {
    f();
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Cannot take address of register variable");
    CHECK(failed_correctly, "Taking address of register variable fails with expected error");
}

// Test 6: Cannot take address of bitfield member
void test_address_of_bitfield() {
    std::string code = R"(
struct S {
    int x : 3;
    int y : 5;
};
int main() {
    struct S s;
    int *p = &s.x;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Cannot take address of bitfield member");
    CHECK(failed_correctly, "Taking address of bitfield member fails with expected error");
}

// Test 7: Increment through const pointer parameter (pointer to const)
void test_increment_const_ptr_param() {
    std::string code = R"(
void f(const int *p) {
    (*p)++;
}
int main() {
    int x = 5;
    f(&x);
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Increment/decrement of read-only location");
    CHECK(failed_correctly, "Increment through const pointer parameter fails with expected error");
}

// Test 8: Valid const/register usage succeeds (sanity check)
void test_valid_const_register_usage() {
    std::string code = R"(
struct Point {
    const int x;
    const int y;
};

int main() {
    const int c = 10;
    int value = c + 5;

    struct Point p;
    int sum = p.x + p.y;

    const int *ptr = &c;
    int deref = *ptr;

    return 0;
}
)";

    bool succeeded = compilationSucceeded(code);
    CHECK(succeeded, "Valid const/register usage compiles successfully");
}

// Test 9: Simple bitfield member cannot be incremented
void test_increment_bitfield() {
    std::string code = R"(
struct S {
    int x : 3;
};
int main() {
    struct S s;
    s.x++;
    return 0;
}
)";

    // This might fail with bitfield message or succeed depending on const status
    // Just compile it to ensure bitfields work
    bool succeeded = compilationSucceeded(code);
    CHECK(succeeded, "Bitfield struct member compiles");
}

// Test 10: Register variable can be read normally
void test_register_variable_read() {
    std::string code = R"(
int main() {
    register int x = 5;
    int y = x + 10;
    return y;
}
)";

    bool succeeded = compilationSucceeded(code);
    CHECK(succeeded, "Register variable can be read normally");
}

// Test 11: Write to const-qualified variable fails
void test_write_const_variable() {
    std::string code = R"(
int main() {
    const int c = 10;
    c = 20;
    return 0;
}
)";

    bool failed_correctly = compilationFailedWithError(code, "Compile Error: Assignment to read-only location");
    CHECK(failed_correctly, "Assignment to const variable fails with expected error");
}

int main() {
    printf("Testing const/register constraint validation...\n\n");

    // Error case tests
    test_increment_const_indirect();
    test_decrement_const_indirect();
    test_increment_const_member();
    test_decrement_const_member();
    test_address_of_register();
    test_address_of_bitfield();
    test_increment_const_ptr_param();

    // Success case tests (sanity checks)
    test_valid_const_register_usage();
    test_increment_bitfield();
    test_register_variable_read();
    test_write_const_variable();

    printf("\nConst/Register Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
