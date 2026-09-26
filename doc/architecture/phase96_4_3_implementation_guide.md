# Phase 96.4.3: Instruction Transformation - Implementation Guide

**Status**: Ready to Implement  
**Date**: 2026-08-21  
**Estimated Effort**: 1-2 hours, ~100-150 lines of code  
**Prerequisite**: Phase 96.4.2 (Pattern Matching) - COMPLETE ✅

---

## Overview

Phase 96.4.3 applies pointer field caching optimizations by transforming assembly instruction sequences. When consecutive pointer field accesses use the same row or column value, we can cache the offset calculation and reuse it, eliminating redundant computations.

### Key Insight

**Before optimization** (16 bytes per access):
```asm
ldy #row0           ; load row
ldx #col0           ; load col
jsr calc_offset     ; calculate: base + (row * width) + col
ldax offset         ; load pointer at [row0,col0]
...
ldy #row0           ; load same row
ldx #col1           ; load different col
jsr calc_offset     ; REDUNDANT: recalculate with same row
ldax offset         ; load pointer at [row0,col1]
```

**After optimization** (10-12 bytes for second access):
```asm
ldy #row0
ldx #col0
jsr calc_offset
ldax offset         ; first access
; CACHED: offset for row0
ldx #col1           ; only change column
; reuse cached row calculation
ldax offset         ; second access (saves ~6 bytes)
```

---

## Implementation Plan

### Phase 96.4.3 Tasks

#### Task 1: Build Optimization Map (20-30 lines)
**Goal**: Create a map of which instruction sequences to optimize

**Location**: `optimizeVariableSizeFieldOffsets()` function

**What to do**:
```cpp
// After detecting opportunities in Phase 96.4.2, build a map:
std::map<size_t, OptimizationInfo> optimizations; // stmt index -> info

struct OptimizationInfo {
    std::string arrayName;
    std::string fieldName;
    size_t firstLoadIdx;        // index of first LDAX/LDAY/LDAZ
    size_t secondLoadIdx;       // index of second LDAX/LDAY/LDAZ
    bool sameRow;               // true if rows match, false if cols match
    int cacheRegister;          // which register holds cached offset (A/X/Y)
};
```

**Algorithm**:
1. For each pair of consecutive pointer loads (LDAX/LDAY/LDAZ)
2. Check if they access same array
3. Determine if row or column is the same
4. Store optimization metadata
5. Mark statements as "optimizable"

#### Task 2: Mark Statements for Transformation (30-40 lines)
**Goal**: Tag assembly statements that should be optimized

**Location**: Inside `optimizeVariableSizeFieldOffsets()`

**What to do**:
```cpp
// For each optimization opportunity found:
for (const auto& [idx, opt] : optimizations) {
    // Mark intermediate statements for removal or modification
    parser->statements[opt.firstLoadIdx]->metadata["cached"] = true;
    parser->statements[opt.secondLoadIdx]->metadata["use_cache"] = true;
    
    // Store rewrite instructions
    Statement* firstStmt = parser->statements[opt.firstLoadIdx].get();
    firstStmt->metadata["cache_register"] = opt.cacheRegister;
    firstStmt->metadata["cache_for"] = opt.fieldName;
}
```

**Key Decisions**:
- Use statement metadata map for rewrite instructions
- Mark both first and second access statements
- Store cache register to use (determined by analysis)

#### Task 3: Apply Transformations (40-80 lines)
**Goal**: Rewrite marked statements to eliminate redundant calculations

**Location**: New function `applyVariableSizeFieldOptimizations()` or inline in existing function

**Implementation Strategy**:

**Option A: Statement-Level Transformation** (Recommended)
```cpp
bool applyVariableSizeFieldOptimizations(AssemblerParser* parser) {
    bool changed = false;
    
    for (size_t i = 0; i < parser->statements.size(); i++) {
        auto& stmt = parser->statements[i];
        
        // Check if this is a marked optimization point
        if (!stmt->metadata.count("use_cache")) continue;
        
        // Get optimization info from previous statement
        auto& firstStmt = parser->statements[i-N];  // Find first access
        
        // For second access: remove offset recalculation
        // Only load new column/row value
        // Reuse cached offset for address calculation
        
        changed = true;
    }
    
    return changed;
}
```

**Option B: Peephole Pattern Matching**
```cpp
// Look for patterns and rewrite inline:
// Pattern: LDY + LDX + JSR + LDAX (repeated)
// Action: Skip LDY/LDX/JSR on second occurrence
```

**Recommended**: Option A (Statement-level) - cleaner, safer

#### Task 4: Validate Transformations (10-20 lines)
**Goal**: Verify that optimizations are safe and correct

**Checks**:
1. ✅ Same array accessed twice
2. ✅ Same row or column value
3. ✅ No register clobber between accesses
4. ✅ No control flow changes
5. ✅ Offset calculation unchanged

```cpp
bool validateOptimization(const OptimizationInfo& opt,
                         AssemblerParser* parser) {
    // Verify cache assumptions hold
    // Check register clobbering
    // Ensure offset calc is deterministic
    return true;  // or false if validation fails
}
```

#### Task 5: Measure Impact (20-30 lines)
**Goal**: Report code size savings

```cpp
if (verbose) {
    int originalSize = calculateInstructionSize(firstAccess) +
                      calculateInstructionSize(secondAccess);
    int optimizedSize = calculateInstructionSize(optimized);
    int savings = originalSize - optimizedSize;
    
    fprintf(stderr, "[Phase 96.4.3] Optimized %s.%s: "
                    "%d → %d bytes (%d byte saving)\n",
            opt.arrayName.c_str(), opt.fieldName.c_str(),
            originalSize, optimizedSize, savings);
}
```

---

## Code Template

### Function Signature
```cpp
// In AssemblerOptimizer class
static bool applyVariableSizeFieldOptimizations(
    AssemblerParser* parser,
    const std::map<std::string, CachedPointerFieldOffset>& cache,
    bool verbose
);
```

### Call Site
In `optimizeInternal()` after `detectVariableSizeFieldArrays()` and `optimizeVariableSizeFieldOffsets()`:

```cpp
// Phase 96.4.3: Apply transformations
if (optFlags.variableSizeOpt) {
    if (applyVariableSizeFieldOptimizations(parser, pointerFieldCache, verbose)) {
        changed = true;
    }
}
```

---

## Testing Strategy

### Unit Tests
1. **Simple case**: Two consecutive pointer loads with same row
2. **Column cache**: Two consecutive loads with same column
3. **Multiple arrays**: Accesses to different arrays in sequence
4. **No optimization**: Loads with different rows and columns
5. **Edge cases**: First/last statement, with labels, with comments

### Validation
1. **Compile Phase 96 test files** - must produce valid assembly
2. **Verify code size** - should be smaller than before
3. **Run all 575 unit tests** - zero regressions
4. **Check assembly output** - correct instruction sequences

### Benchmarking
```bash
# Compare code size before/after
./bin/cc45 -c test_phase96_variable_pointers.c -O2
objdump45 -s output.o45 | grep "TEXT" | wc -l
# Expected: 10-20% smaller on pointer-heavy code
```

---

## Integration Checklist

- [ ] Implement `applyVariableSizeFieldOptimizations()`
- [ ] Add to `optimizeInternal()` call sequence
- [ ] Handle both LDAX/LDAY/LDAZ variants
- [ ] Add verbose logging for optimization tracking
- [ ] Test with Phase 96 test files
- [ ] Verify all 575 tests pass
- [ ] Measure and report code size reduction
- [ ] Document final results in commit message

---

## Success Criteria

✅ **Functional**:
- Code compiles without errors
- All 575 unit tests pass
- Phase 96 test files compile successfully
- No regressions in existing optimizations

✅ **Performance**:
- 10-20% code reduction on pointer field accesses
- Faster offset calculation patterns
- Measurable improvement in test cases

✅ **Quality**:
- Clear, maintainable code
- Comprehensive comments
- Verbose logging for debugging
- Proper error handling

---

## Key Files

| File | Purpose |
|------|---------|
| `src/main/AssemblerOptimizer.cpp` | Main implementation location |
| `include/AssemblerOptimizer.hpp` | Headers (if needed) |
| `include/AssemblerParser.hpp` | Statement/Directive structures |
| `src/test-resources/test_phase96_*.c` | Test validation |

---

## Related Code References

**Phase 96.4.2 Functions** (Already complete):
- `detectVariableSizeFieldArrays()` - Line ~1410
- `optimizeVariableSizeFieldOffsets()` - Line ~1550

**Statement Types to Handle**:
- `AssemblerParser::Statement::LDAX` - Load 16-bit (A and X)
- `AssemblerParser::Statement::LDAY` - Load 16-bit (A and Y)
- `AssemblerParser::Statement::LDAZ` - Load 16-bit (A and Z)

**Instruction Patterns to Recognize**:
```cpp
// Pattern 1: Row cache
LDY #row; LDX #col1; JSR calc; LDAX addr
LDX #col2; JSR calc; LDAX addr  // Can skip JSR if row unchanged

// Pattern 2: Column cache  
LDY #row1; LDX #col; JSR calc; LDAX addr
LDY #row2; JSR calc; LDAX addr  // Can skip JSR if col unchanged
```

---

## Notes for Implementation

1. **Register Selection**: Determine which register (A, X, Y) holds the cached offset
2. **Control Flow Safety**: Ensure no labels/jumps between accesses
3. **Clobber Analysis**: Verify registers aren't modified between accesses
4. **Conservative Approach**: Only optimize when 100% certain it's safe
5. **Verbose Logging**: Add helpful debug output for performance analysis

---

## What Happens Next (Phase 96.4.4+)

After Phase 96.4.3 completes:
- **Benchmarking** - Measure actual code size reduction
- **Performance Profiling** - Test on real MEGA65 hardware
- **Cross-Module Optimization** - Extend to inter-module cases
- **Documentation** - Update architecture guides with results
- **Release** - Include in v1.0.9+ release

---

**Ready to implement**? Start with Task 1 (build optimization map), work through to Task 5 (measure impact), then run full test suite. Good luck! 🚀
