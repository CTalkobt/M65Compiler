# Phase 92 IPO: Cross-Module Function Specialization

**Status:** Design & Implementation (In Progress)  
**Date:** 2026-08-20  
**Extends:** Phase 91 (Cross-Module Optimization)

---

## Overview

Phase 92 extends Phase 91's single-module specialization to work **across module boundaries**. Functions specialized in one module can be reused by other modules that call them with the same constant patterns.

### Key Concept

**Before Phase 92:**
```c
// module_a.c
int multiply_by_n(int x, int n) { return x * n; }
int main_a() { return multiply_by_n(10, 4); }  // Call site A

// module_b.c
extern int multiply_by_n(int x, int n);
void other() { multiply_by_n(20, 4); }  // Call site B (same n=4!)
```

**Phase 91:** Specializes within each module independently
- Module A: Creates `multiply_by_n__spec_n4` for the 4 constant
- Module B: Cannot see module A's specialization, creates its own

**Phase 92:** Coordinates across modules
- Linker sees both modules want `multiply_by_n` with `n=4`
- Emits single specialized variant `multiply_by_n__spec_n4`
- Both modules route their calls to it (saves code)

---

## Design

### Phase 92.1: Cross-Module Call Site Analysis

**Goal:** Build global call graph with constant patterns across all modules

**Data Collection:**
```cpp
struct CrossModuleCallSite {
    std::string functionName;      // "_multiply_by_n"
    std::vector<int> paramIndices; // Which params are constant [1]
    std::vector<uint64_t> values;  // Constant values for those params [4]
    int callCount = 0;             // How many call sites match this pattern
    std::set<std::string> sourceModules; // Which .o45 files have this call site
};

struct CrossModuleSpecProfile {
    std::string functionName;
    std::map<std::vector<uint64_t>, CrossModuleCallSite> patterns;
    // patterns[{4}] = call site info for n=4 across all modules
};
```

**Implementation:**
- Linker collects call site metadata from `.o45` relocatable objects
- Metadata includes function name, constant argument values, call count
- Deduplicates identical patterns across modules
- Identifies specialization candidates (>1 module calling with same constants)

### Phase 92.2: Linker Coordination & Routing

**Goal:** Emit single specialized variant, route all calls to it

**Process:**
1. **Specialization Decision:**
   - If function called with pattern from ≥2 modules: specialize once
   - If pattern appears in only 1 module: keep per-module variant
   - ROI threshold: (code_saved) > (routing_overhead)

2. **Routing Stub Generation:**
   ```asm
   ; Original function in module_a.o45
   _multiply_by_n:
       jmp _multiply_by_n__router
   
   ; Routing logic
   _multiply_by_n__router:
       cmp #4           ; Check if n == 4
       bne @generic
       jmp _multiply_by_n__spec_n4
   @generic:
       jmp _multiply_by_n__generic
   
   ; Specialized variant (single copy in final binary)
   _multiply_by_n__spec_n4:
       asl              ; n is 4, use shifts instead of multiply
       asl
       sta result
       rts
   ```

3. **Symbol Resolution:**
   - `_multiply_by_n__spec_n4` exported globally (not per-module)
   - Both modules' routers reference the same variant
   - Linker handles relocation

### Phase 92.3: Duplicate Elimination

**Goal:** Identify and merge redundant specialized variants

**Detection:**
- Same function name + same pattern = identical generated code
- Group by function+pattern hash
- Keep first, redirect others to canonical variant

**Merging:**
```asm
; In module_a.o45
_multiply_by_n__spec_n4 = _multiply_by_n__spec_n4__canonical

; In module_b.o45  
_multiply_by_n__spec_n4 = _multiply_by_n__spec_n4__canonical

; Canonical version (only one emitted)
_multiply_by_n__spec_n4__canonical:
    asl
    asl
    sta result
    rts
```

### Phase 92.4: Call Site Rewriting

**Goal:** Update call instructions to use routers/specializations

**Per-Module Rewriting:**
```c
// Original in module_a.c
multiply_by_n(x, 4)  →  JSR _multiply_by_n__router

// Router dispatches to specialized variant if constants match
// Otherwise falls through to generic implementation
```

**Linker Integration:**
- Process relocation entries for function calls
- Check constant argument patterns
- Redirect to router/specialized variant as appropriate
- Preserve original symbol for cross-module resolution

### Phase 92.5: Performance Measurement

**Metrics:**
- **Code saved:** Bytes eliminated by merging duplicate variants
- **Routing overhead:** Bytes added by routers
- **Net savings:** code_saved - routing_overhead
- **Call speedup:** Branches eliminated by specialization

**Example:**
```
Function: multiply_by_n
  Generic variant:     40 bytes
  Spec n=4 variant:    25 bytes (saves 15 bytes)
  Spec n=8 variant:    25 bytes (saves 15 bytes)
  Router overhead:     20 bytes (for 2 specializations)
  
  Without Phase 92:
    - Module A: generic + spec_n4 = 65 bytes
    - Module B: generic + spec_n4 = 65 bytes
    - Total: 130 bytes
  
  With Phase 92:
    - Canonical generic:  40 bytes
    - Canonical spec_n4:  25 bytes
    - Router (shared):    20 bytes
    - Total: 85 bytes
    - Savings: 45 bytes (35%)
```

### Phase 92.6: Documentation & Examples

**For Users:**
- How cross-module specialization works
- Performance characteristics
- Configuration (enable/disable per-function)
- Example programs showing benefit

**For Developers:**
- Linker algorithm documentation
- Call site metadata format
- Specialization routing pattern
- Test suite and validation

---

## Implementation Strategy

### Stage 1: Metadata Emission (Compiler Side)

**File:** `src/main/CodeGenerator.cpp`

```cpp
// Existing: Per-module call site tracking
struct CallSiteInfo {
    std::string functionName;
    std::vector<uint64_t> constantArgs;
};

// New: Export call site metadata to .o45
void emitCallSiteMetadata(const std::vector<CallSiteInfo>& sites) {
    // Add .callsite directives to assembly:
    // .callsite _multiply_by_n paramMask=0x2 values=0x4
    // (param mask 0x2 = param[1], value 0x4 = 4)
}
```

### Stage 2: Linker Coordination

**File:** `src/main/ln45_main.cpp` (Linker)

```cpp
// Phase 92.1: Collect cross-module call sites
struct CrossModuleAnalyzer {
    std::map<std::string, std::map<std::vector<uint64_t>, int>> 
        functionCallPatterns;  // func → pattern → count
    
    void analyzeCallSites(const std::vector<ObjectFile>& objects) {
        for (const auto& obj : objects) {
            for (const auto& site : obj.callSites) {
                functionCallPatterns[site.functionName][site.constantArgs]++;
            }
        }
    }
};

// Phase 92.2: Route decisions
struct RoutingDecision {
    std::string functionName;
    std::vector<uint64_t> pattern;
    bool shouldSpecialize;  // true if ≥2 modules call with this pattern
    std::string targetSymbol;  // _func__spec or _func__router
};

// Phase 92.3: Deduplication
struct SpecializationDedup {
    std::map<std::pair<std::string, std::vector<uint64_t>>, std::string>
        canonicalVariants;  // (func, pattern) → canonical_symbol_name
};
```

### Stage 3: Testing

**Test Programs:**
1. `test_ipo_cross_module_multiply.c` — Two modules calling same function with same constant
2. `test_ipo_cross_module_mixed.c` — Multiple specialization patterns
3. `test_ipo_cross_module_dedup.c` — Verify redundant variants eliminated
4. `test_ipo_cross_module_perf.c` — Measure code size reduction

**Validation:**
- Assembly inspection: Routers present, variants deduplicated
- Object file inspection: Call sites correctly rewritten
- Executable size: Reduced vs. Phase 91 baseline

---

## Performance Targets

**Expected Code Reduction:**
- Small programs (simple specialization): 5-10% additional
- Medium programs (multiple functions): 8-15% additional
- Large programs (many cross-module calls): 10-20% additional

**Combined with Phase 90-91:**
- Phase 90 (Frame Pointer): 10-25%
- Phase 91 (IPO single-module): 5-15%
- Phase 92 (IPO cross-module): 8-15% additional
- **Total:** 25-50% code reduction potential

---

## Integration Points

### With Phase 91
- Reuses profiling infrastructure
- Uses same specialization heuristics
- Extends call routing logic

### With Linker
- New `.callsite` directives in `.o45`
- Router stub generation at link time
- Symbol deduplication and relocation

### With Calling Conventions
- Works with Stack, ZP, SAC conventions
- Preserves calling convention boundaries
- No breaking changes

---

## Risk Mitigation

**Potential Issues & Mitigations:**

1. **Router overhead exceeds savings**
   - Mitigation: ROI threshold before specializing
   - Fallback: Keep generic-only if cost > benefit

2. **Call site metadata overhead**
   - Mitigation: Compress metadata format
   - Store only essential info (param mask, values)

3. **Linker complexity**
   - Mitigation: Separate routing phase
   - Incremental implementation (start with simple patterns)

4. **Debugging difficulty**
   - Mitigation: Emit source location metadata in routers
   - Symbol names indicate specialization (searchable)

---

## Success Criteria

- ✅ Cross-module call sites collected and analyzed
- ✅ Specialization decisions made based on ROI
- ✅ Router stubs generated correctly
- ✅ Duplicate variants merged/deduplicated
- ✅ All call sites rewritten to use routers
- ✅ Code size reduction measured (5-20%)
- ✅ No regressions to Phase 91 functionality
- ✅ Test suite passes (new + existing)
- ✅ Documentation complete

---

## Next Steps

1. **Phase 92.1:** Implement metadata emission in compiler
2. **Phase 92.2:** Implement linker analysis and routing decisions
3. **Phase 92.3:** Implement deduplication and symbol resolution
4. **Phase 92.4:** Rewrite call sites in linker
5. **Phase 92.5:** Benchmark and measure impact
6. **Phase 92.6:** Documentation and examples

---

**Estimated Impact:** 10-20% additional code reduction  
**Estimated Complexity:** Medium (builds on Phase 91 foundation)  
**Ready for Implementation:** Yes ✅

