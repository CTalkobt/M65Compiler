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

// --- 16-bit shift/rotate operations with .AX register ---

void test_lsl16_ax_valid() {
    CHECK(assembleSucceeded("lsl.16 .AX"), "LSL.16 .AX succeeds");
}
void test_lsr16_ax_valid() {
    CHECK(assembleSucceeded("lsr.16 .AX"), "LSR.16 .AX succeeds");
}
void test_rol16_ax_valid() {
    CHECK(assembleSucceeded("rol.16 .AX"), "ROL.16 .AX succeeds");
}
void test_ror16_ax_valid() {
    CHECK(assembleSucceeded("ror.16 .AX"), "ROR.16 .AX succeeds");
}
void test_asr16_ax_valid() {
    CHECK(assembleSucceeded("asr.16 .AX"), "ASR.16 .AX succeeds");
}

// --- 16-bit shift/rotate with memory operand ---

void test_lsl16_mem_valid() {
    CHECK(assembleSucceeded("lsl.16 $3000"), "LSL.16 $3000 (memory) succeeds");
}
void test_lsr16_mem_valid() {
    CHECK(assembleSucceeded("lsr.16 $3000"), "LSR.16 $3000 (memory) succeeds");
}
void test_rol16_mem_valid() {
    CHECK(assembleSucceeded("rol.16 $3000"), "ROL.16 $3000 (memory) succeeds");
}
void test_ror16_mem_valid() {
    CHECK(assembleSucceeded("ror.16 $3000"), "ROR.16 $3000 (memory) succeeds");
}
void test_asr16_mem_valid() {
    CHECK(assembleSucceeded("asr.16 $3000"), "ASR.16 $3000 (memory) succeeds");
}

// --- ASW / ROW (16-bit arithmetic/rotate word) ---

void test_asw_ax_valid() {
    CHECK(assembleSucceeded("asw .AX"), "ASW .AX succeeds");
}
void test_asw_mem_valid() {
    CHECK(assembleSucceeded("asw $3000"), "ASW $3000 (memory) succeeds");
}
void test_row_ax_valid() {
    CHECK(assembleSucceeded("row .AX"), "ROW .AX succeeds");
}
void test_row_mem_valid() {
    CHECK(assembleSucceeded("row $3000"), "ROW $3000 (memory) succeeds");
}

// --- Flat memory operations (28-bit) ---

void test_ldw_f_valid() {
    CHECK(assembleSucceeded("ldw.f $12000"), "LDW.F $12000 (flat load) succeeds");
}
void test_stw_f_valid() {
    CHECK(assembleSucceeded("stw.f $12000"), "STW.F $12000 (flat store) succeeds");
}
void test_inc_f_valid() {
    CHECK(assembleSucceeded("inc.f $12000"), "INC.F $12000 (flat inc) succeeds");
}
void test_dec_f_valid() {
    CHECK(assembleSucceeded("dec.f $12000"), "DEC.F $12000 (flat dec) succeeds");
}

// --- Select (ternary) ---

void test_select_valid() {
    const char* code = R"(
    proc test_select
    .var _fp = 0
    select .A, #$10, #$20
    endproc
    )";
    CHECK(assembleSucceeded(code), "SELECT .A, #$10, #$20 succeeds");
}

// --- Ptrstack (compute stack variable address) ---

void test_ptrstack_global_valid() {
    CHECK(assembleSucceeded("ptrstack $4000"), "PTRSTACK $4000 (global addr) succeeds");
}
void test_ptrstack_stack_valid() {
    const char* code = R"(
    proc test_ptrstack
    .var _fp = 0
    .var myvar = 2
    ptrstack myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "PTRSTACK myvar (stack-relative) succeeds");
}

// --- Ptrderef (dereference pointer in ZP) ---

void test_ptrderef_valid() {
    CHECK(assembleSucceeded("ptrderef $20"), "PTRDEREF $20 (ZP pointer deref) succeeds");
}

// --- PHW.SP (push word from stack) ---

void test_phw_sp_valid() {
    const char* code = R"(
    proc test_phw
    .var _fp = 0
    .var myvar = 2
    phw.sp myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "PHW.SP myvar (stack push) succeeds");
}

// --- Frame pointer operations (8-bit) ---

void test_lda_fp_valid() {
    const char* code = R"(
    proc test_fp
    .var _fp = 0
    phw #0
    .local myvar = 0
    lda.fp myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "LDA.FP myvar (8-bit frame load) succeeds");
}
void test_sta_fp_valid() {
    const char* code = R"(
    proc test_fp
    .var _fp = 0
    phw #0
    .local myvar = 0
    sta.fp myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "STA.FP myvar (8-bit frame store) succeeds");
}

// --- MOVE.FP (frame block copy) ---

void test_move_fp_valid() {
    const char* code = R"(
    proc test_move_fp
    .var _fp = 0
    phw #0
    phw #0
    .local src = 0
    .local dst = 2
    move.fp dst, src, 2
    endproc
    )";
    CHECK(assembleSucceeded(code), "MOVE.FP dst, src, 2 (frame copy) succeeds");
}

// --- Division with various widths and operand modes ---

void test_div8_imm_valid() {
    CHECK(assembleSucceeded("div.8 .A, #5"), "DIV.8 .A, #5 (immediate divisor) succeeds");
}
void test_div8_mem_valid() {
    CHECK(assembleSucceeded("div.8 .A, $3000"), "DIV.8 .A, $3000 (memory divisor) succeeds");
}
void test_div16_imm_valid() {
    CHECK(assembleSucceeded("div.16 .AX, #100"), "DIV.16 .AX, #100 (immediate) succeeds");
}
void test_div16_mem_dest_valid() {
    CHECK(assembleSucceeded("div.16 $2000, #10"), "DIV.16 $2000, #10 (memory dest) succeeds");
}
void test_div32_reg_valid() {
    CHECK(assembleSucceeded("div.32 .Q, #1000"), "DIV.32 .Q, #1000 (32-bit immediate) succeeds");
}
void test_div_by_zero_error() {
    CHECK(assembleFailed("div.8 .A, #0", "Division by zero"), "DIV.8 .A, #0 fails with division by zero");
}

// --- Multiplication with various modes ---

void test_mul8_imm_valid() {
    CHECK(assembleSucceeded("mul.8 .A, #7"), "MUL.8 .A, #7 (immediate) succeeds");
}
void test_mul16_imm_valid() {
    CHECK(assembleSucceeded("mul.16 .AX, #50"), "MUL.16 .AX, #50 (immediate) succeeds");
}
void test_mul32_reg_valid() {
    CHECK(assembleSucceeded("mul.32 .Q, #100"), "MUL.32 .Q, #100 (32-bit) succeeds");
}
void test_mul16_mem_dest_valid() {
    CHECK(assembleSucceeded("mul.16 $2000, #10"), "MUL.16 $2000, #10 (memory dest) succeeds");
}

// --- Modulo ---

void test_mod16_imm_valid() {
    CHECK(assembleSucceeded("mod.16 .AX, #7"), "MOD.16 .AX, #7 (immediate) succeeds");
}

// --- Inc/Dec stack (16-bit) ---

void test_inw_stack_valid() {
    const char* code = R"(
    proc test_inw
    .var _fp = 0
    .var myvar = 2
    inw myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "INW myvar, s (stack inc word) succeeds");
}
void test_dew_stack_valid() {
    const char* code = R"(
    proc test_dew
    .var _fp = 0
    .var myvar = 2
    dew myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "DEW myvar, s (stack dec word) succeeds");
}

// --- Inc/Dec 8-bit stack ---

void test_inc8_stack_valid() {
    const char* code = R"(
    proc test_inc8
    .var _fp = 0
    .var myvar = 2
    inc myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "INC myvar, s (8-bit stack inc) succeeds");
}
void test_dec8_stack_valid() {
    const char* code = R"(
    proc test_dec8
    .var _fp = 0
    .var myvar = 2
    dec myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "DEC myvar, s (8-bit stack dec) succeeds");
}

// --- Stack load/store for X, Y, Z registers ---

void test_stx_sp_valid() {
    const char* code = R"(
    proc test_stx
    .var _fp = 0
    .var myvar = 2
    stx myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "STX myvar, s (store X to stack) succeeds");
}
void test_sty_sp_valid() {
    const char* code = R"(
    proc test_sty
    .var _fp = 0
    .var myvar = 2
    sty myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "STY myvar, s (store Y to stack) succeeds");
}
void test_stz_sp_valid() {
    const char* code = R"(
    proc test_stz
    .var _fp = 0
    .var myvar = 2
    stz myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "STZ myvar, s (store Z to stack) succeeds");
}
void test_ldx_sp_valid() {
    const char* code = R"(
    proc test_ldx
    .var _fp = 0
    .var myvar = 2
    ldx myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "LDX myvar, s (load X from stack) succeeds");
}
void test_ldy_sp_valid() {
    const char* code = R"(
    proc test_ldy
    .var _fp = 0
    .var myvar = 2
    ldy myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "LDY myvar, s (load Y from stack) succeeds");
}
void test_ldz_sp_valid() {
    const char* code = R"(
    proc test_ldz
    .var _fp = 0
    .var myvar = 2
    ldz myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "LDZ myvar, s (load Z from stack) succeeds");
}

// --- 16-bit neg/not/abs ---

void test_neg16_ax_valid() {
    CHECK(assembleSucceeded("neg.16 .AX"), "NEG.16 .AX succeeds");
}
void test_neg16_mem_valid() {
    CHECK(assembleSucceeded("neg.16 $3000"), "NEG.16 $3000 (memory) succeeds");
}
void test_not16_ax_valid() {
    CHECK(assembleSucceeded("not.16 .AX"), "NOT.16 .AX succeeds");
}
void test_not16_mem_valid() {
    CHECK(assembleSucceeded("not.16 $3000"), "NOT.16 $3000 (memory) succeeds");
}
void test_abs16_ax_valid() {
    CHECK(assembleSucceeded("abs.16 .AX"), "ABS.16 .AX succeeds");
}
void test_abs16_mem_valid() {
    CHECK(assembleSucceeded("abs.16 $3000"), "ABS.16 $3000 (memory) succeeds");
}

// --- CMP.S16 (signed compare) ---

void test_cmp_s16_ax_valid() {
    CHECK(assembleSucceeded("cmp.s16 .AX, $3000"), "CMP.S16 .AX, $3000 succeeds");
}

// --- Branch.16 ---

void test_branch16_valid() {
    const char* code = R"(
    branch.16 $0500
    )";
    CHECK(assembleSucceeded(code), "BRANCH.16 $0500 (16-bit branch) succeeds");
}

// --- Chknonzero / Chkzero ---

void test_chknonzero16_valid() {
    CHECK(assembleSucceeded("chknonzero.16 .AX"), "CHKNONZERO.16 .AX succeeds");
}
void test_chkzero8_valid() {
    CHECK(assembleSucceeded("chkzero.8 .A"), "CHKZERO.8 .A succeeds");
}

// --- Bitfield ops with stack-relative and indirect modes ---

void test_bfins_sp_valid() {
    const char* code = R"(
    proc test_bfins_sp
    .var _fp = 0
    .var myvar = 2
    bfins.sp A, #3, #$0F, myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "BFINS.SP A, #3, #$0F, myvar (stack-relative) succeeds");
}
void test_bfins_ind_valid() {
    CHECK(assembleSucceeded("bfins.ind A, #3, #$0F, $20"), "BFINS.IND A, #3, #$0F, $20 (indirect) succeeds");
}
void test_bfins16_sp_valid() {
    const char* code = R"(
    proc test_bfins16_sp
    .var _fp = 0
    .var myvar = 2
    bfins16.sp A, #3, #$0F, myvar
    endproc
    )";
    CHECK(assembleSucceeded(code), "BFINS16.SP (stack-relative 16-bit) succeeds");
}

// --- FILL with various modes ---

void test_fill_3arg_valid() {
    CHECK(assembleSucceeded("fill $1000, 64, #$FF"), "FILL $1000, 64, #$FF (3-arg with value) succeeds");
}
void test_fill_dma_valid() {
    CHECK(assembleSucceeded("fill $10000, $100, #$AA"), "FILL $10000, $100, #$AA (DMA fill) succeeds");
}

// --- MOVE with various modes ---

void test_move_2arg_valid() {
    CHECK(assembleSucceeded("move $1000, $2000"), "MOVE $1000, $2000 (2-arg copy) succeeds");
}
void test_move_dma_valid() {
    CHECK(assembleSucceeded("move $10000, $20000, 256"), "MOVE $10000, $20000, 256 (DMA move) succeeds");
}

// --- Signed multiply/divide ---

void test_mul_s16_mem_valid() {
    CHECK(assembleSucceeded("mul.s16 .AX, $3000"), "MUL.S16 .AX, $3000 (memory) succeeds");
}
void test_div_s16_mem_valid() {
    CHECK(assembleSucceeded("div.s16 .AX, $3000"), "DIV.S16 .AX, $3000 (memory) succeeds");
}

// --- Swap register pairs not yet covered ---

void test_swap_xz_valid() {
    CHECK(assembleSucceeded("swap X, Z"), "SWAP X, Z succeeds");
}
void test_swap_ya_valid() {
    CHECK(assembleSucceeded("swap Y, A"), "SWAP Y, A succeeds");
}

// --- 16-bit add/sub with memory dest ---

void test_add16_mem_valid() {
    CHECK(assembleSucceeded("add.16 $3000, #$100"), "ADD.16 $3000, #$100 (memory dest) succeeds");
}
void test_sub16_mem_valid() {
    CHECK(assembleSucceeded("sub.16 $3000, #$100"), "SUB.16 $3000, #$100 (memory dest) succeeds");
}

// --- 16-bit bitwise with .AX dest (memory dest not supported) ---

void test_and16_ax_valid() {
    CHECK(assembleSucceeded("and.16 .AX, #$FF00"), "AND.16 .AX, #$FF00 succeeds");
}
void test_ora16_ax_valid() {
    CHECK(assembleSucceeded("ora.16 .AX, #$00FF"), "ORA.16 .AX, #$00FF succeeds");
}
void test_eor16_ax_valid() {
    CHECK(assembleSucceeded("eor.16 .AX, #$FFFF"), "EOR.16 .AX, #$FFFF succeeds");
}
void test_and16_mem_rejected() {
    CHECK(assembleFailed("and.16 $3000, #$FF00", "only supports .AX"), "AND.16 $3000 rejected (only .AX dest)");
}

// --- 32-bit add/sub with immediate ---

void test_add32_imm_valid() {
    CHECK(assembleSucceeded("add.32 .Q, #1000"), "ADD.32 .Q, #1000 (immediate) succeeds");
}
void test_sub32_imm_valid() {
    CHECK(assembleSucceeded("sub.32 .Q, #500"), "SUB.32 .Q, #500 (immediate) succeeds");
}

// --- Expression code (ldw/stw to named register pairs) ---

void test_ldw_ax_imm() {
    CHECK(assembleSucceeded("ldax #$1234"), "LDAX #$1234 (immediate word load) succeeds");
}
void test_stw_ax_mem() {
    CHECK(assembleSucceeded("stax $3000"), "STAX $3000 (16-bit store) succeeds");
}
void test_ldw_xy_imm() {
    CHECK(assembleSucceeded("ldxy #$5678"), "LDXY #$5678 (immediate word load XY) succeeds");
}

// ====================================================================
// Round 2: Deeper code path coverage for complex simulated ops
// ====================================================================

// --- FILL: register-pair destinations and 24/32-bit widths ---

void test_fill_reg_ax_valid() {
    CHECK(assembleSucceeded("fill .AX, 128"), "FILL .AX, 128 (register dest) succeeds");
}
void test_fill_reg_axy_valid() {
    CHECK(assembleSucceeded("fill .AXY, 64"), "FILL .AXY, 64 (24-bit register dest) succeeds");
}
void test_fill_reg_axyz_valid() {
    CHECK(assembleSucceeded("fill .AXYZ, 32"), "FILL .AXYZ, 32 (32-bit register dest) succeeds");
}
void test_fill_reg_xy_valid() {
    CHECK(assembleSucceeded("fill .XY, 100"), "FILL .XY, 100 (XY register dest) succeeds");
}
void test_fill_reg_axz_valid() {
    CHECK(assembleSucceeded("fill .AXZ, 50"), "FILL .AXZ, 50 (AXZ register dest) succeeds");
}
void test_fill_reg_ayz_valid() {
    CHECK(assembleSucceeded("fill .AYZ, 50"), "FILL .AYZ, 50 (AYZ register dest) succeeds");
}
void test_fill_reg_len_valid() {
    CHECK(assembleSucceeded("fill $1000, .AX"), "FILL $1000, .AX (register length) succeeds");
}
void test_fill_large_addr_valid() {
    CHECK(assembleSucceeded("fill $80000, 256"), "FILL $80000, 256 (32-bit address) succeeds");
}
void test_fill_stack_rel_valid() {
    const char* code = R"(
    __sp_base = $0101
    proc test_fill_stack
    .var _fp = 0
    .var mybuf = 2
    fill.sp mybuf, 16
    endproc
    )";
    CHECK(assembleSucceeded(code), "FILL.SP mybuf, 16 (stack-relative dest) succeeds");
}

// --- MOVE: register-pair operands, stack-relative, various forms ---

void test_move_reg_ax_valid() {
    CHECK(assembleSucceeded("move .AX, $2000, 64"), "MOVE .AX, $2000, 64 (register dest) succeeds");
}
void test_move_reg_axyz_valid() {
    CHECK(assembleSucceeded("move .AXYZ, $2000, 32"), "MOVE .AXYZ, $2000, 32 (32-bit register dest) succeeds");
}
void test_move_reg_src_valid() {
    CHECK(assembleSucceeded("move $1000, .AX, 64"), "MOVE $1000, .AX, 64 (register src) succeeds");
}
void test_move_large_addr_valid() {
    CHECK(assembleSucceeded("move $80000, $90000, 1024"), "MOVE $80000, $90000, 1024 (32-bit addresses) succeeds");
}
void test_move_stack_rel_valid() {
    const char* code = R"(
    __sp_base = $0101
    proc test_move_stack
    .var _fp = 0
    .var src = 2
    .var dst = 4
    move.sp src, 8
    endproc
    )";
    CHECK(assembleSucceeded(code), "MOVE.SP src, 8 (stack-relative) succeeds");
}

// --- Signed math: mod.s16, constant-negative operands ---

void test_mod_s16_valid() {
    CHECK(assembleSucceeded("mod.s16 .AX, #7"), "MOD.S16 .AX, #7 (signed modulo) succeeds");
}
void test_mul_s16_neg_const() {
    CHECK(assembleSucceeded("mul.s16 .AX, #-10"), "MUL.S16 .AX, #-10 (negative constant) succeeds");
}
void test_div_s16_neg_const() {
    CHECK(assembleSucceeded("div.s16 .AX, #-5"), "DIV.S16 .AX, #-5 (negative constant) succeeds");
}

// --- Division: memory source (non-immediate), register-pair results ---

void test_div8_reg_src_valid() {
    CHECK(assembleSucceeded("div.8 .A, .X"), "DIV.8 .A, .X (register divisor) succeeds");
}
void test_div16_reg_src_valid() {
    CHECK(assembleSucceeded("div.16 .AX, .AX"), "DIV.16 .AX, .AX (register divisor) succeeds");
}
void test_div16_mem_src_valid() {
    CHECK(assembleSucceeded("div.16 .AX, $3000"), "DIV.16 .AX, $3000 (memory divisor) succeeds");
}

// --- Multiplication: memory source, various widths ---

void test_mul8_reg_src_valid() {
    CHECK(assembleSucceeded("mul.8 .A, .X"), "MUL.8 .A, .X (register multiplier) succeeds");
}
void test_mul16_mem_src_valid() {
    CHECK(assembleSucceeded("mul.16 .AX, $3000"), "MUL.16 .AX, $3000 (memory multiplier) succeeds");
}

// --- Bitfield: more extract/insert addressing modes ---

void test_bfext_wide_field() {
    CHECK(assembleSucceeded("bfext A, #7, $1000"), "BFEXT A, #7, $1000 (wide field) succeeds");
}
void test_bfins_wide_field() {
    CHECK(assembleSucceeded("bfins A, #7, #$7F, $2000"), "BFINS A, #7, #$7F, $2000 (wide field) succeeds");
}
void test_bfext16_wide() {
    CHECK(assembleSucceeded("bfext16 A, #12, $3000"), "BFEXT16 A, #12, $3000 (16-bit wide) succeeds");
}
void test_bfins16_ind_valid() {
    CHECK(assembleSucceeded("bfins16.ind A, #4, #$0F, $20"), "BFINS16.IND (indirect 16-bit) succeeds");
}

// --- Stack inc/dec: frame-relative (with .local) ---

void test_inw_frame_valid() {
    const char* code = R"(
    proc test_inw_frame
    .var _fp = 0
    phw #0
    .local myvar = 0
    inw myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "INW frame-relative (via .local) succeeds");
}
void test_dew_frame_valid() {
    const char* code = R"(
    proc test_dew_frame
    .var _fp = 0
    phw #0
    .local myvar = 0
    dew myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "DEW frame-relative (via .local) succeeds");
}

// --- Push/Pop: additional register sizes ---

void test_push_y_valid() {
    CHECK(assembleSucceeded("push Y"), "PUSH Y succeeds");
}
void test_push_z_valid() {
    CHECK(assembleSucceeded("push Z"), "PUSH Z succeeds");
}
void test_pop_y_valid() {
    CHECK(assembleSucceeded("pop Y"), "POP Y succeeds");
}
void test_pop_z_valid() {
    CHECK(assembleSucceeded("pop Z"), "POP Z succeeds");
}
void test_push_axyz_valid() {
    CHECK(assembleSucceeded("push .AXYZ"), "PUSH .AXYZ (32-bit) succeeds");
}

// --- Neg/Not 16-bit with memory addresses ---

void test_neg16_zp_valid() {
    CHECK(assembleSucceeded("neg.16 $20"), "NEG.16 $20 (zero page) succeeds");
}
void test_not16_zp_valid() {
    CHECK(assembleSucceeded("not.16 $20"), "NOT.16 $20 (zero page) succeeds");
}

// --- CMP.16 with immediate ---

void test_cmp16_imm_valid() {
    CHECK(assembleSucceeded("cmp.16 .AX, #$1234"), "CMP.16 .AX, #$1234 (immediate) succeeds");
}

// --- CMP.S16 with immediate ---

void test_cmp_s16_imm_valid() {
    CHECK(assembleSucceeded("cmp.s16 .AX, #100"), "CMP.S16 .AX, #100 (immediate) succeeds");
}

// --- LDW/STW: stack-relative and memory ---

void test_ldax_mem_valid() {
    CHECK(assembleSucceeded("ldax $3000"), "LDAX $3000 (memory word load) succeeds");
}
void test_stax_stack_valid() {
    const char* code = R"(
    proc test_stax_stack
    .var _fp = 0
    .var myvar = 2
    stax myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "STAX myvar, s (stack word store) succeeds");
}
void test_ldax_stack_valid() {
    const char* code = R"(
    proc test_ldax_stack
    .var _fp = 0
    .var myvar = 2
    ldax myvar, s
    endproc
    )";
    CHECK(assembleSucceeded(code), "LDAX myvar, s (stack word load) succeeds");
}

// --- 16-bit add/sub with immediate operand ---

void test_add16_ax_imm_valid() {
    CHECK(assembleSucceeded("add.16 .AX, #$0100"), "ADD.16 .AX, #$0100 (imm) succeeds");
}
void test_sub16_ax_imm_valid() {
    CHECK(assembleSucceeded("sub.16 .AX, #$0100"), "SUB.16 .AX, #$0100 (imm) succeeds");
}
void test_add16_ax_mem_valid() {
    CHECK(assembleSucceeded("add.16 .AX, $3000"), "ADD.16 .AX, $3000 (memory) succeeds");
}
void test_sub16_ax_mem_valid() {
    CHECK(assembleSucceeded("sub.16 .AX, $3000"), "SUB.16 .AX, $3000 (memory) succeeds");
}

// --- 32-bit operations with memory operands ---

void test_add32_mem_valid() {
    CHECK(assembleSucceeded("add.32 .Q, $3000"), "ADD.32 .Q, $3000 (memory) succeeds");
}
void test_sub32_mem_valid() {
    CHECK(assembleSucceeded("sub.32 .Q, $3000"), "SUB.32 .Q, $3000 (memory) succeeds");
}
void test_and32_valid() {
    CHECK(assembleSucceeded("and.32 .Q, #$FF00FF00"), "AND.32 .Q, #$FF00FF00 succeeds");
}
void test_ora32_valid() {
    CHECK(assembleSucceeded("ora.32 .Q, #$00FF00FF"), "ORA.32 .Q, #$00FF00FF succeeds");
}
void test_eor32_valid() {
    CHECK(assembleSucceeded("eor.32 .Q, #$FFFFFFFF"), "EOR.32 .Q, #$FFFFFFFF succeeds");
}
void test_cmp32_imm_valid() {
    CHECK(assembleSucceeded("cmp.32 .Q, #1000000"), "CMP.32 .Q, #1000000 (immediate) succeeds");
}
void test_cmp32_mem_valid() {
    CHECK(assembleSucceeded("cmp.32 .Q, $3000"), "CMP.32 .Q, $3000 (memory) succeeds");
}
void test_cmp_s32_valid() {
    CHECK(assembleSucceeded("cmp.s32 .Q, #-100"), "CMP.S32 .Q, #-100 (signed) succeeds");
}
void test_neg32_valid() {
    CHECK(assembleSucceeded("neg.32 .Q"), "NEG.32 .Q succeeds");
}
void test_not32_valid() {
    CHECK(assembleSucceeded("not.32 .Q"), "NOT.32 .Q succeeds");
}
void test_abs32_valid() {
    CHECK(assembleSucceeded("abs.32 .Q"), "ABS.32 .Q succeeds");
}
void test_lsl32_valid() {
    CHECK(assembleSucceeded("lsl.32 .Q"), "LSL.32 .Q succeeds");
}
void test_lsr32_valid() {
    CHECK(assembleSucceeded("lsr.32 .Q"), "LSR.32 .Q succeeds");
}
void test_asr32_valid() {
    CHECK(assembleSucceeded("asr.32 .Q"), "ASR.32 .Q succeeds");
}
void test_rol32_valid() {
    CHECK(assembleSucceeded("rol.32 .Q"), "ROL.32 .Q succeeds");
}
void test_ror32_valid() {
    CHECK(assembleSucceeded("ror.32 .Q"), "ROR.32 .Q succeeds");
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

    // --- New tests ---

    // 16-bit shift/rotate with .AX register
    test_lsl16_ax_valid();
    test_lsr16_ax_valid();
    test_rol16_ax_valid();
    test_ror16_ax_valid();
    test_asr16_ax_valid();

    // 16-bit shift/rotate with memory operand
    test_lsl16_mem_valid();
    test_lsr16_mem_valid();
    test_rol16_mem_valid();
    test_ror16_mem_valid();
    test_asr16_mem_valid();

    // ASW / ROW
    test_asw_ax_valid();
    test_asw_mem_valid();
    test_row_ax_valid();
    test_row_mem_valid();

    // Flat memory operations
    test_ldw_f_valid();
    test_stw_f_valid();
    test_inc_f_valid();
    test_dec_f_valid();

    // Select (ternary)
    test_select_valid();

    // Ptrstack / Ptrderef
    test_ptrstack_global_valid();
    test_ptrstack_stack_valid();
    test_ptrderef_valid();

    // PHW.SP
    test_phw_sp_valid();

    // Frame pointer 8-bit ops
    test_lda_fp_valid();
    test_sta_fp_valid();

    // MOVE.FP
    test_move_fp_valid();

    // Division variants
    test_div8_imm_valid();
    test_div8_mem_valid();
    test_div16_imm_valid();
    test_div16_mem_dest_valid();
    test_div32_reg_valid();
    test_div_by_zero_error();

    // Multiplication variants
    test_mul8_imm_valid();
    test_mul16_imm_valid();
    test_mul32_reg_valid();
    test_mul16_mem_dest_valid();

    // Modulo
    test_mod16_imm_valid();

    // Inc/Dec stack
    test_inw_stack_valid();
    test_dew_stack_valid();
    test_inc8_stack_valid();
    test_dec8_stack_valid();

    // Stack load/store for X, Y, Z
    test_stx_sp_valid();
    test_sty_sp_valid();
    test_stz_sp_valid();
    test_ldx_sp_valid();
    test_ldy_sp_valid();
    test_ldz_sp_valid();

    // 16-bit neg/not/abs
    test_neg16_ax_valid();
    test_neg16_mem_valid();
    test_not16_ax_valid();
    test_not16_mem_valid();
    test_abs16_ax_valid();
    test_abs16_mem_valid();

    // CMP.S16
    test_cmp_s16_ax_valid();

    // Branch.16
    test_branch16_valid();

    // Chknonzero / Chkzero
    test_chknonzero16_valid();
    test_chkzero8_valid();

    // Bitfield stack/indirect modes
    test_bfins_sp_valid();
    test_bfins_ind_valid();
    test_bfins16_sp_valid();

    // FILL / MOVE variants
    test_fill_3arg_valid();
    test_fill_dma_valid();
    test_move_2arg_valid();
    test_move_dma_valid();

    // Signed multiply/divide
    test_mul_s16_mem_valid();
    test_div_s16_mem_valid();

    // Swap additional pairs
    test_swap_xz_valid();
    test_swap_ya_valid();

    // 16-bit add/sub/bitwise with memory dest
    test_add16_mem_valid();
    test_sub16_mem_valid();
    test_and16_ax_valid();
    test_ora16_ax_valid();
    test_eor16_ax_valid();
    test_and16_mem_rejected();

    // 32-bit add/sub with immediate
    test_add32_imm_valid();
    test_sub32_imm_valid();

    // Word load/store
    test_ldw_ax_imm();
    test_stw_ax_mem();
    test_ldw_xy_imm();

    // --- Round 2: Deeper code path coverage ---

    // FILL: register-pair destinations and widths
    test_fill_reg_ax_valid();
    test_fill_reg_axy_valid();
    test_fill_reg_axyz_valid();
    test_fill_reg_xy_valid();
    test_fill_reg_axz_valid();
    test_fill_reg_ayz_valid();
    test_fill_reg_len_valid();
    test_fill_large_addr_valid();
    test_fill_stack_rel_valid();

    // MOVE: register-pair and stack-relative
    test_move_reg_ax_valid();
    test_move_reg_axyz_valid();
    test_move_reg_src_valid();
    test_move_large_addr_valid();
    test_move_stack_rel_valid();

    // Signed math: mod, negative constants
    test_mod_s16_valid();
    test_mul_s16_neg_const();
    test_div_s16_neg_const();

    // Division: register/memory sources
    test_div8_reg_src_valid();
    test_div16_reg_src_valid();
    test_div16_mem_src_valid();

    // Multiplication: register/memory sources
    test_mul8_reg_src_valid();
    test_mul16_mem_src_valid();

    // Bitfield: wider fields, 16-bit indirect
    test_bfext_wide_field();
    test_bfins_wide_field();
    test_bfext16_wide();
    test_bfins16_ind_valid();

    // Stack inc/dec: frame-relative
    test_inw_frame_valid();
    test_dew_frame_valid();

    // Push/Pop: additional registers
    test_push_y_valid();
    test_push_z_valid();
    test_pop_y_valid();
    test_pop_z_valid();
    test_push_axyz_valid();

    // Neg/Not 16-bit zero page
    test_neg16_zp_valid();
    test_not16_zp_valid();

    // CMP.16 / CMP.S16 with immediate
    test_cmp16_imm_valid();
    test_cmp_s16_imm_valid();

    // LDW/STW: memory and stack
    test_ldax_mem_valid();
    test_stax_stack_valid();
    test_ldax_stack_valid();

    // 16-bit add/sub modes
    test_add16_ax_imm_valid();
    test_sub16_ax_imm_valid();
    test_add16_ax_mem_valid();
    test_sub16_ax_mem_valid();

    // 32-bit operations
    test_add32_mem_valid();
    test_sub32_mem_valid();
    test_and32_valid();
    test_ora32_valid();
    test_eor32_valid();
    test_cmp32_imm_valid();
    test_cmp32_mem_valid();
    test_cmp_s32_valid();
    test_neg32_valid();
    test_not32_valid();
    test_abs32_valid();
    test_lsl32_valid();
    test_lsr32_valid();
    test_asr32_valid();
    test_rol32_valid();
    test_ror32_valid();

    printf("\nExtended Simulated Ops Validation Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
