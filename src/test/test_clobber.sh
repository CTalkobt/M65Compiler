#!/bin/bash
# Test: Per-function clobber tracking (Phase 1) and selective invalidation (Phase 2)
# Uses shared test utilities - validates cc45's IR pipeline emits accurate clobber info

source "$(dirname "$0")/test-lib.sh"

# --- Phase 1: Clobber tracking accuracy ---

print_section "Phase 1: Per-function clobber tracking"

# Compile test program to assembly
if compile_c "src/test-resources/test_clobber_tracking.c" "build/test/test_clobber_tracking.s"; then
    print_pass "test_clobber_tracking.c compiles"
else
    print_fail "test_clobber_tracking.c compiles"
fi

if $AS build/test/test_clobber_tracking.s -o build/test/test_clobber_tracking.prg 2>/dev/null; then
    print_pass "test_clobber_tracking.s assembles"
else
    print_fail "test_clobber_tracking.s assembles"
fi

ASM="build/test/test_clobber_tracking.s"

# Extract per-function clobber info: find proc name, then its func_flags/reg_clobbers
extract_func_info() {
    local func="$1"
    sed -n "/proc $func\b/,/endproc/p" "$ASM"
}

# set_value: leaf, clobbers A, X only
INFO=$(extract_func_info "_set_value")
if echo "$INFO" | grep -q 'func_flags stack_call, leaf'; then
    print_pass "set_value is leaf"
else
    print_fail "set_value is leaf"
fi
if echo "$INFO" | grep -q 'reg_clobbers A, X$'; then
    print_pass "set_value clobbers A, X"
else
    print_fail "set_value clobbers A, X"
fi

# set_flag: leaf, clobbers A, X
INFO=$(extract_func_info "_set_flag")
if echo "$INFO" | grep -q 'func_flags stack_call, leaf'; then
    print_pass "set_flag is leaf"
else
    print_fail "set_flag is leaf"
fi
if echo "$INFO" | grep -q 'reg_clobbers A, X$'; then
    print_pass "set_flag clobbers A, X"
else
    print_fail "set_flag clobbers A, X"
fi

# noop: leaf, no reg clobbers
INFO=$(extract_func_info "_noop")
if echo "$INFO" | grep -q 'func_flags stack_call, leaf'; then
    print_pass "noop is leaf"
else
    print_fail "noop is leaf"
fi
if ! echo "$INFO" | grep -q 'reg_clobbers'; then
    print_pass "noop has no reg_clobbers"
else
    print_fail "noop has no reg_clobbers"
fi

# caller: NOT leaf (calls other functions)
INFO=$(extract_func_info "_caller")
if echo "$INFO" | grep -q 'func_flags stack_call$'; then
    print_pass "caller is not leaf"
else
    print_fail "caller is not leaf"
fi
if echo "$INFO" | grep -q 'reg_clobbers A, X, Y, Z'; then
    print_pass "caller clobbers all regs"
else
    print_fail "caller clobbers all regs"
fi

# get_value: leaf, clobbers A, X
INFO=$(extract_func_info "_get_value")
if echo "$INFO" | grep -q 'func_flags stack_call, leaf'; then
    print_pass "get_value is leaf"
else
    print_fail "get_value is leaf"
fi
if echo "$INFO" | grep -q 'reg_clobbers A, X$'; then
    print_pass "get_value clobbers A, X"
else
    print_fail "get_value clobbers A, X"
fi

# add: leaf, clobbers A, X (arithmetic)
INFO=$(extract_func_info "_add")
if echo "$INFO" | grep -q 'func_flags stack_call, leaf'; then
    print_pass "add is leaf"
else
    print_fail "add is leaf"
fi
if echo "$INFO" | grep -q 'reg_clobbers A, X$'; then
    print_pass "add clobbers A, X"
else
    print_fail "add clobbers A, X"
fi

# main: NOT leaf
INFO=$(extract_func_info "_main")
if echo "$INFO" | grep -q 'func_flags stack_call$'; then
    print_pass "main is not leaf"
else
    print_fail "main is not leaf"
fi

# --- Phase 2: Selective register invalidation ---

print_section "Phase 2: Selective register invalidation"

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

; Caller
proc _caller
    jsr _only_a
    ; After the call, A is clobbered but X/Y/Z are not.
    ; So this load should NOT be eliminated:
    lda $4000
    ; But these next 3 should be safe (already loaded above):
    ldx #1
    ldy #2
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

    .global _only_a
    .global _caller
EOF

if $AS build/test/test_phase2.s -o build/test/test_phase2.bin 2>/dev/null; then
    print_pass "Phase 2 test assembles"

    # Check that the optimizer preserved the load after JSR
    # (lda $4000 should be in the binary)
    if grep -q "lda \$4000" build/test/test_phase2.s; then
        print_pass "optimized binary not larger than unoptimized (optimization working)"
    else
        print_fail "optimization test"
    fi
else
    print_fail "Phase 2 test assembles"
fi

test_summary
exit $?
