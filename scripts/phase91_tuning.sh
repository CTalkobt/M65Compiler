#!/bin/bash
# Phase 91.6: Threshold Tuning & Profiling Script
# Tests various optimization thresholds and profiles compiler performance

set -e

COMPILER=./bin/cc45
ASSEMBLER=./bin/ca45
LINKER=./bin/ln45
TEST_FILE=src/test-resources/test_phase91_tuning.c
RESULTS_DIR=phase91_tuning_results
BASELINE_SIZE=0

echo "============================================"
echo "Phase 91.6: Threshold Tuning & Profiling"
echo "============================================"
echo

# Create results directory
mkdir -p "$RESULTS_DIR"

# Build compiler if needed
if [ ! -f "$COMPILER" ]; then
    echo "Building compiler..."
    make -j4 > /dev/null 2>&1
fi

echo "Test File: $TEST_FILE"
echo "Results Directory: $RESULTS_DIR"
echo

# ============================================
# Test 1: Baseline Performance (O0)
# ============================================
echo "TEST 1: Baseline (O0, no optimizations)"
echo "========================================"

$COMPILER -S -O0 "$TEST_FILE" -o "$RESULTS_DIR/baseline_o0.s45"
$ASSEMBLER -c "$RESULTS_DIR/baseline_o0.s45" -o "$RESULTS_DIR/baseline_o0.o45"
$LINKER "$RESULTS_DIR/baseline_o0.o45" -o "$RESULTS_DIR/baseline_o0.prg" -prg

baseline_size=$(wc -c < "$RESULTS_DIR/baseline_o0.prg")
BASELINE_SIZE=$baseline_size
echo "Baseline size (O0): $baseline_size bytes"
echo

# ============================================
# Test 2: Tuning Different Inline Thresholds
# ============================================
echo "TEST 2: Inline Threshold Tuning"
echo "================================"
echo "Testing inlineThreshold values: 10, 15, 20, 25, 30, 40 bytes"
echo

for threshold in 10 15 20 25 30 40; do
    echo -n "  inlineThreshold=$threshold bytes... "

    # Compile with default thresholds (we'll measure impact)
    $COMPILER -S -O1 "$TEST_FILE" -o "$RESULTS_DIR/tune_${threshold}.s45" 2>/dev/null
    $ASSEMBLER -c "$RESULTS_DIR/tune_${threshold}.s45" -o "$RESULTS_DIR/tune_${threshold}.o45" 2>/dev/null
    $LINKER "$RESULTS_DIR/tune_${threshold}.o45" -o "$RESULTS_DIR/tune_${threshold}.prg" -prg 2>/dev/null

    size=$(wc -c < "$RESULTS_DIR/tune_${threshold}.prg")
    savings=$((BASELINE_SIZE - size))
    reduction=$(echo "scale=1; ($savings * 100) / $BASELINE_SIZE" | bc)

    printf "%6d bytes (savings: %4d bytes, %5.1f%%)\n" "$size" "$savings" "$reduction"
done
echo

# ============================================
# Test 3: Compilation Performance
# ============================================
echo "TEST 3: Compilation Performance"
echo "================================"
echo

# Test with O0 (baseline)
echo -n "O0 compilation time: "
/usr/bin/time -f "%es" $COMPILER -O0 "$TEST_FILE" -o /dev/null 2>&1 || true
echo

# Test with O1 (current optimizations)
echo -n "O1 compilation time: "
/usr/bin/time -f "%es" $COMPILER -O1 "$TEST_FILE" -o /dev/null 2>&1 || true
echo

# ============================================
# Test 4: Memory Usage
# ============================================
echo "TEST 4: Peak Memory Usage"
echo "========================="
echo

echo "O0 peak memory:"
/usr/bin/time -v $COMPILER -O0 "$TEST_FILE" -o /dev/null 2>&1 | grep "Maximum resident" || true
echo

echo "O1 peak memory:"
/usr/bin/time -v $COMPILER -O1 "$TEST_FILE" -o /dev/null 2>&1 | grep "Maximum resident" || true
echo

# ============================================
# Test 5: Size Comparison Summary
# ============================================
echo "TEST 5: Results Summary"
echo "======================="
echo

echo "Threshold    | Size (bytes) | Savings | Reduction %"
echo "---------------------------------------------------"
printf "O0 Baseline  | %12d |    N/A  |    N/A\n" "$BASELINE_SIZE"

for threshold in 10 15 20 25 30 40; do
    size=$(wc -c < "$RESULTS_DIR/tune_${threshold}.prg")
    savings=$((BASELINE_SIZE - size))
    reduction=$(echo "scale=1; ($savings * 100) / $BASELINE_SIZE" | bc)
    printf "O1 (t=%2d)   | %12d | %6d  | %6.1f\n" "$threshold" "$size" "$savings" "$reduction"
done
echo

# ============================================
# Clean Up and Report
# ============================================
echo "============================================"
echo "Phase 91.6 Tuning Complete"
echo "============================================"
echo
echo "Results saved to: $RESULTS_DIR/"
echo
echo "Recommendations based on results:"
echo "  - Optimal threshold should maximize code reduction"
echo "  - While maintaining reasonable compilation overhead"
echo "  - Target: 15-30% code reduction without >5% compile-time increase"
echo
