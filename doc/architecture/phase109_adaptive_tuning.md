# Phase 109: Adaptive Optimization Tuning

**Status**: DESIGN & SPECIFICATION (2026-08-21)  
**Phase**: 109 (Adaptive Optimization Tuning)  
**Purpose**: Collect hook data to build optimization effectiveness profiles  

## Overview

Phase 109 extends Phase 108's hook system to collect effectiveness metrics across multiple compilations and dynamically adjust optimization thresholds based on observed patterns.

### Vision

```
Phase 108 (Hooks)           Phase 109 (Adaptive)
  ↓                           ↓
Collect signals         →  Analyze effectiveness
Make decisions          →  Build profiles
                        →  Adjust thresholds
                        →  Learn patterns
```

## Architecture

### Data Collection Pipeline

```
Compilation → Phase 108 Hooks
    ↓
Collect Signals & Decisions
    ↓
Phase109EffectivenessCollector
    ├─ Signal storage
    ├─ Decision tracking
    └─ Output metrics
    ↓
Effectiveness Database
    ├─ Per-optimization metrics
    ├─ Pattern analysis
    └─ Threshold profiles
    ↓
Phase109AdaptiveSelector
    ├─ Effectiveness scoring
    ├─ Threshold adjustment
    └─ Per-file recommendations
```

## Phase 109 Components (Planned)

### 1. Phase109EffectivenessCollector (250+ lines)

**Purpose**: Collect hook data and compilation results

```cpp
class Phase109EffectivenessCollector {
    // Collect signal-decision pairs during compilation
    void recordSignal(const CompilationSignal& signal,
                      const HookDecision& decision);

    // Record final metrics after compilation
    void recordCompilationResult(
        const std::string& sourcefile,
        int finalAssemblySize,
        double compilationTimeMs,
        const std::vector<OptimizationApplied>& optimizations);

    // Get effectiveness metrics for an optimization
    OptimizationEffectiveness getEffectiveness(
        const std::string& optimizationName);

    // Store to database
    void persistToDatabase();
};
```

**Data Stored**:
- Source file characteristics (size, complexity)
- Compilation environment (budget, optimization level)
- Decisions made at each hook
- Final metrics (assembly size, compilation time)
- Optimization effectiveness (speedup, size reduction)

### 2. Phase109ProfileDatabase (300+ lines)

**Purpose**: Maintain effectiveness profiles for optimizations

```cpp
class Phase109ProfileDatabase {
    // Record single optimization application
    void recordOptimizationApplication(
        const std::string& optimization,
        const CompilationContext& context,
        const OptimizationResults& results);

    // Get effectiveness profile for optimization
    OptimizationProfile getProfile(
        const std::string& optimization);

    // Aggregate statistics across compilations
    struct AggregateStats {
        double avgSpeedup;          // Average % speedup
        double avgSizeReduction;    // Average % size reduction
        double avgCompileTimeCost;  // Average ms overhead
        int applicationCount;       // Times applied
        double successRate;         // % beneficial applications
    };
    AggregateStats getAggregateStats(const std::string& opt);
};
```

**Profiles Track**:
- Effectiveness per optimization
- Context-dependent effectiveness (file size, complexity)
- Cost-benefit ratios
- Success rates (when beneficial vs harmful)

### 3. Phase109ThresholdAdjuster (200+ lines)

**Purpose**: Dynamically adjust decision thresholds

```cpp
class Phase109ThresholdAdjuster {
    // Analyze effectiveness trends
    void analyzeTrends();

    // Adjust threshold for optimization
    void adjustThreshold(
        const std::string& optimization,
        double newThreshold);

    // Get recommended threshold
    double getRecommendedThreshold(
        const std::string& optimization);

    // Apply adjustments to Phase 108 decision logic
    void applyAdjustmentsToDecisions();
};
```

**Adjustment Strategy**:
1. Collect 10+ compilation data points
2. Calculate aggregate effectiveness
3. Adjust threshold based on success rate
4. If > 80% beneficial: lower threshold (apply more)
5. If < 50% beneficial: raise threshold (apply less)

### 4. Phase109PatternAnalyzer (250+ lines)

**Purpose**: Identify patterns in optimization effectiveness

```cpp
class Phase109PatternAnalyzer {
    // Analyze file size impact on optimization effectiveness
    void analyzeFileSize();

    // Analyze complexity impact
    void analyzeComplexity();

    // Identify file type patterns
    void analyzeFileTypes();

    // Generate recommendations
    struct Pattern {
        std::string condition;      // "file_size < 5KB"
        std::string optimization;   // "inlining"
        double effectiveness;       // Avg % improvement
        int confidence;             // 1-100 (data point count)
    };
    std::vector<Pattern> getPatterns();
};
```

**Patterns Detected**:
- Small files: Disable expensive optimizations
- Large files: Apply aggressive optimizations
- High complexity: Skip CSE, focus on loop opts
- Low complexity: Apply all optimizations

### 5. Phase109AdaptiveSelector (200+ lines)

**Purpose**: Make per-file optimization recommendations

```cpp
class Phase109AdaptiveSelector {
    // Analyze input file characteristics
    FileCharacteristics analyzeFile(const std::string& source);

    // Get recommended optimization level
    struct Recommendation {
        OptimizationLevel level;    // -O0 to -O9
        std::vector<std::string> enabledOpts;
        std::vector<std::string> disabledOpts;
        double confidenceScore;     // Based on data volume
    };
    Recommendation getRecommendation(
        const FileCharacteristics& fileChar);

    // Learn from compilation results
    void learnFromResult(
        const FileCharacteristics& fileChar,
        const CompilationResults& results);
};
```

## Data Flow

```
Input File
    ↓
Phase109AdaptiveSelector.getRecommendation()
    ↓
File Characteristics
    ├─ Size
    ├─ Function count
    ├─ Loop count
    └─ Branch density
    ↓
Pattern Matching
    ├─ Find similar past compilations
    ├─ Retrieve effectiveness profiles
    └─ Generate recommendations
    ↓
Optimization Strategy
    ├─ Which optimizations to enable
    ├─ Which to disable
    └─ Budget allocation
    ↓
Phase 108 Hooks (apply recommendations)
    ↓
Compilation with adaptive strategy
```

## Learning Cycle

```
Day 1: Collect baseline data
  ├─ 50 compilations
  └─ Build initial profiles

Day 2-7: Refine and adapt
  ├─ Analyze patterns from Day 1
  ├─ Adjust thresholds
  ├─ Generate recommendations
  └─ Collect validation data

Week 2+: Production optimization
  ├─ Apply recommendations
  ├─ Measure effectiveness
  ├─ Update profiles
  └─ Continuous improvement
```

## Implementation Plan

### Phase 109.1: Effectiveness Collector (250 lines)
- Signal-decision pair recording
- Compilation result tracking
- Database persistence

### Phase 109.2: Profile Database (300 lines)
- Aggregate statistics
- Effectiveness profiles
- Context-dependent analysis

### Phase 109.3: Threshold Adjuster (200 lines)
- Trend analysis
- Automatic threshold tuning
- Confidence scoring

### Phase 109.4: Pattern Analyzer (250 lines)
- File size impact
- Complexity impact
- Pattern detection

### Phase 109.5: Adaptive Selector (200 lines)
- Per-file recommendations
- Learning integration
- Confidence-based selection

### Phase 109.6: Integration (150 lines)
- Wire into cc45_main
- Collect training data
- Validate recommendations

### Phase 109.7: Validation (300+ lines)
- Test on real workloads
- Measure improvement
- Edge case handling

## Expected Improvements

### Compilation Time
- 5-10% improvement in average compilation time
- Avoid expensive optimizations on small files
- Skip passes when budget-constrained

### Code Quality
- 2-5% average code size reduction
- 3-7% average performance improvement
- Better optimization for specific file types

### Decision Quality
- 80%+ accuracy in per-file recommendations
- Confidence scoring prevents bad decisions
- Fallback to conservative when uncertain

## Success Criteria

- [ ] Effectiveness collector working (data saved)
- [ ] Profile database populated (50+ compilations)
- [ ] Pattern analysis showing clear trends
- [ ] Threshold adjustment improving decisions
- [ ] Per-file recommendations 80%+ accurate
- [ ] Compilation time stable (< 2% variance)
- [ ] Code quality improved (size or speed)
- [ ] Zero regressions on test suite

## Integration Points

**Phase 108**: Hook data collection
- Signals collected by hooks
- Decisions tracked per compilation
- Results feedback to effectiveness collector

**cc45_main**: Adaptive strategy selection
- Load recommendations before compilation
- Apply recommended optimizations
- Track results for learning

**Phase 107**: Learner feedback
- Online learner benefits from collected data
- Historical effectiveness guides learner
- Adaptive selector improves learner decisions

## Data Storage

### Compilation Log
```json
{
  "file": "test.c",
  "date": "2026-08-21",
  "size": 5000,
  "complexity": 42,
  "hooks": [
    {"point": "PreOptSelect", "decision": "enable"},
    {"point": "PreIROpt", "decision": "skip"}
  ],
  "results": {
    "assemblySize": 8192,
    "compileTimeMs": 120,
    "optimizationsApplied": ["cse", "licm"]
  }
}
```

### Profile Data
```json
{
  "optimization": "cse",
  "applications": 150,
  "successful": 120,
  "avgSpeedup": 3.5,
  "avgSizeReduction": 2.1,
  "avgCost": 25.0,
  "contexts": [
    {
      "fileSize": "small",
      "successRate": 0.65
    },
    {
      "fileSize": "large",
      "successRate": 0.92
    }
  ]
}
```

## Timeline

- **Week 1**: Phase 109.1-109.4 implementation
- **Week 2**: Phase 109.5-109.6 integration
- **Week 3**: Phase 109.7 validation
- **Month 2**: Tuning and refinement
- **Month 3**: Production deployment

## References

- Phase 108: Hook infrastructure
- Phase 107.1: Online learner
- Phase 105-106: Benchmarking harness (can reuse)
