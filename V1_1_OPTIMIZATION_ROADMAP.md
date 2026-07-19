# v1.1 Optimization Roadmap — Code Size & Performance

**Document:** v1.1 Optimization Planning  
**Date:** 2026-07-19  
**Status:** Planning Phase  
**Expected Duration:** 2-3 weeks  
**Expected Improvements:** 5-15% code size reduction, 2-5% performance gain

---

## Executive Summary

Based on findings from PHASE 1-3 bug fixes, this document outlines a comprehensive optimization roadmap for v1.1. The primary target is frame pointer efficiency, which is currently a bottleneck for code size and performance.

Current overhead per function (stack convention):
- Frame setup: 7-9 bytes (TSY/TSX/INX/BNE/INY/STX/STY)
- Leaf functions still pay this cost unnecessarily

---

## Optimization Opportunities

### 1. Lazy Frame Pointer Initialization ⭐ HIGH PRIORITY

**Objective:** Initialize FP only when actually needed (not in leaf functions)

**Current Behavior:**
```asm
; Every function, even leaf functions, sets up FP:
tsy
tsx
inx
bne @fp_no_carry
iny
@fp_no_carry:
stx $FD
sty $FE
; 9 bytes overhead per function
```

**Proposed Behavior:**
```asm
; Leaf functions skip FP setup entirely:
; (no frame setup)

; Non-leaf functions set up FP only if accessing locals/params:
tsy
tsx
inx
bne @fp_no_carry
iny
@fp_no_carry:
stx $FD
sty $FE
```

**Implementation Steps:**
1. **Leaf Detection** — Identify functions that don't call other functions
   - Parse call graph during code generation
   - Mark functions with `is_leaf=true` attribute
   - Skip FP setup for leaf functions

2. **Register Allocation** — Adjust for functions without FP
   - When FP not available, compute frame offsets differently
   - Use SPL/SPH directly for frame variable access
   - Track which functions have FP available

3. **Code Generation** — Conditional FP setup
   - In IRCodeGen::emitFunctionPrologue()
   - Check if function is leaf or has no locals/params
   - Emit FP setup only when needed

**Expected Impact:**
- **Code Size:** 5-10% reduction (7-9 bytes per leaf function)
- **Performance:** 2-3% improvement (fewer operations per call)
- **Complexity:** Medium (requires leaf detection pass)

**Risk:** Medium (affects frame pointer management, needs thorough testing)

### 2. Smart Frame Pointer Recalculation

**Objective:** Avoid redundant FP recalculation after function calls

**Current Behavior:**
```asm
; FP setup
tsy; tsx; inx; bne @skip; iny; @skip: stx $FD; sty $FE

; Call function
jsr some_function

; FP is still valid! Stack pointer hasn't changed.
; But we recalculate it anyway...
tsy; tsx; inx; bne @skip2; iny; @skip2: stx $FD; sty $FE
```

**Proposed Behavior:**
```asm
; FP setup
tsy; tsx; inx; bne @skip; iny; @skip: stx $FD; sty $FE

; Call function
jsr some_function

; Stack pointer unchanged by the call
; Reuse cached FP from before the call
; (No recalculation needed)

; FP still valid for frame variable access
```

**Implementation Steps:**
1. **FP State Tracking** — Track FP validity across instructions
   - FP is valid after setup
   - FP remains valid until stack pointer changes
   - Stack pointer changes: frame allocation, stack operations

2. **Call Site Optimization** — Skip FP recalc when possible
   - Check if SP hasn't changed since last FP calculation
   - If unchanged, reuse cached FP values ($FD/$FE)
   - Document assumptions in .reg_clobbers

3. **Assembler Pass** — Detect and eliminate redundant FP calc
   - Pattern matching: "FP setup → JSR → FP setup"
   - Check if SP hasn't changed between them
   - Eliminate second FP setup

**Expected Impact:**
- **Code Size:** 2-3% reduction (7-9 bytes per function with calls)
- **Performance:** 1-2% improvement (fewer operations)
- **Complexity:** Medium (requires state tracking)

**Risk:** Medium (needs correct SP tracking, potential for incorrect optimization)

### 3. Frame Allocation Optimization

**Objective:** More efficient stack frame layout

**Current Behavior:**
- Frame allocated: parameters (caller's responsibility) + locals (in order)
- Alignment: natural (char: 1, short/int: 2, long: 4)
- Gaps: possible between differently-sized locals

**Example:**
```c
int foo(int a, int b) {
    char local1;           // +1 byte
    // +1 byte gap (alignment)
    int local2;            // +2 bytes
    // +1 byte gap (alignment)
    char local3;           // +1 byte
}
// Frame size: 6 bytes (could be 4 bytes with optimal packing)
```

**Proposed Optimization:**
- Sort locals by size (largest first)
- Minimize alignment gaps
- Pack efficiently: long, long, int, short, char

**Expected Impact:**
- **Code Size:** 1-2% reduction (smaller frame = fewer offset bytes)
- **Performance:** Minimal (<1%)
- **Complexity:** Low (data structure reordering)

**Risk:** Low (doesn't affect correctness, just layout)

### 4. Bitfield-Specific Optimizations

**Objective:** Better code generation for bitfield operations

**Current Status:** Bitfield operations (BFINS/BFEXT) are working correctly (Issue #192 fixed)

**Opportunities:**
- Consecutive bitfield operations on same storage → combine
- Single-bit bitfields → use TSB/TRB instead of BFINS/BFEXT
- Adjacent bitfields → pack more efficiently

**Expected Impact:**
- **Code Size:** 1-3% reduction (smaller bitfield ops)
- **Performance:** 1-2% improvement (fewer operations)
- **Complexity:** Medium (pattern matching)

**Risk:** Medium (bitfield semantics complex, needs careful testing)

### 5. Calling Convention Optimization

**Objective:** Better code generation for parameter passing

**Current Status:** Both stack and ZP calling conventions working

**Opportunities:**
- Detect when parameters fit in ZP → use ZP convention
- Automatic thunks for mixed convention calls
- Optimize parameter register allocation

**Expected Impact:**
- **Code Size:** 2-5% reduction (smaller parameter setup)
- **Performance:** 3-5% improvement (faster parameter passing)
- **Complexity:** High (affects calling convention)

**Risk:** High (calling convention changes risky, needs extensive testing)

---

## Implementation Plan

### Phase 1: Lazy Frame Pointer (1 week, 30-40 hours)

**Week 1 Goals:**
- [ ] Implement leaf function detection in IRCodeGen
- [ ] Add `is_leaf` attribute to function symbols
- [ ] Modify prologue generation to skip FP setup for leaf functions
- [ ] Update frame offset calculation for functions without FP
- [ ] Create test suite for leaf function detection

**Deliverables:**
- Leaf detection pass
- Conditional FP setup in prologue
- Test coverage for leaf/non-leaf functions
- Code size measurements

**Testing:**
- All existing tests must pass
- Measure code size reduction
- Verify correctness on complex test cases

### Phase 2: Smart FP Recalculation (1 week, 20-30 hours)

**Week 2 Goals:**
- [ ] Implement SP state tracking in IR
- [ ] Add FP validity tracking to code generation
- [ ] Pattern matching for "FP setup → JSR → FP setup"
- [ ] Eliminate redundant FP calculations
- [ ] Test coverage for FP reuse

**Deliverables:**
- FP state tracking implementation
- Redundant FP elimination pass
- Test cases for various call patterns
- Performance measurements

**Testing:**
- Verify FP is correct after function calls
- Test nested calls (calls within calls)
- Measure performance improvement

### Phase 3: Frame Allocation Optimization (3-4 days, 15-20 hours)

**Week 3 Goals (partial):**
- [ ] Implement frame allocation sorter
- [ ] Optimize local variable layout
- [ ] Measure impact on frame size
- [ ] Test correctness of reordered layout

**Deliverables:**
- Frame allocation optimization pass
- Tests for various local variable combinations
- Size measurements

**Testing:**
- Verify local variables still accessible correctly
- Test with mixed type locals
- Measure frame size reduction

### Phase 4: Testing & Integration (1 week, 20-30 hours)

**Week 3-4 Goals:**
- [ ] Run full regression test suite
- [ ] Create comprehensive v1.1 test suite
- [ ] Benchmark code size improvements
- [ ] Document performance gains
- [ ] Resolve any issues found

**Deliverables:**
- Full test suite passing
- Performance benchmarks
- Documentation of improvements
- v1.1 release notes

---

## Measurement Plan

### Metrics to Track

**Code Size:**
- Object file size (before optimization)
- Executable size (after linking)
- Average frame size per function
- Total bytes saved per optimization

**Performance:**
- Function call overhead (measured via benchmark)
- Frame setup cost (cycles per function)
- Overall execution time for test suite

**Quality:**
- All tests passing (regression test suite)
- No new compiler warnings
- Code coverage maintained

### Benchmark Suite

Create comprehensive benchmarks:

```c
// Leaf function benchmark
int leaf_add(int a, int b) { return a + b; }
// Expected: ~5-7 bytes smaller

// Non-leaf function benchmark  
int non_leaf(int a) { return leaf_add(a, 10); }
// Expected: ~7-9 bytes smaller from FP reuse

// Complex frame benchmark
int complex_frame(void) {
    int x, y, z;
    char c1, c2;
    return x + y + z;
}
// Expected: ~2-3 bytes smaller from layout optimization
```

---

## Risk Mitigation

### High-Risk Areas

1. **Frame Pointer Correctness**
   - Mitigation: Extensive testing with all frame sizes
   - Validation: Verify frame offsets calculated correctly

2. **Calling Convention Changes**
   - Mitigation: Keep changes backward compatible
   - Validation: All existing calling convention tests pass

3. **Optimization Correctness**
   - Mitigation: Conservative optimizations, many test cases
   - Validation: Compare generated code with known-good baseline

### Testing Strategy

1. **Unit Tests** — Each optimization individually
2. **Integration Tests** — Multiple optimizations together
3. **Regression Tests** — Entire test suite passes
4. **Benchmark Tests** — Performance improvements measured
5. **Edge Case Tests** — Complex function signatures, nested calls, mixed conventions

---

## Success Criteria

v1.1 is complete when:

1. ✅ Lazy FP implemented and tested (5-10% reduction)
2. ✅ FP recalculation optimized (2-3% reduction)
3. ✅ Frame allocation optimized (1-2% reduction)
4. ✅ All regression tests pass
5. ✅ Overall 5-15% code size reduction achieved
6. ✅ No performance regressions (2-5% improvement expected)
7. ✅ Documentation updated
8. ✅ v1.1 release notes written

---

## Timeline

**Start Date:** After v1.0.5 patch  
**Estimated Duration:** 2-3 weeks (80-120 hours)  
**Phases:** 4 overlapping phases  
**Release Target:** Early August 2026

---

## Future Enhancements (v1.2+)

1. **Variable Packing** — Pack variables more efficiently in frame
2. **Inlining** — Inline small functions to eliminate calls
3. **Loop Unrolling** — Better loop-unroll optimizations
4. **LICM Improvements** — Better loop-invariant code motion
5. **Tail Call Optimization** — Convert recursion to loops
6. **Dead Code Elimination** — More aggressive dead code removal

---

## Conclusion

v1.1 focuses on frame pointer efficiency, the primary remaining optimization opportunity. With proper implementation of lazy FP, smart recalculation, and frame layout optimization, we expect 5-15% code size reduction and 2-5% performance improvement while maintaining full backward compatibility.

The implementation plan is conservative and focuses on correctness above aggressive optimization. Each phase is self-contained and can be reviewed independently.

---

**Next Step:** After Phase 4 bug fix validation, begin Phase 1 implementation of v1.1 optimizations.
