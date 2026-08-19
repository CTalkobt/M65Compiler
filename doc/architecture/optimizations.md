# MEGA65 C Compiler: Optimization Guide

## Overview

The MEGA65 C Compiler supports 10 optimization levels (-O0 through -O9) with fine-grained control over individual optimizations. Each optimization is categorized by its effect: **Space** (code size), **Speed** (execution time), or **Both**.

## Optimization Levels

### -O0: No Optimization (Baseline)
- No optimizations applied
- Fastest compilation time
- Largest code size
- **Use case**: Debugging, testing

### -O1: Basic Optimizations
- Constant Folding (~5% benefit)
- Dead Code Elimination (~3% benefit)
- **Total typical savings**: 5-8%
- **Use case**: Development with minor size consideration

### -O2: Standard Optimization (Default)
- Includes all -O1 optimizations, plus:
- Inline Small Functions (~2% benefit)
- Tail Call Optimization (~4% benefit)
- **Total typical savings**: 10-15%
- **Use case**: General-purpose development

### -O3: Aggressive Basic Optimizations
- Includes all -O2 optimizations, plus:
- Strength Reduction (~3% benefit)
- Algebraic Simplification (~2% benefit)
- **Total typical savings**: 15-20%
- **Use case**: Size-critical code

### -O4: Loop Optimizations
- Includes all -O3 optimizations, plus:
- Loop Unrolling (~5% benefit)
- Loop Interchange (~4% benefit)
- Loop-Invariant Code Motion (~3% benefit)
- **Total typical savings**: 25-35%
- **Use case**: Loop-heavy code

### -O5: Cross-Function Optimization
- Includes all -O4 optimizations, plus:
- Cross-Function Inlining (~3% benefit)
- Devirtualization (~2% benefit)
- **Total typical savings**: 30-40%
- **Use case**: Multi-function programs with small helper functions

### -O6: Advanced Inlining
- Includes all -O5 optimizations, plus:
- Common Subexpression Elimination (~4% benefit)
- Copy Propagation (~2% benefit)
- **Total typical savings**: 35-45%
- **Use case**: Complex expression evaluation

### -O7: Branch Optimization
- Includes all -O6 optimizations, plus:
- Branch Inversion (~2% benefit)
- Branch Folding (~3% benefit)
- Jump Optimization (~2% benefit)
- **Total typical savings**: 40-50%
- **Use case**: Branch-heavy code paths

### -O8: Memory & Register Optimization
- Includes all -O7 optimizations, plus:
- Redundant Load Elimination (~3% benefit)
- Dead Store Elimination (~2% benefit)
- Frame Pointer Optimization (~2% benefit)
- **Total typical savings**: 45-55%
- **Use case**: Memory-intensive operations

### -O9: Maximum Optimization (Aggressive)
- Includes all -O8 optimizations, plus:
- Co-Optimization (~3% benefit)
- Procedure Inlining (~5% benefit)
- Interprocedural Optimization (~4% benefit)
- **Total typical savings**: 50-65%
- **Use case**: Final release builds, performance-critical code

## Individual Optimization Flags

Control optimizations individually using -O<Name> and -PNo<Name> flags:

```bash
# Enable specific optimization (at current level)
cc45 input.c -O3 -OLoopUnrolling

# Disable specific optimization
cc45 input.c -O6 -PNoRedundantLoadElim

# Combine multiple individual controls
cc45 input.c -O4 -OConstantFolding -PNoBranchFolding
```

## Optimization Catalog

### Level 1: Basic

**Constant Folding** (-OConstantFolding)
- Evaluates constant expressions at compile time
- Reduces runtime work and code size
- Example: `a = 2 * (3 + 4)` → `a = 14`

**Dead Code Elimination** (-ODeadCodeElimination)
- Removes unreachable code and unused statements
- Requires: Flow analysis
- Benefit: ~3% code size reduction

### Level 2: Inlining & Tail Calls

**Inline Small Functions** (-OInlineSmallFunctions)
- Inlines functions < 20 bytes to eliminate call overhead
- Requires: Function size analysis
- Benefit: ~2% speedup
- Trade-off: Can increase code size for large programs

**Tail Call Optimization** (-OTailCallOptimization)
- Converts `JSR + RTS` (tail calls) to `JMP`
- Saves 1-2 bytes per tail call
- Benefit: ~4% code size and speed improvement
- No stack frame creation

### Level 3: Strength & Algebra

**Strength Reduction** (-OStrengthReduction)
- Replaces multiply/divide by powers of 2 with bit shifts
- Example: `a * 4` → `a << 2`, `a / 8` → `a >> 3`
- Benefit: ~3% speedup

**Algebraic Simplification** (-OAlgebraicSimplify)
- Eliminates identity and annihilator patterns
- Examples: `a * 1 = a`, `a * 0 = 0`, `a + 0 = a`
- Benefit: ~2% code reduction

### Level 4: Loop Optimizations

**Loop Unrolling** (-OLoopUnrolling)
- Unrolls small loops (20-1000 iterations) to reduce branch overhead
- Dynamic unroll factors (2-8x based on characteristics)
- Benefit: ~5% speedup for loop-heavy code
- Trade-off: Code size increase

**Loop Interchange** (-OLoopInterchange)
- Reorders nested loops for better cache locality
- Requires: Loop dependency analysis
- Benefit: ~4% speedup for nested loops

**Loop-Invariant Code Motion** (-OLICM)
- Hoists loop-invariant computations outside loops
- Example: Loop-invariant multiplications
- Benefit: ~3% speedup

### Level 5: Cross-Function

**Cross-Function Inlining** (-OCrossFunctionInlining)
- Inlines functions with 1-3 call sites (Phase 86-87 analysis)
- Eliminates call overhead for functions called from specific locations
- Requires: Call graph analysis
- Benefit: ~3% code size reduction

**Devirtualization** (-ODevirtualization)
- Replaces single-implementation virtual calls with direct calls
- Detects virtual methods with only one known implementation
- Benefit: ~2% for OOP code

### Level 6: Redundancy Elimination

**Common Subexpression Elimination** (-OCSE)
- Eliminates redundant subexpression computations
- Example: `a = b + c; d = b + c;` → `a = b + c; d = a;`
- Requires: Data flow analysis
- Benefit: ~4% speedup

**Copy Propagation** (-OCopyPropagation)
- Replaces copies with original values to reduce moves
- Reduces unnecessary register moves
- Benefit: ~2% code reduction

### Level 7: Branch Optimization

**Branch Inversion** (-OBranchInversion)
- Inverts branch conditions to eliminate jumps
- Pattern: `BEQ skip / BRA target / skip:` → `BNE target`
- Benefit: ~2% code size reduction

**Branch Folding** (-OBranchFolding)
- Eliminates unreachable code via conditional branches
- Requires: Flow analysis
- Benefit: ~3% code reduction

**Jump Optimization** (-OJumpOptimization)
- Converts `JMP` to `BRA` for backward branches
- Eliminates no-op jump instructions
- Benefit: ~2% code size reduction

### Level 8: Memory & Register

**Redundant Load Elimination** (-ORedundantLoadElim)
- Eliminates redundant memory loads via reverse store-forwarding
- Tracks memory state to detect identical loads
- Requires: Data flow analysis
- Benefit: ~3% speedup

**Dead Store Elimination** (-ODeadStoreElim)
- Eliminates stores whose values are never used
- Requires: Live variable analysis
- Benefit: ~2% code reduction

**Frame Pointer Optimization** (-OFramePointerOpt)
- Lazy frame pointer initialization (SAC mode)
- Only initializes FP when needed for variable access
- Requires: Frame analysis
- Benefit: ~2% code reduction

### Level 9: Interprocedural

**Co-Optimization** (-OCoOptimization)
- Coordinated optimization of related function groups
- Applies batch strategies to functions that call each other
- Requires: Call graph analysis and group identification
- Benefit: ~3% code reduction

**Procedure Inlining** (-OProcedureInlining)
- Aggressive inlining of functions including recursive calls
- Larger inline thresholds and recursive handling
- Benefit: ~5% speedup (with code size trade-off)

**Interprocedural Optimization** (-OInterproceduralOpt)
- Cross-module optimization hints and analysis
- Prepares data for link-time optimization
- Requires: Extra compilation passes
- Benefit: ~4% overall for multi-file projects

## Function-Level Pragma Control

Override optimization settings for individual functions:

```c
// Disable all optimizations for this function (debugging)
#pragma cc45 optimize(none)
void debug_function() { }

// Speed-focused optimization (only speed optimizations)
#pragma cc45 optimize(speed)
void fast_path() { }

// Space-focused optimization (only space optimizations)
#pragma cc45 optimize(space)
void compressed_code() { }

// Back to default
#pragma cc45 optimize(default)
void normal_function() { }
```

## Command-Line Examples

```bash
# Use level 2 (default)
cc45 input.c

# Maximum optimization
cc45 input.c -O9

# Level 5 with specific tweaks
cc45 input.c -O5 -OLoopUnrolling -PNoRedundantLoadElim

# Start at 0, enable only specific optimizations
cc45 input.c -O0 -OConstantFolding -OTailCallOptimization

# Speed-focused (enable all, but disable space optimizations)
cc45 input.c -O8 -PNoDeadStoreElim -PNoBranchInversion

# Space-focused (enable size optimizations only)
cc45 input.c -O9 -PNoProcedureInlining -PNoLoopUnrolling
```

## Configuration File Support

Set defaults in `~/.config/m65/cc45.conf`:

```ini
# Default optimization level
-O3

# Disable specific optimizations globally
-PNoLoopUnrolling
-PNoProcedureInlining

# Enable specific ones
-OConstantFolding
```

## Performance Trade-offs

### Code Size vs Speed

| Optimization | Space | Speed |
|---|---|---|
| Constant Folding | - | - |
| Dead Code Elim | ✓ | - |
| Inlining | ✗ | ✓ |
| Loop Unrolling | ✗ | ✓ |
| CSE | - | ✓ |
| Dead Store Elim | ✓ | - |
| Branch Inversion | ✓ | - |
| Devirtualization | ✓ | ✓ |

### Compilation Time Impact

- -O0 to -O3: Minimal impact (< 10% slower)
- -O4 to -O6: Moderate impact (10-30% slower)
- -O7 to -O9: Significant impact (30-100% slower)

Use lower levels for rapid iteration, higher levels for release builds.

## Best Practices

1. **Development**: Use -O2 (default) for balance
2. **Debugging**: Use -O0 to disable optimizations
3. **Testing**: Use -O3 to catch edge cases
4. **Release**: Use -O8 or -O9 for size/speed
5. **Critical Paths**: Use pragmas to selectively optimize
6. **Large Projects**: Start with -O6, tune with individual flags

## Troubleshooting

### "Optimization X is not working"
- Check if enabled at current level: `cc45 --help-optimizations`
- Verify not explicitly disabled with -PNo flag
- Use function pragma to override global settings

### Code size increased with -O9
- Some optimizations trade size for speed
- Use -O9 -PNoProcedureInlining for size-focused
- Try -O7 or -O8 instead

### Compilation too slow
- Reduce optimization level
- Disable expensive passes: -PNoCSE, -PNoLICM
- Use -O4 or -O5 for reasonable balance

## Reporting Issues

When reporting optimization-related bugs, always include:
- Optimization level (-O<n>)
- Individual optimization flags used
- Input source code
- Expected vs actual output
