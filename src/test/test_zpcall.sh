#!/bin/bash
# Comprehensive ZP calling convention regression tests - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

# Additional tool variables
NM="${NM:-./bin/nm45}"
MMEMU="${MMEMU:-mmemu-cli}"

print_section "Test 1: Hand-written ZP calling convention (asm)"

$AS src/test-resources/test_zpcall_asm.s -o build/test/test_zpcall_asm.bin 2>build/test/test_zpcall_asm.err
if [ $? -eq 0 ]; then
    OUTPUT=$(echo -e "load build/test/test_zpcall_asm.bin \$2000\nsetpc \$2000\nstep 500\nm \$4000 8\nq" | $MMEMU -m rawMega65 2>/dev/null)
    EXPECTED="0F 30 75 E0 93 04 00 AA"
    echo "$OUTPUT" | grep -qi "4000: $EXPECTED" && print_pass "asm: basic params, nested caller-save, long return AXYZ" || print_fail "asm: basic params, nested caller-save, long return AXYZ"
else
    print_fail "test_zpcall_asm.s assembly"
fi

# Verify function attributes in .o45
$AS -c src/test-resources/test_zpcall_asm.s -o build/test/test_zpcall_asm.o45 2>/dev/null
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_zpcall_asm.o45 2>/dev/null)
    echo "$NM_OUT" | grep -q "add_chars.*uses:03-04" && print_pass "asm: function attributes present in .o45" || print_fail "asm: function attributes in .o45"
    echo "$NM_OUT" | grep -q "add_long_fn.*releases:07-0A" && print_pass "asm: .zp_release attribute correct in .o45" || print_fail "asm: .zp_release in .o45"
else
    print_fail "asm: .o45 assembly"
fi

print_section "Test 2: Compiler zpcall regression (C → mmemu)"

$CC -fzpcall src/test-resources/test_zpcall_regression.c -o build/test/test_zpcall_regression.s 2>build/test/test_zpcall_regression.err
if [ $? -eq 0 ]; then
    $AS build/test/test_zpcall_regression.s -o build/test/test_zpcall_regression.prg 2>build/test/test_zpcall_regression_asm.err
    if [ $? -eq 0 ]; then
        OUTPUT=$(echo -e "load build/test/test_zpcall_regression.prg\nsetpc \$2000\nstep 500000\nm \$4000 18\nq" | $MMEMU -m rawMega65 2>/dev/null)
        ALL_BYTES=$(echo "$OUTPUT" | grep -i "40[01][0-9a-f]:" | sed 's/.*://I' | tr '\n' ' ' | tr -s ' ')
        read -ra BARR <<< "$ALL_BYTES"
        byte() { echo "${BARR[$1]}" | tr a-f A-F; }

        [ "$(byte 17)" = "AA" ] && print_pass "program execution completed (sentinel AA found)" || print_fail "program execution"
        [ "$(byte 0)" = "70" ] && [ "$(byte 1)" = "00" ] && print_pass "nested call save/restore: nested_test(3,4,100)=112" || print_fail "nested call save/restore"
        [ "$(byte 2)" = "07" ] && print_pass "deep nesting: level1(1,2,3)=7" || print_fail "deep nesting"
        [ "$(byte 3)" = "4D" ] && [ "$(byte 4)" = "00" ] && print_pass "address-of param write: addr_of_param(99)=77" || print_fail "address-of param write"
        [ "$(byte 5)" = "37" ] && [ "$(byte 6)" = "00" ] && print_pass "address-of param read: addr_of_read(55)=55" || print_fail "address-of param read"
        [ "$(byte 7)" = "E0" ] && [ "$(byte 8)" = "93" ] && [ "$(byte 9)" = "04" ] && [ "$(byte 10)" = "00" ] && print_pass "long return AXYZ: add_long(100000,200000)=300000" || print_fail "long return AXYZ"
        [ "$(byte 11)" = "78" ] && [ "$(byte 12)" = "00" ] && print_pass "recursion: factorial(5)=120" || print_fail "recursion"
        [ "$(byte 13)" = "01" ] && [ "$(byte 14)" = "00" ] && print_pass "mutual recursion: is_even(4)=1, is_even(3)=0" || print_fail "mutual recursion"
        [ "$(byte 15)" = "64" ] && print_pass "4 params: sum4(10,20,30,40)=100" || print_fail "4 params"
        [ "$(byte 16)" = "3C" ] && print_pass "char params: char_add(10,20,30)=60" || print_fail "char params"
    else
        print_fail "test_zpcall_regression.s assembly"
    fi
else
    print_fail "test_zpcall_regression.c compilation"
fi

print_section "Test 2b: Mixed convention (zpCall calling variadic)"

$CC -fzpcall src/test-resources/test_zpcall_mixed.c -o build/test/test_zpcall_mixed.s 2>build/test/test_zpcall_mixed.err
if [ $? -eq 0 ]; then
    $AS build/test/test_zpcall_mixed.s -o build/test/test_zpcall_mixed.prg 2>build/test/test_zpcall_mixed_asm.err
    if [ $? -eq 0 ]; then
        OUTPUT=$(echo -e "load build/test/test_zpcall_mixed.prg\nsetpc \$2000\nstep 500000\nm \$4000 9\nq" | $MMEMU -m rawMega65 2>/dev/null)
        ALL_BYTES=$(echo "$OUTPUT" | grep -i "400[0-9a-f]:" | sed 's/.*://I' | tr '\n' ' ' | tr -s ' ')
        read -ra BARR <<< "$ALL_BYTES"
        byte() { echo "${BARR[$1]}" | tr a-f A-F; }

        [ "$(byte 8)" = "AA" ] && print_pass "mixed convention: program completed (sentinel AA)" || print_fail "mixed convention: execution"
        [ "$(byte 0)" = "3C" ] && [ "$(byte 1)" = "00" ] && print_pass "mixed: direct variadic call sum(3,10,20,30)=60" || print_fail "mixed: direct variadic"
        [ "$(byte 2)" = "48" ] && [ "$(byte 3)" = "00" ] && print_pass "mixed: zpCall with live params across variadic call=72" || print_fail "mixed: live params across variadic"
        [ "$(byte 4)" = "96" ] && [ "$(byte 5)" = "00" ] && print_pass "mixed: interleaved zpCall+variadic calls=150" || print_fail "mixed: interleaved calls"
        [ "$(byte 6)" = "63" ] && print_pass "mixed: max_of variadic=99" || print_fail "mixed: max_of"
        [ "$(byte 7)" = "50" ] && print_pass "mixed: variadic result as zpCall arg=80" || print_fail "mixed: variadic as zpCall arg"
    else
        print_fail "test_zpcall_mixed.s assembly"
    fi
else
    print_fail "test_zpcall_mixed.c compilation"
fi

print_section "Test 3: Existing tests with -fzpcall"

DIFF_TESTS=(
    "test_char"
    "test_ops"
    "test_inc_dec"
    "test_func_args"
    "test_multi_call"
    "test_switch"
    "test_ternary"
    "test_break_continue"
    "test_short_circuit"
    "test_complex_math"
    "test_constant_folding"
    "test_dead_store"
    "test_enum"
    "test_for_decl"
    "test_modulo"
    "test_global_vars"
    "test_sub"
    "test_signed_cc"
    "test_bool"
)

for name in "${DIFF_TESTS[@]}"; do
    src="src/test-resources/${name}.c"
    if [ ! -f "$src" ]; then
        print_skip "$name (not found)"
        continue
    fi

    $CC -fzpcall "$src" -o "build/test/${name}_zp.s" 2>/dev/null
    if [ $? -ne 0 ]; then
        print_fail "$name -fzpcall compilation"
        continue
    fi

    $AS "build/test/${name}_zp.s" -o "build/test/${name}_zp.prg" 2>/dev/null
    if [ $? -ne 0 ]; then
        print_fail "$name -fzpcall assembly"
        continue
    fi

    OUTPUT=$(echo -e "load build/test/${name}_zp.prg\nsetpc \$2000\nrun 500000\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)
    A_VAL=$(echo "$OUTPUT" | grep -oP 'A:\s*\$\K[0-9A-Fa-f]+' | tail -1)

    if [ "$A_VAL" = "00" ]; then
        print_pass "$name (A=\$00)"
    else
        print_fail "$name with -fzpcall"
    fi
done

print_section "Test 4: Examples build"

if [ -d "examples" ] && [ -f "examples/Makefile" ]; then
    make -C examples clean >/dev/null 2>&1
    if make -C examples >/dev/null 2>&1; then
        print_pass "All examples build successfully"
    else
        print_fail "Examples build"
    fi
else
    print_skip "examples/ directory not configured for build"
fi

test_summary
exit $?
