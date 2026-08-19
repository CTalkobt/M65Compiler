# Phase 87: Compound Assignment Fusion - Strategic Plan

**Objective:** Fix the remaining -69.4% code bloat in compound assignment chains  
**Target:** Reduce from 525 bytes to ~350 bytes for test_compound.c (-33% improvement)  
**Effort:** 4-6 hours (implementation + testing)  

---

## Problem Analysis

### Current Situation
```c
// test_compound.c - 10 consecutive compound assignments
a = 10;
a += 5;   // 15
a -= 2;   // 13
a *= 3;   // 39
a /= 4;   // 9
a %= 7;   // 2
a <<= 3;  // 16
a >>= 2;  // 4
a |= 8;   // 12
a &= 13;  // 12
a ^= 15;  // 3
```

### Generated IR (Inefficient)
Each compound assignment generates:
```
vreg_t = a op const    (binary operation, load a implicitly)
store vreg_t to a      (store result)
vreg_t2 = a op const   (binary operation, RELOAD a!)
store vreg_t2 to a     (store result)
...
```

### Result
- **O0:** 310 bytes (baseline)
- **O1:** 525 bytes (-69.4% regression)
- **Problem:** 9 redundant loads + 8 unnecessary stores

---

## Solution Approach

### Phase 87.1: Chain Detection (Foundation ✅ Laid)
Created three optimization passes:

1. **CompoundAssignmentFusionPass.hpp** - IR-level pattern detection
   - Detects: `binary_op → store → load → binary_op → store` sequences
   - Identifies fusible chains (3+ ops on same variable)
   - Marks intermediate stores as NOP candidates

2. **CompoundChainOptimizer.hpp** - IR consolidation
   - Groups consecutive stores to same variable
   - Validates they're actually consecutive (gap ≤ 3 instructions)
   - Marks intermediate stores for elimination

3. **AssemblerPeephole.hpp** - Assembler-level cleanup
   - Detects `sta $ZP; ... lda $ZP` patterns where value not clobbered
   - Removes redundant loads at assembly level
   - Handles both 8-bit and 16-bit operations

### Phase 87.2: Integration (Next Step)
Need to wire these passes into the optimization pipeline:

1. **In IROptimizer.cpp**: Add CompoundChainOptimizer after ConstantFolding
2. **In AssemblerOptimizer.cpp**: Add peephole pass after other passes
3. **Pipeline ordering**: Must run AFTER strength reduction, BEFORE codegen

### Phase 87.3: Testing & Validation
- **Unit tests**: Create fusion-specific test cases
- **Regression tests**: Ensure no impact on non-compound code
- **Profile regression**: Re-run profiling_dual_metric.py to measure improvement
- **Expected delta**: -69.4% → -20% or better

---

## Implementation Roadmap

### Step 1: Integrate CompoundChainOptimizer (30 min)
```cpp
// In src/main/IROptimizer.cpp, after constant folding:
if (irOptFlags.constantFolding) {
    CompoundChainOptimizer::optimize(mod);  // New line
}
```

### Step 2: Wire CompoundAssignmentFusionPass (30 min)
```cpp
// In cc45_main.cpp, replace the disabled fusion with:
if (irOptFlags.strengthReduction) {
    CompoundAssignmentFusionPass pass;
    pass.optimize(module);
}
```

### Step 3: Test & Measure (1-2 hours)
```bash
./bin/cc45 src/test-resources/test_compound.c -O1 -S -o /tmp/compound_new.s45
wc -l /tmp/compound_new.s45  # Compare to before
python3 profiling_dual_metric.py  # Full profiling
```

### Step 4: Fix Issues & Iterate (1-2 hours)
- Debug IR pattern detection if needed
- Add safety checks for edge cases
- Handle complex ops (multiply/divide) properly

---

## Technical Challenges & Solutions

### Challenge 1: Pattern Complexity
Some ops need special handling:
- **Multiply/Divide:** Generate temporary vreg allocations that break chain
- **Solution:** Detect temp allocation patterns and consolidate them

### Challenge 2: Live Range Conflicts
Chaining operations changes vreg live ranges:
- **Issue:** VReg allocator may have already decided on allocations
- **Solution:** Run fusion BEFORE vreg analysis, or adjust allocator

### Challenge 3: Validation Complexity
Hard to prove that fused chain is equivalent to original:
- **Solution:** Add debug output to compare generated assembly
- **Validation:** Run test_compound.c and verify output is still 3

---

## Expected Results

### Before Phase 87
```
test_compound: O0=310B, O1=525B (-69.4% regression)
```

### After Phase 87
```
test_compound: O0=310B, O1≈350-380B (better, -13% to -22% regression)
```

### Overall Compiler Metrics
- **Space:** +4.1% → +8-12% (improvement)
- **Speed:** +6.6% → +10-15% (improvement)

---

## Success Criteria

✅ **Must Have:**
1. test_compound.c compiles to correct output (returns 3)
2. Code size for test_compound reduces by at least 30%
3. No regression on other tests (constant_folding, bitfield, cast, etc.)
4. Overall profiling shows +8% or better space improvement

⚠️ **Nice to Have:**
1. Assembly output shows single load/store per chain
2. Array loop regression partially fixed (-5.8% → -3% or better)
3. Handle loop-unrolled compound assignments

---

## Dependencies & Assumptions

**Assumptions:**
- VReg retention optimization (Phase 86) is working correctly
- Strength reduction is enabled and producing SHR for divide-by-power-of-2
- IR maintains definition/use chains that fusion can traverse

**Dependencies:**
- Requires Phase 86 (VReg Retention) to be active
- May interact with: constant folding, algebraic simplification
- Must run BEFORE code generation (before IRCodeGen)

---

## Fallback Strategy (If Full Fusion Too Complex)

If full IR-level fusion proves too complex to implement safely:

**Fallback: Assembler-level optimization only**
- Keep CompoundChainOptimizer as framework for future
- Focus on AssemblerPeephole for redundant load elimination
- Expected improvement: 15-20% (vs. 33% from full fusion)
- Time: ~2 hours vs. 4-6 hours

---

## Future Phases (Phase 88+)

1. **Cross-block optimization:** Extend fusion across if/loop boundaries
2. **Loop-aware fusion:** Detect compound assignments inside loops, fuse smartly
3. **Register pressure:** Profile-guided decision on when to keep in registers
4. **Vectorization:** Recognize SIMD-like patterns in compound chains

---

## Files to Modify/Create

**Created (Foundation):**
- `src/main/CompoundAssignmentFusionPass.hpp` - IR pattern detection
- `src/main/CompoundChainOptimizer.hpp` - Chain consolidation
- `src/main/AssemblerPeephole.hpp` - Peephole optimization

**To Modify (Integration):**
- `src/main/IROptimizer.cpp` - Add fusion pass to pipeline
- `src/main/cc45_main.cpp` - Wire up passes
- `src/main/AssemblerOptimizer.cpp` - Add peephole pass

**Testing:**
- Add test_compound-specific tests
- Run full profiling suite

---

## Status: Ready for Implementation ✅

All groundwork laid. Ready to integrate and test in Phase 87.2.
