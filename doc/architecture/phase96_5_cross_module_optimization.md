# Phase 96.5: Cross-Module Pointer Field Optimization — Design Guide

**Document Version:** 1.0  
**Date:** 2026-08-21  
**Status:** Infrastructure Design Complete  
**Related:** `CLAUDE.md`, `cross-module-optimization.md`, `phase96_4_3_implementation_guide.md`

---

## Executive Summary

Phase 96.5 extends Phase 96 (Extended Striped Arrays) with cross-module pointer field caching optimization. By analyzing pointer field access patterns across translation unit boundaries, the compiler can:

- **Identify high-value pointer fields** that appear in multiple TUs
- **Detect optimization patterns** (sequential access, loop nesting, reuse)
- **Cache pointer fields in ZP registers** ($60-$70) to eliminate redundant offset calculations
- **Reduce code size by 5-10%** on multi-TU programs
- **Improve cache hit rates** through intelligent register allocation

---

## Architecture Overview

### High-Level Pipeline

```
Phase 1: Recording (IR Generation Phase)
    ├─ IRCodeGen emits field access instructions
    ├─ GlobalPointerFieldDatabase records accesses
    └─ InterTUPatternDetector records field sequences

Phase 2: Analysis (Optimization Phase)
    ├─ GlobalPointerFieldDatabase analyzes patterns
    ├─ InterTUPatternDetector detects cross-module patterns
    ├─ Caching analyzer (Phase 96.5.3) computes register assignments
    └─ IPOAnalyzer coordinates with Phase 91

Phase 3: Linking (Linker Phase)
    ├─ CrossModuleOptimizer (Phase 91.4) extended
    ├─ Field-aware dispatchers generated
    ├─ Assembly hints emitted
    └─ AssemblerOptimizer (Phase 95.5) applies optimizations

Phase 4: Assembly (Assembler Phase)
    └─ AssemblerOptimizer uses caching hints (Phase 96.4.3)
```

### Component Hierarchy

```
GlobalPointerFieldDatabase (Phase 96.5.1)
├─ PointerFieldProfile[]     (per field, per TU)
├─ StructPointerFieldSet[]   (per struct)
└─ Methods:
    ├─ recordFieldAccess()
    ├─ recordFieldInLoop()
    ├─ recordSequentialAccess()
    ├─ analyzePatterns()
    └─ getOptimizationCandidates()

InterTUPatternDetector (Phase 96.5.2)
├─ functionFieldAccesses[]   (field accesses per function)
├─ detectedPatterns[]        (pattern instances)
└─ Methods:
    ├─ recordFieldAccess()
    ├─ detectPatterns()
    └─ getHighValuePatterns()

FieldCachingAnalyzer (Phase 96.5.3 - TBD)
├─ registerPressureAnalyzer()
├─ lifetimeAnalyzer()
└─ Methods:
    ├─ computeRegisterAllocation()
    └─ computeInvalidationStrategy()

CrossModuleOptimizer (Phase 91.4 - Extended)
└─ Methods:
    ├─ analyzeFieldPatterns()
    ├─ generateFieldDispatchers()
    └─ emitFieldCachingHints()
```

---

## Phase 96.5.1: Global Pointer Field Database

### Purpose

Track pointer field usage across all translation units and identify candidates for cross-module caching.

### Key Data Structures

```cpp
// Per-field profile (Phase 96.5.1)
struct PointerFieldProfile {
    std::string fieldName;                    // "vertices"
    std::string structName;                   // "Mesh"
    size_t fieldOffset;                       // Byte offset in struct
    size_t fieldSize;                         // Always 2 bytes (pointer)
    
    std::set<std::string> accessingFunctions; // Cross-TU access
    std::map<std::string, AccessPattern> patterns; // Per-function patterns
    
    bool isCacheCandidate;                    // Should be cached?
    int suggestedCacheRegister;               // ZP register ($60-$70)
    double estimatedSavings;                  // Bytes saved by caching
};
```

### Recording Phase

When IRCodeGen generates field access instructions:

```cpp
// Example: mesh[i][j].vertices = ...
database.recordFieldAccess("process_mesh",    // Function
                          "Mesh",             // Struct
                          "vertices",         // Field
                          0x04,               // Offset in struct
                          AccessType::Write); // Operation type
```

### Analysis Phase

```cpp
database.analyzePatterns();  // Compute cache candidates, savings
auto candidates = database.getOptimizationCandidates();
// Returns fields ranked by estimated savings
```

### Cache Candidate Criteria

A field is a cache candidate if:
1. **Multi-function access**: Used in 2+ different functions (cross-TU indicator)
2. **High frequency**: More than 3 accesses in the same function
3. **Loop-nested**: Accessed inside loops (significant savings)
4. **Sequential pattern**: Often accessed with other fields

---

## Phase 96.5.2: Inter-TU Pattern Detector

### Purpose

Detect specific pointer field access patterns across module boundaries that can benefit from caching.

### Pattern Types

#### 1. Sequential Field Access
```c
// Pattern in Function A:
v = mesh[i][j].vertices;  // Access pointer field
x = v->x;                 // Dereference

// Optimization: Cache mesh[i][j].vertices in ZP register
```

**Savings**: 10-15 bytes (offset calc eliminated)

#### 2. Loop-Nested Access
```c
// Pattern in Function B:
for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
        process(sprite[y][x].vertices);  // Accessed in nested loops
    }
}

// Optimization: Cache in loop (outer loop level)
```

**Savings**: 25+ bytes (repeated calculations eliminated)

#### 3. Field Reuse
```c
// Pattern in Function C:
p = data[i].ptr;        // Access 1
v = data[i].ptr;        // Access 2 (reuse same field)
w = data[i].ptr;        // Access 3

// Optimization: Load once, reuse across accesses
```

**Savings**: 8+ bytes per reuse

#### 4. Pointer Dereference
```c
// Pattern in Function D:
ptr = array[i].pointer;  // Load pointer field
val = ptr->value;        // Use loaded pointer
next = ptr->next;        // Reuse loaded pointer

// Optimization: Keep pointer in register
```

**Savings**: 6-12 bytes

#### 5. Alias Chain
```c
// Pattern across TUs:
// TU1: global_mesh[x].vertices
// TU2: mesh_copy[x].vertices
// TU3: sprite_data[x].vertices

// All alias to same underlying pointer field
// Optimization: Coordinate caching across TUs
```

**Savings**: 5+ bytes per TU

### Pattern Detection

```cpp
InterTUPatternDetector detector;

// Recording phase
detector.recordFieldAccess("func1", "Mesh", "vertices", true, false);
detector.recordFieldAccess("func1", "Mesh", "vertices", false, true);

// Analysis phase
detector.detectPatterns();

// Query results
auto patterns = detector.getHighValuePatterns(5.0);  // >= 5 bytes savings
for (const auto& p : patterns) {
    printf("Pattern: %s, Savings: %.1f bytes\n", 
           p.description.c_str(), p.savingsEstimate);
}
```

---

## Phase 96.5.3: Field Caching Analysis (TBD)

### Overview

Determines optimal cache register allocation and invalidation strategy.

### Key Components (Planned)

1. **Register Pressure Analyzer**
   - Analyze ZP register availability per function
   - Coordinate with existing register allocation
   - Handle register reuse across function calls

2. **Lifetime Analyzer**
   - Compute field pointer lifetime (load to last use)
   - Determine cache scope (function vs loop level)
   - Handle nested function calls

3. **Cost-Benefit Calculator**
   ```
   Benefit = (num_accesses - 1) * (load_cost - register_cost)
   Cost = cache_setup + cache_register + invalidation_cost
   Optimize if: Benefit > Cost + overhead
   ```

4. **Invalidation Strategy Planner**
   - Determine when to invalidate cache
   - Handle function calls (conservative: always invalidate)
   - Handle memory writes (precise invalidation)

### Cache Register Allocation

- **Reserved region**: ZP $60-$70 (5 registers)
- **Allocation strategy**: Highest-savings-first
- **Register size**: 2 bytes per cached pointer

Example allocation:
```
$60-$61: mesh[16][16].vertices
$62-$63: sprite.data
$64-$65: global_buffer.ptr
$66-$67: image.pixels
$68-$69: (reserved for Phase 95 needs)
```

---

## Phase 96.5.4: Linker-Level Optimization (TBD)

### Overview

Coordinate field caching across all compiled modules and emit optimization directives.

### Key Tasks

1. **Cross-Module Field Merger**
   - Merge field profiles from all `.o45` objects
   - Detect global patterns
   - Compute final cache decisions

2. **Dispatcher Generation** (extends Phase 91.4)
   - Generate field-aware dispatchers for virtual calls
   - Cache pointer across virtual dispatch
   - Handle polymorphic access

3. **Assembly Hint Emission**
   - Emit field caching annotations in linked assembly
   - Pass hints to assembler optimizer
   - Coordinate with Phase 95.5

4. **Linker Coordination**
   - Extend CrossModuleOptimizer from Phase 91.4
   - Integrate with specialization analysis
   - Handle cross-TU pointer aliasing

---

## Phase 96.5.5: Validation & Benchmarking (TBD)

### Test Coverage

```
Phase 96.5 Tests
├── Unit Tests (15-20)
│   ├── GlobalPointerFieldDatabase
│   ├── InterTUPatternDetector
│   └── FieldCachingAnalyzer
├── Integration Tests (10-15)
│   ├── Multi-TU compilation
│   ├── Cross-module patterns
│   └── Linker coordination
└── Benchmarking (5-10)
    ├── Code size reduction
    ├── Performance on MEGA65
    └── Cache hit rate measurement
```

### Benchmarking Metrics

- **Code size reduction**: Target 5-10% on multi-TU programs
- **Cache hit rate**: Target >80% for cached fields
- **Compilation time overhead**: <5% increase
- **Memory usage**: <10MB for database

### Test Programs (Planned)

1. **simple_multi_tu.c** - Basic two-TU program with pointer field access
2. **array_stripe_cross_tu.c** - Striped array access across TUs
3. **nested_loop_field_cache.c** - Loop-nested field access optimization
4. **virtual_dispatch_field.c** - Virtual function calls with field caching
5. **complex_cross_module.c** - Comprehensive multi-TU benchmark

---

## Integration with Existing Phases

### Phase 91 (Cross-Module Optimization)

**Reused Components**:
- IPOProfiler infrastructure
- GlobalFunctionDatabase pattern
- Dispatcher generation framework

**New Extensions**:
- Field profiling in IPOProfiler
- Field-aware specialization decisions
- Field caching dispatch hints

**Integration Points**:
```cpp
// In IPOProfiler (Phase 91.1)
void recordFieldAccess(function, struct, field, type);

// In IPOAnalyzer (Phase 91.2)
void analyzeFieldCachingOpportunities();

// In CrossModuleOptimizer (Phase 91.4)
void generateFieldCachingDispatchers();
```

### Phase 96.1-96.4 (Extended Striped Arrays)

**Reused Components**:
- Pointer field detection infrastructure (Phase 96.2)
- Instruction transformation patterns (Phase 96.4)
- Field offset calculations (Phase 95.2)

**New Extensions**:
- Cross-module field pattern analysis
- Inter-TU offset calculation optimization
- Virtual dispatch coordination

### Phase 95 (Field Offset Caching)

**Reused Components**:
- Assembler optimizer integration point (Phase 95.5)
- Field metadata directives (Phase 95.1)
- Cache hint framework

**New Extensions**:
- Cross-module caching hints
- Register allocation coordination
- Field pattern annotations

---

## API Reference

### GlobalPointerFieldDatabase

```cpp
namespace phase96_5 {

class GlobalPointerFieldDatabase {
    static GlobalPointerFieldDatabase& instance();
    
    void recordFieldAccess(const std::string& functionName,
                          const std::string& structName,
                          const std::string& fieldName,
                          size_t fieldOffset,
                          AccessType accessType);
    
    void recordFieldInLoop(const std::string& functionName,
                          const std::string& structName,
                          const std::string& fieldName,
                          int loopDepth);
    
    void recordSequentialAccess(const std::string& functionName,
                               const std::string& structName,
                               const std::string& field1,
                               const std::string& field2);
    
    void analyzePatterns();
    
    std::vector<std::shared_ptr<PointerFieldProfile>>
        getOptimizationCandidates() const;
    
    std::vector<std::string> getHotStructs() const;
    std::vector<std::string> getGlobalAliasChains() const;
    
    void clear();
};

}  // namespace phase96_5
```

### InterTUPatternDetector

```cpp
namespace phase96_5 {

class InterTUPatternDetector {
    void recordFieldAccess(const std::string& functionName,
                          const std::string& structName,
                          const std::string& fieldName,
                          bool isLoad,
                          bool isDereference);
    
    void detectPatterns();
    
    std::vector<DetectedPattern> getDetectedPatterns() const;
    std::vector<DetectedPattern> getHighValuePatterns(
        double minSavings = 5.0) const;
    
    void printReport(std::ostream& out) const;
};

}  // namespace phase96_5
```

---

## Performance Characteristics

### Database Performance

| Operation | Complexity | Time |
|-----------|-----------|------|
| recordFieldAccess | O(1) | <1ms per access |
| recordFieldInLoop | O(1) | <1ms per access |
| analyzePatterns | O(n) | ~100ms for 1000 fields |
| getOptimizationCandidates | O(n log n) | ~50ms for sorting |

### Memory Usage

| Data Structure | Size (per 1000 fields) |
|---|---|
| PointerFieldProfile[] | ~50KB |
| AccessPattern[] | ~30KB |
| StructPointerFieldSet[] | ~20KB |
| **Total** | **~100KB** |

---

## Known Limitations

### Current (Phase 96.5.1-96.5.2)

1. **Module Tracking**: Limited (uses "unknown")
   - Will improve with Phase 91 integration
   
2. **Loop Depth**: Placeholder implementation
   - Needs IRBuilder loop tracking integration

3. **Register Allocation**: Simple linear
   - Will improve with Phase 96.5.3

### Planned (Phase 96.5.3+)

1. **Cache Invalidation**: Conservative (always invalidate on calls)
   - Will refine with lifetime analysis

2. **Register Pressure**: Limited analysis
   - Will add detailed pressure tracking

3. **Linker Integration**: Not yet implemented
   - Planned for Phase 96.5.4

---

## Future Enhancements

### Immediate (v1.0.10)
- Hardware validation on MEGA65
- Benchmark collection
- Documentation updates

### Short-Term (v1.0.11)
- Pointer field caching v2 improvements
- Control flow integration for patterns
- Register pressure optimization

### Long-Term (v1.1+)
- Adaptive instruction transformation
- Hardware profiling (cycle counts)
- Machine learning-based pattern detection

---

**Document Version**: 1.0  
**Last Updated**: 2026-08-21  
**Status**: Design Complete, Implementation In Progress
