#include <cstdio>
#include <fstream>
#include <string>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s\n", msg); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

// Test helper: compile C code and check for expected error
static bool compileAndCheckError(const std::string& cCode, const std::string& expectedError) {
    // Write C code to temp file
    std::ofstream c_file("build/test_validation_semantic_temp.c");
    c_file << cCode;
    c_file.close();

    // Compile and capture stderr
    system("./bin/cc45 build/test_validation_semantic_temp.c -o build/test_validation_semantic_temp.o 2>build/test_validation_semantic_err.txt");

    // Check error file for expected message
    std::ifstream err_file("build/test_validation_semantic_err.txt");
    if (!err_file) {
        return false;
    }

    std::string line;
    bool found = false;
    while (std::getline(err_file, line)) {
        if (line.find(expectedError) != std::string::npos) {
            found = true;
            break;
        }
    }
    err_file.close();

    return found;
}

// Test helper: compile C code and verify it succeeds
static bool compileAndSucceed(const std::string& cCode) {
    // Write C code to temp file
    std::ofstream c_file("build/test_validation_semantic_temp.c");
    c_file << cCode;
    c_file.close();

    // Compile - should succeed
    int ret = system("./bin/cc45 build/test_validation_semantic_temp.c -o build/test_validation_semantic_temp.o 2>/dev/null");
    return (ret == 0);
}

// Test: Implicit function declaration
void test_implicit_function_declaration() {
    std::string code = R"(
int main() {
    undeclared_func();
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "implicit declaration of function");
    CHECK(hasError, "implicit function declaration error detected");
}

// Test: va_start with parameter name not in function
void test_va_start_bad_param() {
    std::string code = R"(
#include <stdarg.h>

int f(int x, ...) {
    va_list ap;
    va_start(ap, y);
    va_end(ap);
    return 0;
}

int main() {
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "va_start: 'y' is not a parameter");
    CHECK(hasError, "va_start bad parameter error detected");
}

// Test: va_start first arg must be a variable
void test_va_start_non_variable() {
    std::string code = R"(
#include <stdarg.h>

int f(int x, ...) {
    va_start(0, x);
    return 0;
}

int main() {
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "va_start: first argument must be a variable");
    CHECK(hasError, "va_start non-variable error detected");
}

// Test: va_arg first arg must be a variable
void test_va_arg_non_variable() {
    std::string code = R"(
#include <stdarg.h>

int f(int x, ...) {
    va_list *ap = 0;
    va_arg(0, int);
    return 0;
}

int main() {
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "va_arg: first argument must be a variable");
    CHECK(hasError, "va_arg non-variable error detected");
}

// Test: Unknown struct type in sizeof
void test_unknown_struct_sizeof() {
    std::string code = R"(
int main() {
    int size = sizeof(struct Unknown);
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Unknown struct/union type");
    CHECK(hasError, "unknown struct type error detected");
}

// Test: No matching _Generic association
void test_generic_no_match() {
    std::string code = R"(
int main() {
    int x = 5;
    int y = _Generic(x, float: 1.0, double: 2.0);
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Expected type name in _Generic association");
    CHECK(hasError, "Generic no match error detected");
}

// Test: Static assertion fails (0)
void test_static_assert_fail() {
    std::string code = R"(
int main() {
    _Static_assert(0, "This assertion should fail");
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Static assertion failed");
    CHECK(hasError, "Static assertion failure error detected");
}

// Test: Static assertion non-constant expression
void test_static_assert_non_constant() {
    std::string code = R"(
int x;  // global non-constant

int main() {
    _Static_assert(x, "Non-constant expression");
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Static assertion condition is not a constant expression");
    CHECK(hasError, "Static assertion non-constant error detected");
}

// Test: Undefined function reference (different from implicit declaration)
void test_undefined_function_reference() {
    std::string code = R"(
int main() {
    return undefined_function();
}
)";

    bool hasError = compileAndCheckError(code, "implicit declaration of function");
    CHECK(hasError, "Undefined function reference error detected");
}

// Test: Valid semantic code succeeds (sanity check)
void test_valid_code() {
    std::string code = R"(
#include <stdio.h>

void declared_func(void) {
    printf("Hello\n");
}

int main() {
    declared_func();
    return 0;
}
)";

    bool succeeded = compileAndSucceed(code);
    CHECK(succeeded, "valid code compiles successfully");
}

int main() {
    printf("Running semantic/type validation error tests...\n");

    // Create build directory if needed
    system("mkdir -p build");

    test_implicit_function_declaration();
    test_va_start_bad_param();
    test_va_start_non_variable();
    test_va_arg_non_variable();
    test_unknown_struct_sizeof();
    test_generic_no_match();
    test_static_assert_fail();
    test_static_assert_non_constant();
    test_undefined_function_reference();
    test_valid_code();

    printf("\n=== Semantic/Type Validation Error Tests ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
