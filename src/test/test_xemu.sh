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

# Test 1: test_short.c (short type, arithmetic, function params/returns)
echo "Testing test_short.c (short type)..."
compile_link_test "src/test-resources/test_short.c" "build/test/test_short_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_short" "build/test/test_short_xemu.prg" "C010" 7 "1E 05 02 0C 0A 14 AA"
else
    echo "FAIL: test_short.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 2: test_struct_return.c (struct return by value)
echo "Testing test_struct_return.c (struct return)..."
compile_link_test "src/test-resources/test_struct_return.c" "build/test/test_struct_return_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_struct_return" "build/test/test_struct_return_xemu.prg" "C010" 7 "01 02 03 04 0A 14 AA"
else
    echo "FAIL: test_struct_return.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 3: test_array_init.c (global array initializers)
# NOTE: may fail until #262 (initialized globals in BSS) is fixed
echo "Testing test_array_init.c (array initialization)..."
compile_link_test "src/test-resources/test_array_init.c" "build/test/test_array_init_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_array_init" "build/test/test_array_init_xemu.prg" "C010" 11 "10 40 64 2C 0B 16 00 00 00 00 AA"
else
    echo "FAIL: test_array_init.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 4: test_compound_literal.c (struct/scalar compound literals)
echo "Testing test_compound_literal.c (compound literals)..."
compile_link_test "src/test-resources/test_compound_literal.c" "build/test/test_compound_literal_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_compound_literal" "build/test/test_compound_literal_xemu.prg" "C010" 8 "1E 2A 07 2C 01 14 00 AA"
else
    echo "FAIL: test_compound_literal.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 5: test_long_mmemu.c (32-bit long type)
# NOTE: may fail until #262 (initialized globals in BSS) is fixed
echo "Testing test_long_mmemu.c (long type)..."
compile_link_test "src/test-resources/test_long_mmemu.c" "build/test/test_long_mmemu_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_long_mmemu" "build/test/test_long_mmemu_xemu.prg" "C010" 12 "04 C0 01 A0 2A A0 00 E0 93 04 00 AA"
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

# Test 7: test_mem_ops_xemu.c (stdlib memset/memcpy/memcmp/memmove)
echo "Testing test_mem_ops_xemu.c (memory operations)..."
compile_link_test "src/test-resources/test_mem_ops_xemu.c" "build/test/test_mem_ops_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_mem_ops" "build/test/test_mem_ops_xemu.prg" "C000" 11 "01 02 03 04 05 06 07 08 09 0A AA"
else
    echo "FAIL: test_mem_ops_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 8: test_string_xemu.c (string.h functions)
echo "Testing test_string_xemu.c (string operations)..."
compile_link_test "src/test-resources/test_string_xemu.c" "build/test/test_string_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_string" "build/test/test_string_xemu.prg" "C020" 13 "01 02 03 04 05 06 07 08 09 0A 0B 0C AA"
else
    echo "FAIL: test_string_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 9: test_ctype_xemu.c (ctype.h functions)
echo "Testing test_ctype_xemu.c (character classification)..."
compile_link_test "src/test-resources/test_ctype_xemu.c" "build/test/test_ctype_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_ctype" "build/test/test_ctype_xemu.prg" "C030" 9 "01 02 03 04 05 06 07 08 AA"
else
    echo "FAIL: test_ctype_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 10: test_stdlib_xemu.c (stdlib.h functions)
echo "Testing test_stdlib_xemu.c (stdlib functions)..."
compile_link_test "src/test-resources/test_stdlib_xemu.c" "build/test/test_stdlib_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_stdlib" "build/test/test_stdlib_xemu.prg" "C040" 6 "01 02 03 04 05 AA"
else
    echo "FAIL: test_stdlib_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 11: test_math_xemu.c (integer math + hardware divider)
echo "Testing test_math_xemu.c (integer math)..."
compile_link_test "src/test-resources/test_math_xemu.c" "build/test/test_math_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_math" "build/test/test_math_xemu.prg" "C070" 6 "01 02 03 04 05 AA"
else
    echo "FAIL: test_math_xemu.c (compilation/linking failed)"
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
