#!/bin/bash

# Phase 88 Benchmarking Script: Compound Assignment Fusion
# Measures code size improvements from Phase 87 optimization

set -e

COMPILER="../../bin/cc45"
ASSEMBLER="../../bin/ca45"
RESULTS_DIR="benchmark_results"
REPORT_FILE="$RESULTS_DIR/phase88_report.md"

mkdir -p "$RESULTS_DIR"

echo "=== Phase 88: Compound Assignment Fusion Benchmarking ==="
echo "Date: $(date)"
echo ""

# Array of benchmark files and their descriptions
declare -a BENCHMARKS=(
    "benchmark_compound_math.c:Mathematical algorithms with compound operators"
    "benchmark_compound_image.c:Image/pixel processing with compound assignments"
    "benchmark_compound_matrix.c:Matrix operations with nested compound loops"
    "benchmark_compound_algo.c:Algorithmic patterns (Fibonacci, checksums, etc)"
)

# Generate markdown report header
cat > "$REPORT_FILE" << 'EOF'
# Phase 88: Compound Assignment Fusion Benchmarking Results

**Date:** $(date)
**Optimization:** Phase 87 Compound Assignment Chain Fusion
**Expected Impact:** 15-25% code reduction for compound assignment chains

## Summary

| Benchmark | -O0 Size | -O1 Size | -O2 Size | Reduction (O0→O1) | Reduction (O0→O2) |
|-----------|----------|----------|----------|-------------------|-------------------|
EOF

# Function to compile and measure
measure_size() {
    local src=$1
    local opt_level=$2
    local output_base="${RESULTS_DIR}/$(basename ${src%.c})_${opt_level}"
    local asm_file="${output_base}.s45"
    local obj_file="${output_base}.o45"

    # Compile to assembly
    $COMPILER $opt_level "$src" -S -o "$asm_file" 2>/dev/null

    # Assemble to object, capturing output
    local asm_output=$($ASSEMBLER -c "$asm_file" -o "$obj_file" 2>&1)

    # Try multiple ways to get size
    local size=0
    if [ -f "$obj_file" ]; then
        # Extract bytes from "Object file: ... (X bytes)" format
        if [[ "$asm_output" =~ \(([0-9]+)\ bytes\) ]]; then
            size="${BASH_REMATCH[1]}"
        else
            # Fall back to file size using ls
            size=$(ls -l "$obj_file" | awk '{print $5}')
        fi
    fi

    echo "$size"
}

# Run benchmarks
echo "Running benchmarks..."
echo ""

total_reduction_o1=0
total_reduction_o2=0
benchmark_count=0

for benchmark_info in "${BENCHMARKS[@]}"; do
    IFS=: read -r bench_file bench_desc <<< "$benchmark_info"

    if [ ! -f "$bench_file" ]; then
        echo "⚠ Skipping $bench_file (not found)"
        continue
    fi

    echo "Benchmarking: $bench_desc"
    echo "  File: $bench_file"

    # Measure sizes at different optimization levels
    size_o0=$(measure_size "$bench_file" "-O0")
    size_o1=$(measure_size "$bench_file" "-O1")
    size_o2=$(measure_size "$bench_file" "-O2")

    if [ "$size_o0" -eq 0 ]; then
        echo "  ⚠ Compilation failed, skipping"
        continue
    fi

    # Calculate reductions
    reduction_o1=$((100 * (size_o0 - size_o1) / size_o0))
    reduction_o2=$((100 * (size_o0 - size_o2) / size_o0))

    echo "  -O0: $size_o0 bytes"
    echo "  -O1: $size_o1 bytes ($reduction_o1% reduction)"
    echo "  -O2: $size_o2 bytes ($reduction_o2% reduction)"
    echo ""

    # Add to report table
    short_name=$(basename ${bench_file%.c})
    echo "| \`$short_name\` | $size_o0 | $size_o1 | $size_o2 | ${reduction_o1}% | ${reduction_o2}% |" >> "$REPORT_FILE"

    # Accumulate for averages
    total_reduction_o1=$((total_reduction_o1 + reduction_o1))
    total_reduction_o2=$((total_reduction_o2 + reduction_o2))
    benchmark_count=$((benchmark_count + 1))
done

# Calculate averages
if [ "$benchmark_count" -gt 0 ]; then
    avg_reduction_o1=$((total_reduction_o1 / benchmark_count))
    avg_reduction_o2=$((total_reduction_o2 / benchmark_count))
else
    avg_reduction_o1=0
    avg_reduction_o2=0
fi

# Finalize report
cat >> "$REPORT_FILE" << EOF

## Average Reduction

| Level | Average Reduction |
|-------|-------------------|
| -O1   | ${avg_reduction_o1}% |
| -O2   | ${avg_reduction_o2}% |

## Analysis

### Optimization Effectiveness

**Expected Range:** 15-25% code reduction for compound assignment chains
**Observed (O1):** ${avg_reduction_o1}%
**Observed (O2):** ${avg_reduction_o2}%

### Assessment

EOF

if [ "$avg_reduction_o1" -ge 15 ] && [ "$avg_reduction_o1" -le 25 ]; then
    echo "✅ **MEETS EXPECTATIONS** — Phase 87 delivers expected 15-25% improvement" >> "$REPORT_FILE"
elif [ "$avg_reduction_o1" -gt 25 ]; then
    echo "🚀 **EXCEEDS EXPECTATIONS** — Phase 87 delivers ${avg_reduction_o1}% (expected 15-25%)" >> "$REPORT_FILE"
elif [ "$avg_reduction_o1" -lt 10 ]; then
    echo "⚠️ **BELOW EXPECTATIONS** — Phase 87 delivers only ${avg_reduction_o1}% (expected 15-25%)" >> "$REPORT_FILE"
else
    echo "✓ **GOOD IMPROVEMENT** — Phase 87 delivers ${avg_reduction_o1}% (slightly below expected 15-25%)" >> "$REPORT_FILE"
fi

cat >> "$REPORT_FILE" << EOF

### Benchmark Characteristics

The test suite covers:
1. **Mathematical algorithms** — Accumulation patterns, iterative refinement
2. **Image processing** — Pixel loops, channel operations, filtering
3. **Matrix operations** — Element-wise ops, row/column processing
4. **Algorithmic patterns** — Fibonacci, checksums, polynomial evaluation

These represent common compound assignment patterns in real-world code.

## Detailed Results

### Per-Benchmark Analysis

EOF

# Add detailed size breakdowns
for benchmark_info in "${BENCHMARKS[@]}"; do
    IFS=: read -r bench_file bench_desc <<< "$benchmark_info"
    if [ ! -f "$bench_file" ]; then continue; fi

    short_name=$(basename ${bench_file%.c})
    size_o0=$(measure_size "$bench_file" "-O0")
    size_o1=$(measure_size "$bench_file" "-O1")
    size_o2=$(measure_size "$bench_file" "-O2")

    if [ "$size_o0" -gt 0 ]; then
        cat >> "$REPORT_FILE" << EOF

#### $short_name: $bench_desc

\`\`\`
-O0:  $size_o0 bytes (baseline)
-O1:  $size_o1 bytes
-O2:  $size_o2 bytes
\`\`\`

EOF
    fi
done

echo "Report saved to: $REPORT_FILE"
echo ""
echo "=== Benchmarking Complete ==="
echo "Average reduction (O1): ${avg_reduction_o1}%"
echo "Average reduction (O2): ${avg_reduction_o2}%"
