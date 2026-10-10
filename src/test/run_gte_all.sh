#!/bin/bash

# run_gte_all.sh — Run all GTE (GCC torture) tests on xemu-xmega65 and report failures
#
# A test passes when the program returns to BASIC (xemu -prgexit exits on READY).
# abort() executes BRK, which stops in the monitor, so a failing test hits the timeout.
#
# Usage: run_gte_all.sh [-j N] [-l names.txt]   (default: all tests, one job per CPU)

CC="./bin/cc45"
LD="./bin/ln45"
CRT0="lib/build/crt0.o45"
LIB="lib/build/c45.lib"
XEMU="xemu-xmega65"
GTE_DIR="src/test-resources/gte"
BUILD_DIR="build/test/gte"
RUN_TIMEOUT=8
# GTE tests are standard (often C89) C: plain char/int/long are signed and
# functions may be called without a declaration
GTE_CFLAGS="${GTE_CFLAGS--fsigned-default -fimplicit-function-declaration}"

JOBS=$(nproc 2>/dev/null || echo 4)
LIST=""
while [ $# -gt 0 ]; do
    case "$1" in
        -j) JOBS=$2; shift 2 ;;
        -l) LIST=$2; shift 2 ;;   # only run the test names listed in this file
        *) shift ;;
    esac
done

if ! command -v $XEMU &>/dev/null; then
    echo "xemu-xmega65 not found — skipping GTE tests"
    exit 0
fi

mkdir -p "$BUILD_DIR"

run_one() {
    local f="$1"
    local name; name=$(basename "$f" .c)
    local obj="$BUILD_DIR/$name.o45"
    local prg="$BUILD_DIR/$name.prg"

    case "$name" in
        # Bitfield widths >16 bits exceed the 16-bit target's limits
        bitfld-6|bitfld-7|pr103417|pr19689|pr31448-2|pr31448|pr40404|pr52979-1|pr52979-2|pr57344-2|pr58570|pr70602|pr78170|pr78436|pr89195|pr97764)
            echo "SKIP: $name (bitfield width >16 bits)"
            return
            ;;
        # GCC vector extension (__attribute__((vector_size))) is not supported
        pr53645|pr53645-2)
            echo "SKIP: $name (GCC vector types unsupported)"
            return
            ;;
    esac

    if ! $CC -c $GTE_CFLAGS -I"$GTE_DIR" "$f" -o "$obj" > "$BUILD_DIR/$name.compile.log" 2>&1; then
        echo "COMPILE FAIL: $name"; return
    fi
    if ! $LD -basic -o "$prg" "$CRT0" "$obj" "$LIB" > "$BUILD_DIR/$name.link.log" 2>&1; then
        echo "LINK FAIL: $name"; return
    fi
    timeout "$RUN_TIMEOUT" $XEMU -headless -besure -prgmode 65 -prgexit -prg "$prg" </dev/null >/dev/null 2>&1
    local res=$?
    if [ $res -eq 0 ]; then
        echo "PASS: $name"
    elif [ $res -eq 124 ]; then
        echo "RUN FAIL (abort or hang): $name"
    else
        echo "RUN FAIL (xemu exit $res): $name"
    fi
}
export -f run_one
export CC LD CRT0 LIB XEMU GTE_DIR BUILD_DIR RUN_TIMEOUT GTE_CFLAGS

if [ -n "$LIST" ]; then
    TEST_FILES=$(grep -v '^\s*\(#\|$\)' "$LIST" | sed "s|^|$GTE_DIR/|; s|$|.c|")
else
    TEST_FILES=$(ls "$GTE_DIR"/*.c)
fi
TOTAL=$(echo "$TEST_FILES" | wc -l)
echo "Running $TOTAL GTE tests ($JOBS parallel)..."

RESULTS="$BUILD_DIR/results.txt"
echo "$TEST_FILES" | xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} | sort -t: -k2 > "$RESULTS"
grep -v "^PASS" "$RESULTS"

passed=$(grep -c "^PASS" "$RESULTS")
skipped=$(grep -c "^SKIP" "$RESULTS")
failed_compile=$(grep -c "^COMPILE FAIL" "$RESULTS")
failed_link=$(grep -c "^LINK FAIL" "$RESULTS")
failed_run=$(grep -c "^RUN FAIL" "$RESULTS")

echo ""
echo "Summary:"
echo "Total: $TOTAL"
echo "Passed: $passed"
echo "Skipped: $skipped"
echo "Failed Compile: $failed_compile"
echo "Failed Link: $failed_link"
echo "Failed Run: $failed_run"

if [ $((failed_compile + failed_link + failed_run)) -gt 0 ]; then
    exit 1
fi
exit 0
