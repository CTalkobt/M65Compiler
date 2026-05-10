#!/bin/bash
# test_cc_interop.sh — Comprehensive tests for calling convention interoperability
#
# Tests safe and unsafe calling convention combinations:
#   1. Stack → ZP calls (ALLOWED — fastcall interop)
#   2. ZP → Stack calls (FORBIDDEN — linker error)
#   3. Indirect calls through function pointers
#   4. Struct returns across conventions
#   5. Long (32-bit) returns across conventions
#   6. Parameter passing edge cases
#   7. Variadic calls from ZP functions

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
MMEMU="mmemu-cli"
NM="./bin/nm45"
mkdir -p build/test

failed=0
passed=0

pass() { echo "  ✓ PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  ✗ FAIL: $1"; if [ -n "$2" ]; then echo "         $2"; fi; failed=$((failed + 1)); }

run_mmemu() {
    local bin=$1
    local addr=$2
    local len=$3
    echo -e "load $bin $addr\nsetpc $addr\nstep 1000\nm $addr $len\nq" | $MMEMU -m rawMega65 2>/dev/null
}

echo "=========================================================================="
echo "  Calling Convention Interoperability Tests"
echo "=========================================================================="

# Test 1: Stack → ZP calls (ALLOWED)
# ===================================================================
echo ""
echo "=== Test 1: Stack → ZP calls (ALLOWED — fastcall interop) ==="

$CC -S src/test-resources/test_cc_stack_to_zp.c -o build/test/test_cc_stack_to_zp.s 2>build/test/test_cc_stack_to_zp.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_stack_to_zp.s -o build/test/test_cc_stack_to_zp.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_stack_to_zp.bin 0x2000 6)
        # Expected results: [0]=30, [1]=0, [2]=155, [3]=0, [4]=35, [5]=0xFF
        if echo "$OUTPUT" | grep -qi "4000:.*1e.*9b.*23.*ff"; then
            pass "Stack→ZP: zp_add(10,20)=30, zp_add(100,55)=155, zp_char_mul(5,7)=35"
        else
            ACTUAL=$(echo "$OUTPUT" | grep -i '4000:' | head -1)
            fail "Stack→ZP execution results" "Expected: 1E 00 9B 00 23 00, Got: $(echo "$ACTUAL" | sed 's/.*4000: //')"
        fi
    else
        fail "Stack→ZP assembly" "Assembler failed"
    fi
else
    fail "Stack→ZP compilation" "Compiler failed: $(cat build/test/test_cc_stack_to_zp.err | head -3)"
fi

# Test 2: ZP → Stack calls (FORBIDDEN)
# ===================================================================
echo ""
echo "=== Test 2: ZP → Stack calls (FORBIDDEN — linker error) ==="

$CC -c src/test-resources/test_cc_zp_to_stack.c -o build/test/test_cc_zp_to_stack.o45 2>build/test/test_cc_zp_to_stack_cc.err
if [ $? -eq 0 ]; then
    $LD -t 0x2000 build/test/test_cc_zp_to_stack.o45 -o build/test/test_cc_zp_to_stack.bin 2>build/test/test_cc_zp_to_stack_ld.err
    if [ $? -ne 0 ]; then
        LINKER_ERR=$(cat build/test/test_cc_zp_to_stack_ld.err)
        if echo "$LINKER_ERR" | grep -q "calling convention mismatch"; then
            pass "ZP→Stack linker error detected" "Error message: $(echo "$LINKER_ERR" | grep -o 'calling convention mismatch.*' | head -1)"
        else
            fail "ZP→Stack linker error" "Linker reported error but wrong message: $(echo "$LINKER_ERR" | head -1)"
        fi
    else
        fail "ZP→Stack linker enforcement" "Linker should have rejected ZP→Stack call, but succeeded"
    fi
else
    fail "ZP→Stack compilation" "Compiler failed (unexpected)"
fi

# Test 3: Struct returns across conventions
# ===================================================================
echo ""
echo "=== Test 3: Struct returns across conventions ==="

$CC -S src/test-resources/test_cc_struct_return.c -o build/test/test_cc_struct_return.s 2>build/test/test_cc_struct_return.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_struct_return.s -o build/test/test_cc_struct_return.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_struct_return.bin 0x2000 7)
        # Expected: p.x=10, p.y=20, r.left={0,1}, r.top={0,2}
        if echo "$OUTPUT" | grep -qi "4000:.*0a.*14.*00.*01.*00.*02.*ff"; then
            pass "Struct returns: stack Point(10,20) and ZP Rect(256,512)"
        else
            ACTUAL=$(echo "$OUTPUT" | grep -i '4000:' | head -1)
            fail "Struct returns execution" "Expected: 0A 14 00 01 00 02 FF, Got: $(echo "$ACTUAL" | sed 's/.*4000: //')"
        fi
    else
        fail "Struct returns assembly" "Assembler failed"
    fi
else
    fail "Struct returns compilation" "Compiler failed: $(cat build/test/test_cc_struct_return.err | head -3)"
fi

# Test 4: Long (32-bit) returns across conventions
# ===================================================================
echo ""
echo "=== Test 4: Long (32-bit) returns across conventions ==="

$CC -S src/test-resources/test_cc_long_return.c -o build/test/test_cc_long_return.s 2>build/test/test_cc_long_return.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_long_return.s -o build/test/test_cc_long_return.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_long_return.bin 0x2000 9)
        # Expected: r1 = 0x12345678, r2 = 0xABCDEF01
        if echo "$OUTPUT" | grep -qi "4000:.*78.*56.*34.*12.*01.*ef.*cd.*ab.*ff"; then
            pass "Long returns: stack (0x12345678) and ZP (0xABCDEF01)"
        else
            ACTUAL=$(echo "$OUTPUT" | grep -i '4000:' | head -1)
            fail "Long returns execution" "Expected AXYZ registers to preserve 32-bit values"
        fi
    else
        fail "Long returns assembly" "Assembler failed"
    fi
else
    fail "Long returns compilation" "Compiler failed: $(cat build/test/test_cc_long_return.err | head -3)"
fi

# Test 5: Parameter passing edge cases
# ===================================================================
echo ""
echo "=== Test 5: Parameter passing edge cases ==="

$CC -S src/test-resources/test_cc_param_edge_cases.c -o build/test/test_cc_param_edge_cases.s 2>build/test/test_cc_param_edge_cases.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_param_edge_cases.s -o build/test/test_cc_param_edge_cases.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_param_edge_cases.bin 0x2000 5)
        # Expected: r1=10 (1+2+3+4), r2=150 (10+20+30+40+50), r3=105 (5+100), r4=110 (11+22+33+44)
        if echo "$OUTPUT" | grep -qi "4000:.*0a.*96.*69.*6e.*ff"; then
            pass "Parameter edge cases: 4-param ZP, 5-param stack, mixed types"
        else
            ACTUAL=$(echo "$OUTPUT" | grep -i '4000:' | head -1)
            fail "Parameter edge cases execution" "Expected: 0A 96 69 6E FF, Got: $(echo "$ACTUAL" | sed 's/.*4000: //')"
        fi
    else
        fail "Parameter edge cases assembly" "Assembler failed"
    fi
else
    fail "Parameter edge cases compilation" "Compiler failed: $(cat build/test/test_cc_param_edge_cases.err | head -3)"
fi

# Test 6: Variadic calls from ZP functions
# ===================================================================
echo ""
echo "=== Test 6: Variadic calls from ZP functions ==="

$CC -S src/test-resources/test_cc_zp_variadic.c -o build/test/test_cc_zp_variadic.s 2>build/test/test_cc_zp_variadic.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_zp_variadic.s -o build/test/test_cc_zp_variadic.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_zp_variadic.bin 0x2000 7)
        # Expected: r1=30 (5+10+15), r2=20 (7+3+10), r3=312 (100+56+156)
        if echo "$OUTPUT" | grep -qi "4000:.*1e.*14.*38.*01.*ff"; then
            pass "Variadic from ZP: auto-fallback to stack convention"
        else
            ACTUAL=$(echo "$OUTPUT" | grep -i '4000:' | head -1)
            fail "Variadic from ZP execution" "Expected: 1E 14 38 01 FF, Got: $(echo "$ACTUAL" | sed 's/.*4000: //')"
        fi
    else
        fail "Variadic from ZP assembly" "Assembler failed"
    fi
else
    fail "Variadic from ZP compilation" "Compiler failed: $(cat build/test/test_cc_zp_variadic.err | head -3)"
fi

# Test 7: Function attribute verification in .o45 objects
# ===================================================================
echo ""
echo "=== Test 7: Function attribute verification in .o45 objects ==="

$CC -c src/test-resources/test_cc_stack_to_zp.c -o build/test/test_cc_stack_to_zp.o45 2>/dev/null
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_cc_stack_to_zp.o45 2>/dev/null)

    if echo "$NM_OUT" | grep -q "zp_add.*zp_call"; then
        pass "ZP function attribute: zp_add has zp_call flag"
    else
        fail "ZP function attribute" "zp_add missing zp_call flag in .o45 object"
    fi

    if echo "$NM_OUT" | grep -q "main.*stack_call"; then
        pass "Stack function attribute: main has stack_call flag"
    else
        fail "Stack function attribute" "main missing stack_call flag in .o45 object"
    fi
else
    fail "Function attribute verification" "Could not create .o45 object"
fi

# Test 8: Assembly-level .func_flags directive and attributes
# ===================================================================
echo ""
echo "=== Test 8: Assembly-level .func_flags directive ==="

$AS -c src/test-resources/test_cc_asm.s -o build/test/test_cc_asm.o45 2>build/test/test_cc_asm.err
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_cc_asm.o45 2>/dev/null)

    if echo "$NM_OUT" | grep -q "zp_add.*zp_call"; then
        pass "Assembly: zp_add has zp_call from .func_flags directive"
    else
        fail "Assembly: zp_add .func_flags" "zp_add missing zp_call flag"
    fi

    if echo "$NM_OUT" | grep -q "zp_mul.*zp_call.*leaf"; then
        pass "Assembly: zp_mul has zp_call and leaf from .func_flags"
    else
        fail "Assembly: zp_mul .func_flags" "zp_mul missing zp_call or leaf flag"
    fi

    if echo "$NM_OUT" | grep -q "stack_add.*stack_call"; then
        pass "Assembly: stack_add has stack_call from .func_flags"
    else
        fail "Assembly: stack_add .func_flags" "stack_add missing stack_call flag"
    fi
else
    fail "Assembly: .func_flags directive parsing" "Assembler failed: $(cat build/test/test_cc_asm.err | head -1)"
fi

# Summary
# ===================================================================
echo ""
echo "=========================================================================="
echo "  Summary: $passed passed, $failed failed"
echo "=========================================================================="

if [ $failed -eq 0 ]; then
    echo "All tests PASSED ✓"
    exit 0
else
    echo "Some tests FAILED ✗"
    exit 1
fi
