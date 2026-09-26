#!/bin/bash

# Phase 86 Integration Test Suite
# Validates cross-function optimization framework functionality

COMPILER="./bin/cc45"
TESTS=(
    "test_callgraph.c"
    "test_cooptimization.c"
    "test_devirtualization.c"
)

PASS=0
FAIL=0

echo "=== Phase 86 Integration Tests ==="
echo

for test in "${TESTS[@]}"; do
    test_file="src/test-resources/$test"
    test_name=$(basename "$test" .c)

    if [ ! -f "$test_file" ]; then
        echo "✗ $test_name: Test file not found"
        ((FAIL++))
        continue
    fi

    # Try to compile with Phase 86 analysis enabled
    if output=$($COMPILER "$test_file" -c -O2 -o "build/${test_name}.o45" 2>&1); then
        echo "✓ $test_name: Compilation successful"
        ((PASS++))
    else
        echo "✗ $test_name: Compilation failed"
        echo "  Error: $output"
        ((FAIL++))
    fi
done

echo
echo "=== Test Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"

if [ $FAIL -eq 0 ]; then
    echo "✓ All Phase 86 tests passed"
    exit 0
else
    echo "✗ Some Phase 86 tests failed"
    exit 1
fi
