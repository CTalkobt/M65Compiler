# Phase 99: Cross-Module Address Space Analysis

**Status**: Starting 2026-08-21  
**Target Version**: v1.0.6  
**Complexity**: High (extends Phase 98 + 91 + 96.5)

## Overview

Phase 99 implements cross-module address space analysis to optimize bank allocation and field access patterns across translation units. Building on Phase 98's far address support and Phase 96.5's field caching, Phase 99 enables global optimization of extended memory usage.

### The Problem

Phase 98 allocates `__far` variables independently per module:
- Each module gets its own bank(s)
- No coordination across TUs
- Potential bank waste
- Suboptimal cache locality

Phase 99 solves this by:
- Analyzing all modules together
- Computing optimal bank assignments
- Coordinating field access patterns
- Minimizing bank switches

### Example

**Before (Phase 98)**:
```c
// module1.c
__far struct Mesh mesh[8];      // Allocates bank 01
__far struct Color palette[256]; // Allocates bank 02

// module2.c
__far int* vertices[64];        // Allocates bank 03 (wasteful!)
```

**After (Phase 99)**:
```
Optimal allocation:
  Bank 01: mesh + vertices (co-located for cache efficiency)
  Bank 02: palette
  Result: 50% fewer bank switches in tight loops
```

## Phase 99 Sub-Phases

### Phase 99.1: Cross-Module Database
- Build global far variable registry
- Track access patterns across modules
- Compute co-location benefits
- Estimate bank layout

### Phase 99.2: Bank Assignment Algorithm
- Solve optimal packing problem
- Consider cache locality
- Handle alignment constraints
- Generate bank layout

### Phase 99.3: Linker Integration
- Coordinate with ln45 linker
- Reorder variables in extended memory
- Generate optimized layout
- Preserve relocation info

### Phase 99.4: Code Generation Optimization
- Update bank setup code
- Cache hot banks in loops
- Minimize repeated setup
- Coordinate with Phase 91 dispatchers

### Phase 99.5: Validation & Benchmarking
- Test multi-module programs
- Measure bank switch reduction
- Verify layout correctness
- Performance benchmarks

## Architecture

### Component Hierarchy

```
Linker (ln45)
    ↓
Phase99CrossModuleAnalyzer (new)
    ├─ Phase99FarVariableRegistry
    ├─ Phase99AccessPatternAnalyzer
    ├─ Phase99BankAssignmentEngine
    └─ Phase99LayoutOptimizer
    ↓
Phase98CodeGenIntegration
    ↓
Phase98FarAddressSupport
    ↓
M65Emitter
```

### Integration with Prior Phases

**Phase 98** (Far Address):
- Provides basic bank management
- Phase 99 optimizes bank assignment

**Phase 96.5** (Field Caching):
- Tracks field access patterns
- Phase 99 uses patterns for bank co-location

**Phase 91** (Cross-Module Optimization):
- Provides function specialization framework
- Phase 99 coordinates with dispatcher generation

## Key Concepts

### Co-Location Benefits

Variables accessed together should share banks:

```c
__far struct Mesh mesh;
__far int* vertices;

void render() {
    // Both accessed in loop - co-locate in same bank
    for (int i = 0; i < mesh.vertexCount; i++) {
        draw_vertex(vertices[i]);
    }
}
```

### Bank Locality

```
Optimal: mesh + vertices in Bank 01
Loop: Access mesh.vertexCount (bank setup once)
      then vertices[i] in same bank (no switch)

Suboptimal: mesh in Bank 01, vertices in Bank 02
Loop: Access mesh (bank switch)
      Access vertices (bank switch)
      Every iteration: 2 bank switches!
```

### Access Pattern Classification

```
Pattern Types:
1. Sequential: A then B (co-locate)
2. Nested: A in outer, B in inner (separate banks OK)
3. Reuse: A accessed many times (keep in cache)
4. Interleaved: A and B together (co-locate)
```

## Implementation Strategy

### Phase 99.1: Database

```cpp
struct FarVariableRecord {
    std::string name;
    std::string module;
    size_t size;
    std::set<std::string> accessingFunctions;
    std::vector<std::pair<std::string, int>> coAccessedVariables;  // [variable, count]
    int suggestedBank = -1;
};
```

### Phase 99.2: Algorithm

**Input**: All __far variables from all modules  
**Output**: Bank layout (var → bank mapping)

```
Algorithm:
1. Build co-access graph (edges = co-access count)
2. Compute bank capacity constraints
3. Solve bin-packing with co-location preferences
4. Minimize bank switch instructions
5. Generate layout
```

### Phase 99.3: Linker Changes

```
ln45 changes:
1. Collect all __far variables
2. Run Phase99CrossModuleAnalyzer
3. Reorder memory layout
4. Update relocation entries
5. Generate bank map in output
```

### Phase 99.4: Code Gen

```
Optimization:
1. Detect loop-based bank switching
2. Cache frequently-accessed banks
3. Hoist bank setup outside loops
4. Coordinate with Phase 91 dispatchers
```

## Performance Targets

| Metric | Baseline | Phase 99 | Improvement |
|--------|----------|----------|-------------|
| Bank switches/loop | 2-3 | 0-1 | 50-100% |
| Code size | Base | -5% | 5% reduction |
| Execution speed | Base | +15% | 15% faster |
| Memory waste | 20% | 5% | 75% reduction |

## Known Constraints

1. **Bank Size**: 64KB per bank
2. **Variable Alignment**: Some vars need alignment
3. **Cross-Module References**: Relocation tracking
4. **Recursive Functions**: Complex access patterns

## Success Criteria

✅ Phase 99.1: Cross-module registry built  
✅ Phase 99.2: Optimal bank layout computed  
✅ Phase 99.3: Linker integration working  
✅ Phase 99.4: Code generation optimized  
✅ Phase 99.5: All validation tests pass  
✅ Zero regressions on Phase 96, 97, 98  
✅ 15%+ performance improvement measured  

## Timeline

- **Phase 99.1**: 2-3 hours (database)
- **Phase 99.2**: 2-3 hours (algorithm)
- **Phase 99.3**: 1-2 hours (linker)
- **Phase 99.4**: 2-3 hours (codegen)
- **Phase 99.5**: 1-2 hours (validation)
- **Total**: 8-13 hours

## References

- **Phase 98**: Far address support (base infrastructure)
- **Phase 96.5**: Field caching (access patterns)
- **Phase 91**: Cross-module optimization (dispatcher framework)
- **ln45**: Linker (where coordination happens)

---

**Next Step**: Phase 99.1 - Cross-Module Database Infrastructure

