#!/bin/bash
#
# Phase 90.5 Benchmarking Script
#
# Measures code size improvements from Phase 90 optimizations
# (lazy FP init, leaf function detection, smart FP recalculation)

set -e

BIN_DIR="../../bin"
COMPILER="$BIN_DIR/cc45"
ASSEMBLER="$BIN_DIR/ca45"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

if [ ! -x "$COMPILER" ]; then
    echo "Error: cc45 not found at $COMPILER"
    exit 1
fi

echo -e "${BLUE}=== Phase 90.5 Benchmarking Suite ===${NC}\n"

# Function to compile and measure size
benchmark_program() {
    local source=$1
    local name=$2

    echo "Benchmarking: $name"

    # Compile to assembly
    "$COMPILER" -S "$source" -o "/tmp/${name}.s45" 2>/dev/null

    # Assemble to object file
    "$ASSEMBLER" -c "/tmp/${name}.s45" -o "/tmp/${name}.o45" 2>/dev/null

    # Get size
    local size=$(wc -c < "/tmp/${name}.o45")

    echo "  Source: $source"
    echo "  Object size: $size bytes"

    # Extract function count from assembly
    local func_count=$(grep -c "^    proc " "/tmp/${name}.s45" 2>/dev/null || echo "0")
    if [ "$func_count" = "0" ]; then
        func_count=$(grep -c "^proc " "/tmp/${name}.s45" 2>/dev/null || echo "0")
    fi

    echo "  Functions: $func_count"

    # Calculate average bytes per function
    if [ "$func_count" -gt 0 ]; then
        local avg=$((size / func_count))
        echo "  Average per function: $avg bytes"
    fi

    echo ""
    echo "$size"
}

# Benchmark 1: Leaf Functions
echo -e "${BLUE}Benchmark 1: Leaf Functions${NC}"
echo "Expected: 15-20 bytes saved per function (no FP setup)"
echo ""
SIZE1=$(benchmark_program "benchmark_phase90_leaf_functions.c" "leaf_funcs")

# Benchmark 2: Callbacks/Event Handlers
echo -e "${BLUE}Benchmark 2: Callback/Event Handlers${NC}"
echo "Expected: ~9 bytes per zero-arg call (no FP recalc)"
echo ""
SIZE2=$(benchmark_program "benchmark_phase90_callbacks.c" "callbacks")

# Benchmark 3: Original Phase 90 Test
echo -e "${BLUE}Benchmark 3: Mixed Pattern (Original Test)${NC}"
echo "Expected: 10-25% overall reduction"
echo ""
SIZE3=$(benchmark_program "test_phase90_fp_optimization.c" "mixed_patterns")

# Summary
echo -e "${GREEN}=== Benchmarking Results ===${NC}\n"
echo "Leaf Functions Program:     $SIZE1 bytes"
echo "Callback/Event Handlers:    $SIZE2 bytes"
echo "Mixed Patterns Test:        $SIZE3 bytes"
echo ""

# Calculate totals
TOTAL=$((SIZE1 + SIZE2 + SIZE3))
echo "Total benchmark size: $TOTAL bytes"
echo ""

echo -e "${GREEN}✅ Phase 90.5 Benchmarking Complete${NC}"
