#!/bin/bash

# run_gte_all.sh — Run all GTE tests and report failures

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
LIB="lib/build/c45.lib"
MMEMU="mmemu-cli"
GTE_DIR="src/test-resources/gte"
BUILD_DIR="build/test/gte"

mkdir -p "$BUILD_DIR"

passed=0
skipped=0
failed_compile=0
failed_link=0
failed_run=0

# Check mmemu is available
if ! command -v $MMEMU &>/dev/null; then
    echo "mmemu-cli not found"
    exit 1
fi

TEST_FILES=$(ls "$GTE_DIR"/*.c)
TOTAL=$(echo "$TEST_FILES" | wc -l)
echo "Running $TOTAL GTE tests..."

for f in $TEST_FILES; do
    name=$(basename "$f" .c)
    obj="$BUILD_DIR/$name.o45"
    prg="$BUILD_DIR/$name.prg"
    
    # Check if skipped
    case "$name" in
        # Issue #1: 16-bit Bitfield Limits (exceeds 16-bit targets)
        bitfld-6|bitfld-7|pr103417|pr19689|pr31448-2|pr31448|pr40404|pr52979-1|pr52979-2|pr57344-2|pr58570|pr70602|pr78170|pr78436|pr89195|pr97764)
            echo "SKIP: $name (Bitfield width limits (>16 bits) exceed 16-bit target architecture constraints)"
            skipped=$((skipped + 1))
            continue
            ;;
    esac
    
    # 1. Compile
    $CC -c -I"$GTE_DIR" "$f" -o "$obj" > "$BUILD_DIR/$name.compile.log" 2>&1
    res=$?
    if [ $res -ne 0 ]; then
        echo "COMPILE FAIL: $name"
        failed_compile=$((failed_compile + 1))
        continue
    fi
    
    # 2. Link
    $LD -basic -o "$prg" "$obj" "$LIB" > "$BUILD_DIR/$name.link.log" 2>&1
    res=$?
    if [ $res -ne 0 ]; then
        echo "LINK FAIL: $name"
        failed_link=$((failed_link + 1))
        continue
    fi
    
    # 3. Run
    OUTPUT=$(echo -e "load $prg\nsetpc \$200D\nrun\nregs\nq" | timeout 5s $MMEMU -m rawMega65 2>/dev/null)
    res=$?
    
    if [ $res -eq 124 ]; then
        echo "RUN FAIL (timeout): $name"
        failed_run=$((failed_run + 1))
    elif echo "$OUTPUT" | grep -q "B: \$01"; then
        echo "RUN FAIL (abort): $name"
        failed_run=$((failed_run + 1))
    else
        echo "PASS: $name"
        passed=$((passed + 1))
    fi
done

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
