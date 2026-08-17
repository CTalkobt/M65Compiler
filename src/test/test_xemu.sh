#!/bin/bash

# Test script for xemu-xmega65 validation with ca45 and cc45
# Alternative to mmemu-cli with better MEGA65 hardware emulation

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
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

# Helper: Run PRG with xemu and dump memory
# Usage: run_xemu_test "name" "prg_file" "memory_addr" "num_bytes" "expected_hex"
run_xemu_test() {
    local name="$1"
    local prg_file="$2"
    local mem_addr="$3"
    local num_bytes="$4"
    local expected="$5"
    
    local dump_file="build/test/${name}_memdump.txt"
    
    # Run in headless mode, exit on program completion, dump memory
    timeout 30 $XEMU -headless -prgexit -dumpmem "$dump_file" "$prg_file" >/dev/null 2>&1
    if [ $? -ne 0 ] && [ $? -ne 124 ]; then
        echo "FAIL: $name (xemu execution error)"
        failed=$((failed + 1))
        return 1
    fi
    
    # Parse memory dump and extract target address
    if [ ! -f "$dump_file" ]; then
        echo "FAIL: $name (no memory dump generated)"
        failed=$((failed + 1))
        return 1
    fi
    
    # Extract bytes from memory dump (format: address: XX XX XX ...)
    actual=$(grep -A 1 "^$mem_addr:" "$dump_file" 2>/dev/null | tail -1 | awk '{print substr($0,1,3*'$num_bytes')}')
    
    if [ -z "$actual" ]; then
        echo "FAIL: $name (memory not found at $mem_addr)"
        failed=$((failed + 1))
        return 1
    fi
    
    # Case-insensitive comparison
    actual_upper=$(echo "$actual" | tr '[:lower:]' '[:upper:]')
    expected_upper=$(echo "$expected" | tr '[:lower:]' '[:upper:]')
    
    if echo "$actual_upper" | grep -qi "$expected_upper"; then
        echo "SUCCESS: $name"
        passed=$((passed + 1))
        return 0
    else
        echo "FAIL: $name"
        echo "  Expected at $mem_addr: $expected"
        echo "  Actual:                 $actual"
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
        $LD "$o_file" lib/build/c45_zp.lib -z 0x08 -o "$prg_out" 2>/dev/null
    else
        $LD "$o_file" lib/build/c45.lib -z 0x08 -o "$prg_out" 2>/dev/null
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

echo ""
echo "======================================================"
echo "Xemu Test Results: $passed passed, $failed failed"
echo "======================================================"

if [ $failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
