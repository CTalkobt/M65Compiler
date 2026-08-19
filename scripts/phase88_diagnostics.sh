#!/bin/bash

# Phase 88 Diagnostics
# Shows what inline recommendations and optimizations are being found

COMPILER="./bin/cc45"

echo "=== Phase 88 Diagnostics ==="
echo

for test_file in src/test-resources/test_callgraph.c src/test-resources/test_cooptimization.c; do
    test_name=$(basename "$test_file" .c)
    echo "Analyzing $test_name..."
    echo

    # Compile with verbose output (only capture optimization-related lines)
    $COMPILER "$test_file" -c -O2 -o "build/${test_name}_diag.o45" 2>&1 | grep -E "Found|Marked|Applied|Inline|Executing|call graph|devirtualization|co-optimization"

    echo "---"
    echo
done

echo "=== Analysis Complete ==="
