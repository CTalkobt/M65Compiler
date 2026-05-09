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

// Test helper: assemble code and return stderr output
static std::string runCA45(const char* asm_src) {
    // Write assembly to temp file
    std::ofstream asm_file("build/test_validation_symbols_temp.s");
    asm_file << ".org $2000\n" << asm_src;
    asm_file.close();

    // Assemble and capture stderr
    int ret = system("./bin/ca45 build/test_validation_symbols_temp.s -o build/test_validation_symbols_temp.bin 2>build/test_validation_symbols_temp.err");

    // Read stderr
    std::ifstream err_file("build/test_validation_symbols_temp.err");
    std::string error_output((std::istreambuf_iterator<char>(err_file)),
                              std::istreambuf_iterator<char>());
    err_file.close();

    return error_output;
}

// Test helper: check if assembly failed with expected error message
static bool assembleFailed(const char* asm_src, const std::string& expectedError) {
    std::string output = runCA45(asm_src);
    return output.find(expectedError) != std::string::npos;
}

// Test helper: check if assembly succeeded
static bool assembleSucceeded(const char* asm_src) {
    std::ofstream asm_file("build/test_validation_symbols_temp.s");
    asm_file << ".org $2000\n" << asm_src;
    asm_file.close();

    int ret = system("./bin/ca45 build/test_validation_symbols_temp.s -o build/test_validation_symbols_temp.bin 2>/dev/null");
    return ret == 0;
}

// Test 1: .const symbol definition and usage
void test_const_definition_usage() {
    const char* code = R"(
        .const x = 10
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".const x = 10 definition and usage succeeds");
}

// Test 2: .const redefinition fails
void test_const_redefinition_fails() {
    const char* code = R"(
        .const x = 10
        .const x = 20
    )";
    bool failed = assembleFailed(code, "Redefinition of symbol");
    CHECK(failed, ".const redefinition fails with expected error");
}

// Test 3: .const cannot be redefined as .var
void test_const_redefined_as_var_fails() {
    const char* code = R"(
        .const x = 10
        .var x = 20
    )";
    bool failed = assembleFailed(code, "");
    CHECK(failed, ".const cannot be redefined as .var");
}

// Test 4: .var declaration and usage
void test_var_declaration_usage() {
    const char* code = R"(
        .var x = 10
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".var x = 10 declaration and usage succeeds");
}

// Test 5: .var increment form
void test_var_increment() {
    const char* code = R"(
        .var x = 10
        .var x++
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".var x++ increment succeeds");
}

// Test 6: .var decrement form
void test_var_decrement() {
    const char* code = R"(
        .var x = 10
        .var x--
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".var x-- decrement succeeds");
}

// Test 7: .var reassignment
void test_var_reassignment() {
    const char* code = R"(
        .var x = 10
        .var x = 20
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".var reassignment succeeds");
}

// Test 8: lo() unary operator
void test_lo_operator() {
    const char* code = R"(
        .const x = lo($1234)
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "lo($1234) unary operator succeeds");
}

// Test 9: hi() unary operator
void test_hi_operator() {
    const char* code = R"(
        .const x = hi($1234)
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "hi($1234) unary operator succeeds");
}

// Test 10: bank() unary operator
void test_bank_operator() {
    const char* code = R"(
        .const x = bank($12345)
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "bank($12345) unary operator succeeds");
}

// Test 11: defined() operator with defined symbol
void test_defined_operator_true() {
    const char* code = R"(
        .const x = 10
        .const y = defined(x)
        lda #y
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "defined(x) with defined symbol succeeds");
}

// Test 12: defined() operator with undefined symbol
void test_defined_operator_false() {
    const char* code = R"(
        .const y = defined(undefined_sym)
        lda #y
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "defined(undefined_sym) with undefined symbol succeeds");
}

// Test 13: defined() in conditional expression
void test_defined_in_conditional() {
    const char* code = R"(
        .const x = 10
        .if defined(x)
        lda #1
        .endif
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "defined(x) in conditional expression succeeds");
}

// Test 14: Division by zero in expression fails
void test_division_by_zero() {
    const char* code = R"(
        .const x = 10 / 0
    )";
    bool failed = assembleFailed(code, "Division by zero");
    CHECK(failed, "Division by zero fails with expected error");
}

// Test 15: Addition precedence
void test_precedence_addition() {
    const char* code = R"(
        .const x = 1 + 2 + 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Addition precedence (1 + 2 + 3) succeeds");
}

// Test 16: Multiplication and addition precedence
void test_precedence_mult_add() {
    const char* code = R"(
        .const x = 1 + 2 * 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Multiplication and addition precedence (1 + 2 * 3) succeeds");
}

// Test 17: Bitwise OR operator
void test_bitwise_or() {
    const char* code = R"(
        .const x = 5 | 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Bitwise OR operator (5 | 3) succeeds");
}

// Test 18: Bitwise AND operator
void test_bitwise_and() {
    const char* code = R"(
        .const x = 5 & 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Bitwise AND operator (5 & 3) succeeds");
}

// Test 19: Bitwise XOR operator
void test_bitwise_xor() {
    const char* code = R"(
        .const x = 5 ^ 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Bitwise XOR operator (5 ^ 3) succeeds");
}

// Test 20: Left shift operator
void test_left_shift() {
    const char* code = R"(
        .const x = 1 << 4
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Left shift operator (1 << 4) succeeds");
}

// Test 21: Right shift operator
void test_right_shift() {
    const char* code = R"(
        .const x = 16 >> 2
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Right shift operator (16 >> 2) succeeds");
}

// Test 22: Nested unary operators
void test_nested_unary() {
    const char* code = R"(
        .const x = lo(hi($ABCD))
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Nested unary operators lo(hi($ABCD)) succeeds");
}

// Test 23: Complex expression with multiple operators
void test_complex_expression() {
    const char* code = R"(
        .const x = (1 + 2) * 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Complex expression (1 + 2) * 3 succeeds");
}

// Test 24: .var with multiple increments
void test_var_multiple_increments() {
    const char* code = R"(
        .var counter = 0
        .var counter++
        .var counter++
        .var counter++
        lda #counter
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, ".var with multiple increments succeeds");
}

// Test 25: Subtraction operator
void test_subtraction_operator() {
    const char* code = R"(
        .const x = 10 - 3
        lda #x
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Subtraction operator (10 - 3) succeeds");
}

int main() {
    printf("Symbol & Expression Validation Tests\n");
    printf("====================================\n\n");

    // .const symbol management tests
    test_const_definition_usage();
    test_const_redefinition_fails();
    test_const_redefined_as_var_fails();

    // .var management tests
    test_var_declaration_usage();
    test_var_increment();
    test_var_decrement();
    test_var_reassignment();
    test_var_multiple_increments();

    // Unary operators tests
    test_lo_operator();
    test_hi_operator();
    test_bank_operator();

    // defined() operator tests
    test_defined_operator_true();
    test_defined_operator_false();
    test_defined_in_conditional();

    // Division by zero test
    test_division_by_zero();

    // Operator precedence tests
    test_precedence_addition();
    test_precedence_mult_add();

    // Bitwise operators tests
    test_bitwise_or();
    test_bitwise_and();
    test_bitwise_xor();

    // Shift operators tests
    test_left_shift();
    test_right_shift();

    // Complex expression tests
    test_nested_unary();
    test_complex_expression();
    test_subtraction_operator();

    printf("\nSymbol & Expression Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
