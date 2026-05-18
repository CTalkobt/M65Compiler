#!/bin/bash
# Test: Per-function clobber tracking (Phase 1) and selective invalidation (Phase 2)
# Validates that cc45's IR pipeline emits accurate .reg_clobbers / .func_flags
# directives, and that the assembler optimizer uses them correctly.

CC="./bin/cc45"
AS="./bin/ca45"
mkdir -p build/test

passed=0
failed=0

check() {
    local desc="$1"
    local cond="$2"
    if eval "$cond"; then
        echo "PASS: $desc"
        passed=$((passed + 1))
    else
        echo "FAIL: $desc"
        failed=$((failed + 1))
    fi
}

# --- Phase 1: Clobber tracking accuracy ---

echo "Phase 1: Per-function clobber tracking"
echo "======================================="

# Compile test program to assembly
$CC -S src/test-resources/test_clobber_tracking.c -o build/test/test_clobber_tracking.s 2>/dev/null
check "test_clobber_tracking.c compiles" "[ $? -eq 0 ]"

$AS build/test/test_clobber_tracking.s -o build/test/test_clobber_tracking.prg 2>/dev/null
check "test_clobber_tracking.s assembles" "[ $? -eq 0 ]"

ASM="build/test/test_clobber_tracking.s"

# Extract per-function clobber info: find proc name, then its func_flags/reg_clobbers
extract_func_info() {
    local func="$1"
    sed -n "/proc $func\b/,/endproc/p" "$ASM"
}

# set_value: leaf, clobbers A, X only
INFO=$(extract_func_info "_set_value")
check "set_value is leaf" "echo '$INFO' | grep -q 'func_flags stack_call, leaf'"
check "set_value clobbers A, X" "echo '$INFO' | grep -q 'reg_clobbers A, X$'"

# set_flag: leaf, clobbers A, X
INFO=$(extract_func_info "_set_flag")
check "set_flag is leaf" "echo '$INFO' | grep -q 'func_flags stack_call, leaf'"
check "set_flag clobbers A, X" "echo '$INFO' | grep -q 'reg_clobbers A, X$'"

# noop: leaf, no reg clobbers
INFO=$(extract_func_info "_noop")
check "noop is leaf" "echo '$INFO' | grep -q 'func_flags stack_call, leaf'"
check "noop has no reg_clobbers" "! echo '$INFO' | grep -q 'reg_clobbers'"

# caller: NOT leaf (calls other functions)
INFO=$(extract_func_info "_caller")
check "caller is not leaf" "echo '$INFO' | grep -q 'func_flags stack_call$'"
check "caller clobbers all regs" "echo '$INFO' | grep -q 'reg_clobbers A, X, Y, Z'"

# get_value: leaf, clobbers A, X
INFO=$(extract_func_info "_get_value")
check "get_value is leaf" "echo '$INFO' | grep -q 'func_flags stack_call, leaf'"
check "get_value clobbers A, X" "echo '$INFO' | grep -q 'reg_clobbers A, X$'"

# add: leaf, clobbers A, X (arithmetic)
INFO=$(extract_func_info "_add")
check "add is leaf" "echo '$INFO' | grep -q 'func_flags stack_call, leaf'"
check "add clobbers A, X" "echo '$INFO' | grep -q 'reg_clobbers A, X$'"

# main: NOT leaf
INFO=$(extract_func_info "_main")
check "main is not leaf" "echo '$INFO' | grep -q 'func_flags stack_call$'"

echo ""
echo "Phase 2: Selective register invalidation"
echo "========================================="

# Test: assembler uses .reg_clobbers at JSR sites
# We assemble a hand-written test where redundant loads should be eliminated

cat > build/test/test_phase2.s << 'EOF'
.org $2000
__sp_base = $0101
__zp_scratch = $08

; Leaf that only clobbers A
proc _only_a
    .var _fp = 0
    lda #42
    sta $4000
    .func_flags stack_call, leaf
    .reg_clobbers A
    .flag_clobbers N, Z
    endproc

; Test: LDX #$22 after JSR _only_a should be eliminated (X not clobbered)
proc _test_selective
    .var _fp = 0
@entry:
    ldx #$22
    jsr _only_a
    ; X still holds #$22 — this load should be eliminated:
    ldx #$22
    stx $4001
    .func_flags stack_call
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc

; Control: same pattern but callee clobbers A, X — load should NOT be eliminated
proc _clobbers_ax
    .var _fp = 0
    lda #42
    ldx #0
    sta $4000
    stx $4001
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc

proc _test_no_eliminate
    .var _fp = 0
@entry:
    ldx #$22
    jsr _clobbers_ax
    ; X was clobbered — this load should NOT be eliminated:
    ldx #$22
    stx $4002
    .func_flags stack_call
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc
EOF

$AS build/test/test_phase2.s -o build/test/test_phase2_opt.bin 2>/dev/null
check "Phase 2 test assembles" "[ $? -eq 0 ]"

# Get binary size — the optimized version should be smaller than
# a version where all registers are invalidated
OPT_SIZE=$(wc -c < build/test/test_phase2_opt.bin)

# Create unoptimized version: change _only_a to clobber A, X (prevents elimination)
sed 's/reg_clobbers A$/reg_clobbers A, X/' build/test/test_phase2.s > build/test/test_phase2_noopt.s
$AS build/test/test_phase2_noopt.s -o build/test/test_phase2_noopt.bin 2>/dev/null
NOOPT_SIZE=$(wc -c < build/test/test_phase2_noopt.bin)

check "optimized binary smaller than unoptimized ($OPT_SIZE < $NOOPT_SIZE)" "[ $OPT_SIZE -lt $NOOPT_SIZE ]"

# The difference should be exactly 2 bytes (one eliminated LDX #$22)
DIFF=$((NOOPT_SIZE - OPT_SIZE))
check "size difference is 2 bytes (one eliminated LDX #imm)" "[ $DIFF -eq 2 ]"

echo ""
echo "Clobber tracking tests: $passed passed, $failed failed"
[ $failed -eq 0 ] && exit 0 || exit 1
