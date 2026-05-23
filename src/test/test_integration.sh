#!/bin/bash
# Integration tests: full cc45 -c → ca45 -c → ln45 → mmemu pipeline
# These test the separate compilation flow that unit tests don't cover.

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
MMEMU="mmemu-cli"
TESTDIR="src/test-resources/integration"
BUILD="build/test/integration"
mkdir -p "$BUILD"

failed=0
passed=0

# Helper: compile C to .o45
compile_obj() {
    local src="$1" out="$2" flags="${3:-}"
    $CC -c $flags -I"$TESTDIR" "$src" -o "$out" 2>/dev/null
    return $?
}

# Helper: link objects to PRG with crt0
link_prg() {
    local out="$1"; shift
    local crt0="$BUILD/crt0.o45"
    $LD -basic -o "$out" "$crt0" "$@" 2>/dev/null
    return $?
}

# Helper: run in mmemu and check result bytes
run_check() {
    local prg="$1" expected="$2" size="$3" name="$4"
    local output
    # -basic links __init at $200D (after BASIC SYS stub)
    output=$(echo -e "load $prg\nsetpc \$200D\nstep 5000000\nm \$4000 $size\nq" | $MMEMU -m rawMega65 2>/dev/null)
    if echo "$output" | grep -qi "4000:.*$expected"; then
        echo "PASS: $name"
        passed=$((passed + 1))
    else
        echo "FAIL: $name"
        echo "  Expected: $expected"
        echo "  Actual:   $(echo "$output" | grep '4000:')"
        failed=$((failed + 1))
    fi
}

# Check mmemu is available
if ! command -v $MMEMU &>/dev/null; then
    echo "mmemu-cli not found — skipping integration tests"
    exit 0
fi

echo "Running integration tests (cc45 -c → ln45 → mmemu)..."
echo ""

# Build crt0.o45 once
$AS -c lib/crt0.s -o "$BUILD/crt0.o45" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "FAIL: could not assemble crt0.s"
    exit 1
fi

# =========================================================================
# Test 1: Separate compilation with global variables
# =========================================================================
echo "Test 1: Global variables (DATA+TEXT segments)..."
compile_obj "$TESTDIR/test1_globals.c" "$BUILD/test1.o45" && \
link_prg "$BUILD/test1.prg" "$BUILD/test1.o45" && \
run_check "$BUILD/test1.prg" "2A 05 AA" 3 "globals"

# =========================================================================
# Test 2: Multi-object cross-calls
# =========================================================================
echo "Test 2: Multi-object cross-calls..."
compile_obj "$TESTDIR/test2_multi_obj_a.c" "$BUILD/test2a.o45" && \
compile_obj "$TESTDIR/test2_multi_obj_b.c" "$BUILD/test2b.o45" && \
link_prg "$BUILD/test2.prg" "$BUILD/test2a.o45" "$BUILD/test2b.o45" && \
run_check "$BUILD/test2.prg" "1E 07 AA" 3 "multi-object"

# =========================================================================
# Test 3: Struct across translation units
# =========================================================================
echo "Test 3: Struct across TUs..."
compile_obj "$TESTDIR/test3_struct_a.c" "$BUILD/test3a.o45" && \
compile_obj "$TESTDIR/test3_struct_b.c" "$BUILD/test3b.o45" && \
link_prg "$BUILD/test3.prg" "$BUILD/test3a.o45" "$BUILD/test3b.o45" && \
run_check "$BUILD/test3.prg" "1E AA" 2 "struct-across-TU"

# =========================================================================
# Test 4: cbm.h through linker (issue #46 regression)
# =========================================================================
echo "Test 4: cbm.h + globals through linker (issue #46)..."
compile_obj "$TESTDIR/test4_cbm_getin.c" "$BUILD/test4.o45" && \
link_prg "$BUILD/test4.prg" "$BUILD/test4.o45" && \
run_check "$BUILD/test4.prg" "01 AA" 2 "cbm-globals-linker"

# =========================================================================
# Test 5: 32-bit globals across link
# =========================================================================
echo "Test 5: Long (32-bit) globals through linker..."
compile_obj "$TESTDIR/test5_long_globals.c" "$BUILD/test5.o45" && \
link_prg "$BUILD/test5.prg" "$BUILD/test5.o45" && \
run_check "$BUILD/test5.prg" "E0 01 AA" 3 "long-globals"

# =========================================================================
# Test 6: BSS initialization
# =========================================================================
echo "Test 6: BSS zeroing through linker..."
compile_obj "$TESTDIR/test6_bss_init.c" "$BUILD/test6.o45" && \
link_prg "$BUILD/test6.prg" "$BUILD/test6.o45" && \
run_check "$BUILD/test6.prg" "00 00 2A AA" 4 "bss-init"

# =========================================================================
# Summary
# =========================================================================
echo ""
echo "========================================="
echo "Integration tests: $passed passed, $failed failed"
echo "========================================="

if [ $failed -gt 0 ]; then
    exit 1
fi
exit 0
