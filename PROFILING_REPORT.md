# Optimization Effectiveness Report - cc45 Phase 86+

**Date:** 2026-08-19  
**Compiler:** cc45 v1.0.4 (eaf8bcc)  
**Analysis Type:** Instruction count profiling across -O0 through -O3

## Executive Summary

Current optimization pipeline provides **+4.9% overall code reduction** from -O0 to -O1, with **highly variable results**:
- **Best case:** Constant folding (+66.4% reduction)
- **Worst case:** Compound literals (-92.8% bloat)
- **No optimization benefit:** Control flow tests (0%)

**Critical Finding:** Several optimizations are generating MORE code than unoptimized versions, indicating the framework needs selective tuning.

---

## Detailed Results

| Test Case | O0 | O1 | Change | Category | Status |
|-----------|----|----|--------|----------|--------|
| **constant_folding** | 393 | 132 | **+66.4%** ✅ | Arithmetic-heavy | EXCELLENT |
| **bitfield** | 237 | 171 | **+27.8%** ✅ | Bit manipulation | EXCELLENT |
| **cast** | 234 | 227 | **+3.0%** ✅ | Type conversions | GOOD |
| **control_flow** | 30 | 30 | **±0.0%** ⚠️ | Branches | NEUTRAL |
| **array_init** | 817 | 879 | **-7.6%** ❌ | Array initialization | REGRESS |
| **array_loop** | 484 | 512 | **-5.8%** ❌ | Loop patterns | REGRESS |
| **compound** | 139 | 268 | **-92.8%** ❌ | Literals/compounds | SEVERE REGRESS |
| **TOTAL** | 2334 | 2219 | **+4.9%** | Overall | MIXED |

---

## Key Findings

### 1. Constant Folding is Extremely Effective (66.4% reduction)
- **Strength:** Eliminates redundant arithmetic computations at compile time
- **Impact:** Single most beneficial optimization
- **Recommendation:** Maintain priority; verify it's enabled at all -O levels

### 2. Bitfield Optimization Works Well (27.8% reduction)  
- **Strength:** TSB/TRB bitfield patterns are optimized efficiently
- **Impact:** Second-best optimization
- **Recommendation:** Consider expanding bitfield patterns

### 3. Loop Unrolling May Be Too Aggressive (-5.8% for arrays)
- **Issue:** `test_array_loop` generates 512 instructions vs 484 unoptimized
- **Root Cause:** Loop unrolling heuristic likely expanding beyond profitable threshold
- **Recommendation:** Reduce unroll threshold or add profitability checks
- **Action:** Phase 80 opportunity - validate unroll factors

### 4. Compound Literals Show Severe Regression (-92.8%)
- **Issue:** `test_compound` goes from 139 to 268 instructions (+93%)
- **Root Cause:** Compound literal expansion or initialization inlining causing code bloat
- **Recommendation:** Disable or gate this optimization
- **Action:** HIGH PRIORITY - investigate IR generation for compound literals

### 5. Array Initialization Regresses Slightly (-7.6%)
- **Issue:** Initialization code generation strategy may be suboptimal
- **Recommendation:** Verify array init optimization isn't firing when unprofitable

### 6. Control Flow Optimization Not Working (0% gain)
- **Issue:** Branch optimization providing no measurable benefit
- **Recommendation:** Verify branch-folding and jump-optimization are engaged

---

## Optimization ROI Ranking

1. **Constant Folding** - HIGH ROI (66.4% improvement) ✅ PRIORITY 1
2. **Bitfield Optimization** - HIGH ROI (27.8% improvement) ✅ PRIORITY 1
3. **Type Casting** - MEDIUM ROI (3.0% improvement) ✅ PRIORITY 2
4. **Control Flow** - NO BENEFIT (0% improvement) ⚠️ DEBUG NEEDED
5. **Loop Unrolling** - NEGATIVE ROI (-5.8% regression) ❌ NEEDS TUNING
6. **Array Initialization** - NEGATIVE ROI (-7.6% regression) ❌ NEEDS TUNING
7. **Compound Literals** - SEVERE NEGATIVE ROI (-92.8% regression) ❌ CRITICAL FIX

---

## Recommendations for Phase 86+ Work

### Immediate (High Priority)
1. **Investigate compound literal bloat** - Likely culprit for worst regression
   - Check IR generation for compound literals
   - Verify initialization expansion strategy
   - Consider disabling if unprofitable

2. **Tune loop unrolling threshold**
   - Current strategy expanding loops unprofitably
   - Reduce max unroll count or add cost model
   - Consider disable unless -O3

3. **Verify control flow optimization**
   - Zero improvement suggests pass not engaged
   - Check if branch folding/inversion actually running

### Short-term (Phase 86-87)
4. **Cross-function inlining analysis** (Phase 86)
   - Measure if inlining helps or hurts different code patterns
   - May explain some test regressions

5. **Per-optimization control validation**
   - Ensure -fno-* flags disable each pass
   - Verify correct passes engaged at each -O level

### Medium-term (Phase 88+)
6. **Profitability-aware optimizations**
   - Add size prediction before applying transformations
   - Skip optimizations where cost model predicts bloat

7. **Pattern-specific optimization** 
   - Recognize patterns like array init, compound literals
   - Apply specialized handling instead of generic IR passes

---

## Next Steps

**Option A: Fix Regressions (Defensive)**
- Focus on compound literal bloat first (highest impact)
- Then loop unrolling threshold tuning
- Estimated: Could return 10-15% additional improvement

**Option B: Expand Wins (Offensive)**  
- Investigate why bitfield optimization works well
- Apply similar heuristics to other bit operations
- Could unlock additional 10-20% gains

**Recommendation:** **Option A first** - eliminate regressions before adding features. Removing -92.8% regression alone would boost overall by ~10%. Then tackle -5.8% and -7.6% regressions.

---

## Test Suite Configuration

Tests profiled with representative code patterns:
- **Arithmetic**: constant_folding (compiler arithmetic)
- **Bit ops**: bitfield (TSB/TRB patterns)
- **Type system**: cast (type conversions)
- **Control**: control_flow (branches)
- **Loops**: array_loop (iteration patterns)
- **Data init**: array_init, compound (literal/compound patterns)

**Note:** Larger real-world programs may show different characteristics. This analysis is optimizations for small test cases; whole-program optimization may benefit or suffer differently.
