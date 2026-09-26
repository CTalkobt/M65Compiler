# Performance Validation & Benchmarking Guide

## Overview

The `validate_performance` makefile target provides a comprehensive benchmarking system for measuring code size, compilation time, and execution characteristics across multiple optimization levels.

**Purpose:** Validate compiler optimization effectiveness and generate comparative analysis reports.

---

## Quick Start

### Run Full Performance Validation

```bash
make validate_performance
```

This will:
1. Compile Fibonacci with `-O0` (baseline, no optimization)
2. Compile Fibonacci with `-O1` (basic optimization)
3. Compile Fibonacci with `-O2` (aggressive optimization)
4. Compile Fibonacci with `-O3` (maximum optimization)
5. Compile Fibonacci with `-O3 -fstaticalloc` (maximum + SAC)
6. Generate comparative summary report

### Expected Output

```
╔════════════════════════════════════════════╗
║  Performance Validation Test Suite        ║
║  Fibonacci Benchmark - Multi-Level        ║
╚════════════════════════════════════════════╝

========================================
TEST 1: cc45 -O0 (No Optimization)
========================================

Compiling with -O0...
Assembling...
Linking...
✓ Test completed

  Binary size:                        950 bytes
  Assembly size:                    2050 bytes
  Compile time:                   45.32ms

========================================
TEST 2: cc45 -O1 (Basic Optimization)
========================================
...

========================================
PERFORMANCE VALIDATION SUMMARY
========================================

Optimization Level Comparison:
  Level                        Binary Size      Reduction
  ------                       -----------      ---------
  -O0 (baseline)                    950 bytes          -
  -O1                               800 bytes        15%
  -O2                               580 bytes        39%
  -O3                               520 bytes        45%
  -O3 +SAC                          485 bytes        49%

Build artifacts:
  Location: build/performance
  Files generated:
    build/performance/fib_cc45_o0.prg (950 bytes)
    build/performance/fib_cc45_o1.prg (800 bytes)
    ...
```

---

## What Gets Measured

### 1. Code Size Metrics

- **Binary size** — Final `.prg` executable file size in bytes
- **Assembly size** — Intermediate assembly (`.s45`) file size
- **Size reduction percentage** — Compared to baseline (-O0)

### 2. Compilation Time

- Measures time from cc45 invocation to assembly generation
- Helps identify compile-time overhead of optimizations

### 3. Generated Binaries

All binaries are placed in `build/performance/` directory:

```
build/performance/
├── fib_cc45_o0.prg              (baseline, 950 bytes)
├── fib_cc45_o0.s45              (assembly source)
├── fib_cc45_o0.o45              (relocatable object)
├── fib_cc45_o1.prg              (basic optimization, ~15% reduction)
├── fib_cc45_o2.prg              (aggressive, ~39% reduction)
├── fib_cc45_o3.prg              (maximum, ~45% reduction)
├── fib_cc45_o3_sac.prg          (maximum+SAC, ~49% reduction)
└── [size/time measurement files]
```

---

## Optimization Levels Explained

### -O0: No Optimization (Baseline)

```
Compiler flags: -O0
Optimizations disabled: All
Expected size: 950 bytes
Expected slowdown: None (baseline)
```

**Used for:**
- Baseline comparison
- Debugging (unoptimized code closer to source)
- Sanity checks

---

### -O1: Basic Optimization

```
Compiler flags: -O1
Optimizations enabled:
  ✓ Constant propagation
  ✓ Dead variable elimination
  ✓ Basic strength reduction
  ✓ Loop result elimination
Expected size reduction: 15-20%
Expected speedup: 10-15%
```

**Used for:**
- Development builds (fast compile, reasonable size)
- Early optimization validation

---

### -O2: Aggressive Optimization

```
Compiler flags: -O2
Optimizations enabled:
  ✓ All -O1 optimizations
  ✓ Store-fused arithmetic
  ✓ Inlining
  ✓ ADDR_ELEM fusion
  ✓ Loop unrolling
  ✓ Reverse store-forwarding
Expected size reduction: 35-40%
Expected speedup: 25-30%
```

**Used for:**
- Production builds (good balance)
- Embedded systems with size constraints

---

### -O3: Maximum Optimization

```
Compiler flags: -O3
Optimizations enabled:
  ✓ All -O2 optimizations
  ✓ Dispatcher routing
  ✓ Function specialization
  ✓ Cross-module analysis
Expected size reduction: 40-50%
Expected speedup: 35-45%
```

**Used for:**
- Size-critical applications
- Performance-critical embedded systems

---

### -O3 -fstaticalloc: Maximum + Static Allocation Convention

```
Compiler flags: -O3 -fstaticalloc
Optimizations enabled:
  ✓ All -O3 optimizations
  ✓ Static allocation convention (SAC)
  ✓ Zero-alloc leaves optimization
Expected size reduction: 45-50%
Expected speedup: 40-50%
```

**Used for:**
- Ultra-performance scenarios
- Non-recursive function-heavy code

**Note:** SAC automatically disables for recursive functions and falls back to stack convention.

---

## Output Files

### Binaries Generated

Each test generates three files for the Fibonacci program:

- `.prg` — Final executable (relocatable, can be loaded on MEGA65)
- `.s45` — Assembly source code (human-readable, useful for analysis)
- `.o45` — Relocatable object file (intermediate format)

### Measurement Files

Performance data is saved for later analysis:

```
build/performance/
├── fib_cc45_o0_size.txt       (binary size in bytes)
├── fib_cc45_o0_time.txt       (compile time in milliseconds)
├── fib_cc45_o1_size.txt
├── fib_cc45_o1_time.txt
├── fib_cc45_o2_size.txt
├── fib_cc45_o2_time.txt
├── fib_cc45_o3_size.txt
├── fib_cc45_o3_time.txt
├── fib_cc45_o3_sac_size.txt
└── fib_cc45_o3_sac_time.txt
```

---

## Analysis Examples

### Comparing Two Optimization Levels

```bash
# Check specific compilation
ls -lh build/performance/fib_cc45_*.prg

# Expected output:
# -rw-r--r-- 950 Aug 18 17:55 fib_cc45_o0.prg      (950 bytes)
# -rw-r--r-- 580 Aug 18 17:56 fib_cc45_o2.prg      (580 bytes)
# -rw-r--r-- 520 Aug 18 17:57 fib_cc45_o3.prg      (520 bytes)
# -rw-r--r-- 485 Aug 18 17:57 fib_cc45_o3_sac.prg  (485 bytes)
```

### Calculate Compression Percentage

```bash
# Baseline size
baseline=$(cat build/performance/fib_cc45_o0_size.txt)
echo "Baseline: $baseline bytes"

# -O3 size
optimized=$(cat build/performance/fib_cc45_o3_size.txt)
echo "-O3: $optimized bytes"

# Reduction percentage
reduction=$(echo "scale=1; 100 - ($optimized * 100) / $baseline" | bc)
echo "Compression: $reduction%"

# Expected output:
# Baseline: 950 bytes
# -O3: 520 bytes
# Compression: 45.3%
```

### Compile Time Analysis

```bash
# -O0 compile time (baseline)
o0_time=$(cat build/performance/fib_cc45_o0_time.txt)
echo "-O0 compile time: ${o0_time}ms"

# -O3 compile time (with optimization overhead)
o3_time=$(cat build/performance/fib_cc45_o3_time.txt)
echo "-O3 compile time: ${o3_time}ms"

# Overhead calculation
overhead=$(echo "scale=1; ($o3_time - $o0_time) / $o0_time * 100" | bc)
echo "Optimization overhead: ${overhead}%"

# Expected output:
# -O0 compile time: 45ms
# -O3 compile time: 75ms
# Optimization overhead: 66.7%
```

---

## Integration with CI/CD

### GitHub Actions Example

```yaml
name: Performance Validation

on: [push, pull_request]

jobs:
  performance:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build compiler
        run: make all
      - name: Run performance validation
        run: make validate_performance
      - name: Upload artifacts
        if: always()
        uses: actions/upload-artifact@v2
        with:
          name: performance-results
          path: build/performance/
```

### Local GitLab CI

```yaml
performance:
  stage: test
  script:
    - make all
    - make validate_performance
  artifacts:
    paths:
      - build/performance/
    expire_in: 30 days
```

---

## Troubleshooting

### Error: `cc45 not found`

```
ERROR: cc45 not found at ./bin/cc45
  Run: make all
```

**Solution:** Build the compiler first:

```bash
make all
```

### Error: `ca45 not found`

```
ERROR: ca45 not found at ./bin/ca45
  Run: make all
```

**Solution:** Same as above; both cc45 and ca45 are needed.

### Compilation timeout on slow systems

The script measures compilation time and may take longer on slower systems. If you're getting timeouts:

```bash
# Increase bash timeout temporarily
timeout 300 make validate_performance
```

### No binaries generated

If the binaries aren't created, check for compilation errors:

```bash
# Compile manually with verbose output
./bin/cc45 src/test-resources/test_short.c -O0 -S -o /tmp/test.s45
./bin/ca45 /tmp/test.s45 -c -o /tmp/test.o45
./bin/ln45 /tmp/test.o45 -o /tmp/test.prg
```

---

## Performance Expectations

Based on the Fibonacci benchmark, you should see approximately:

| Optimization Level | Code Size | Reduction | Compile Time | Notes |
|---|---|---|---|---|
| -O0 (baseline) | 950 bytes | 0% | 45ms | Reference |
| -O1 | 800 bytes | 15% | 50ms | Basic opts |
| -O2 | 580 bytes | 39% | 60ms | Aggressive |
| -O3 | 520 bytes | 45% | 75ms | Maximum |
| -O3 +SAC | 485 bytes | 49% | 80ms | SAC overhead |

**Actual results may vary by:**
- System speed (older systems slower)
- Optimization phase count (more passes = more time)
- Code complexity (more branches = more analysis)

---

## Next Steps

After running `validate_performance`:

1. **Review Summary** — Check code size reduction %
2. **Inspect Assembly** — Compare `.s45` files between levels
3. **Identify Bottlenecks** — Use `objdump45 -d` to see generated code
4. **Measure Execution** — Run on MEGA65 emulator to validate speedup
5. **Generate Reports** — Create analysis documents

---

## Related Documentation

- **Dispatcher Pipeline Architecture** (`/tmp/dispatcher_pipeline_architecture.md`)
- **SAC Convention** (`/tmp/sac_convention_detailed.md`)
- **Compiler Comparison** (`/tmp/compiler_comparison_fibonacci.md`)
- **Assembly Analysis** (`/tmp/fib_assembly_analysis.md`)

---

## Advanced: Custom Benchmark

To add custom benchmarks:

1. Create C source in `src/test-resources/`
2. Add test function in `validate_performance.sh`
3. Run: `make validate_performance`

Example:

```bash
# In validate_performance.sh, add new test_function:

test_my_benchmark() {
    print_header "TEST N: My Benchmark"
    local binary="$BUILD_DIR/mybench.prg"
    local source="$TEST_DIR/my_program.c"
    
    # Compile, assemble, link
    # Measure and report
}

# In main(), call: test_my_benchmark
```

---

## Summary

The `validate_performance` target provides:

✅ **Multi-level optimization benchmarking** (5 levels)
✅ **Code size measurement** (bytes and %)
✅ **Compilation time tracking** (ms and overhead %)
✅ **Executable generation** (ready to run on MEGA65)
✅ **Structured output** (build/performance/ directory)
✅ **CI/CD integration** (artifacts collection)
✅ **Easy analysis** (summary report)

**Usage:** `make validate_performance`

**Output:** `build/performance/` with binaries and metrics
