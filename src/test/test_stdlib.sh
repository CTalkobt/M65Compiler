#!/bin/bash
# Stdlib validation tests via mmemu-cli - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

LN="${LN:-./bin/ln45}"
MMEMU="${MMEMU:-mmemu-cli}"
LIB="lib/build/c45.lib"
SRCDIR="src/test-resources/stdlib"

mkdir -p build/test/stdlib

run_stdlib_test() {
    local name="$1" src="$SRCDIR/${name}.c" o45="build/test/stdlib/${name}.o45" prg="build/test/stdlib/${name}.prg"
    [ ! -f "$src" ] && print_skip "$name (source not found)" && return
    $CC -c "$src" -o "$o45" 2>/dev/null || { print_fail "$name (compile error)"; return; }
    $LN -basic -o "$prg" "$o45" "$LIB" 2>/dev/null || { print_fail "$name (link error)"; return; }
    OUTPUT=$(echo -e "load $prg\nrun\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)
    echo "$OUTPUT" | grep -q "A: \$00" && print_pass "$name" || print_fail "$name"
}

print_section "Stdlib validation tests (mmemu-cli)"

print_section "string.h"
run_stdlib_test "test_strlen"
run_stdlib_test "test_strcpy"
run_stdlib_test "test_strcmp"
run_stdlib_test "test_memcpy"
run_stdlib_test "test_memset"

print_section "stdlib.h"
run_stdlib_test "test_atoi"
run_stdlib_test "test_itoa"
run_stdlib_test "test_malloc"
run_stdlib_test "test_rand"

print_section "ctype.h"
run_stdlib_test "test_ctype"

print_section "math.h"
run_stdlib_test "test_min_max"
run_stdlib_test "test_gcd_lcm"

print_section "time.h"
run_stdlib_test "test_time"

test_summary
exit $?
