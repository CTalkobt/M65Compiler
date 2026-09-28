#!/bin/bash

# Test script for xemu-xmega65 validation with ca45 and cc45
# Alternative to mmemu-cli with better MEGA65 hardware emulation

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
CRT0="lib/build/crt0.o45"
STDLIB="lib/build/c45.lib"
STDLIB_ZP="lib/build/c45_zp.lib"
XEMU="xemu-xmega65"
mkdir -p build/test

# ── xemu-xmega65 availability check ──────────────────────────────────────
if ! command -v "$XEMU" &>/dev/null; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║  WARNING: xemu-xmega65 is not installed or not on PATH      ║"
    echo "║                                                             ║"
    echo "║  Xemu-based execution tests CANNOT run without it.          ║"
    echo "║  These tests verify runtime correctness of generated code.  ║"
    echo "║                                                             ║"
    echo "║  Install from: https://github.com/lgblgblgb/xemu            ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    exit 1
fi

failed=0
passed=0

# Helper: Run PRG with xemu and check memory via binary dump
# Usage: run_xemu_test "name" "prg_file" "memory_addr_hex" "num_bytes" "expected_hex"
#   memory_addr_hex: e.g. "4000" (without 0x prefix)
#   expected_hex: space-separated hex bytes e.g. "01 02 03 04 AA"
run_xemu_test() {
    local name="$1"
    local prg_file="$2"
    local mem_addr="$3"
    local num_bytes="$4"
    local expected="$5"

    local dump_file="build/test/${name}_memdump.bin"

    # Run in headless mode with -prgexit (auto-SYS and exit on READY)
    # -prgmode 65 forces MEGA65 mode without dialog
    # -prgexit auto-runs BASIC programs and exits on next READY prompt
    timeout 30 $XEMU -headless -besure -prgmode 65 -prgexit -dumpmem "$dump_file" -prg "$prg_file" </dev/null >/dev/null 2>&1
    local exit_code=$?
    # Accept exit 0 (clean) or 124 (timeout — program may halt instead of returning)
    if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
        echo "FAIL: $name (xemu exit code $exit_code)"
        failed=$((failed + 1))
        return 1
    fi

    if [ ! -f "$dump_file" ]; then
        echo "FAIL: $name (no memory dump generated)"
        failed=$((failed + 1))
        return 1
    fi

    # Extract bytes from raw binary dump using xxd
    local addr_dec=$((16#$mem_addr))
    actual=$(xxd -s $addr_dec -l $num_bytes -p "$dump_file" 2>/dev/null | \
             sed 's/\(..\)/\1 /g' | sed 's/ $//' | tr '[:lower:]' '[:upper:]')

    if [ -z "$actual" ]; then
        echo "FAIL: $name (could not read memory at \$$mem_addr)"
        failed=$((failed + 1))
        return 1
    fi

    expected_upper=$(echo "$expected" | tr '[:lower:]' '[:upper:]')

    if [ "$actual" = "$expected_upper" ]; then
        echo "SUCCESS: $name"
        passed=$((passed + 1))
        return 0
    else
        echo "FAIL: $name"
        echo "  Expected at \$$mem_addr: $expected_upper"
        echo "  Actual:                $actual"
        failed=$((failed + 1))
        return 1
    fi
}

# Helper: compile and link test
compile_link_test() {
    local src="$1"
    local prg_out="$2"
    local flags="${3:-}"
    local o_file="${prg_out%.prg}.o45"

    $CC -c $flags "$src" -o "$o_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 1; fi

    if [[ "$flags" == *"-fzpcall"* ]]; then
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $STDLIB_ZP 2>/dev/null
    else
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $STDLIB 2>/dev/null
    fi
    if [ $? -ne 0 ]; then return 2; fi

    return 0
}

echo "Testing with xemu-xmega65 (MEGA65 Hardware Emulator)"
echo "======================================================"
echo ""

# Test 1: test_short.c (SAC parameter passing)
echo "Testing test_short.c (short type with SAC)..."
compile_link_test "src/test-resources/test_short.c" "build/test/test_short_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_short" "build/test/test_short_xemu.prg" "4000" 7 "1E 05 02 0C 0A C8 AA"
else
    echo "FAIL: test_short.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 2: test_struct_return.c
echo "Testing test_struct_return.c (struct return)..."
compile_link_test "src/test-resources/test_struct_return.c" "build/test/test_struct_return_xemu.prg"
if [ $? -eq 0 ]; then
    # Expected: x=5, y=10 at $4000
    run_xemu_test "test_struct_return" "build/test/test_struct_return_xemu.prg" "4000" 2 "05 0A"
else
    echo "FAIL: test_struct_return.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 3: test_array_init.c
echo "Testing test_array_init.c (array initialization)..."
compile_link_test "src/test-resources/test_array_init.c" "build/test/test_array_init_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_array_init" "build/test/test_array_init_xemu.prg" "4000" 5 "01 02 03 04 05"
else
    echo "FAIL: test_array_init.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 4: test_compound_literal.c
echo "Testing test_compound_literal.c (compound literals)..."
compile_link_test "src/test-resources/test_compound_literal.c" "build/test/test_compound_literal_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_compound_literal" "build/test/test_compound_literal_xemu.prg" "4000" 4 "2A 2B 2C 2D"
else
    echo "FAIL: test_compound_literal.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 5: test_long_mmemu.c
echo "Testing test_long_mmemu.c (long type)..."
compile_link_test "src/test-resources/test_long_mmemu.c" "build/test/test_long_mmemu_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_long_mmemu" "build/test/test_long_mmemu_xemu.prg" "4000" 4 "78 56 34 12"
else
    echo "FAIL: test_long_mmemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 6: test_alloca_xemu.c (stack-based dynamic allocation)
echo "Testing test_alloca_xemu.c (alloca)..."
compile_link_test "src/test-resources/test_alloca_xemu.c" "build/test/test_alloca_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_alloca" "build/test/test_alloca_xemu.prg" "C000" 4 "01 02 03 AA"
else
    echo "FAIL: test_alloca_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

echo ""
echo "======================================================"
echo "Xemu Test Results: $passed passed, $failed failed"
echo "======================================================"

if [ $failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
