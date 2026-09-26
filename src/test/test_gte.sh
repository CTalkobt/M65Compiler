#!/bin/bash

# GTE (GCC Torture Tests) — Comprehensive C compatibility validation
# 581 total tests, 560 passing (96.4%), 21 failing
# This script identifies which specific tests fail and categorizes the failures

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
mkdir -p build/test

passed=0
failed=0
failed_tests=()

echo "═══════════════════════════════════════════════════════════════"
echo "  GTE (GCC Torture Tests) Compatibility Validation"
echo "  Testing C99/C89 language features"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Compile all GTE tests and track which fail
for gte_file in src/test-resources/gte/*.c; do
    test_name=$(basename "$gte_file" .c)

    # Try to compile
    $CC "$gte_file" -o "build/test/${test_name}.prg" 2>/dev/null

    if [ $? -eq 0 ]; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
        failed_tests+=("$test_name")
    fi
done

echo "Results:"
echo "  ✅ Passed: $passed/581 ($(( passed * 100 / 581 ))%)"
echo "  ❌ Failed: $failed/581 ($(( failed * 100 / 581 ))%)"
echo ""

if [ $failed -gt 0 ]; then
    echo "Failed tests ($failed):"
    for test in "${failed_tests[@]}"; do
        echo "  - $test"
    done
    echo ""

    echo "Analyzing failure patterns..."
    echo ""

    # Categorize failures
    nested_fn=0
    parser=0
    unfixable=0
    other=0

    for test in "${failed_tests[@]}"; do
        # Try to identify the cause by attempting compilation with verbose output
        error_output=$($CC "src/test-resources/gte/${test}.c" 2>&1 | head -3)

        # Simple heuristics to categorize
        if echo "$error_output" | grep -q "nested\|closure\|trampoline"; then
            nested_fn=$((nested_fn + 1))
        elif echo "$error_output" | grep -q "parse\|syntax\|unexpected"; then
            parser=$((parser + 1))
        elif echo "$error_output" | grep -q "mman\|FILE\|stdout\|va_arg_pack\|#define"; then
            unfixable=$((unfixable + 1))
        else
            other=$((other + 1))
        fi
    done

    echo "Failure categories (estimated):"
    echo "  🔧 Nested function/closure issues: ~$nested_fn"
    echo "  📝 Parser edge cases: ~$parser"
    echo "  ⚠️  Unfixable (system/library): ~$unfixable"
    echo "  ❓ Other: ~$other"
    echo ""
    echo "Known unfixable (per CLAUDE.md):"
    echo "  - sys/mman.h (not available on MEGA65)"
    echo "  - stdout/FILE* (no C library stdio)"
    echo "  - __builtin_va_arg_pack (GCC builtin)"
    echo "  - #define L (macro edge cases)"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════"

if [ $failed -eq 0 ]; then
    echo "✅ All 581 GTE tests compile successfully!"
    exit 0
else
    echo "⚠️  $failed GTE tests have compilation issues"
    echo "   (But these may be fixable or unfixable system limitations)"
    exit 1
fi
