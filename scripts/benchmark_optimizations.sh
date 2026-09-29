#!/bin/bash

# Phase 82.3.5: Per-Function Optimization Benchmarking Suite
# Measures code size impact of per-function optimization selections

set -e

COMPILER="./bin/cc45"
RESULTS_DIR="./benchmark_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "$RESULTS_DIR"

echo "=== Phase 82.3.5: Optimization Benchmarking ==="
echo "Timestamp: $TIMESTAMP"
echo ""

# Test programs with different characteristics
TEST_PROGRAMS=(
    "src/test-resources/test_short.c:Fibonacci_Recursive"
    "src/test-resources/test_long.c:ComplexArithmetic"
)

# Function to run benchmark
benchmark_program() {
    local source=$1
    local name=$2
    local opt_level=$3

    echo "Benchmarking: $name ($source, $opt_level)"

    local output="/tmp/bench_${name}_${opt_level}.prg"
    local size=0

    if timeout 15 $COMPILER "$opt_level" "$source" -o "$output" 2>&1 | tail -1; then
        size=$(wc -c < "$output")
        echo "  ✅ Size: $size bytes"
        echo "$name,$opt_level,$size" >> "$RESULTS_DIR/benchmark_${TIMESTAMP}.csv"
    else
        echo "  ❌ Compilation failed"
    fi
}

# Create CSV header
echo "Program,Optimization,SizeBytes" > "$RESULTS_DIR/benchmark_${TIMESTAMP}.csv"

# Run benchmarks
for prog_spec in "${TEST_PROGRAMS[@]}"; do
    source="${prog_spec%:*}"
    name="${prog_spec#*:}"

    if [ ! -f "$source" ]; then
        echo "⚠️  Skipping $name: $source not found"
        continue
    fi

    echo ""
    echo "Testing: $name"
    echo "─────────────────────────"

    benchmark_program "$source" "$name" "-O0"
    benchmark_program "$source" "$name" "-O1"
    benchmark_program "$source" "$name" "-O2"
done

# Generate summary report
echo ""
echo "=== Benchmark Results Summary ==="
echo ""

if [ -f "$RESULTS_DIR/benchmark_${TIMESTAMP}.csv" ]; then
    echo "Results saved to: $RESULTS_DIR/benchmark_${TIMESTAMP}.csv"
    echo ""
    echo "Program,O0 Size,O1 Size,O2 Size,O1 Savings,O2 Savings"

    # Parse and display results
    awk -F',' '
    NR > 1 {
        prog = $1
        opt = $2
        size = $3
        if (opt == "-O0") o0[prog] = size
        if (opt == "-O1") o1[prog] = size
        if (opt == "-O2") o2[prog] = size
    }
    END {
        for (p in o0) {
            delta1 = o0[p] - o1[p]
            pct1 = (delta1 * 100) / o0[p]
            delta2 = o0[p] - o2[p]
            pct2 = (delta2 * 100) / o0[p]
            printf "%s,%d,%d,%d,%d (%.1f%%),%d (%.1f%%)\n",
                   p, o0[p], o1[p], o2[p], delta1, pct1, delta2, pct2
        }
    }
    ' "$RESULTS_DIR/benchmark_${TIMESTAMP}.csv"
else
    echo "❌ No results generated"
fi

echo ""
echo "✅ Benchmark complete"
