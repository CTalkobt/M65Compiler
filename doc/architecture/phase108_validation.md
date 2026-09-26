# Phase 108.4: Compiler Tuning Hooks — Validation & Measurement

**Status**: VALIDATION & TESTING (2026-08-21)  
**Phase**: 108.4 (Hook Infrastructure Validation)  
**Purpose**: Validate all 12 hook points, signal collection, and decision logic  

## Overview

Phase 108.4 validates the complete hook infrastructure implemented in Phases 108.1-108.3:
- 12 strategic hook points throughout compilation pipeline
- Real-time signal collection (metrics at each phase)
- Budget-aware decision logic (4-tier strategy)
- Integration with Phase 107 online learner
- Integration with Phase 109 adaptive tuning

## Hook Points Validation Matrix

| # | Hook Point | Stage | Signal Type | Decision Type | Critical |
|---|-----------|-------|------------|---------------|----------|
| 1 | PreParse | Lexer setup | File info | Skip/continue | ✓ |
| 2 | PostLex | After lexer | Token count | Continue | ✓ |
| 3 | PostParse | After parser | AST metrics | Budget check | ✓ |
| 4 | PostConstFold | After folding | Optimization stats | IR opt enable | ✓ |
| 5 | PostFuncAnalysis | After analysis | Function count | Inlining enable | ✓ |
| 6 | PreOptSelect | Before opt selection | Budget status | Skip expensive | ✓ |
| 7 | PostInlineSelect | After inlining | Candidates analyzed | Continue | ✓ |
| 8 | PostCallGraph | After call graph | Call density | Framework choice | - |
| 9 | PostIRBuild | After IR gen | IR node count | Memory monitor | - |
| 10 | PreIROpt | Before IR opts | Preliminary metrics | Pass selection | ✓ |
| 11 | PostCodeGen | After code gen | Assembly size | Peephole enable | - |
| 12 | PostAsmOpt | After asm opt | Final assembly | Finalization | ✓ |

**Critical**: Affects compilation decisions and timing.

## Validation Test Categories

### Category 1: Signal Collection Accuracy

**Test 1.1: Lexer Signal Collection**
```c
// Input: simple variable declaration
int x = 10;
```
**Expected Signal**:
- tokenCount = 5 (int, x, =, 10, ;)
- elapsedTime >= 0
- phaseCompleteTime recorded

**Validation**: Count matches actual token stream

**Test 1.2: Parser Signal Collection**
```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```
**Expected Signal**:
- astNodeCount >= 20
- functionCount = 1
- branchCount >= 2

**Validation**: AST walk confirms counts

**Test 1.3: Constant Fold Signal**
```c
int a = 5 + 3;
int b = a * 2;
```
**Expected Signal**:
- constantsFolded >= 1
- deadCodeRemoved >= 0
- compilationTimeMs recorded

**Validation**: Folded values match IR

**Test 1.4: Function Analysis Signal**
```c
int helper1(int x) { return x + 1; }
int helper2(int x) { return x * 2; }
int main() {
    return helper1(helper2(5));
}
```
**Expected Signal**:
- functionsAnalyzed = 3
- callDensity = 2 (calls in main)
- leafFunctions >= 2

**Validation**: CFG walk confirms analysis

### Category 2: Decision Logic Correctness

**Test 2.1: Budget-Aware Decisions**
```
Scenario: File takes 200ms to compile, budget = 500ms
Expected: remainingBudget = 300ms, can apply expensive opts
Decision: enableOptimizations = true

Scenario: File takes 400ms, budget = 500ms
Expected: remainingBudget = 100ms, skip expensive opts
Decision: skipExpensivePasses = true
```

**Test 2.2: Threshold-Based Decisions**
```
Scenario: AST has 10,000 nodes (large file)
Threshold: > 5,000 nodes triggers PR limit

Expected Decision: "skipExpensivePasses = true"
Rationale: "File complexity high, skip expensive"
```

**Test 2.3: Confidence-Based Decisions**
```
Scenario: Insufficient data for recommendation (< 5 compilations)
Confidence: < 40%

Expected Decision: Use default/conservative strategy
Fallback: enableOptimizations = true (conservative)
```

**Test 2.4: Learner Integration Decision**
```
Scenario: Phase 107 learner available and trained
Learner prediction: "inlining ineffective for small files"

Expected Decision: Learner hint incorporated
Result: inlineThreshold adjusted up
```

### Category 3: Hook Point Sequencing

**Test 3.1: Hook Order Validation**
```
Expected Sequence:
1. PreParse
2. PostLex → decision continues
3. PostParse → decision checks budget
4. PostConstFold → if optimize enabled
5. PostFuncAnalysis
6. PreOptSelect → critical decision point
7. PostInlineSelect (if inlining enabled)
8. PostCallGraph (optional)
9. PostIRBuild
10. PreIROpt
11. PostCodeGen
12. PostAsmOpt

Validation: Verify order in hook statistics
```

**Test 3.2: Early Exit Validation**
```
Scenario: Budget exhausted at PostConstFold
Expected: skipExpensivePasses = true propagated
Result: No further IR optimization hooks invoked
```

**Test 3.3: Decision Propagation**
```
Scenario: PreOptSelect decides skipExpensivePasses = true
Expected: PostInlineSelect respects this decision
Result: Inline candidates still analyzed but not applied
```

### Category 4: Integration with Phase 109

**Test 4.1: Signal → Profile Mapping**
```
CompilationSignal from Phase 108
    ↓
Phase109EffectivenessCollector (records signal)
    ↓
Phase109ProfileDatabase (aggregates)
    ↓
Verify: Signal data stored correctly
```

**Test 4.2: Decision → Recommendation Feedback**
```
Phase 108 Decision (optimize = true)
    ↓
Phase 109 records optimization applied
    ↓
Phase 109 measures effectiveness
    ↓
Verify: Feedback loop intact
```

**Test 4.3: Threshold Adjustment Impact**
```
Initial: Phase 108 threshold = 0.5 (50%)
After Phase 109 analysis: threshold adjusted to 0.4
Verify: Phase 108 uses new threshold on next compilation
```

### Category 5: Performance & Overhead

**Test 5.1: Hook Invocation Overhead**
```
Baseline (no hooks): 100ms compilation
With hooks: 100ms + overhead
Target overhead: < 5ms total (< 5%)

Measurement: Time each hook invocation
Verify: onPostLex + onPostParse + ... < 5ms
```

**Test 5.2: Signal Collection Performance**
```
Large file (50KB, 5000 functions)
Signal collection should not block compilation
Target: < 1ms per signal collection

Verify: Signal collection < 1% compilation time
```

**Test 5.3: Decision Logic Performance**
```
Complex decision (learner + threshold + budget checks)
Target: < 2ms per decision

Verify: Decision logic fast enough for real-time use
```

**Test 5.4: Memory Overhead**
```
Baseline memory usage: X MB
With Phase 108: X + Y MB
Target overhead: < 5% (Y < 0.05 * X)

Verify: Hook structures minimal footprint
```

### Category 6: Error Handling

**Test 6.1: Missing Phase 107 Learner**
```
Scenario: Phase 107 learner not initialized
Expected: Phase 108 continues without learner
Result: Fallback decision logic used
```

**Test 6.2: Budget Underestimate**
```
Scenario: Compilation takes longer than estimated budget
Expected: skipExpensivePasses triggered gracefully
Result: No crash, compilation completes
```

**Test 6.3: Invalid Signal Data**
```
Scenario: AST node count = -1 (impossible value)
Expected: Data validation catches error
Result: Sanitized to 0 or rejected
```

**Test 6.4: Hook Not Registered**
```
Scenario: Custom hook called but not registered
Expected: No-op or default behavior
Result: Compilation continues normally
```

### Category 7: Verbose Logging Validation

**Test 7.1: Verbose Output Content**
```
Expected log entries:
[Phase108] Starting compilation: file.c
[Phase108] PostLex: 25 tokens
[Phase108] PostParse decision: Enable IR optimization
[Phase108] PreOptSelect: Budget check passed (250ms remaining)
[Phase108] Compilation complete (125ms total)
```

**Test 7.2: Log Accuracy**
```
Verify: Log values match actual metrics
- tokenCount in log = actual token count
- budgetMs in log = remaining budget
- compileTime in log = actual elapsed time
```

**Test 7.3: No Log Overhead**
```
Measurement: With verbose logging enabled
Target overhead: < 10% additional time
Verify: Logging doesn't dominate compilation time
```

## Validation Test Suite

### Test File 1: Simple Programs (Baseline)

**test_phase108_simple.c**: 5 small functions
- Validates basic signal collection
- Expected compile time: ~50ms
- Hook invocations: 12 (all hooks)

**test_phase108_medium.c**: 15 medium functions
- Validates budget tracking across phases
- Expected compile time: ~100ms
- Hook invocations: 12-14 (some skipped)

**test_phase108_large.c**: 50+ large functions
- Validates skipExpensivePasses decision
- Expected compile time: ~150ms
- Hook invocations: 12 (PreOptSelect → skip)

**test_phase108_complex.c**: Many branches, loops, recursion
- Validates function analysis signal
- Expected compile time: ~120ms
- Hook invocations: 12 (all)

### Test File 2: Integration Tests

**test_phase108_phase109_integration.c**: Signals → profiles
- Compile same file 5 times
- Verify Phase 109 receives all signals
- Verify profiles update correctly
- Expected: 5 × 100ms = 500ms total

**test_phase108_learner_integration.c**: With Phase 107
- Initialize Phase 107 learner
- Verify Phase 108 consults learner
- Verify learner adjusts thresholds
- Expected: 150ms + learner overhead

## Validation Checklist

### Checklist 1: Signal Collection
- [ ] PreParse signal collected
- [ ] PostLex signal collected (token count accurate)
- [ ] PostParse signal collected (AST metrics accurate)
- [ ] PostConstFold signal collected
- [ ] PostFuncAnalysis signal collected
- [ ] PreOptSelect signal collected
- [ ] PostInlineSelect signal collected
- [ ] PostCallGraph signal collected
- [ ] PostIRBuild signal collected
- [ ] PreIROpt signal collected
- [ ] PostCodeGen signal collected
- [ ] PostAsmOpt signal collected

### Checklist 2: Decision Logic
- [ ] Budget tracking accurate
- [ ] Remaining budget calculated correctly
- [ ] skipExpensivePasses decision correct
- [ ] enableOptimizations decision correct
- [ ] inlineFunctions decision correct
- [ ] Fallback strategy when data insufficient
- [ ] Learner integration works
- [ ] Decision rationale generated

### Checklist 3: Hook Integration
- [ ] All 12 hooks invoked in correct order
- [ ] Hook decisions propagate correctly
- [ ] Early exit handled properly
- [ ] Hook registration works
- [ ] Custom hooks can be registered
- [ ] Verbose logging works

### Checklist 4: Phase 109 Integration
- [ ] Signals sent to Phase109EffectivenessCollector
- [ ] Decisions recorded for feedback
- [ ] Profiles updated from signals
- [ ] Thresholds adjusted based on profiles
- [ ] Next compilation uses updated thresholds

### Checklist 5: Performance
- [ ] Hook overhead < 5% (< 5ms for 100ms compile)
- [ ] Signal collection < 1ms each
- [ ] Decision logic < 2ms each
- [ ] Total hook time < 10ms
- [ ] Memory overhead < 5%
- [ ] Verbose logging < 10% overhead

### Checklist 6: Error Handling
- [ ] Handles missing learner gracefully
- [ ] Handles budget overrun gracefully
- [ ] Validates signal data
- [ ] Handles unregistered hooks
- [ ] Recovers from errors without crashing

### Checklist 7: Documentation
- [ ] All hooks documented
- [ ] Signal types documented
- [ ] Decision types documented
- [ ] Budget strategy documented
- [ ] Integration points documented
- [ ] Verbose logging format documented

## Execution Plan

### Phase 108.4.1: Unit Tests
```bash
cd src/test-resources
../scripts/test_compiler.sh test_phase108_simple.c
../scripts/test_compiler.sh test_phase108_medium.c
../scripts/test_compiler.sh test_phase108_large.c
../scripts/test_compiler.sh test_phase108_complex.c
```

### Phase 108.4.2: Integration Tests
```bash
../scripts/test_compiler.sh test_phase108_phase109_integration.c
../scripts/test_compiler.sh test_phase108_learner_integration.c
```

### Phase 108.4.3: Overhead Measurement
```bash
# Time compilation with and without hooks
time cc45 test.c -o test.prg                    # Baseline
time cc45 test.c -o test.prg --phase108-hooks   # With hooks
# Calculate overhead percentage
```

### Phase 108.4.4: Verbose Output Validation
```bash
cc45 test.c --verbose-phase108 2>&1 | grep "\[Phase108\]"
# Verify all expected log entries present
# Verify metrics in logs match actual values
```

## Expected Results

### Compilation Performance
- Simple files: 50ms baseline → 51-52ms with hooks (2-4% overhead)
- Medium files: 100ms baseline → 102-105ms with hooks (2-5% overhead)
- Large files: 150ms baseline → 151-157ms with hooks (1-5% overhead)

### Hook Coverage
- All 12 hooks invoked: 100% ✓
- All hooks record correct metrics: 100% ✓
- All hooks make correct decisions: 100% ✓

### Integration Quality
- Phase 109 receives all signals: 100% ✓
- Signals accurately recorded: 100% ✓
- Profiles update correctly: 100% ✓

### Error Resilience
- Handles missing learner: ✓
- Handles budget overrun: ✓
- Validates data: ✓
- Recovers from errors: ✓

## Success Criteria

- ✅ All 12 hook points functional and tested
- ✅ Signal collection accurate (>99% agreement with actual metrics)
- ✅ Decision logic correct (>95% accuracy on test cases)
- ✅ Hook overhead < 5% of compilation time
- ✅ Phase 109 integration working (signals → profiles → decisions)
- ✅ Error handling robust (no crashes on edge cases)
- ✅ Verbose logging accurate and helpful
- ✅ Zero regressions (all existing tests still pass)
- ✅ Documentation complete and accurate

## References

- **Phase 108.1**: Hook infrastructure and signal collection
- **Phase 108.2**: cc45_main integration
- **Phase 108.3**: Decision logic and learner integration
- **Phase 109**: Adaptive optimization tuning (consumer of signals)
- **Phase 107**: Online learner (consulted by decision logic)
