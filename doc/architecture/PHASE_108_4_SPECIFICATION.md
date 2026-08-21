# Phase 108.4: Compiler Tuning Hooks — Validation Specification

**Status**: SPECIFICATION & TEST PLAN (2026-08-21)  
**Phase**: 108.4 (Hook Infrastructure Validation)  
**Purpose**: Comprehensive validation of 12 hook points across compilation pipeline  

## Overview

Phase 108.4 validates the complete hook infrastructure:
- **Phase 108.1**: Hook infrastructure (12 strategic hook points)
- **Phase 108.2**: cc45_main integration (hook wiring)
- **Phase 108.3**: Decision logic (budget-aware decisions, learner integration)

Phase 108.4 ensures:
1. All 12 hook points invoked correctly
2. Signal collection accurate (metrics match actual compilation state)
3. Decision logic sound (decisions based on correct reasoning)
4. Integration seamless (Phase 109 receives all signals)
5. Performance acceptable (hook overhead < 5%)
6. Reliability robust (no crashes, graceful error handling)

## Hook Points Overview

| Hook | Stage | Purpose | Signal Type | Decision |
|------|-------|---------|------------|----------|
| 1. PreParse | Lexer setup | Initialize state | File info | Skip/continue |
| 2. PostLex | Token stream ready | Count tokens | Token count | Continue |
| 3. PostParse | AST available | Analyze AST | Node/function count | Budget check |
| 4. PostConstFold | Constants folded | Track optimization | Fold count | IR opt enable |
| 5. PostFuncAnalysis | Functions analyzed | Call graph built | Function count | Inlining |
| 6. PreOptSelect | Before opt selection | **CRITICAL** | Budget status | Skip expensive |
| 7. PostInlineSelect | Inlining candidates found | Inlining metrics | Candidate count | Continue |
| 8. PostCallGraph | Call graph complete | Call density | Call metrics | Framework |
| 9. PostIRBuild | IR generated | Memory check | Node count | Monitor |
| 10. PreIROpt | Before IR passes | **CRITICAL** | Budget status | Pass selection |
| 11. PostCodeGen | Assembly generated | Code size | Assembly size | Peephole |
| 12. PostAsmOpt | Assembly optimized | **CRITICAL** | Final metrics | Finalization |

**Critical hooks**: 6 (PreParse, PostParse, PreOptSelect, PreIROpt, PostCodeGen, PostAsmOpt)

## Validation Strategy

### Level 1: Unit Validation (Individual Hooks)

**Objective**: Verify each hook invoked with correct signals

**Method**:
1. Compile test file
2. Enable verbose logging
3. Verify hook log entries present
4. Check signal metrics accuracy
5. Confirm decision made

**Test File**: test_phase108_hooks.c (19 test functions covering all scenarios)

**Expected Results**:
- All 12 hooks logged
- All metrics accurate
- All decisions reasonable
- Compilation succeeds

### Level 2: Integration Validation (Hook Coordination)

**Objective**: Verify hooks invoked in correct order with proper data flow

**Method**:
1. Trace hook execution order
2. Verify decision propagation
3. Check early exit behavior
4. Validate error handling

**Test Scenario 1: Normal Compilation**
```
PreParse → PostLex → PostParse → PostConstFold → 
PostFuncAnalysis → PreOptSelect (continue) → 
PostInlineSelect → PostCallGraph → PostIRBuild → 
PreIROpt → PostCodeGen → PostAsmOpt
```

**Test Scenario 2: Budget Exhaustion**
```
PreParse → PostLex → PostParse → 
PreOptSelect (skipExpensivePasses=true) → 
PostInlineSelect (still analyzed but not applied) → 
PostCallGraph → PostIRBuild (memory limited) → 
PreIROpt (skip expensive passes) → PostCodeGen → PostAsmOpt
```

**Test Scenario 3: Early Exit**
```
PreParse → PostLex → PostParse → 
[Decision: abort compilation] → 
Exit (remaining hooks skipped)
```

### Level 3: Phase 109 Integration Validation

**Objective**: Verify signals reach Phase 109 and drive adaptive recommendations

**Method**:
1. Compile file 5 times
2. Track signals sent to Phase 109
3. Verify profiles updated
4. Confirm next compilation uses updated thresholds
5. Measure adaptation effectiveness

**Test Scenario**:
```
Compilation 1-5: Collect signals
    ↓
Phase 109EffectivenessCollector records all signals
    ↓
Phase 109ProfileDatabase aggregates
    ↓
Phase 109ThresholdAdjuster analyzes trends
    ↓
Compilation 6: Uses adapted thresholds
```

### Level 4: Performance Validation

**Objective**: Measure hook overhead and ensure < 5% impact

**Method**:
1. Compile without hooks (baseline)
2. Compile with hooks (with logging)
3. Compile with hooks (silent)
4. Calculate overhead percentages

**Overhead Budget**:
- Total hook time: < 10ms
- Per hook: < 1ms average
- Logging overhead: < 5% additional

**Test Files**:
- Small (50ms baseline): Overhead 1-2%
- Medium (100ms baseline): Overhead 2-4%
- Large (150ms baseline): Overhead 2-5%

### Level 5: Error Handling Validation

**Objective**: Verify robustness and graceful degradation

**Test Cases**:
1. Missing Phase 107 learner → Use fallback logic ✓
2. Budget overrun → skipExpensivePasses triggered ✓
3. Invalid signal data → Data validation or sanitization ✓
4. Unregistered hook → No-op or default behavior ✓
5. Phase 109 unavailable → Continue without adaptation ✓

## Test Execution Plan

### Phase 108.4.1: Basic Validation (1 hour)

```bash
# Compile test file and verify hooks invoked
cd src/test-resources
../scripts/test_compiler.sh test_phase108_hooks.c

# Expected: All 19 test functions compile and execute correctly
# Expected: All 12 hooks should be invoked
# Output: Test results match expected values
```

### Phase 108.4.2: Verbose Logging Validation (30 min)

```bash
# Run with verbose Phase 108 logging
cc45 test_phase108_hooks.c --verbose-phase108 2>&1 | tee phase108_verbose.log

# Verify log contains:
# [Phase108] Starting compilation
# [Phase108] PostLex: NNN tokens
# [Phase108] PostParse decision: ...
# [Phase108] PreOptSelect: ...
# [Phase108] Compilation complete (XXXms)

# Metrics in log should match actual values
grep "PostLex:" phase108_verbose.log | grep -oP '\d+(?= tokens)'
# Should match actual token count in test file
```

### Phase 108.4.3: Phase 109 Integration Validation (1 hour)

```bash
# Compile same file 5 times with Phase 109 enabled
for i in {1..5}; do
    time cc45 test_phase108_hooks.c -o test_$i.prg
done

# Verify Phase 109 receives signals
# Check Phase109EffectivenessCollector::getHistory()
# Confirm profiles updated each iteration
# Verify compilation time improves or stabilizes
```

### Phase 108.4.4: Performance Measurement (1 hour)

```bash
# Baseline (no hooks)
time cc45 test_phase108_hooks.c -o baseline.prg

# With hooks (verbose on)
time cc45 test_phase108_hooks.c -o with_verbose.prg --verbose-phase108

# With hooks (silent)
time cc45 test_phase108_hooks.c -o with_silent.prg --phase108-hooks

# Calculate overhead percentage
# overhead% = (with_hooks_time - baseline_time) / baseline_time * 100
# Target: < 5%
```

## Validation Checklist

### Checklist: Signal Collection

- [ ] PreParse hook invoked
- [ ] PostLex hook invoked with token count
- [ ] PostParse hook invoked with AST metrics
- [ ] PostConstFold hook invoked with fold metrics
- [ ] PostFuncAnalysis hook invoked with function count
- [ ] PreOptSelect hook invoked with budget check
- [ ] PostInlineSelect hook invoked with candidate count
- [ ] PostCallGraph hook invoked with call density
- [ ] PostIRBuild hook invoked with IR metrics
- [ ] PreIROpt hook invoked with budget status
- [ ] PostCodeGen hook invoked with assembly size
- [ ] PostAsmOpt hook invoked with final metrics

**Success Criteria**: All 12 hooks invoked, all signals collected

### Checklist: Decision Logic

- [ ] Budget tracking accurate
- [ ] Remaining budget calculated correctly
- [ ] skipExpensivePasses decision correct
- [ ] enableOptimizations decision correct
- [ ] Decision rationale generated
- [ ] Fallback strategy works
- [ ] Learner integration works (if available)
- [ ] Confidence scoring works

**Success Criteria**: All decisions correct, fallback works

### Checklist: Hook Sequencing

- [ ] Hooks invoked in documented order
- [ ] Early exit handled correctly
- [ ] Decision propagation works
- [ ] No hook invoked twice
- [ ] No hooks skipped unexpectedly
- [ ] Error hooks invoked on error path

**Success Criteria**: Correct ordering, proper propagation

### Checklist: Phase 109 Integration

- [ ] Signals sent to EffectivenessCollector
- [ ] Decisions recorded for feedback
- [ ] Profiles updated after compilation
- [ ] Thresholds adjusted based on effectiveness
- [ ] Next compilation uses new thresholds
- [ ] Adaptation measurable and positive

**Success Criteria**: Full feedback loop working

### Checklist: Performance

- [ ] Hook overhead < 5% (< 5ms for 100ms compile)
- [ ] Signal collection < 1ms each
- [ ] Decision logic < 2ms each
- [ ] Total hook time < 10ms
- [ ] Memory overhead < 5%
- [ ] Verbose logging < 10% overhead

**Success Criteria**: All overhead targets met

### Checklist: Error Handling

- [ ] Handles missing learner
- [ ] Handles budget overrun
- [ ] Validates signal data
- [ ] Handles unregistered hooks
- [ ] Recovers without crashing

**Success Criteria**: All error cases handled

### Checklist: Documentation

- [ ] All hooks documented
- [ ] Signal types documented
- [ ] Decision types documented
- [ ] Budget strategy documented
- [ ] Integration points documented
- [ ] Verbose logging format documented
- [ ] Error handling documented

**Success Criteria**: Complete documentation

## Expected Results

### Compilation Success
- Test file compiles: ✓
- All 19 functions execute correctly: ✓
- Output matches expected: ✓

### Hook Coverage
- All 12 hooks invoked: 100% ✓
- All hooks record signals: 100% ✓
- All hooks make decisions: 100% ✓

### Signal Accuracy
- Token count matches lexer: 100% ✓
- AST metrics match parser: 100% ✓
- Function count matches analyzer: 100% ✓
- Budget tracking accurate: 100% ✓

### Performance
- Hook overhead: 2-4% ✓
- Signal collection: < 1ms ✓
- Decision logic: < 2ms ✓
- Total hook time: < 10ms ✓

### Integration
- Phase 109 receives signals: 100% ✓
- Profiles updated: 100% ✓
- Thresholds adjusted: 100% ✓
- Adaptation measurable: ✓

## Success Criteria

1. ✅ All 12 hook points functional and tested
2. ✅ Signal collection accurate (>99% agreement)
3. ✅ Decision logic correct (>95% accuracy)
4. ✅ Hook overhead < 5%
5. ✅ Phase 109 integration working
6. ✅ Error handling robust
7. ✅ Verbose logging accurate
8. ✅ Zero regressions
9. ✅ Complete documentation
10. ✅ Ready for production deployment

## Timeline

- **Phase 108.4.1**: 1 hour (basic validation)
- **Phase 108.4.2**: 30 min (verbose logging)
- **Phase 108.4.3**: 1 hour (Phase 109 integration)
- **Phase 108.4.4**: 1 hour (performance)
- **Phase 108.4.5**: 30 min (error handling)
- **Phase 108.4.6**: 30 min (documentation review)

**Total**: 4.5 hours estimated

## Sign-Off Criteria

Phase 108.4 complete when:
1. All 12 hooks tested and working
2. All signal collection accurate
3. All decisions correct
4. All performance targets met
5. All error cases handled
6. Phase 109 integration verified
7. Documentation complete
8. No regressions on existing tests

**Target Completion**: 2026-08-21 (today)

---

**References**:
- Phase 108.1: Hook infrastructure
- Phase 108.2: cc45_main integration
- Phase 108.3: Decision logic & learner integration
- Phase 109: Adaptive optimization tuning
