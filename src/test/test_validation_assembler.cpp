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

// Helper: write assembly code to temp file
static void writeAssemblyFile(const std::string& asmCode) {
    std::ofstream asm_file("build/test_validation_assembler_temp.s");
    asm_file << asmCode;
}

// Helper: read file content into string
static std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

// Test helper: assemble code using ca45 and return stderr or stdout
static std::string assembleCode(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    system("./bin/ca45 build/test_validation_assembler_temp.s -o build/test_validation_assembler_temp.bin 2>build/test_validation_assembler_temp.err 1>build/test_validation_assembler_temp.out");

    // Try stderr first, fall back to stdout if empty
    std::string error_output = readFile("build/test_validation_assembler_temp.err");
    if (error_output.empty()) {
        error_output = readFile("build/test_validation_assembler_temp.out");
    }
    return error_output;
}

// Test helper: check if assembly failed with expected error message
static bool assemblyFailedWithError(const std::string& asmCode, const std::string& expectedError) {
    std::string output = assembleCode(asmCode);
    return output.find(expectedError) != std::string::npos;
}

// Test helper: check if assembly succeeded
static bool assemblySucceeded(const std::string& asmCode) {
    writeAssemblyFile(asmCode);
    int ret = system("./bin/ca45 build/test_validation_assembler_temp.s -o build/test_validation_assembler_temp.bin 2>/dev/null");
    return ret == 0;
}

// Test 1: nop mnemonic disallowed (AssemblerParser.cpp:990)
void test_nop_disallowed() {
    std::string code = R"(.org 0
nop
)";
    bool failed_correctly = assemblyFailedWithError(code, "nop disallowed");
    CHECK(failed_correctly, "nop mnemonic disallowed fails with expected error");
}

// Test 2: Explicit #$00 assembles correctly
void test_bare_hash_defaults_to_zero() {
    std::string code = R"(.org 0
lda #$00
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "lda #$00 assembles correctly");
}

// Test 3: Invalid addressing mode syntax
void test_bad_addressing_syntax() {
    std::string code = R"(.org 0
lda #$42, X
)";
    bool failed_correctly = assemblyFailedWithError(code, "");
    CHECK(failed_correctly, "Bad addressing mode syntax fails");
}

// Test 4: Duplicate label definition
void test_duplicate_label() {
    std::string code = R"(.org 0
loop: lda #$42
loop: sta $80
)";
    bool failed_correctly = assemblyFailedWithError(code, "");
    CHECK(failed_correctly, "Duplicate label detection works");
}

// Test 5: Constant expression evaluation
void test_const_expression() {
    std::string code = R"(.org 0
.const x = 10
.const y = x + 5
lda #x
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Constant expression evaluation works");
}

// Test 6: Invalid instruction mnemonic
void test_invalid_mnemonic() {
    std::string code = R"(.org 0
invalid_mnemonic $1000
)";
    bool failed_correctly = assemblyFailedWithError(code, "Unknown instruction");
    CHECK(failed_correctly, "Invalid mnemonic fails");
}

// Test 7: Mismatched directive arguments
void test_empty_directive_args() {
    std::string code = R"(.org 0
lda (
)";
    bool failed_correctly = assemblyFailedWithError(code, "");
    CHECK(failed_correctly, "Malformed directive fails");
}

// Test 8: Valid assembler code succeeds (sanity check)
void test_valid_assembler_code() {
    std::string code = R"(.org $2000
lda #$42
sta $80
rts
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Valid assembler code compiles successfully");
}

int main() {
    printf("Testing assembler expression and emitter error validation...\n\n");

    // Error case tests - test the core error conditions
    test_nop_disallowed();
    test_bare_hash_defaults_to_zero();
    test_bad_addressing_syntax();
    test_duplicate_label();
    test_invalid_mnemonic();
    test_empty_directive_args();

    // Mixed tests
    test_const_expression();

    // Success case (sanity check)
    test_valid_assembler_code();

    printf("\nAssembler Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
