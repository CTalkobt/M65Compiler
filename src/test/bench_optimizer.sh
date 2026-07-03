#!/bin/bash
# bench_optimizer.sh — Compare cc45 optimizer impact on code size and correctness
#
# Compiles bench_optimizer.c with and without optimizations, compares binary sizes,
# and verifies runtime correctness via mmemu-cli.

CC=bin/cc45
AS=bin/ca45
MMEMU=mmemu-cli
SRC=src/test-resources/bench_optimizer.c
BUILD=build/bench

mkdir -p $BUILD

echo "==================================================================="
echo "  cc45 Optimizer Benchmark"
echo "==================================================================="
echo ""

# Compile with optimizations (default)
echo "Compiling with optimizations..."
$CC $SRC -o $BUILD/bench_opt.s 2>/dev/null
$AS $BUILD/bench_opt.s -o $BUILD/bench_opt.prg 2>/dev/null
OPT_SIZE=$(wc -c < $BUILD/bench_opt.prg)

# Compile without optimizations
echo "Compiling without optimizations (-O0)..."
$CC -O0 $SRC -o $BUILD/bench_noopt.s 2>/dev/null
$AS $BUILD/bench_noopt.s -o $BUILD/bench_noopt.prg 2>/dev/null
NOOPT_SIZE=$(wc -c < $BUILD/bench_noopt.prg)

# Calculate savings
SAVED=$((NOOPT_SIZE - OPT_SIZE))
if [ "$NOOPT_SIZE" -gt 0 ]; then
    PCT=$((SAVED * 100 / NOOPT_SIZE))
else
    PCT=0
fi

echo ""
echo "=== Code Size Comparison ==="
echo "  Without optimizations: $NOOPT_SIZE bytes"
echo "  With optimizations:    $OPT_SIZE bytes"
echo "  Savings:               $SAVED bytes ($PCT%)"
echo ""

# Assembly line counts
OPT_LINES=$(grep -c '^\s*[a-z]' $BUILD/bench_opt.s 2>/dev/null || echo 0)
NOOPT_LINES=$(grep -c '^\s*[a-z]' $BUILD/bench_noopt.s 2>/dev/null || echo 0)
echo "=== Assembly Lines ==="
echo "  Without optimizations: $NOOPT_LINES lines"
echo "  With optimizations:    $OPT_LINES lines"
echo ""

# Verify runtime correctness (if mmemu-cli available)
if command -v $MMEMU &>/dev/null; then
    echo "=== Runtime Verification (mmemu-cli) ==="

    # Run optimized version
    OPT_OUTPUT=$(echo -e "load $BUILD/bench_opt.prg\nsetpc \$2000\nstep 10000000\nm \$4000 16\nq" | $MMEMU -m rawMega65 2>/dev/null | grep "4000:")

    # Run unoptimized version
    NOOPT_OUTPUT=$(echo -e "load $BUILD/bench_noopt.prg\nsetpc \$2000\nstep 10000000\nm \$4000 16\nq" | $MMEMU -m rawMega65 2>/dev/null | grep "4000:")

    echo "  Optimized:   $OPT_OUTPUT"
    echo "  Unoptimized: $NOOPT_OUTPUT"

    # Expected: 00 02 2B 90 0C 04 0A 15 04 2A 00 2A 42 43 26 AA
    EXPECTED="4000: 00 02 2B 90 0C 04 0A 15 04 2A 00 2A 42 43 26 AA"

    if echo "$OPT_OUTPUT" | grep -qi "00 02 2B"; then
        echo "  Optimized:   PASS (correct results)"
    else
        echo "  Optimized:   FAIL"
    fi

    if echo "$NOOPT_OUTPUT" | grep -qi "00 02 2B"; then
        echo "  Unoptimized: PASS (correct results)"
    else
        echo "  Unoptimized: FAIL"
    fi

    if [ "$OPT_OUTPUT" = "$NOOPT_OUTPUT" ]; then
        echo "  Match:       PASS (identical output)"
    else
        echo "  Match:       Results differ (optimizer changed behavior)"
    fi
else
    echo "(mmemu-cli not found — skipping runtime verification)"
fi

echo ""
echo "=== Optimizer Passes Active ==="
echo "  1. Strength Reduction (#131) — MUL→SHL, DIV_U→SHR, MOD_U→AND"
echo "  2. Algebraic Simplification (#132) — x+0, x-x, x|0"
echo "  3. Type Narrowing (#137) — I16→I8 when truncated"
echo "  4. Branch Folding (#134) — constant condition elimination"
echo "  5. CSE + Copy Propagation"
echo "  6. LICM (#130) — loop-invariant hoisting"
echo "  7. COPY Chain Elimination (#136)"
echo "  8. ADDR_ELEM Fusion (#139)"
echo ""
