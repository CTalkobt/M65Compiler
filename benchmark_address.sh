#!/bin/bash
# Phase 89.4.4: Address Template Optimization Benchmarking
# Measure code size reduction for address calculation patterns

set -e

COMPILER="./bin/cc45"
TEST_DIR="src/test-resources"

echo "=== Phase 89.4.4: Address Template Benchmarking ==="
echo ""

if [ ! -f "$COMPILER" ]; then
    echo "Error: cc45 compiler not found. Build with: make all"
    exit 1
fi

# Test 1: Text Screen Addressing (row * 40 + col)
echo "Test 1: Text Screen Addressing (benchmark_address_text.c)"
echo "=================================================="

$COMPILER $TEST_DIR/benchmark_address_text.c -S -o /tmp/text_opt.s45 2>/dev/null
LINES_OPT=$(grep -c "row.*40\|40.*row" /tmp/text_opt.s45 || echo 0)
BYTES_OPT=$(wc -c < /tmp/text_opt.s45)

# Generate naive version (without optimization)
$COMPILER $TEST_DIR/benchmark_address_text.c -S -o /tmp/text_naive.s45 -fno-address-templates 2>/dev/null || \
  $COMPILER $TEST_DIR/benchmark_address_text.c -S -o /tmp/text_naive.s45 2>/dev/null

BYTES_NAIVE=$(wc -c < /tmp/text_naive.s45)
REDUCTION=$((($BYTES_NAIVE - $BYTES_OPT) * 100 / $BYTES_NAIVE))

echo "Optimized: $BYTES_OPT bytes"
echo "Naive:     $BYTES_NAIVE bytes"
echo "Reduction: $REDUCTION%"
echo ""

# Test 2: Sprite Offset Addressing (base + index * size)
echo "Test 2: Sprite Offset Addressing (benchmark_address_sprite.c)"
echo "================================================================"

$COMPILER $TEST_DIR/benchmark_address_sprite.c -S -o /tmp/sprite_opt.s45 2>/dev/null
BYTES_OPT=$(wc -c < /tmp/sprite_opt.s45)

$COMPILER $TEST_DIR/benchmark_address_sprite.c -S -o /tmp/sprite_naive.s45 -fno-address-templates 2>/dev/null || \
  $COMPILER $TEST_DIR/benchmark_address_sprite.c -S -o /tmp/sprite_naive.s45 2>/dev/null

BYTES_NAIVE=$(wc -c < /tmp/sprite_naive.s45)
REDUCTION=$((($BYTES_NAIVE - $BYTES_OPT) * 100 / $BYTES_NAIVE))

echo "Optimized: $BYTES_OPT bytes"
echo "Naive:     $BYTES_NAIVE bytes"
echo "Reduction: $REDUCTION%"
echo ""

echo "=== Benchmarking Complete ==="
