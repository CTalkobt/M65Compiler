# PHASE 2 Investigation Plan - Variable Offset Tracking

## Objective

Fix the variable offset corruption bug that occurs after function calls. When a variable is accessed before and after a function call, the second access uses an incorrect frame offset.

## Root Cause Analysis (from Phase 1)

The bug manifests as:
```
Before call: ldax.fp 0    // Correct - load variable 'a' from FP+0
After call:  lda.fp 4     // WRONG - load from FP+4 (wrong variable location)
```

**Root Cause**: IR builder generates different IR instructions for the second variable access, requesting a different vreg instead of reusing the original vreg.

**Why NOT these**: 
- ✗ NOT code generation (vregOffset_ is correct)
- ✗ NOT CSE optimization (persists without CSE)
- ✗ NOT register state tracking (investigated and cleared)
- ✓ LIKELY IR generation/vreg assignment inconsistency

## Investigation Strategy

### Step 1: Trace Variable References in IR
1. Add debug output to IRBuilder's variable reference handling
2. Log which vreg is assigned for each variable access
3. Compare vregs before vs. after function calls
4. Identify if IR builder is creating new vregs for same variable

### Step 2: Check Frame Allocator State
1. Review VRegAllocator frame layout calculations
2. Verify vreg-to-offset mapping is consistent
3. Check if allocator re-runs or updates mappings after calls
4. Look for state that might differ on second pass

### Step 3: Examine IR Generation Caching
1. Check IRBuilder for caches that might be invalidated
2. Look for "value X is in vreg Y" mappings
3. Verify variable name → vreg mapping is consistent
4. Check for state that differs between passes

### Step 4: Compare Generated IR
1. Generate IR dump before and after calls
2. Show which IR instructions are generated for each variable access
3. Identify where vreg assignments diverge
4. Root cause analysis based on IR structure

## Implementation Plan

### Phase 2A: Debugging Setup (2 hours)
- Add comprehensive debug logging to IRBuilder
- Create IR dump format showing vreg assignments
- Set up test infrastructure for IR comparison
- Test on failing mmemu tests

### Phase 2B: Root Cause Identification (3 hours)
- Run debug version on failing tests
- Collect IR dumps showing variable accesses
- Analyze vreg assignment patterns
- Identify exact location of inconsistency

### Phase 2C: Fix Implementation (2 hours)
- Fix the inconsistency in IR generation
- Verify vreg assignments are stable
- Test on all 6 mmemu tests
- Commit and document fix

### Phase 2D: Regression Testing (1 hour)
- Full test suite run
- Verify no new regressions
- Verify all 6 mmemu tests pass

## Success Criteria

- ✅ All 6 mmemu tests compile without errors
- ✅ Generated assembly shows correct frame offsets for all variable accesses
- ✅ Variable accesses before and after calls use same vreg/offset
- ✅ No regressions in existing tests

## Estimated Effort: 8 hours

- Phase 2A: 2 hours (debug infrastructure)
- Phase 2B: 3 hours (root cause analysis)
- Phase 2C: 2 hours (fix implementation)
- Phase 2D: 1 hour (regression testing)

## Key Files to Investigate

### IR Generation
- `src/main/IRBuilder.cpp` - Variable reference handling
- `src/main/IRCodeGen.cpp` - IR to assembly emission

### Frame Allocation
- `src/main/VRegAllocator.cpp` - Vreg-to-frame offset mapping
- `src/main/IRCodeGen.cpp` - Frame layout calculations

### State Management
- `src/main/IROptimizer.cpp` - IR optimization/caching
- `src/main/cc45_main.cpp` - IR generation pipeline

## Test Cases

### Primary Test: test_short.c
```c
int add_short(int a, int b) {
    return a + b;
}

int mul_short(int a, int b) {
    return a * b;
}

int main() {
    int x = 10;
    int y = 20;
    int z = add_short(x, y);        // Call function
    printf("%d", z);                // z accessed AFTER call (offset bug here)
}
```

### Symptom
- Expected: z contains result of add_short (30)
- Actual: z gets data from wrong frame location

## Next Steps

1. Build current code and verify tests still fail
2. Add IR generation logging
3. Generate IR dumps for failing tests
4. Compare IR before vs. after function calls
5. Identify vreg assignment divergence
6. Implement fix based on findings
7. Test and commit

---

**Status**: Ready to begin Phase 2 investigation
**Estimated Start**: 2026-07-19
**Expected Completion**: 2026-07-20
