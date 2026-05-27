#!/bin/bash

# test_stdlib.sh — Stdlib validation tests via mmemu-cli
#
# Each test returns A=$00 on success, non-zero on failure.
# Tests are compiled with cc45 -c, linked with ln45 + c45.lib,
# and executed in mmemu-cli.

CC="./bin/cc45"
LD="./bin/ln45"
MMEMU="mmemu-cli"
LIB="lib/build/c45.lib"
SRCDIR="src/test-resources/stdlib"

mkdir -p build/test/stdlib

passed=0
failed=0
skipped=0

# Compile, link, and run a stdlib test.
# Usage: run_test "test_name" [extra_cc_flags]
run_test() {
    local name="$1"
    local flags="${2:-}"
    local src="$SRCDIR/${name}.c"
    local o45="build/test/stdlib/${name}.o45"
    local prg="build/test/stdlib/${name}.prg"

    if [ ! -f "$src" ]; then
        echo "  SKIP: $name (source not found)"
        skipped=$((skipped + 1))
        return
    fi

    # Compile to .o45
    $CC -c $flags "$src" -o "$o45" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (compile error)"
        failed=$((failed + 1))
        return
    fi

    # Link with stdlib
    $LD -basic -o "$prg" "$o45" "$LIB" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (link error)"
        failed=$((failed + 1))
        return
    fi

    # Run in mmemu-cli: load PRG, run, check A register
    local OUTPUT
    OUTPUT=$(echo -e "load $prg\nrun\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)

    if echo "$OUTPUT" | grep -q "A: \$00"; then
        echo "  PASS: $name"
        passed=$((passed + 1))
    else
        local areg
        areg=$(echo "$OUTPUT" | grep "A: \\\$" | head -1)
        echo "  FAIL: $name ($areg)"
        failed=$((failed + 1))
    fi
}

echo "========================================"
echo "Stdlib validation tests (mmemu-cli)"
echo "========================================"

# string.h tests
echo ""
echo "--- string.h ---"
run_test "test_strlen"
run_test "test_strcpy"
run_test "test_strcmp"
run_test "test_memcpy"
run_test "test_memset"

# stdlib.h tests
echo ""
echo "--- stdlib.h ---"
run_test "test_atoi"
run_test "test_itoa"
run_test "test_malloc"
run_test "test_rand"

# ctype.h tests
echo ""
echo "--- ctype.h ---"
run_test "test_ctype"

# math.h tests
echo ""
echo "--- math.h ---"
run_test "test_min_max"
run_test "test_gcd_lcm"

# time.h tests
echo ""
echo "--- time.h ---"
run_test "test_time"

echo ""
echo "========================================"
echo "Stdlib tests: $passed passed, $failed failed, $skipped skipped"
echo "========================================"

if [ $failed -ne 0 ]; then
    exit 1
fi
exit 0
