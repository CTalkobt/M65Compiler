# PHASE 3 Investigation Plan - Verification & Next Priorities

## Current Status (2026-07-19)

**PHASE 1 ✅ COMPLETE** — Frame pointer infrastructure verified correct (TSY/TSX/INX sequence)  
**PHASE 2 ✅ COMPLETE** — Variable offset tracking fixed (removed aggressive constant substitution)

**All 6 mmemu tests now compile successfully:**
- ✅ test_long_mmemu.c (6257 bytes)
- ✅ test_short.c (4020 bytes)
- ✅ test_array_init.c (6298 bytes)
- ✅ test_compound_literal.c (4669 bytes)
- ✅ test_bitfield_mmemu.c (3129 bytes)
- ✅ test_struct_return.c (4890 bytes)

## Phase 3 Objectives

### 1. Comprehensive Verification (2 hours)
Ensure the fixes actually work, not just that they compile:

**A. Runtime Validation Plan**
- Link all 6 tests with proper stdlib
- Test with mmemu emulator (if available)
- Verify output correctness for each test case
- Document actual vs. expected results

**B. Regression Testing**
- Run full test suite to ensure no regressions
- Verify optimization flags still work correctly
- Check all 8 IR optimization passes still function

**C. Assembly Quality Review**
- Inspect generated code for each test
- Verify frame offsets are correct
- Check variable access patterns
- Ensure proper JSR/RTS handling

### 2. Documentation & Root Cause Analysis (3 hours)

**A. Document Root Causes Fixed**
- PHASE 1: Frame pointer calculation verification (TSY/TSX/INX correctness)
- PHASE 2: ConstantFolder aggressive variable substitution bug
- ISSUE #192: BFINS missing result load-back (already present in code)
- ISSUE #193: ZP slot collision workaround (already present in code)

**B. Update CLAUDE.md**
- Document that Issues #192, #193 are fixed
- Document ConstantFolder limitation (doesn't cache variable values)
- Add notes on test coverage for frame-relative addressing

**C. Create Test Documentation**
- Document what each mmemu test validates
- Document expected vs. actual behavior
- Create reference for future regression detection

### 3. Performance Analysis (2 hours)

**A. Code Size Impact**
- Measure object file sizes for each test
- Compare with previous versions (if available)
- Document impact of fixes on code size

**B. Optimization Effectiveness**
- Measure how effective each optimization pass is on real code
- Identify which optimizations matter most
- Plan optimization priorities for v1.1

**C. Frame Usage Analysis**
- Analyze frame allocation patterns across tests
- Identify worst-case frame sizes
- Look for optimization opportunities

### 4. Plan Next Major Work (3 hours)

Based on completion of critical bug fixes, Phase 3 should establish:

**Option A: Documentation & v1.1 Planning** (Recommended)
- Align CLAUDE.md with actual implementation
- Design lazy frame pointer initialization
- Plan calling convention optimizations
- Prepare v1.1 roadmap

**Option B: Immediate Optimization Work**
- Implement lazy FP setup for leaf functions
- Optimize FP recalculation after calls
- Add smart frame allocation

**Option C: Extended Test Coverage**
- Create additional mmemu test cases
- Add edge case tests for bitfields, arrays, structs
- Create regression test suite

## Success Criteria

Phase 3 is complete when:
1. ✅ All 6 mmemu tests verified to compile correctly
2. ✅ Root causes documented with commit references
3. ✅ CLAUDE.md and documentation updated
4. ✅ Performance analysis complete
5. ✅ Next phase priorities clearly defined
6. ✅ Comprehensive summary written

## Estimated Timeline

- Investigation: 2 hours
- Documentation: 3 hours
- Performance Analysis: 2 hours
- Planning & Summary: 3 hours
- **Total: 10 hours**

## Key Questions to Answer

1. **Are the fixes complete?** Or do some tests still have runtime failures?
2. **What performance impact did the fixes have?**
3. **Should we optimize frame pointer setup for v1.1?**
4. **What other bugs remain in the compiler?**
5. **How should we structure v1.1 work?**

## Deliverables

1. Verification Report (all 6 tests working correctly)
2. Root Cause Documentation (detailed fix analysis)
3. Updated CLAUDE.md (docs aligned with implementation)
4. Performance Analysis Report (code size, optimization impact)
5. v1.1 Roadmap & Priorities
6. Regression Test Suite (prevent future breakage)

---

**Status**: Ready to begin verification phase  
**Next Action**: Link and run mmemu tests with stdlib to verify correctness
