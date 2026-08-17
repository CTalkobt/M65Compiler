#!/bin/bash

# Test script for xemu-xmega65 validation
# Alternative to mmemu-cli for MEGA65 hardware emulation
# Uses direct compilation (no stdlib linking) for SAC tests

CC="./bin/cc45"
AS="./bin/ca45"
XEMU="xemu-xmega65"
mkdir -p build/test

# Check availability
if ! command -v "$XEMU" &>/dev/null; then
    echo "ERROR: xemu-xmega65 not found. Install from https://github.com/lgblgblgb/xemu"
    exit 1
fi

failed=0
passed=0

# Compile directly to PRG (no stdlib linking)
compile_direct() {
    local src="$1"
    local s_file="$2"
    local prg_file="$3"

    $CC "$src" -o "$s_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 1; fi

    $AS "$s_file" -o "$prg_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 2; fi

    return 0
}

# Run PRG with xemu and extract memory at $4000
run_xemu_test() {
    local name="$1"
    local prg_file="$2"
    local expected="$3"

    local dump_file="/tmp/xemu_memdump_$$.bin"
    local timeout_secs=15

    # Run in headless mode with memory dump on exit
    timeout $timeout_secs $XEMU -headless -prgexit -dumpmem "$dump_file" "$prg_file" >/dev/null 2>&1

    if [ ! -f "$dump_file" ]; then
        echo "FAIL: $name (execution timeout or error)"
        failed=$((failed + 1))
        return 1
    fi

    # Extract 7 bytes from $4000 (offset 16384 decimal)
    # Memory dump is binary, so extract raw bytes and convert to hex
    local actual=$(od -An -tx1 -N7 -j16384 "$dump_file" 2>/dev/null | tr -d ' \n' | tr '[:lower:]' '[:upper:]')
    rm -f "$dump_file"

    if [ -z "$actual" ]; then
        echo "FAIL: $name (couldn't extract memory)"
        failed=$((failed + 1))
        return 1
    fi

    local expected_upper=$(echo "$expected" | tr '[:lower:]' '[:upper:]' | tr -d ' ')

    if [ "$actual" = "$expected_upper" ]; then
        echo "SUCCESS: $name — output: $actual"
        passed=$((passed + 1))
        return 0
    else
        echo "FAIL: $name"
        echo "  Expected: $expected_upper"
        echo "  Got:      $actual"
        failed=$((failed + 1))
        return 1
    fi
}

echo "Testing with xemu-xmega65 (MEGA65 Hardware Emulator)"
echo "===================================================="
echo ""

# Test 1: test_short.c (SAC parameter passing)
echo "Testing test_short.c (short type with SAC)..."
compile_direct "src/test-resources/test_short.c" "build/test/test_short_xemu.s45" "build/test/test_short_xemu.prg"
if [ $? -eq 0 ]; then
    # Expected: 1E 05 02 0C 0A C8 AA (7 bytes)
    run_xemu_test "test_short" "build/test/test_short_xemu.prg" "1E05020C0AC8AA"
else
    echo "FAIL: test_short.c (compile/assemble error)"
    failed=$((failed + 1))
fi

# Test 2: test_compound_literal.c
echo "Testing test_compound_literal.c (compound literals)..."
compile_direct "src/test-resources/test_compound_literal.c" "build/test/test_compound_literal_xemu.s45" "build/test/test_compound_literal_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_compound_literal" "build/test/test_compound_literal_xemu.prg" "2A2B2C2D"
else
    echo "FAIL: test_compound_literal.c (compile/assemble error)"
    failed=$((failed + 1))
fi

# Test 3: test_array_loop.c
echo "Testing test_array_loop.c (array indexing)..."
compile_direct "src/test-resources/test_array_loop.c" "build/test/test_array_loop_xemu.s45" "build/test/test_array_loop_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_array_loop" "build/test/test_array_loop_xemu.prg" "0102030405"
else
    echo "FAIL: test_array_loop.c (compile/assemble error)"
    failed=$((failed + 1))
fi

# Test 4: test_array_init.c
echo "Testing test_array_init.c (array initialization)..."
compile_direct "src/test-resources/test_array_init.c" "build/test/test_array_init_xemu.s45" "build/test/test_array_init_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_array_init" "build/test/test_array_init_xemu.prg" "0102030405"
else
    echo "FAIL: test_array_init.c (compile/assemble error)"
    failed=$((failed + 1))
fi

# Test 5: test_long_mmemu.c
echo "Testing test_long_mmemu.c (long type)..."
compile_direct "src/test-resources/test_long_mmemu.c" "build/test/test_long_mmemu_xemu.s45" "build/test/test_long_mmemu_xemu.prg"
if [ $? -eq 0 ]; then
    # Expected: 78 56 34 12 (little-endian)
    run_xemu_test "test_long_mmemu" "build/test/test_long_mmemu_xemu.prg" "78563412"
else
    echo "FAIL: test_long_mmemu.c (compile/assemble error)"
    failed=$((failed + 1))
fi

echo ""
echo "===================================================="
echo "Xemu Test Results: $passed passed, $failed failed"
echo "===================================================="
echo ""

if [ $failed -gt 0 ]; then
    echo "NOTE: If tests fail, possible causes:"
    echo "  1. xemu-xmega65 not properly installed"
    echo "  2. Generated PRG files have invalid 6502 code"
    echo "  3. Programs don't write expected values to \$4000"
    echo ""
    exit 1
else
    echo "All xemu tests passed!"
    exit 0
fi
