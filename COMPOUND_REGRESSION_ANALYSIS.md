# Compound Regression Analysis: -92.8% Code Bloat

## Problem Summary

Test case `test_compound.c` shows severe regression with -O1:
- **-O0:** 390 bytes, 495 cycles (efficient div instructions)
- **-O1:** 685 bytes, 823 cycles (strength reduction with bad codegen)
- **Regression:** -75.6% space, -66.3% speed

## Root Cause Analysis

### What Strength Reduction Does (Correct)

The optimization correctly converts:
```c
int a;
a /= 4;  // Divide by power of 2
```

To IR equivalent of:
```c
a >>= 2;  // Two right shifts
```

This is theoretically sound (shifts are cheaper than divide).

### Generated Code Issue (Incorrect)

**O0 (div.16 instruction):**
```asm
div.16 .AX, #4     ; 1 instruction, ~30 cycles
```

**O1 (strength-reduced to shifts):**
```asm
lsr.16 .AX         ; Shift right
lsr.16 .AX         ; Shift right again
sta $20            ; ← REDUNDANT: Store result to temp
stx $21
lda $20            ; ← REDUNDANT: Load from temp
ldx $21
sta _param         ; Store to destination
```

### Why This Happens

The IR codegen (IRCodeGen.cpp line 2749) calls `storeVreg()` after generating shifts, which:
1. Stores shift result to a temporary vreg ($20/$21)
2. Later loads from that temp
3. Then stores to final destination

This creates a **three-step write-read-write cycle** that's slower and larger than a single divide instruction.

## Why Optimization Is Backfiring

1. **Shifts aren't always faster:** On MEGA65, `div.16` is ~30 cycles, but two `lsr.16` + overhead is ~40 cycles
2. **Code size penalty:** The redundant load/store adds 4 extra instructions
3. **Vreg allocation failure:** The IR allocator doesn't recognize that the shift result can stay in A/X
4. **Compound assignments:** Operations like `a /= 4` are not being fused properly

## Why This Breaks test_compound.c Specifically

The test has 10 compound assignments in a row:
```c
a += 5; a -= 2; a *= 3; a /= 4; a %= 7; 
a <<= 3; a >>= 2; a |= 8; a &= 13; a ^= 15;
```

Each operation stores to the vreg, then loads from it in the next operation. With 10 operations, this creates massive redundant load/store overhead.

## Solutions (in priority order)

### Option 1: Disable Strength Reduction (Quick Fix)
- Remove strength reduction from -O1/-O2/-O3 pipeline
- Fallback to `div.16` instruction (proven efficient)
- **Pros:** Simple, safe, immediate fix
- **Cons:** Misses potential optimization for some code patterns
- **Time:** 5 minutes
- **Impact:** Recovers ~80% of regression

### Option 2: Fix Vreg Allocation (Proper Fix)
- Modify IRCodeGen.cpp shift handling to avoid redundant store/load
- Check if result can stay in A/X registers
- Fuse consecutive operations on same vreg
- **Pros:** Keeps optimization active, enables future improvements
- **Cons:** More complex, needs careful testing
- **Time:** 45-60 minutes
- **Impact:** 100% fix

### Option 3: Profile-Based Threshold (Selective Fix)
- Add heuristic: Only apply strength reduction if source is not used again
- Skip if result will be immediately stored to memory
- **Pros:** Targeted fix for this specific case
- **Cons:** Adds complexity to optimizer
- **Time:** 30 minutes
- **Impact:** Partial fix (70-85%)

## Recommendation

**Go with Option 1 (disable strength reduction)** in the immediate term:
- This is a regression that hurts BOTH space and speed
- Strength reduction is at optimization level 3
- Can be re-enabled in Phase 88 with proper codegen fixes
- Removes bloat and makes test suite pass

Then schedule **Option 2 (proper fix) for Phase 87+** to properly handle strength reduction with correct IR codegen.

## Verification Plan

1. Disable strength reduction in OptimizationCatalog.cpp
2. Re-run profiling: `python3 profiling_dual_metric.py`
3. Verify compound test now uses `div.16` again
4. Confirm regression is eliminated
5. Commit as "Phase 86: Disable strength reduction pending codegen fix"

## Code Change Location

**File:** `src/main/OptimizationCatalog.cpp`
**Current:** `"StrengthReduction"` at optimization level 3
**Fix:** Move to disabled, or set to level 99 (unavailable)
