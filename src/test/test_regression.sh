#!/bin/bash
# 45GS02 regression tests - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

LN="${LN:-./bin/ln45}"
LIB="lib/build/c45.lib"
SRCDIR="src/test-resources/regression"

mkdir -p build/test/regression

run_regression_test() {
    local name="$1" src="$SRCDIR/${name}.c" o45="build/test/regression/${name}.o45" prg="build/test/regression/${name}.prg"
    [ ! -f "$src" ] && print_skip "$name (source not found)" && return
    $CC -c "$src" -o "$o45" 2>/dev/null || { print_fail "$name (compile error)"; return; }
    $LN -basic -o "$prg" "$o45" "$LIB" 2>/dev/null || { print_fail "$name (link error)"; return; }
    print_pass "$name"
}

print_section "45GS02 Regression Tests"

run_regression_test "test_long_arith"
run_regression_test "test_struct_ops"
run_regression_test "test_mixed_width"
run_regression_test "test_ptr_arith"
run_regression_test "test_switch_range"
run_regression_test "test_bitfield"
run_regression_test "test_deep_calls"
run_regression_test "test_issue83_cast_ptr_width"
run_regression_test "test_issue84_nested_struct_array"

test_summary
exit $?
