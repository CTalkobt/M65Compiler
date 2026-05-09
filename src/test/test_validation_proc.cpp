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
    std::ofstream asm_file("build/test_validation_proc_temp.s");
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
    system("./bin/ca45 build/test_validation_proc_temp.s -o build/test_validation_proc_temp.bin 2>build/test_validation_proc_temp.err 1>build/test_validation_proc_temp.out");

    // Try stderr first, fall back to stdout if empty
    std::string error_output = readFile("build/test_validation_proc_temp.err");
    if (error_output.empty()) {
        error_output = readFile("build/test_validation_proc_temp.out");
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
    int ret = system("./bin/ca45 build/test_validation_proc_temp.s -o build/test_validation_proc_temp.bin 2>/dev/null");
    return ret == 0;
}

// Test 1: Valid proc/endproc block
void test_valid_proc_endproc() {
    std::string code = R"(.org $2000
proc foo
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "Valid proc/endproc block assembles successfully");
}

// Test 2: proc with arguments
void test_proc_with_arguments() {
    std::string code = R"(.org $2000
proc foo, W#arg1, B#arg2
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "proc with argument specs assembles successfully");
}

// Test 3: endproc with RTS cleanup
void test_endproc_rts() {
    std::string code = R"(.org $2000
proc foo
    lda #$42
endproc RTS
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "endproc with RTS assembles successfully");
}

// Test 4: endproc with RTS and cleanup argument
void test_endproc_rts_cleanup() {
    std::string code = R"(.org $2000
proc foo
    lda #$42
endproc RTS #4
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "endproc with RTS and cleanup argument assembles successfully");
}

// Test 5: .zp_uses inside proc
void test_zp_uses_inside_proc() {
    std::string code = R"(.org $2000
proc foo
    .zp_uses $10, $11
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".zp_uses inside proc assembles successfully");
}

// Test 6: .zp_clobbers inside proc
void test_zp_clobbers_inside_proc() {
    std::string code = R"(.org $2000
proc foo
    .zp_clobbers $20, $21
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".zp_clobbers inside proc assembles successfully");
}

// Test 7: .zp_release inside proc
void test_zp_release_inside_proc() {
    std::string code = R"(.org $2000
proc foo
    .zp_release $03, $04
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".zp_release inside proc assembles successfully");
}

// Test 8: .reg_clobbers inside proc
void test_reg_clobbers_inside_proc() {
    std::string code = R"(.org $2000
proc foo
    .reg_clobbers A, X, Y
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".reg_clobbers inside proc assembles successfully");
}

// Test 9: .flag_clobbers inside proc
void test_flag_clobbers_inside_proc() {
    std::string code = R"(.org $2000
proc foo
    .flag_clobbers C, N, Z, V
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".flag_clobbers inside proc assembles successfully");
}

// Test 10: .zp_uses outside proc (error case)
void test_zp_uses_outside_proc_error() {
    std::string code = R"(.org $2000
.zp_uses $10, $11
lda #$42
)";
    bool failed = assemblyFailedWithError(code, ".zp_uses outside proc/endproc block");
    CHECK(failed, ".zp_uses outside proc fails with expected error");
}

// Test 11: .zp_clobbers outside proc (error case)
void test_zp_clobbers_outside_proc_error() {
    std::string code = R"(.org $2000
.zp_clobbers $20, $21
lda #$42
)";
    bool failed = assemblyFailedWithError(code, ".zp_clobbers outside proc/endproc block");
    CHECK(failed, ".zp_clobbers outside proc fails with expected error");
}

// Test 12: .zp_release outside proc (error case)
void test_zp_release_outside_proc_error() {
    std::string code = R"(.org $2000
.zp_release $30, $31
lda #$42
)";
    bool failed = assemblyFailedWithError(code, ".zp_release outside proc/endproc block");
    CHECK(failed, ".zp_release outside proc fails with expected error");
}

// Test 13: .reg_clobbers outside proc (error case)
void test_reg_clobbers_outside_proc_error() {
    std::string code = R"(.org $2000
.reg_clobbers A, X, Y
lda #$42
)";
    bool failed = assemblyFailedWithError(code, ".reg_clobbers outside proc/endproc block");
    CHECK(failed, ".reg_clobbers outside proc fails with expected error");
}

// Test 14: .flag_clobbers outside proc (error case)
void test_flag_clobbers_outside_proc_error() {
    std::string code = R"(.org $2000
.flag_clobbers C, N, Z, V
lda #$42
)";
    bool failed = assemblyFailedWithError(code, ".flag_clobbers outside proc/endproc block");
    CHECK(failed, ".flag_clobbers outside proc fails with expected error");
}

// Test 15: Unknown register in .reg_clobbers
void test_unknown_register_in_reg_clobbers() {
    std::string code = R"(.org $2000
proc foo
    .reg_clobbers Q
    lda #$42
endproc
)";
    bool failed = assemblyFailedWithError(code, "unknown register");
    CHECK(failed, "Unknown register Q in .reg_clobbers fails with expected error");
}

// Test 16: Unknown flag in .flag_clobbers
void test_unknown_flag_in_flag_clobbers() {
    std::string code = R"(.org $2000
proc foo
    .flag_clobbers X
    lda #$42
endproc
)";
    bool failed = assemblyFailedWithError(code, "unknown flag");
    CHECK(failed, "Unknown flag X in .flag_clobbers fails with expected error");
}

// Test 17: ZP address out of range in .zp_uses
void test_zp_uses_out_of_range() {
    std::string code = R"(.org $2000
proc foo
    .zp_uses $FFFF
    lda #$42
endproc
)";
    bool failed = assemblyFailedWithError(code, "out of range");
    CHECK(failed, "ZP address $FFFF out of range fails with expected error");
}

// Test 18: .array with missing dimension (error case)
void test_array_missing_dimension_error() {
    std::string code = R"(.org $2000
.array myarr, 2
)";
    bool failed = assemblyFailedWithError(code, "requires at least one dimension");
    CHECK(failed, ".array without dimension fails with expected error");
}

// Test 19: Valid .array with dimension
void test_array_with_dimension() {
    std::string code = R"(.org $2000
.array myarr, 2, 4
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".array with dimension assembles successfully");
}

// Test 20: Valid .array with multiple dimensions
void test_array_with_multiple_dimensions() {
    std::string code = R"(.org $2000
.array myarr, 4, 10, 20
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, ".array with multiple dimensions assembles successfully");
}

// Test 21: All function attributes in one proc
void test_all_function_attributes() {
    std::string code = R"(.org $2000
proc foo
    .zp_uses $03, $04
    .zp_clobbers $05, $06
    .zp_release $07, $08
    .reg_clobbers A, X
    .flag_clobbers C, Z
    lda #$42
endproc
)";
    bool succeeded = assemblySucceeded(code);
    CHECK(succeeded, "All function attributes together assemble successfully");
}

int main() {
    printf("Testing proc/endproc and function attribute validation...\n\n");

    // Valid proc/endproc tests
    test_valid_proc_endproc();
    test_proc_with_arguments();
    test_endproc_rts();
    test_endproc_rts_cleanup();

    // Valid function attribute tests (inside proc)
    test_zp_uses_inside_proc();
    test_zp_clobbers_inside_proc();
    test_zp_release_inside_proc();
    test_reg_clobbers_inside_proc();
    test_flag_clobbers_inside_proc();
    test_all_function_attributes();

    // Error case tests: attributes outside proc
    test_zp_uses_outside_proc_error();
    test_zp_clobbers_outside_proc_error();
    test_zp_release_outside_proc_error();
    test_reg_clobbers_outside_proc_error();
    test_flag_clobbers_outside_proc_error();

    // Error case tests: invalid register/flag
    test_unknown_register_in_reg_clobbers();
    test_unknown_flag_in_flag_clobbers();

    // Error case tests: ZP out of range
    test_zp_uses_out_of_range();

    // Array directive tests
    test_array_missing_dimension_error();
    test_array_with_dimension();
    test_array_with_multiple_dimensions();

    printf("\nProc/Function Attribute Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
