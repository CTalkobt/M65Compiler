# PHASE 2 Initial Investigation Findings

**Date**: 2026-07-19  
**Status**: Investigation in progress

## Summary

Initial examination of generated assembly for test_short.c and minimal test case reveals variable offset assignments that may not be the direct cause of the failures, but does show potential frame allocation inconsistencies.

## Key Observations

### Test Case: test_phase2_minimal.c

```c
int test_offset() {
    int a = 10;        // Should be at FP+0
    int b = helper(a); // Should be at FP+2
    return b + a;      // Access both variables
}
```

**Generated Assembly Issue**:
```asm
.local @_l_a = 0    ; a at FP+0 (correct)
.local @_l_b = 2    ; b at FP+2 (correct)
...
ldax.fp 2           ; Load b (correct)
add.16 .AX, #10     ; Add IMMEDIATE 10, not 'a'!
```

**Finding**: The return statement `return b + a` is being compiled as `b + 10` instead of `b + a`. The variable 'a' is completely replaced with its literal value!

This suggests an **IR constant folding or compilation optimization issue** rather than a frame offset bug.

### Test Case: test_short.c

Variable offset assignments in main():
```
.local @_l_x = 4    → __vr0 = 4      (x is first, allocated at offset 4)
.local @_l_y = 6    → __vr2 = 6      (y is second, allocated at offset 6)
.local @_l_z = 8    → __vr4 = 8      (z is third, allocated at offset 8)
.local @_l_neg = 0  → __vr8 = 0      (neg is fourth, but allocated at offset 0!)
```

**Finding**: Variables are NOT allocated in declaration order. There's a non-obvious allocation strategy (possibly based on vreg IDs rather than declaration order).

## Root Cause Analysis - Revised Hypothesis

The original investigation focused on "variable gets wrong offset after call" but the actual issue appears to be:

1. **Constant Folding Too Aggressive**: Variable references may be replaced with their constant values during IR generation or optimization
2. **Vreg Allocation Order**: The frame allocator uses vreg IDs to assign frame offsets, not declaration order
3. **Variable Lifetime Tracking**: After a call, variables might be considered "dead" and optimized away

## Next Investigation Steps

### Step 1: Disable Optimizations
- Rebuild with `-O0` flag
- Test if failures persist
- If failures disappear: optimization issue
- If failures remain: correctness issue in codegen

### Step 2: Check Constant Folding
- Review ConstantFolder.cpp for variable replacement logic
- Check if literal initializers cause variables to be replaced with immediates
- Verify IR shows variable references, not immediates

### Step 3: Examine Frame Allocator
- Why are vregs allocated out of declaration order?
- Is this intentional or a bug?
- Does this affect offset correctness?

### Step 4: IR Dump Analysis
- Generate IR intermediate representation
- Compare IR for variables before vs. after calls
- Look for LOAD ops being replaced with IMM ops

## Test Status

- ✗ test_long_mmemu.c - needs investigation with optimization disabled
- ✗ test_short.c - needs investigation with optimization disabled
- ✗ test_array_init.c - pending
- ✗ test_compound_literal.c - pending
- ✗ test_bitfield_mmemu.c - pending
- ✗ test_struct_return.c - pending

## Hypothesis for Next Phase

The "offset bug" described in PHASE 1 investigation may actually be:
- **Constant folding replacing variable references with immediate values**
- Variables like `a = 10` get "optimized" to their literal value everywhere
- After a function call, the compiler assumes `a` is still 10 and uses immediate instead of loading from frame
- This would explain why only specific patterns fail (those with literal assignments)

## Code to Investigate

1. `src/main/ConstantFolder.cpp` - Check if variables are replaced
2. `src/main/IROptimizer.cpp` - Check if LOAD ops are being folded to IMM
3. `src/main/VRegAllocator.cpp` - Why are vregs allocated out of order?
4. `src/main/IRBuilder.cpp` - Variable reference → LOAD IR generation

## Next Action

Test with `-O0` (no optimizations) to determine if this is an optimization bug or correctness bug.


## CRITICAL FINDING - Optimization Bug Confirmed!

### Hypothesis Confirmed with -O0 Testing

Compiled test_phase2_minimal.c with `-O0` (no optimizations):

**Optimized (default) output - WRONG**:
```asm
ldax.fp 2           ; Load b
add.16 .AX, #10     ; Add immediate 10 (WRONG - should be variable a!)
```

**Unoptimized (-O0) output - CORRECT**:
```asm
ldax.fp 0           ; Load a from FP+0
sta __zp_scratch2   ; Save to temp
stx __zp_scratch2+1
ldax.fp 2           ; Load b from FP+2
add.16 .AX, __zp_scratch2  ; Add a to b (CORRECT!)
```

### Root Cause

**The optimizer is replacing variable loads with immediate constants!**

When a variable is initialized with a constant:
```c
int a = 10;
...
return b + a;  // Optimizer replaces 'a' with immediate #10
```

The optimizer incorrectly assumes:
- If variable `a` starts with value 10
- All uses of `a` should just use the immediate 10
- This is WRONG because `a` can be modified or live across calls

### The Real Bug

The IR optimizer or constant folder is being too aggressive:
- It replaces LOAD operations for const-initialized variables with immediate values
- This works only if the variable is never modified
- It FAILS for variables that might change or are accessed after function calls

### Solution Strategy

For PHASE 2, we need to:
1. Identify which optimization pass is doing this
2. Make it more conservative: don't replace variable loads with immediates
3. Or: track when variables might be modified and disable the optimization
4. Or: regenerate the immediate value each time it's used (not replace the load)

### Files to Investigate

1. `src/main/ConstantFolder.cpp` - Likely culprit
2. `src/main/IROptimizer.cpp` - May have store-folding that's too aggressive
3. `src/main/IRBuilder.cpp` - Check LOAD IR generation

### Test Results with -O0

Expected: If we compile all failing tests with `-O0`, they should pass!

Test this hypothesis on all 6 mmemu tests.

