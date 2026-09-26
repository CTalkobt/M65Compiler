# Phase 108: Frontend Integration — Compiler Tuning Hooks

**Status**: IN PROGRESS (Design & Specification)  
**Phase**: 108 (Compiler Tuning Frontend Integration)  
**Target**: Wire Phase 107 tuning framework into actual compilation pipeline  
**Timeline**: Started 2026-08-21  

## Overview

Phase 108 integrates the Phase 107 runtime adaptation framework (Online Learner + Tuning Hooks) into the actual compiler frontend, enabling dynamic optimization selection during compilation.

### Goals

1. **Hook Integration Points** — Identify and wire hooks at 8-12 strategic compiler phases
2. **Decision Points** — Connect Phase 107.1 cost-benefit analysis to real optimization enable/disable
3. **Signal Collection** — Capture compilation signals at each hook point
4. **Feedback Loop** — Enable intra-compilation adaptation based on live profiling
5. **Zero-Cost Path** — Ensure no overhead when hooks are not active

## Architecture

### Compilation Pipeline with Hooks

```
Input: C Source Code
  ↓
[HOOK: PreParse] — Collect file statistics
  ↓
Lexer → [HOOK: PostLex] — Signal: token count, complexity
  ↓
Parser → [HOOK: PostParse] — Signal: AST size, nesting depth
  ↓
ConstantFolder → [HOOK: PostConstFold] — Signal: folding effectiveness
  ↓
FunctionAnalyzer → [HOOK: PostFuncAnalysis] — Signal: function profiles
  ↓
OptimizationSelector → [HOOK: PreOptSelect] — **DECISION POINT**
                          (should we run opts given budget?)
  ↓
InlineSelector → [HOOK: PostInlineSelect] — Signal: inlining candidates
  ↓
CallGraphAnalyzer → [HOOK: PostCallGraph] — Signal: call density
  ↓
IRBuilder → [HOOK: PostIRBuild] — Signal: IR size, complexity
  ↓
IROptimizer → [HOOK: PreIROpt] → **DECISION POINT**
                (skip expensive passes if over budget?)
  ↓
CodeGenerator → [HOOK: PostCodeGen] — Signal: assembly size
  ↓
AssemblerOptimizer → [HOOK: PostAsmOpt] — Signal: final code size
  ↓
Output: Assembly (or .prg)
```

### Hook Categories

**1. Signal Collection Hooks** (read-only)
- Post-lexer: token count, language feature distribution
- Post-parser: AST size, function count, nesting depth
- Post-constant-fold: folding effectiveness (% constants eliminated)
- Post-function-analysis: function profiles (complexity, callability)
- Post-IR-build: IR size, control flow complexity
- Post-code-gen: assembly size, memory pressure

**2. Decision Points** (modify optimization state)
- Pre-opt-select: Should we enable optimization selections?
- Pre-IR-opt: Should we enable IR-level optimizations?
- Pre-codegen: Should we enable aggressive code generation strategies?

**3. Control Hooks** (enable/disable passes)
- Disable inlining if over budget
- Skip expensive IR optimizations if compile time > 300ms
- Use size-optimized code generation if remaining budget < 100ms

## Implementation Plan

### Phase 108.1: Hook Point Infrastructure (200 lines)

**File**: `src/main/Phase108HookPoints.hpp`

```cpp
// Enum of all hook point identifiers
enum class HookPoint {
    PreParse,
    PostLex,
    PostParse,
    PostConstFold,
    PostFuncAnalysis,
    PreOptSelect,
    PostInlineSelect,
    PostCallGraph,
    PostIRBuild,
    PreIROpt,
    PostCodeGen,
    PostAsmOpt
};

// Signal capture at each hook
struct CompilationSignal {
    HookPoint phase;
    double compileTimeSoFar;  // ms
    int tokenCount;
    int astSize;
    int functionCount;
    int irSize;
    int assemblySize;
    // ... more metrics
};

// Hook registry binding
class HookRegistry {
    void registerCallback(HookPoint pt, std::function<void(const CompilationSignal&)> fn);
    void invokeHook(const CompilationSignal& signal);
};
```

### Phase 108.2: Frontend Integration (400 lines)

**File**: `src/main/Phase108FrontendIntegrator.hpp/cpp`

Modifies `cc45_main.cpp` compilation pipeline:

```cpp
class FrontendIntegrator {
    // Connect hooks to actual compiler phases
    void integrateHookPoints(cc45_main context);
    
    // Invoke hooks at each phase
    void invokePreParseHook();
    void invokePostLexHook(const std::vector<Token>& tokens);
    void invokePostParseHook(const AST& ast);
    void invokePreOptSelectHook();
    void invokePreIROptHook();
};
```

### Phase 108.3: Decision Logic (300 lines)

**File**: `src/main/Phase108DecisionEngine.hpp/cpp`

Connects Phase 107.1 learner decisions to optimization flags:

```cpp
class DecisionEngine {
    // Consult online learner for optimization decisions
    bool shouldEnableInlining(const CompilationContext& ctx);
    bool shouldEnableIROpts(const CompilationContext& ctx);
    bool shouldSkipExpensivePass(const std::string& passName);
    
    // Adjust optimization flags based on learner recommendations
    void applyLearnerDecisions(OptimizationFlags& flags);
};
```

### Phase 108.4: Signal Collection (250 lines)

**File**: `src/main/Phase108SignalCollector.hpp/cpp`

Captures metrics at each phase:

```cpp
class SignalCollector {
    void collectLexSignals(const Lexer& lexer);
    void collectParseSignals(const AST& ast);
    void collectFoldSignals(int constantsEliminated);
    void collectIRSignals(const IRModule& ir);
    
    CompilationSignal getCurrentSignal();
};
```

### Phase 108.5: Integration Testing (150 lines)

**File**: `src/test-resources/test_phase108_integration.c`

Test programs exercising hook integration:

```c
// Test 1: Large function with inlining decision
int recursive_func(int n) {
    if (n <= 1) return 1;
    return n * recursive_func(n - 1);
}

// Test 2: IR optimization budget constraint
int budget_test(int a, int b, int c) {
    return (a + b) * (b + c) + (c + a);  // Multiple CSE opportunities
}

// Test 3: Signal collection feedback
void signal_test() {
    for (int i = 0; i < 1000; i++) {
        // Loop that should trigger LICM decision
    }
}
```

## Integration Points in cc45_main.cpp

### After Lexing (line ~741)
```cpp
Lexer lexer(source);
std::vector<Token> tokens = lexer.tokenize();
// HOOK: PostLex
integrator.invokePostLexHook(tokens);
```

### After Parsing (line ~778)
```cpp
auto ast = parser.parse();
// HOOK: PostParse
integrator.invokePostParseHook(*ast);
```

### Before OptimizationSelector (line ~802)
```cpp
// HOOK: PreOptSelect
HookDecision decision = integrator.invokePreOptSelectHook();
if (!decision.enableOptimizations) {
    optimize = false;
    optimizationLevel = 0;
}
```

### Before IROptimizer (line ~1050+)
```cpp
IROptimizer optimizer(irOptFlags);
// HOOK: PreIROpt
HookDecision decision = integrator.invokePreIROptHook();
optimizer.setSkipExpensivePasses(!decision.enableIROpts);
```

### After CodeGenerator (line ~1150+)
```cpp
IRCodeGen codegen(...);
auto assembly = codegen.generate(ir);
// HOOK: PostCodeGen
integrator.invokePostCodeGenHook(assembly);
```

## Hook Decision Flow

```
CompilationSignal {
    phase = PreOptSelect,
    compileTimeSoFar = 45ms,
    astSize = 8340 bytes,
    functionCount = 12,
    ...
}
    ↓
OnlineLearner.analyzeCostBenefit()
    ↓
Score = (speedup + size_reduction) / cost
    ↓
if (score > 1.5) && (compilTime < 500ms)
    HookDecision { enableOptimizations = true }
else
    HookDecision { enableOptimizations = false }
    ↓
FrontendIntegrator applies decision to flags
```

## Configuration & Control

### Environment Variables

- `CC45_HOOKS_ENABLED=0` — Disable hook system (zero overhead path)
- `CC45_HOOKS_VERBOSE=1` — Print hook invocations and decisions
- `CC45_COMPILE_BUDGET=500` — Compilation time budget in ms (default: 500)
- `CC45_LEARNER_THRESHOLD=1.5` — Cost-benefit threshold (default: 1.5)

### Pragma Support

```c
#pragma cc45 hooks enable        // Enable hooks (default)
#pragma cc45 hooks disable       // Disable hooks (skip all)
#pragma cc45 hooks nolearn       // Skip learning, use defaults
#pragma cc45 no_hook <name>      // Disable specific hook
```

## Performance Characteristics

### Expected Overhead

- Per-hook invocation: 0.1-0.5ms
- Total hook overhead: < 5ms for typical compilation
- Zero overhead when disabled (CC45_HOOKS_ENABLED=0)

### Compilation Time Impact

| Scenario | Time | Overhead |
|----------|------|----------|
| Small file (< 1KB) | 50ms | +2ms (4%) |
| Medium file (5-10KB) | 200ms | +3ms (1.5%) |
| Large file (50KB+) | 1000ms | +5ms (0.5%) |

## Success Criteria

- [ ] All 8 hook points wired into cc45_main
- [ ] Phase 107.1 learner consulted at decision points
- [ ] Hook statistics tracked (invocation count, decisions changed)
- [ ] Zero compilation time regression (< 2%)
- [ ] 468/468 unit tests passing
- [ ] Hooks demonstrate 2-5% code size improvement

## Next Phase (Phase 109)

**Adaptive Optimization Tuning** — Use hook feedback to dynamically adjust thresholds:

- Collect hook effectiveness data over multiple compilations
- Build effectiveness profile (which optimizations help most)
- Automatic threshold tuning based on observed patterns
- Machine learning model for per-file optimization selection

## References

- Phase 107.1: Online Learning Engine — `Phase107OnlineLearner.hpp`
- Phase 107.2: Tuning Hooks — `Phase107TuningHooks.hpp`
- Compiler Main: `cc45_main.cpp` (lines 412-1300+)
