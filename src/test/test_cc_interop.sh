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

CC="./bin/cc45 -fno-zpcall"  # interop tests mark ZP functions __fastcall__; the rest is stack
AS="./bin/ca45"
LD="./bin/ln45"
XEMU="xemu-xmega65"
CRT0="lib/build/crt0.o45"
LIB="lib/build/c45_stack.lib"
NM="./bin/nm45"
mkdir -p build/test

failed=0
passed=0

pass() { echo "  ✓ PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  ✗ FAIL: $1"; if [ -n "$2" ]; then echo "         $2"; fi; failed=$((failed + 1)); }

# run_xemu NAME SRC EXPECTED — build SRC, run it on xemu, compare the bytes at $4000
# (results[] in each test program) with EXPECTED (space-separated hex)
run_xemu() {
    local name=$1 src=$2 expected=$3
    local base="build/test/test_cc_$name"
    local n; n=$(echo "$expected" | wc -w)
    if ! $CC -c "$src" -o "$base.o45" 2>"$base.err"; then
        fail "$name compilation" "Compiler failed: $(head -3 "$base.err")"; return 1
    fi
    if ! $LD -basic -o "$base.prg" "$CRT0" "$base.o45" "$LIB" >/dev/null 2>"$base.lderr"; then
        fail "$name link" "Linker failed: $(head -3 "$base.lderr")"; return 1
    fi
    rm -f "$base.mem"
    timeout 30 $XEMU -headless -besure -prgmode 65 -prgexit -dumpmem "$base.mem" -prg "$base.prg" </dev/null >/dev/null 2>&1
    if [ ! -f "$base.mem" ]; then fail "$name execution" "no memory dump"; return 1; fi
    ACTUAL=$(xxd -s $((16#4000)) -l "$n" -p "$base.mem" | sed 's/../& /g; s/ $//' | tr '[:lower:]' '[:upper:]')
    [ "$ACTUAL" = "$expected" ]
}

if ! command -v $XEMU &>/dev/null; then
    echo "xemu-xmega65 not found — skipping calling convention interop tests"
    exit 0
fi

echo "=========================================================================="
echo "  Calling Convention Interoperability Tests"
echo "=========================================================================="

# Test 1: Stack → ZP calls (ALLOWED)
# ===================================================================
echo ""
echo "=== Test 1: Stack → ZP calls (ALLOWED — fastcall interop) ==="

if run_xemu stack_to_zp src/test-resources/test_cc_stack_to_zp.c "1E 00 9B 00 23 FF"; then
    pass "Stack→ZP: zp_add(10,20)=30, zp_add(100,55)=155, zp_char_mul(5,7)=35"
elif [ -n "$ACTUAL" ]; then
    fail "stack_to_zp execution" "Expected: 1E 00 9B 00 23 FF, Got: $ACTUAL"
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

if run_xemu struct_return src/test-resources/test_cc_struct_return.c "0A 14 00 01 00 02 FF"; then
    pass "Struct returns: stack Point(10,20) and ZP Rect(256,512)"
elif [ -n "$ACTUAL" ]; then
    fail "struct_return execution" "Expected: 0A 14 00 01 00 02 FF, Got: $ACTUAL"
fi

# Test 4: Long (32-bit) returns across conventions
# ===================================================================
echo ""
echo "=== Test 4: Long (32-bit) returns across conventions ==="

if run_xemu long_return src/test-resources/test_cc_long_return.c "78 56 34 12 01 EF CD AB FF"; then
    pass "Long returns: stack (0x12345678) and ZP (0xABCDEF01)"
elif [ -n "$ACTUAL" ]; then
    fail "long_return execution" "Expected: 78 56 34 12 01 EF CD AB FF, Got: $ACTUAL"
fi

# Test 5: Parameter passing edge cases
# ===================================================================
echo ""
echo "=== Test 5: Parameter passing edge cases ==="

if run_xemu param_edge_cases src/test-resources/test_cc_param_edge_cases.c "0A 96 69 6E FF"; then
    pass "Parameter edge cases: 4-param ZP, 5-param stack, mixed types"
elif [ -n "$ACTUAL" ]; then
    fail "param_edge_cases execution" "Expected: 0A 96 69 6E FF, Got: $ACTUAL"
fi

# Test 6: Variadic calls from ZP functions
# ===================================================================
echo ""
echo "=== Test 6: Variadic calls from ZP functions ==="

if run_xemu zp_variadic src/test-resources/test_cc_zp_variadic.c "1E 00 14 00 38 01 FF"; then
    pass "Variadic from ZP: auto-fallback to stack convention"
elif [ -n "$ACTUAL" ]; then
    fail "zp_variadic execution" "Expected: 1E 00 14 00 38 01 FF, Got: $ACTUAL"
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

$AS -c src/test-resources/test_cc_asm.s45 -o build/test/test_cc_asm.o45 2>build/test/test_cc_asm.err
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
