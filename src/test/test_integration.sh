#!/bin/bash
# Integration tests: cc45 -c → ln45 → mmemu pipeline - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

AS="${AS:-./bin/ca45}"
LN="${LN:-./bin/ln45}"
MMEMU="${MMEMU:-mmemu-cli}"
TESTDIR="src/test-resources/integration"
BUILD="build/test/integration"
mkdir -p "$BUILD"

# Check mmemu is available
command -v $MMEMU &>/dev/null || { print_skip "mmemu-cli not found — skipping integration tests"; exit 0; }

# Build crt0.o45 once
$AS -c lib/crt0.s -o "$BUILD/crt0.o45" 2>/dev/null || { print_fail "assemble crt0.s"; exit 1; }

compile_obj() { $CC -c -I"$TESTDIR" "$1" -o "$2" 2>/dev/null; }
link_prg() { local out="$1"; shift; $LN -basic -o "$out" "$BUILD/crt0.o45" "$@" 2>/dev/null; }
run_check() {
    local prg="$1" expected="$2" size="$3" name="$4"
    local output=$(echo -e "load $prg\nsetpc \$200D\nstep 5000000\nm \$4000 $size\nq" | $MMEMU -m rawMega65 2>/dev/null)
    echo "$output" | grep -qi "4000:.*$expected" && print_pass "$name" || print_fail "$name"
}

print_section "Integration tests"

compile_obj "$TESTDIR/test1_globals.c" "$BUILD/test1.o45" && link_prg "$BUILD/test1.prg" "$BUILD/test1.o45" && run_check "$BUILD/test1.prg" "2A 05 AA" 3 "globals"
compile_obj "$TESTDIR/test2_multi_obj_a.c" "$BUILD/test2a.o45" && compile_obj "$TESTDIR/test2_multi_obj_b.c" "$BUILD/test2b.o45" && link_prg "$BUILD/test2.prg" "$BUILD/test2a.o45" "$BUILD/test2b.o45" && run_check "$BUILD/test2.prg" "1E 07 AA" 3 "multi-object"
compile_obj "$TESTDIR/test3_struct_a.c" "$BUILD/test3a.o45" && compile_obj "$TESTDIR/test3_struct_b.c" "$BUILD/test3b.o45" && link_prg "$BUILD/test3.prg" "$BUILD/test3a.o45" "$BUILD/test3b.o45" && run_check "$BUILD/test3.prg" "1E AA" 2 "struct-across-TU"
compile_obj "$TESTDIR/test4_cbm_getin.c" "$BUILD/test4.o45" && link_prg "$BUILD/test4.prg" "$BUILD/test4.o45" && run_check "$BUILD/test4.prg" "01 AA" 2 "cbm-globals"
compile_obj "$TESTDIR/test5_long_globals.c" "$BUILD/test5.o45" && link_prg "$BUILD/test5.prg" "$BUILD/test5.o45" && run_check "$BUILD/test5.prg" "E0 01 AA" 3 "long-globals"
compile_obj "$TESTDIR/test6_bss_init.c" "$BUILD/test6.o45" && link_prg "$BUILD/test6.prg" "$BUILD/test6.o45" && run_check "$BUILD/test6.prg" "00 00 2A AA" 4 "bss-init"

test_summary
exit $?
