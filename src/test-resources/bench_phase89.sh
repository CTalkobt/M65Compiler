#!/bin/bash

# Phase 89 Benchmarking Script
# Measures code size improvements from Address Template Optimization

set -e

COMPILER="../../bin/cc45"
ASSEMBLER="../../bin/ca45"
RESULTS_DIR="phase89_results"

mkdir -p "$RESULTS_DIR"

echo "═══════════════════════════════════════════════════════════════════"
echo "  Phase 89: Address Template Optimization - Benchmarking"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

# Test programs
declare -a TESTS=(
    "test_phase89_simple.c:Basic address patterns"
    "benchmark_address_text.c:Text screen addressing"
    "benchmark_address_sprite.c:Sprite/table addressing"
)

echo "Compiling test programs..."
echo ""

total_o0=0
total_o1=0
total_o2=0
count=0

for test_info in "${TESTS[@]}"; do
    IFS=: read -r test_file test_desc <<< "$test_info"

    if [ ! -f "$test_file" ]; then
        echo "⚠️  Skipping $test_file (not found)"
        continue
    fi

    echo "Testing: $test_desc"
    echo "  File: $test_file"

    # Compile with different optimization levels
    for opt_level in "-O0" "-O1" "-O2"; do
        output="${RESULTS_DIR}/$(basename ${test_file%.c})${opt_level//./}"
        asm_file="${output}.s45"
        obj_file="${output}.o45"

        echo "  Compiling with $opt_level..."
        $COMPILER $opt_level "$test_file" -S -o "$asm_file" 2>/dev/null

        $ASSEMBLER -c "$asm_file" -o "$obj_file" 2>/dev/null

        # Get file size
        size=$(ls -l "$obj_file" 2>/dev/null | awk '{print $5}' || echo "0")

        # Check for Phase 89 optimization markers
        templates=$(grep -c "Phase 89: Address Template" "$asm_file" 2>/dev/null || echo "0")

        echo "    $opt_level: $size bytes ($templates templates)"

        # Store sizes
        if [ "$opt_level" = "-O0" ]; then
            total_o0=$((total_o0 + size))
        elif [ "$opt_level" = "-O1" ]; then
            total_o1=$((total_o1 + size))
        else
            total_o2=$((total_o2 + size))
        fi
    done

    count=$((count + 1))
    echo ""
done

echo "═══════════════════════════════════════════════════════════════════"
echo "  SUMMARY"
echo "═══════════════════════════════════════════════════════════════════"
echo ""

if [ $count -gt 0 ]; then
    avg_o0=$((total_o0 / count))
    avg_o1=$((total_o1 / count))
    avg_o2=$((total_o2 / count))

    reduction_o1=$((100 * (avg_o0 - avg_o1) / avg_o0))
    reduction_o2=$((100 * (avg_o0 - avg_o2) / avg_o0))

    echo "Average Code Sizes:"
    echo "  -O0: $avg_o0 bytes (baseline)"
    echo "  -O1: $avg_o1 bytes ($reduction_o1% reduction)"
    echo "  -O2: $avg_o2 bytes ($reduction_o2% reduction)"
    echo ""
    echo "Test programs compiled: $count"
    echo "Results directory: $RESULTS_DIR"
else
    echo "No tests compiled successfully"
fi

echo ""
echo "═══════════════════════════════════════════════════════════════════"
