#!/bin/bash
# test_zpcall.sh — Comprehensive regression tests for the ZP calling convention
#
# Tests:
#   1. Hand-written asm: param passing, nested calls, long return, attributes
#   2. Compiler: nested calls, address-of, long return, recursion, mixed types
#   3. Compiler output diff: -fzpcall vs -fno-zpcall produce correct results
#   4. Examples build

CC="./bin/cc45"
AS="./bin/ca45"
MMEMU="mmemu-cli"
NM="./bin/nm45"
mkdir -p build/test

failed=0
passed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; echo "        $2"; failed=$((failed + 1)); }

# ===================================================================
# Test 1: Hand-written asm — ZP calling convention with attributes
# ===================================================================
echo "=== Test 1: Hand-written ZP calling convention (asm) ==="

$AS src/test-resources/test_zpcall_asm.s -o build/test/test_zpcall_asm.bin 2>build/test/test_zpcall_asm.err
if [ $? -ne 0 ]; then
    fail "test_zpcall_asm.s assembly" "$(cat build/test/test_zpcall_asm.err)"
else
    OUTPUT=$(echo -e "load build/test/test_zpcall_asm.bin \$2000\nsetpc \$2000\nstep 500\nm \$4000 8\nq" | $MMEMU -m rawMega65 2>/dev/null)

    EXPECTED="0F 30 75 E0 93 04 00 AA"
    if echo "$OUTPUT" | grep -qi "4000: $EXPECTED"; then
        pass "asm: basic params, nested caller-save, long return AXYZ"
    else
        ACTUAL=$(echo "$OUTPUT" | grep -i '4000:')
        fail "test_zpcall_asm.s execution" "Expected: $EXPECTED, Got: $ACTUAL"
    fi
fi

# Verify function attributes in .o45
$AS -c src/test-resources/test_zpcall_asm.s -o build/test/test_zpcall_asm.o45 2>/dev/null
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_zpcall_asm.o45 2>/dev/null)
    if echo "$NM_OUT" | grep -q "add_chars.*uses:03-04"; then
        pass "asm: function attributes present in .o45"
    else
        fail "asm: function attributes in .o45" "nm45 -f output: $(echo "$NM_OUT" | head -3)"
    fi
    # Verify specific attribute content
    if echo "$NM_OUT" | grep -q "add_long_fn.*releases:07-0A"; then
        pass "asm: .zp_release attribute correct in .o45"
    else
        fail "asm: .zp_release in .o45" "Expected releases:07-0A for add_long_fn"
    fi
else
    fail "asm: .o45 assembly" "ca45 -c failed"
fi

# ===================================================================
# Test 2: Compiler — comprehensive zpcall regression (C → mmemu)
# ===================================================================
echo ""
echo "=== Test 2: Compiler zpcall regression (C → mmemu) ==="

$CC -fzpcall src/test-resources/test_zpcall_regression.c -o build/test/test_zpcall_regression.s 2>build/test/test_zpcall_regression.err
if [ $? -ne 0 ]; then
    fail "test_zpcall_regression.c compilation" "$(cat build/test/test_zpcall_regression.err)"
else
    $AS build/test/test_zpcall_regression.s -o build/test/test_zpcall_regression.prg 2>build/test/test_zpcall_regression_asm.err
    if [ $? -ne 0 ]; then
        fail "test_zpcall_regression.s assembly" "$(cat build/test/test_zpcall_regression_asm.err)"
    else
        OUTPUT=$(echo -e "load build/test/test_zpcall_regression.prg\nsetpc \$2000\nstep 500000\nm \$4000 18\nq" | $MMEMU -m rawMega65 2>/dev/null)

        # Collect all hex bytes from memory dump lines (handles multi-line output)
        ALL_BYTES=$(echo "$OUTPUT" | grep -i "40[01][0-9a-f]:" | sed 's/.*://I' | tr '\n' ' ' | tr -s ' ')
        # Parse into array
        read -ra BARR <<< "$ALL_BYTES"

        byte() { echo "${BARR[$1]}" | tr a-f A-F; }

        # Check sentinel to verify execution completed
        if [ "$(byte 17)" = "AA" ]; then
            pass "program execution completed (sentinel AA found)"
        else
            fail "program execution" "Sentinel not found at offset 17. Output: $BYTES"
        fi

        # Test 2a: nested call save/restore
        # nested_test(3, 4, 100) = 3*4 + 100 = 112 = $0070
        if [ "$(byte 0)" = "70" ] && [ "$(byte 1)" = "00" ]; then
            pass "nested call save/restore: nested_test(3,4,100)=112"
        else
            fail "nested call save/restore" "Expected 70 00, got $(byte 0) $(byte 1)"
        fi

        # Test 2b: deep nesting (3 levels)
        # level1(1, 2, 3) = ((1+1)+2)+3 = 7
        if [ "$(byte 2)" = "07" ]; then
            pass "deep nesting: level1(1,2,3)=7"
        else
            fail "deep nesting" "Expected 07, got $(byte 2)"
        fi

        # Test 2c: address-of param (write through pointer)
        # addr_of_param(99) = 77 = $004D
        if [ "$(byte 3)" = "4D" ] && [ "$(byte 4)" = "00" ]; then
            pass "address-of param write: addr_of_param(99)=77"
        else
            fail "address-of param write" "Expected 4D 00, got $(byte 3) $(byte 4)"
        fi

        # Test 2d: address-of param (read through pointer)
        # addr_of_read(55) = 55 = $0037
        if [ "$(byte 5)" = "37" ] && [ "$(byte 6)" = "00" ]; then
            pass "address-of param read: addr_of_read(55)=55"
        else
            fail "address-of param read" "Expected 37 00, got $(byte 5) $(byte 6)"
        fi

        # Test 2e: long return in AXYZ
        # add_long(100000, 200000) = 300000 = $000493E0
        if [ "$(byte 7)" = "E0" ] && [ "$(byte 8)" = "93" ] && \
           [ "$(byte 9)" = "04" ] && [ "$(byte 10)" = "00" ]; then
            pass "long return AXYZ: add_long(100000,200000)=300000"
        else
            fail "long return AXYZ" "Expected E0 93 04 00, got $(byte 7) $(byte 8) $(byte 9) $(byte 10)"
        fi

        # Test 2f: recursion
        # factorial(5) = 120 = $0078
        if [ "$(byte 11)" = "78" ] && [ "$(byte 12)" = "00" ]; then
            pass "recursion: factorial(5)=120"
        else
            fail "recursion: factorial(5)" "Expected 78 00, got $(byte 11) $(byte 12)"
        fi

        # Test 2g: mutual recursion
        # is_even(4)=1, is_even(3)=0
        if [ "$(byte 13)" = "01" ] && [ "$(byte 14)" = "00" ]; then
            pass "mutual recursion: is_even(4)=1, is_even(3)=0"
        else
            fail "mutual recursion" "Expected 01 00, got $(byte 13) $(byte 14)"
        fi

        # Test 2h: 4 params
        # sum4(10, 20, 30, 40) = 100 = $64
        if [ "$(byte 15)" = "64" ]; then
            pass "4 params: sum4(10,20,30,40)=100"
        else
            fail "4 params" "Expected 64, got $(byte 15)"
        fi

        # Test 2i: char params
        # char_add(10, 20, 30) = 60 = $3C
        if [ "$(byte 16)" = "3C" ]; then
            pass "char params: char_add(10,20,30)=60"
        else
            fail "char params" "Expected 3C, got $(byte 16)"
        fi
    fi
fi

# ===================================================================
# Test 3: Compiler output diff — -fzpcall correctness for existing tests
# ===================================================================
echo ""
echo "=== Test 3: Existing tests with -fzpcall (A=\$00 = pass) ==="

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
        echo "  Skip: $name (not found)"
        continue
    fi

    # Compile with -fzpcall
    $CC -fzpcall "$src" -o "build/test/${name}_zp.s" 2>/dev/null
    if [ $? -ne 0 ]; then
        fail "$name -fzpcall compilation" "cc45 -fzpcall failed"
        continue
    fi

    $AS "build/test/${name}_zp.s" -o "build/test/${name}_zp.prg" 2>/dev/null
    if [ $? -ne 0 ]; then
        fail "$name -fzpcall assembly" "ca45 failed"
        continue
    fi

    # Run on mmemu
    OUTPUT=$(echo -e "load build/test/${name}_zp.prg\nsetpc \$2000\nrun 500000\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)
    A_VAL=$(echo "$OUTPUT" | grep -oP 'A:\s*\$\K[0-9A-Fa-f]+' | tail -1)

    if [ "$A_VAL" = "00" ]; then
        pass "$name (A=\$00)"
    else
        fail "$name with -fzpcall" "A=\$$A_VAL (expected \$00)"
    fi
done

# ===================================================================
# Test 4: Examples build
# ===================================================================
echo ""
echo "=== Test 4: Examples build ==="

if [ -d "examples" ] && [ -f "examples/Makefile" ]; then
    make -C examples clean >/dev/null 2>&1
    EXAMPLE_OUT=$(make -C examples 2>&1)
    if [ $? -eq 0 ]; then
        pass "All examples build successfully"
    else
        fail "Examples build" "$(echo "$EXAMPLE_OUT" | grep -i 'error' | head -3)"
    fi
else
    echo "  Skip: examples/ directory not configured for build"
fi

# ===================================================================
# Summary
# ===================================================================
echo ""
echo "=============================="
echo "Results: $passed passed, $failed failed"
echo "=============================="

if [ $failed -eq 0 ]; then
    echo "All zpcall regression tests passed!"
    exit 0
else
    exit 1
fi
