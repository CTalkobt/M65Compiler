#!/bin/bash

# test_regression.s45h — 45GS02 regression tests
#
# Validates that diverse 45GS02-specific patterns compile, assemble,
# and link correctly. Covers: 32-bit long arithmetic, struct operations,
# mixed-width promotions, pointer arithmetic, switch/case ranges,
# bitfields, and deep call chains.
#
# Each test returns 0 (A=$00) on success when executed.
# Compile+link validation catches codegen, assembler, and linker regressions.

CC="./bin/cc45"
LD="./bin/ln45"
LIB="lib/build/c45.lib"
SRCDIR="src/test-resources/regression"

mkdir -p build/test/regression

passed=0
failed=0

run_test() {
    local name="$1"
    local src="$SRCDIR/${name}.c"
    local o45="build/test/regression/${name}.o45"
    local prg="build/test/regression/${name}.prg"

    if [ ! -f "$src" ]; then
        echo "  SKIP: $name (source not found)"
        return
    fi

    # Compile
    $CC -c "$src" -o "$o45" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (compile error)"
        failed=$((failed + 1))
        return
    fi

    # Link
    $LD -basic -o "$prg" "$o45" "$LIB" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  FAIL: $name (link error)"
        failed=$((failed + 1))
        return
    fi

    local prg_size
    prg_size=$(stat -c%s "$prg" 2>/dev/null || stat -f%z "$prg" 2>/dev/null)
    echo "  PASS: $name ($prg_size bytes)"
    passed=$((passed + 1))
}

echo "========================================"
echo "45GS02 Regression Tests"
echo "========================================"
echo ""

echo "--- 32-bit long arithmetic ---"
run_test "test_long_arith"

echo "--- Struct operations ---"
run_test "test_struct_ops"

echo "--- Mixed-width promotions ---"
run_test "test_mixed_width"

echo "--- Pointer arithmetic ---"
run_test "test_ptr_arith"

echo "--- Switch/case ranges ---"
run_test "test_switch_range"

echo "--- Bitfield operations ---"
run_test "test_bitfield"

echo "--- Deep call chains ---"
run_test "test_deep_calls"

echo "--- Issue #83: cast-pointer store width ---"
run_test "test_issue83_cast_ptr_width"

echo "--- Issue #84: nested struct array member access ---"
run_test "test_issue84_nested_struct_array"

echo ""
echo "========================================"
echo "Regression tests: $passed passed, $failed failed"
echo "========================================"

if [ $failed -ne 0 ]; then
    exit 1
fi
exit 0
