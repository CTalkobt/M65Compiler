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

// Test: va_arg second arg must be a type
void test_va_arg_non_variable() {
    std::string code = R"(
#include <stdarg.h>

int f(int x, ...) {
    va_list ap;
    va_start(ap, x);
    int y = va_arg(ap, 123);
    va_end(ap);
    return 0;
}

int main() {
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Expected type in __builtin_va_arg");
    CHECK(hasError, "va_arg non-type error detected");
}

// Test: sizeof unknown struct
void test_unknown_struct_sizeof() {
    std::string code = R"(
int main() {
    int s = sizeof(struct Unknown);
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "Unknown struct/union type: Unknown");
    CHECK(hasError, "sizeof unknown struct error detected");
}

// Test: _Generic no match
void test_generic_no_match() {
    std::string code = R"(
int main() {
    int x = _Generic((long)0, int: 1, char: 2);
    return 0;
}
)";

    bool hasError = compileAndCheckError(code, "No matching association in _Generic selection");
    CHECK(hasError, "_Generic no match error detected");
}

// Test: _Static_assert fail
void test_static_assert_fail() {
    std::string code = R"(
_Static_assert(0, "failure");
int main() { return 0; }
)";

    bool hasError = compileAndCheckError(code, "Static assertion failed");
    CHECK(hasError, "_Static_assert failure detected");
}

// Test: _Static_assert non-constant
void test_static_assert_non_constant() {
    std::string code = R"(
extern int x;
_Static_assert(x, "failure");
int main() { return 0; }
)";

    bool hasError = compileAndCheckError(code, "condition is not a constant expression");
    CHECK(hasError, "_Static_assert non-constant detected");
}

// Test: Undefined function reference
void test_undefined_function_reference() {
    std::string code = R"(
int f();
int main() {
    return f();
}
)";

    // Should compile - linker handles undefined symbols
    bool ok = compileAndSucceed(code);
    CHECK(ok, "undefined function reference succeeds (linker task)");
}

// Test: Cannot take address of CPU register
void test_cpu_reg_address() {
    std::string code = R"(
int main() {
    int p = (int)&__cpu.A;
    return 0;
}
)";
    bool hasError = compileAndCheckError(code, "Cannot take address of CPU register");
    CHECK(hasError, "address-of CPU register error detected");
}

// Test: Cannot take address of CPU flag
void test_cpu_flag_address() {
    std::string code = R"(
int main() {
    int p = (int)&__flags.Carry;
    return 0;
}
)";
    bool hasError = compileAndCheckError(code, "Cannot take address of CPU flag");
    CHECK(hasError, "address-of CPU flag error detected");
}

// Test: CPU intrinsics r-value and l-value
void test_cpu_intrinsics_valid() {
    std::string code = R"(
int main() {
    __cpu.A = 42;
    if (__cpu.X > 10) __flags.Zero = 1;
    return 0;
}
)";
    bool ok = compileAndSucceed(code);
    CHECK(ok, "valid CPU intrinsics usage succeeds");
}

// Test: Valid code
void test_valid_code() {
    std::string code = R"(
struct S { int x; };
int main() {
    struct S s = {1};
    return s.x - 1;
}
)";

    bool ok = compileAndSucceed(code);
    CHECK(ok, "valid code compiles successfully");
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
    test_cpu_reg_address();
    test_cpu_flag_address();
    test_cpu_intrinsics_valid();
    test_valid_code();

    printf("\n=== Semantic/Type Validation Error Tests ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);

    return tests_failed == 0 ? 0 : 1;
}
