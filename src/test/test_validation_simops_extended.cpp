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
        printf("PASS: %s\n", msg); \
        tests_passed++; \
    } \
} while(0)

// Test helper: assemble code and return stderr output
static std::string runCA45(const char* asm_src) {
    // Write assembly to temp file
    std::ofstream asm_file("build/test_validation_simops_extended_temp.s");
    asm_file << ".org $2000\n" << asm_src;
    asm_file.close();

    // Assemble and capture stderr
    system("./bin/ca45 build/test_validation_simops_extended_temp.s -o build/test_validation_simops_extended_temp.bin 2>build/test_validation_simops_extended_temp.err >/dev/null");

    // Read stderr
    std::ifstream err_file("build/test_validation_simops_extended_temp.err");
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
    std::ofstream asm_file("build/test_validation_simops_extended_temp.s");
    asm_file << ".org $2000\n" << asm_src;
    asm_file.close();

    int ret = system("./bin/ca45 build/test_validation_simops_extended_temp.s -o build/test_validation_simops_extended_temp.bin 2>/dev/null");
    return ret == 0;
}

// Bitfield operations tests

void test_bfext_valid() {
    const char* code = R"(
        bfext A, #5, $1000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFEXT with immediate and address succeeds");
}

void test_bfext16_valid() {
    const char* code = R"(
        bfext16 A, #8, $2000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFEXT16 variant succeeds");
}

void test_bfext_valid_operand() {
    const char* code = R"(
        bfext A, #3, $1000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFEXT with second operand succeeds");
}

void test_bfins_valid() {
    const char* code = R"(
        bfins A, #3, #$FF, $2000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFINS with immediates and address succeeds");
}

void test_bfins16_valid() {
    const char* code = R"(
        bfins16 X, #2, #$7F, $3000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFINS16 variant succeeds");
}

void test_bfins_three_args() {
    const char* code = R"(
        bfins A, #3, #$FF
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "BFINS with three arguments succeeds");
}

// Math operations tests

void test_mul_unsigned_valid() {
    const char* code = R"(
        mul A, B
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MUL (unsigned 8x8) succeeds");
}

void test_div_unsigned_valid() {
    const char* code = R"(
        div A, B
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "DIV (unsigned) succeeds");
}

void test_mul_s16_valid() {
    const char* code = R"(
        mul.s16 .AX, #$100
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MUL.S16 (signed 16-bit) succeeds");
}

void test_div_s16_valid() {
    const char* code = R"(
        div.s16 .AX, #2
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "DIV.S16 (signed 16-bit) succeeds");
}

void test_mul16_valid() {
    const char* code = R"(
        mul.16 .AX, $1000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MUL.16 (16-bit) succeeds");
}

void test_div16_valid() {
    const char* code = R"(
        div.16 .AX, $1000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "DIV.16 (16-bit) succeeds");
}

void test_mod16_valid() {
    const char* code = R"(
        mod.16 .AX, $1000
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MOD.16 (modulo) succeeds");
}

// Sign extension tests

void test_sxt8_valid() {
    const char* code = R"(
        sxt.8
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SXT.8 (sign-extend byte) succeeds");
}

void test_sxt16_valid() {
    const char* code = R"(
        sxt.16 .AX
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SXT.16 (sign-extend word) succeeds");
}

// Push/Pop stack operations tests

void test_push_a_valid() {
    const char* code = R"(
        push A
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "PUSH A succeeds");
}

void test_push_ax_valid() {
    const char* code = R"(
        push .AX
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "PUSH .AX (16-bit) succeeds");
}

void test_push_axy_valid() {
    const char* code = R"(
        push .AXY
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "PUSH .AXY (24-bit) succeeds");
}

void test_pop_a_valid() {
    const char* code = R"(
        pop A
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "POP A succeeds");
}

void test_pop_ax_valid() {
    const char* code = R"(
        pop .AX
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "POP .AX (16-bit) succeeds");
}

void test_pop_axyz_valid() {
    const char* code = R"(
        pop .AXYZ
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "POP .AXYZ (32-bit) succeeds");
}

void test_multiple_push_pop_valid() {
    const char* code = R"(
        push A
        push X
        pop X
        pop A
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "Multiple PUSH/POP in sequence succeeds");
}

// Memory block operations tests

void test_fill_basic_valid() {
    const char* code = R"(
        fill A, $1000, 256
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "FILL (basic) succeeds");
}

void test_fill_sp_valid() {
    const char* code = R"(
        fill.sp A, 16
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "FILL.SP (stack) succeeds");
}

void test_move_basic_valid() {
    const char* code = R"(
        move $1000, $2000, 256
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MOVE (basic) succeeds");
}

void test_move_sp_valid() {
    const char* code = R"(
        move.sp $04, 16
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "MOVE.SP (stack) succeeds");
}

// Register-specific operations tests

void test_swap_ax_valid() {
    const char* code = R"(
        swap A, X
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SWAP A, X succeeds");
}

void test_swap_xy_valid() {
    const char* code = R"(
        swap X, Y
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SWAP X, Y succeeds");
}

void test_swap_yz_valid() {
    const char* code = R"(
        swap Y, Z
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SWAP Y, Z succeeds");
}

void test_swap_za_valid() {
    const char* code = R"(
        swap Z, A
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "SWAP Z, A succeeds");
}

void test_swap_b_invalid() {
    const char* code = R"(
        swap B
    )";
    bool failed = assembleFailed(code, "SWAP only supports A, X, Y, Z");
    CHECK(failed, "SWAP B fails with expected error");
}

void test_zero_a_valid() {
    const char* code = R"(
        zero A
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "ZERO A succeeds");
}

void test_zero_x_valid() {
    const char* code = R"(
        zero X
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "ZERO X succeeds");
}

void test_zero_y_valid() {
    const char* code = R"(
        zero Y
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "ZERO Y succeeds");
}

void test_zero_z_valid() {
    const char* code = R"(
        zero Z
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "ZERO Z succeeds");
}

void test_zero_q_invalid() {
    const char* code = R"(
        zero Q
    )";
    bool failed = assembleFailed(code, "ZERO only supports A, X, Y, Z");
    CHECK(failed, "ZERO Q fails with expected error");
}

void test_zero_multiple_valid() {
    const char* code = R"(
        zero A, X, Y, Z
    )";
    bool passed = assembleSucceeded(code);
    CHECK(passed, "ZERO with multiple registers succeeds");
}

int main() {
    printf("Extended Simulated Ops Validation Tests\n");
    printf("========================================\n\n");

    // Bitfield operations tests
    test_bfext_valid();
    test_bfext16_valid();
    test_bfext_valid_operand();
    test_bfins_valid();
    test_bfins16_valid();
    test_bfins_three_args();

    // Math operations tests
    test_mul_unsigned_valid();
    test_div_unsigned_valid();
    test_mul_s16_valid();
    test_div_s16_valid();
    test_mul16_valid();
    test_div16_valid();
    test_mod16_valid();

    // Sign extension tests
    test_sxt8_valid();
    test_sxt16_valid();

    // Push/Pop stack operations tests
    test_push_a_valid();
    test_push_ax_valid();
    test_push_axy_valid();
    test_pop_a_valid();
    test_pop_ax_valid();
    test_pop_axyz_valid();
    test_multiple_push_pop_valid();

    // Memory block operations tests
    test_fill_basic_valid();
    test_fill_sp_valid();
    test_move_basic_valid();
    test_move_sp_valid();

    // Register-specific operations tests
    test_swap_ax_valid();
    test_swap_xy_valid();
    test_swap_yz_valid();
    test_swap_za_valid();
    test_swap_b_invalid();

    test_zero_a_valid();
    test_zero_x_valid();
    test_zero_y_valid();
    test_zero_z_valid();
    test_zero_q_invalid();
    test_zero_multiple_valid();

    printf("\nExtended Simulated Ops Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
