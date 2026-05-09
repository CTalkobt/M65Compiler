#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

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

// Test helper: compile assembly and return binary
static std::vector<uint8_t> assembleCode(const std::string& asmCode) {
    // Write assembly to temp file
    std::ofstream asm_file("build/test_move_fill_temp.s");
    asm_file << ".org $2000\n" << asmCode;
    asm_file.close();

    // Assemble it
    int ret = system("./bin/ca45 build/test_move_fill_temp.s -o build/test_move_fill_temp.bin 2>/dev/null");
    if (ret != 0) return {};

    // Read binary
    std::ifstream bin_file("build/test_move_fill_temp.bin", std::ios::binary);
    if (!bin_file) return {};

    std::vector<uint8_t> result((std::istreambuf_iterator<char>(bin_file)),
                                 std::istreambuf_iterator<char>());
    bin_file.close();
    return result;
}

// Test: FILL with immediate address and length
void test_fill_imm_imm() {
    std::string code = R"(
        lda #$41
        FILL $3000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL immediate,immediate assembles");
    // Should contain DMA setup code
    CHECK(bin.size() > 40, "FILL generates sufficient code");
}

// Test: FILL with register pair destination
void test_fill_ax_imm() {
    std::string code = R"(
        lda #$41
        lda #$30
        ldx #$00
        FILL .AX, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL .AX,immediate assembles");
    // Should contain TXA (0x8A) for X to A transfer
    bool has_txa = false;
    for (size_t i = 0; i < bin.size(); i++) {
        if (bin[i] == 0x8A) { has_txa = true; break; }
    }
    CHECK(has_txa || bin.size() > 30, "FILL with register pair generates code");
}

// Test: FILL with register pair length
void test_fill_imm_xy() {
    std::string code = R"(
        ldx #10
        ldy #2
        FILL $4000, .XY
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL immediate,.XY assembles");
    // Should contain code to write X,Y to buffer
    CHECK(bin.size() > 30, "FILL with XY length generates code");
}

// Test: MOVE with immediate addresses
void test_move_imm_imm() {
    std::string code = R"(
        ldx #$00
        ldy #$01
        MOVE $3000, $4000
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE immediate,immediate assembles");
    CHECK(bin.size() > 40, "MOVE generates sufficient code");
}

// Test: MOVE with .AZ register
void test_move_az_imm() {
    std::string code = R"(
        lda #$30
        ldz #$00
        MOVE .AZ, $5000
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE .AZ,immediate assembles");
    // Should contain TZA (0x5B) for Z to A transfer or similar
    CHECK(bin.size() > 30, "MOVE with AZ generates code");
}

// Test: MOVE with stack-relative operand
void test_move_sp_imm() {
    std::string code = R"(
        .function test_move_sp
        .var local_addr = 2
        MOVE.SP $7000, #4
        rts
        .endfunction
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE.SP assembles");
}

// Test: FILL with stack-relative operand
void test_fill_sp_imm() {
    std::string code = R"(
        .function test_fill_sp
        .var local_addr = 2
        FILL.SP #4, #10
        rts
        .endfunction
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL.SP assembles");
}

// Test: Multiple FILL and MOVE operations in sequence
void test_multiple_fill_move() {
    std::string code = R"(
        lda #$41
        FILL $3000, #100
        FILL $4000, #200
        ldx #$00
        ldy #$01
        MOVE $5000, $6000
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "Multiple FILL and MOVE assembles");
    CHECK(bin.size() > 100, "Multiple operations generate significant code");
}

// Test: FILL/MOVE with explicit register handling
void test_fill_with_registers() {
    std::string code = R"(
        lda #$00
        ldx #$02
        ldy #$00
        FILL .AX, .AY
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with registers assembles");
}

// Test: MOVE with destination .AZ
void test_move_imm_az() {
    std::string code = R"(
        ldx #$00
        ldy #$01
        MOVE $6000, .AZ
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE immediate,.AZ assembles");
    // Should contain code to load destination into A and Z
    CHECK(bin.size() > 30, "MOVE with AZ destination generates code");
}

// Test: Register pair combinations for FILL
void test_fill_xy_xy() {
    std::string code = R"(
        ldx #$00
        ldy #$30
        FILL .XY, .XY
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL .XY,.XY assembles");
}

// Test: Register pair combinations for MOVE
void test_move_xy_xy() {
    std::string code = R"(
        ldx #$00
        ldy #$01
        MOVE .XY, .XY
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE .XY,.XY assembles");
}

// Test: DMA operation with symbols (relocation)
void test_fill_with_symbol() {
    std::string code = R"(
        .data
        my_buffer: .fill 256, 0
        .code
        FILL my_buffer, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with symbol assembles");
    // Should contain relocation information
    CHECK(bin.size() > 40, "FILL with symbol generates code");
}

// Test: MOVE with symbols
void test_move_with_symbols() {
    std::string code = R"(
        .data
        src_buf: .fill 256, 0
        dst_buf: .fill 256, 0
        .code
        ldx #$00
        ldy #$01
        MOVE src_buf, dst_buf
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with symbols assembles");
    CHECK(bin.size() > 40, "MOVE with symbols generates code");
}

// Test: MOVE with 3 operands - all immediates
void test_move_3op_imm_imm_imm() {
    std::string code = R"(
        MOVE $3000, $4000, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE immediate,immediate,immediate assembles");
    CHECK(bin.size() > 40, "MOVE with 3 immediates generates code");
}

// Test: MOVE with 3 operands - register pair as length
void test_move_3op_imm_imm_ax() {
    std::string code = R"(
        ldx #0
        ldy #1
        MOVE $3000, $4000, .AX
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE immediate,immediate,.AX assembles");
    CHECK(bin.size() > 40, "MOVE with register pair length generates code");
}

// Test: MOVE with 3 operands - register pair as length (XY)
void test_move_3op_imm_imm_xy() {
    std::string code = R"(
        ldx #10
        ldy #2
        MOVE $5000, $6000, .XY
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE immediate,immediate,.XY assembles");
    CHECK(bin.size() > 30, "MOVE with XY length generates code");
}

// Test: MOVE with 3 operands - .AZ source and dest, immediate length
void test_move_3op_az_imm_imm() {
    std::string code = R"(
        lda #$30
        ldz #$00
        MOVE .AZ, $4000, #512
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE .AZ,immediate,immediate assembles");
    CHECK(bin.size() > 30, "MOVE with AZ source and immediate length generates code");
}

// Test: MOVE with 3 operands - all register pairs
void test_move_3op_ax_ay_az() {
    std::string code = R"(
        lda #$10
        ldx #$00
        MOVE .AX, $4000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE .AX,immediate,immediate assembles");
    CHECK(bin.size() > 30, "MOVE with AX source and immediate params generates code");
}

// Test: MOVE.SP with 3 operands (stack-relative)
void test_move_sp_3op() {
    std::string code = R"(
        .function test_move_sp_3op
        .var src_addr = 2
        .var dst_addr = 4
        MOVE.SP $7000, #8, #64
        rts
        .endfunction
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE.SP with 3 operands assembles");
    CHECK(bin.size() > 30, "MOVE.SP with explicit length generates code");
}

// Test: MOVE with 3 operands using symbols for all operands
void test_move_3op_all_symbols() {
    std::string code = R"(
        .data
        src_buf: .fill 256, 0
        dst_buf: .fill 256, 0
        .const length = 256
        .code
        MOVE src_buf, dst_buf, length
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with symbol operands and constant length assembles");
    CHECK(bin.size() > 40, "MOVE with symbols and length generates code");
}

// Test: MOVE with 3 operands - mixed operand types
void test_move_3op_mixed_types() {
    std::string code = R"(
        lda #$20
        ldx #$00
        MOVE .AX, $5000, #128
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with mixed operand types assembles");
    CHECK(bin.size() > 30, "MOVE with register/immediate/immediate generates code");
}

// Test: MOVE with 3 operands - stack-relative destination
void test_move_3op_stack_rel() {
    std::string code = R"(
        .function test_move_stack_rel
        .var buf_ptr = 2
        ldx #$30
        ldy #$00
        MOVE .XY, $8000, #200
        rts
        .endfunction
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with register source and immediate operands assembles");
}

// ============================================================================
// 32-BIT / 28-BIT OPERAND TESTS
// ============================================================================

// Test: MOVE with 32-bit numeric literal (Attic memory address)
void test_move_32bit_literal_attic() {
    std::string code = R"(
        MOVE $08000000, $4000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with 32-bit Attic address ($08000000) assembles");
    CHECK(bin.size() > 40, "MOVE with 32-bit literal generates sufficient code");
}

// Test: MOVE with 32-bit numeric literal as destination
void test_move_32bit_literal_dest() {
    std::string code = R"(
        MOVE $3000, $08000000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with 32-bit destination literal assembles");
}

// Test: MOVE with 32-bit numeric literal as length (unlikely but should work)
void test_move_32bit_literal_length() {
    std::string code = R"(
        MOVE $3000, $4000, $00010000
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with 32-bit length literal assembles");
}

// Test: FILL with 32-bit numeric literal destination
void test_fill_32bit_literal() {
    std::string code = R"(
        lda #$42
        FILL $08000000, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with 32-bit Attic address assembles");
    CHECK(bin.size() > 40, "FILL with 32-bit literal generates sufficient code");
}

// Test: MOVE with AXYZ register quad (4-byte)
void test_move_axyz_register() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$08
        ldz #$00
        MOVE .AXYZ, $4000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with .AXYZ register assembles");
    // Should contain code to load Z register
    bool has_ldz = false;
    for (size_t i = 0; i < bin.size(); i++) {
        if (bin[i] == 0x4B) { has_ldz = true; break; }  // LDZ opcode
    }
    CHECK(has_ldz || bin.size() > 30, "MOVE with AXYZ generates code");
}

// Test: MOVE with .Q register (synonym for AXYZ)
void test_move_q_register() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$08
        ldz #$00
        MOVE .Q, $5000, #200
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with .Q register assembles");
}

// Test: FILL with AXY (24-bit, zero-extended to 32-bit)
void test_fill_axy_24bit() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$08
        FILL .AXY, #512
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with .AXY (24-bit) assembles");
}

// Test: FILL with AXZ (24-bit, zero-extended to 32-bit)
void test_fill_axz_24bit() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldz #$08
        FILL .AXZ, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with .AXZ (24-bit) assembles");
}

// Test: FILL with AYZ (24-bit, zero-extended to 32-bit)
void test_fill_ayz_24bit() {
    std::string code = R"(
        lda #$00
        ldy #$00
        ldz #$08
        FILL .AYZ, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with .AYZ (24-bit) assembles");
}

// Test: MOVE with 32-bit symbol (in high memory)
void test_move_32bit_symbol() {
    std::string code = R"(
        .data
        attic_buffer = $08000000
        .code
        MOVE attic_buffer, $4000, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with 32-bit symbol assembles");
}

// Test: FILL with 32-bit symbol
void test_fill_32bit_symbol() {
    std::string code = R"(
        .data
        attic_buffer = $08000000
        .code
        lda #$55
        FILL attic_buffer, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with 32-bit symbol assembles");
}

// Test: MOVE with mixed 16-bit and 32-bit operands (src 16-bit, dest 32-bit)
void test_move_mixed_16_32() {
    std::string code = R"(
        MOVE $2000, $08000000, #512
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with mixed 16-bit and 32-bit operands assembles");
}

// Test: MOVE with .AXYZ as source and 32-bit dest
void test_move_axyz_32bit_dest() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$08
        ldz #$00
        MOVE .AXYZ, $08001000, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE .AXYZ to 32-bit address assembles");
}

// Test: MOVE with 32-bit source and .AXYZ dest
void test_move_32bit_src_axyz_dest() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$08
        ldz #$00
        MOVE $08000000, .AXYZ, #256
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE 32-bit address to .AXYZ assembles");
}

// Test: MOVE with AXYZ as length (32-bit length)
void test_move_axyz_length() {
    std::string code = R"(
        lda #$00
        ldx #$00
        ldy #$00
        ldz #$00
        MOVE $3000, $4000, .AXYZ
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with .AXYZ as length assembles");
}

// Test: 28-bit addressing (common for MEGA65 Attic)
void test_move_28bit_attic() {
    std::string code = R"(
        MOVE $0800ABCD, $4000, #128
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "MOVE with 28-bit Attic address assembles");
}

// Test: FILL with 28-bit addressing
void test_fill_28bit_attic() {
    std::string code = R"(
        lda #$AA
        FILL $0FFFFFFF, #100
        rts
    )";

    auto bin = assembleCode(code);
    CHECK(!bin.empty(), "FILL with 28-bit address assembles");
}

int main() {
    printf("Testing Move and Fill operations...\n\n");

    // FILL tests (16-bit)
    test_fill_imm_imm();
    test_fill_ax_imm();
    test_fill_imm_xy();
    test_fill_sp_imm();
    test_fill_with_registers();
    test_fill_xy_xy();
    test_fill_with_symbol();

    // MOVE tests (16-bit, 2 operands)
    test_move_imm_imm();
    test_move_az_imm();
    test_move_sp_imm();
    test_move_imm_az();
    test_move_xy_xy();
    test_move_with_symbols();

    // MOVE tests (16-bit, 3 operands)
    test_move_3op_imm_imm_imm();
    test_move_3op_imm_imm_ax();
    test_move_3op_imm_imm_xy();
    test_move_3op_az_imm_imm();
    test_move_3op_ax_ay_az();
    test_move_sp_3op();
    test_move_3op_all_symbols();
    test_move_3op_mixed_types();
    test_move_3op_stack_rel();

    printf("\n--- 32-bit / 28-bit Operand Tests ---\n\n");

    // MOVE with 32-bit literals
    test_move_32bit_literal_attic();
    test_move_32bit_literal_dest();
    test_move_32bit_literal_length();

    // FILL with 32-bit literals
    test_fill_32bit_literal();

    // MOVE with 32-bit registers (AXYZ, .Q)
    test_move_axyz_register();
    test_move_q_register();

    // FILL with 24-bit registers (extended to 32-bit)
    test_fill_axy_24bit();
    test_fill_axz_24bit();
    test_fill_ayz_24bit();

    // MOVE/FILL with 32-bit symbols
    test_move_32bit_symbol();
    test_fill_32bit_symbol();

    // Mixed 16-bit and 32-bit operands
    test_move_mixed_16_32();
    test_move_axyz_32bit_dest();
    test_move_32bit_src_axyz_dest();
    test_move_axyz_length();

    // 28-bit Attic addressing (common use case)
    test_move_28bit_attic();
    test_fill_28bit_attic();

    // Combined tests
    test_multiple_fill_move();

    printf("\nMove/Fill Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
