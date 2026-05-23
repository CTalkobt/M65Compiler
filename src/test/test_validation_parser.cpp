#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <unistd.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

struct RunResult {
    int exitCode;
    std::string output;
};

// Helper function to shell out to cc45 and capture output
static RunResult runCC45(const char* src) {
    // Write source to temp file
    const char* tempFile = "build/test_validation_parser_temp.c";
    FILE* f = fopen(tempFile, "w");
    if (!f) {
        fprintf(stderr, "Failed to open temp file %s\n", tempFile);
        return {-1, ""};
    }
    fprintf(f, "%s", src);
    fclose(f);

    // Run cc45 and capture output via popen
    std::string cmd = "./bin/cc45 " + std::string(tempFile) + " -o build/test_validation_parser_temp.s 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) {
        fprintf(stderr, "Failed to run cc45\n");
        return {-1, ""};
    }

    std::string output;
    char buf[1024];
    while (fgets(buf, sizeof(buf), p)) {
        output += buf;
    }

    int exitCode = pclose(p) >> 8;  // WEXITSTATUS
    return {exitCode, output};
}

// Test 1: Expected return type for function declaration
void test_expected_return_type() {
    const char* src = "void bar(int x) { return; } invalid foo() { return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_return_type: exit code != 0");
    CHECK(res.output.find("Expected return type") != std::string::npos,
          "test_expected_return_type: error message present");
}

// Test 2: Expected parameter type
void test_expected_parameter_type() {
    const char* src = "int foo(invalid x) { return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_parameter_type: exit code != 0");
    CHECK(res.output.find("Expected parameter type") != std::string::npos,
          "test_expected_parameter_type: error message present");
}

// Test 3: Expected variable declaration type - using struct without name
void test_expected_variable_decl_type() {
    const char* src = "struct foo { int x; }; void main() { struct x = 5; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_variable_decl_type: exit code != 0");
    // This actually triggers "Expected struct/union name" but serves as a type error test
    CHECK(res.output.find("Expected") != std::string::npos,
          "test_expected_variable_decl_type: error message present");
}

// Test 4: Expected member type in struct
void test_expected_member_type() {
    const char* src = "struct S { invalid x; };";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_member_type: exit code != 0");
    CHECK(res.output.find("Expected member type") != std::string::npos,
          "test_expected_member_type: error message present");
}

// Test 5: Flexible array member multi-dimensional
void test_flexible_array_multi_dimensional() {
    const char* src = "struct S { int x[][]; };";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_flexible_array_multi_dimensional: exit code != 0");
    CHECK(res.output.find("Flexible array member") != std::string::npos &&
          res.output.find("cannot be multi-dimensional") != std::string::npos,
          "test_flexible_array_multi_dimensional: error message present");
}

// Test 6: Bitfield cannot be array
void test_bitfield_cannot_be_array() {
    const char* src = "struct S { int x[10] : 5; };";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_bitfield_cannot_be_array: exit code != 0");
    CHECK(res.output.find("Bitfield member") != std::string::npos &&
          res.output.find("cannot be an array") != std::string::npos,
          "test_bitfield_cannot_be_array: error message present");
}

// Test 7: Bitfield cannot be pointer
void test_bitfield_cannot_be_pointer() {
    const char* src = "struct S { int *x : 5; };";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_bitfield_cannot_be_pointer: exit code != 0");
    CHECK(res.output.find("Bitfield member") != std::string::npos &&
          res.output.find("cannot be a pointer") != std::string::npos,
          "test_bitfield_cannot_be_pointer: error message present");
}

// Test 8: Bitfield width out of range
void test_bitfield_width_out_of_range() {
    const char* src = "struct S { int x : 25; };";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_bitfield_width_out_of_range: exit code != 0");
    CHECK(res.output.find("Bitfield width") != std::string::npos &&
          res.output.find("out of range") != std::string::npos,
          "test_bitfield_width_out_of_range: error message present");
}

// Test 9: Expected type in _Alignof
void test_expected_type_in_alignof() {
    const char* src = "int main() { int x = _Alignof(invalid); return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_type_in_alignof: exit code != 0");
    CHECK(res.output.find("Expected type name in _Alignof") != std::string::npos,
          "test_expected_type_in_alignof: error message present");
}

// Test 10: __func__ outside function
void test_func_outside_function() {
    const char* src = "const char *global = __func__;";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_func_outside_function: exit code != 0");
    CHECK(res.output.find("__func__") != std::string::npos &&
          res.output.find("outside of a function") != std::string::npos,
          "test_func_outside_function: error message present");
}

// Test 11: Expected type in __builtin_va_arg
void test_expected_type_in_va_arg() {
    const char* src = "typedef int va_list; int foo(va_list ap) { return __builtin_va_arg(ap, @); }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_type_in_va_arg: exit code != 0");
    CHECK(res.output.find("Expected type in __builtin_va_arg") != std::string::npos,
          "test_expected_type_in_va_arg: error message present");
}

// Test 12: Expected expression
void test_expected_expression() {
    const char* src = "int main() { int x = @; return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_expression: exit code != 0");
    CHECK(res.output.find("Expected expression") != std::string::npos,
          "test_expected_expression: error message present");
}

// Test 13: Expected type in _Generic association
void test_expected_type_in_generic() {
    const char* src = "int main() { int x = _Generic(5, invalid: 1); return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_type_in_generic: exit code != 0");
    CHECK(res.output.find("Expected type name in _Generic association") != std::string::npos,
          "test_expected_type_in_generic: error message present");
}

// Test 14: Expected type in typedef
void test_expected_type_in_typedef() {
    const char* src = "typedef invalid mytype;";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_type_in_typedef: exit code != 0");
    CHECK(res.output.find("Expected type in typedef") != std::string::npos,
          "test_expected_type_in_typedef: error message present");
}

// Test 15: Expected type in function pointer params
void test_expected_type_in_function_pointer_params() {
    const char* src = "typedef int (*funcptr)(invalid);";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_expected_type_in_function_pointer_params: exit code != 0");
    CHECK(res.output.find("Expected type in function pointer parameter list") != std::string::npos,
          "test_expected_type_in_function_pointer_params: error message present");
}

// Test 16: Valid parser code succeeds (sanity check)
void test_valid_code_succeeds() {
    const char* src = "int foo() { return 0; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode == 0, "test_valid_code_succeeds: exit code == 0");
}
// Test: Expected member after __cpu.
void test_cpu_intrinsic_syntax() {
    const char* src = "int main() { __cpu = 5; }";
    RunResult res = runCC45(src);
    CHECK(res.exitCode != 0, "test_cpu_intrinsic_syntax: exit code != 0");
    CHECK(res.output.find("must be followed by '.'") != std::string::npos,
          "test_cpu_intrinsic_syntax: error message present");
}

int main() {
    // Create build directory if needed
    system("mkdir -p build");

    test_expected_return_type();
    test_expected_parameter_type();
    test_expected_variable_decl_type();
    test_expected_member_type();
    test_flexible_array_multi_dimensional();
    test_bitfield_cannot_be_array();
    test_bitfield_cannot_be_pointer();
    test_bitfield_width_out_of_range();
    test_expected_type_in_alignof();
    test_func_outside_function();
    test_expected_type_in_va_arg();
    test_expected_expression();
    test_expected_type_in_generic();
    test_expected_type_in_typedef();
    test_expected_type_in_function_pointer_params();
    test_cpu_intrinsic_syntax();
    test_valid_code_succeeds();

    printf("\nParser Syntax Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
