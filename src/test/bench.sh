#!/bin/bash

# bench.sh — Performance microbenchmark suite for cc45 toolchain
#
# Measures compile time, link time, and binary size for a set of
# benchmark programs. Results are compared against a saved baseline.
#
# Usage:
#   make bench              Run benchmarks, compare against baseline
#   make bench-save         Run benchmarks and save as new baseline
#
# Baseline file: src/test/bench_baseline.json
# Results file:  build/test/bench_results.json

set -e

CC="./bin/cc45"
LD="./bin/ln45"
LIB="lib/build/c45.lib"
BUILDDIR="build/test/bench"
BASELINE="src/test/bench_baseline.json"
RESULTS="build/test/bench_results.json"
THRESHOLD="0.10"  # 10% deviation threshold

mkdir -p "$BUILDDIR"

# Benchmark programs: name, source file, needs_stdlib (0/1)
BENCHMARKS=(
    # Compiler stress tests
    "optimizations|src/test-resources/test_optimizations.c|1"
    "compound_types|src/test-resources/test_compound_types.c|1"
    "i8_arith|src/test-resources/test_i8_arith.c|1"
    "shift_edge|src/test-resources/test_shift_edge.c|1"
    "bool|src/test-resources/test_bool.c|1"
    "many_params|src/test-resources/test_many_params_locals.c|1"
    "variadic|src/test-resources/test_variadic.c|1"
    # Stdlib integration
    "stdlib_strlen|src/test-resources/stdlib/test_strlen.c|1"
    "stdlib_strcmp|src/test-resources/stdlib/test_strcmp.c|1"
    "stdlib_memcpy|src/test-resources/stdlib/test_memcpy.c|1"
    "stdlib_atoi|src/test-resources/stdlib/test_atoi.c|1"
    "stdlib_itoa|src/test-resources/stdlib/test_itoa.c|1"
    "stdlib_ctype|src/test-resources/stdlib/test_ctype.c|1"
    "stdlib_malloc|src/test-resources/stdlib/test_malloc.c|1"
    "stdlib_gcd_lcm|src/test-resources/stdlib/test_gcd_lcm.c|1"
)

# Time a command, return wall-clock milliseconds
time_ms() {
    local start end
    start=$(date +%s%N)
    "$@" >/dev/null 2>&1
    local rc=$?
    end=$(date +%s%N)
    echo $(( (end - start) / 1000000 ))
    return $rc
}

passed=0
failed=0
regressions=0
results="{"

echo "========================================"
echo "Performance Microbenchmarks"
echo "========================================"
echo ""

printf "%-20s %8s %8s %8s %8s\n" "Benchmark" "Compile" "Link" "ObjSize" "PrgSize"
printf "%-20s %8s %8s %8s %8s\n" "---------" "-------" "----" "-------" "-------"

for entry in "${BENCHMARKS[@]}"; do
    IFS='|' read -r name src needs_stdlib <<< "$entry"

    if [ ! -f "$src" ]; then
        printf "%-20s SKIP (not found)\n" "$name"
        continue
    fi

    o45="$BUILDDIR/${name}.o45"
    prg="$BUILDDIR/${name}.prg"

    # Compile
    compile_ms=$(time_ms $CC -c "$src" -o "$o45")
    if [ ! -f "$o45" ]; then
        printf "%-20s FAIL (compile)\n" "$name"
        failed=$((failed + 1))
        continue
    fi
    obj_size=$(stat -c%s "$o45" 2>/dev/null || stat -f%z "$o45" 2>/dev/null)

    # Link
    if [ "$needs_stdlib" = "1" ]; then
        link_ms=$(time_ms $LD -basic -o "$prg" "$o45" "$LIB")
    else
        link_ms=$(time_ms $LD -basic -o "$prg" "$o45")
    fi

    if [ ! -f "$prg" ]; then
        prg_size=0
        link_ms=0
    else
        prg_size=$(stat -c%s "$prg" 2>/dev/null || stat -f%z "$prg" 2>/dev/null)
    fi

    printf "%-20s %6dms %6dms %7d %7d\n" "$name" "$compile_ms" "$link_ms" "$obj_size" "$prg_size"

    # Append to JSON results (obj_size and prg_size are the stable metrics)
    if [ "$results" != "{" ]; then results="$results,"; fi
    results="$results\"$name\":{\"compile_ms\":$compile_ms,\"link_ms\":$link_ms,\"obj_size\":$obj_size,\"prg_size\":$prg_size}"

    passed=$((passed + 1))
done

results="$results}"

echo ""
echo "Benchmarks run: $passed passed, $failed failed"

# Write results
echo "$results" | python3 -m json.tool > "$RESULTS" 2>/dev/null || echo "$results" > "$RESULTS"

# Compare against baseline
if [ -f "$BASELINE" ]; then
    echo ""
    echo "========================================"
    echo "Baseline Comparison"
    echo "========================================"
    echo ""

    # Use python3 for JSON comparison
    python3 - "$BASELINE" "$RESULTS" "$THRESHOLD" <<'PYEOF'
import json, sys

baseline_path, results_path, threshold_str = sys.argv[1], sys.argv[2], sys.argv[3]
threshold = float(threshold_str)

with open(baseline_path) as f:
    baseline = json.load(f)
with open(results_path) as f:
    results = json.load(f)

regressions = 0
improvements = 0

# Only compare size metrics (stable); timing varies by machine load
size_metrics = ["obj_size", "prg_size"]

for name in sorted(results.keys()):
    if name not in baseline:
        print(f"  NEW:  {name} (no baseline)")
        continue

    b = baseline[name]
    r = results[name]

    for metric in size_metrics:
        if metric not in b or metric not in r:
            continue
        bv = b[metric]
        rv = r[metric]
        if bv == 0:
            continue

        pct = (rv - bv) / bv
        if pct > threshold:
            print(f"  REGRESSION: {name}.{metric}: {bv} -> {rv} (+{pct*100:.1f}%)")
            regressions += 1
        elif pct < -threshold:
            print(f"  IMPROVED:   {name}.{metric}: {bv} -> {rv} ({pct*100:.1f}%)")
            improvements += 1

for name in sorted(baseline.keys()):
    if name not in results:
        print(f"  MISSING: {name} (in baseline but not in results)")

if regressions == 0 and improvements == 0:
    print("  All metrics within threshold.")

print()
print(f"Regressions: {regressions}, Improvements: {improvements}")

if regressions > 0:
    sys.exit(1)
PYEOF

    bench_rc=$?
    if [ $bench_rc -ne 0 ]; then
        echo ""
        echo "FAIL: Size regressions detected (>${THRESHOLD} threshold)"
        exit 1
    fi
else
    echo ""
    echo "No baseline found at $BASELINE"
    echo "Run 'make bench-save' to create one."
fi

echo ""
echo "Results saved to $RESULTS"
