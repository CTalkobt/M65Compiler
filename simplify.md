# MEGA65 C Compiler — Simplification and Complexity Reduction Roadmap

**Last Updated:** 2026-07-07  
**Status:** Phase 1 in progress (1.1 COMPLETED, 1.2 TODO, 1.3 TODO)  
**Target:** Reduce complexity by 15-25%, improve maintainability without changing functionality

## Phase 1 Progress

### ✅ 1.1 Unify Type Size Calculation — COMPLETED
- **Commit:** a58380d (2026-07-07)
- **Changes:** ~150 lines of duplicate code removed, 4 files modified, 2 new files
- **Impact:** Single source of truth for type sizes across CodeGenerator, IRBuilder, ConstantFolder
- **Testing:** All 500+ unit tests passing; one pre-existing register test failure (unrelated)

### ✅ 1.2 Audit Constant Folding — COMPLETED
- **Finding:** ConstantFolder and IROptimizer are complementary (AST-level vs IR-level), not redundant
- **Decision:** No consolidation needed; ConstantFolder is already minimal at 159 lines
- **Result:** Identified that real duplication is in disk image classes (Phase 1.3)

### ⏳ 1.3 Begin Disk Image Refactor — NEXT (40-120 hours)
- **Target:** Consolidate 19 disk format classes (5,054 lines → ~800 lines)
- **Approach:** Template-based design with format traits
- **Scope:** D64, D81, D71, D80 in Phase 1; remaining formats deferred to future PRs

---

## Overview

Comprehensive analysis identified **8 major complexity areas** and **~8,000+ lines of redundant code**. This document tracks the systematic simplification effort organized in 3 phases.

### Previous Work Completed

- [x] Centralized numeric literal parsing (Section 5)
- [x] Hardware address constants (Section 6)
- [x] Direct CPU register usage optimization (Section 10)

---

## Phase 1: Quick Wins (Weeks 1-2, ~55 hours, Low Risk)

### 1.1 Unify Type Size Calculation (20 hours)
**File(s):** `src/main/CodeGenerator.cpp`, `src/main/IRBuilder.cpp`  
**Issue:** Two near-identical type size functions with different signatures  
**Status:** COMPLETED (2026-07-07)

**Implementation Steps:**
1. Create `src/common/TypeSystem.hpp` with unified `getTypeSize()` implementation
2. Migrate `CodeGenerator::getTypeSize()` to call `TypeSystem::getTypeSize()`
3. Migrate `IRBuilder::getTypeSize()` to call `TypeSystem::getTypeSize()`
4. Remove duplicate implementations
5. Run full test suite to verify no regressions

**Files Changed:**
- `src/common/TypeSystem.hpp` (new)
- `src/main/CodeGenerator.cpp` (refactor)
- `src/main/IRBuilder.cpp` (refactor)

**Estimate:** 20 hours  
**Benefit:** Single source of truth for type sizes; 150 lines removed

---

### 1.2 Audit: Verify ConstantFolder and IROptimizer (3 hours)
**File(s):** `src/main/ConstantFolder.cpp`, `src/main/IROptimizer.cpp`  
**Issue:** Initial analysis suggested overlap; audit to verify  
**Status:** COMPLETED - No redundancy found

**Finding:** ConstantFolder and IROptimizer are **complementary, not redundant**:
- **ConstantFolder** (159 lines, AST-level):
  - Folds constant expressions early (before IR generation)
  - Constant propagation via variable tracking
  - Removes dead branches (while/for with false conditions)
  - Folds sizeof/alignof to constants
  - Evaluates static_assert
  
- **IROptimizer** (1243 lines, IR-level):
  - CSE, strength reduction, algebraic simplification
  - Dead code elimination at IR level
  - Copy propagation, type narrowing, branch folding
  - LICM and address element fusion

**Conclusion:** Both passes provide distinct value. No consolidation needed. Early AST-level folding reduces IR size and catches errors before codegen.

**Decision:** Skip aggressive consolidation. ConstantFolder is already minimal. Focus Phase 1.3 on actual duplication (disk images: 4,200 lines).

---

### 1.3 Begin Disk Image Refactor (40 of 120 hours)
**File(s):** `src/main/D*Image.cpp` (19 disk format classes)  
**Issue:** 85-90% code duplication across disk format implementations  
**Status:** TODO (Phase 1: design + 40% implementation)

**Implementation Steps:**

**Part 1a: Design and Type Definitions (8 hours)**
1. Analyze current disk format classes to extract commonalities
2. Design `DiskImage` base template and format traits
3. Create `src/main/DiskImageTemplate.hpp` with:
   - `struct FormatTraits` interface (track layout, BAM structure, geometry)
   - `class DiskImage<Traits>` template with unified implementations
4. Document how each format specializes the template

**Part 1b: Consolidate Simple Formats (32 hours)**
1. Implement traits for D64 (simplest format)
2. Migrate `D64Image` to template-based implementation
3. Implement traits for D81
4. Migrate `D81Image` to template-based implementation
5. Do the same for D71, D80 (common formats with ~80% of usage)
6. Test each format independently via `make test`
7. Validate disk image creation and file I/O

**Deferred to Phase 1b (remaining formats):**
- D82, D90, D60, D1M, D2M, D4M, DNP (less common, can parallelize in later PR)
- GCR, X64, TAP, T64, ARK, ARC, LNX, CVT, P00, Zipcode (specialized, separate PRs)

**Files Changed:**
- `src/main/DiskImageTemplate.hpp` (new, ~250 lines)
- `src/main/D64Image.cpp` (refactor, ~80 lines from ~323)
- `src/main/D81Image.cpp` (refactor, ~80 lines from ~341)
- `src/main/D71Image.cpp` (refactor, ~80 lines from ~304)
- `src/main/D80Image.cpp` (refactor, ~100 lines from ~411)
- Potential removal of duplicated utility functions

**Estimate:** 40 hours (Phase 1 scope); total 120 hours for all formats  
**Benefit:** 4,200+ lines removed; clearer architecture; easier to add new formats

**Testing:**
```bash
make clean && make all
make test
# Manual testing of D64/D81/D71/D80 image creation and manipulation
```

---

## Phase 2: High-Impact Refactoring (Weeks 3-5, ~240 hours, Medium Risk)

### 2.1 Extract Large Visitor Methods (80 hours)
**Files:** `src/main/CodeGenerator.cpp`, `src/main/IRBuilder.cpp`, `src/main/Parser.cpp`  
**Issue:** Individual methods exceed 500-700 lines (CodeGenerator::visit(Assignment&) is 722 lines)  
**Status:** TODO

**Scope:**
- `CodeGenerator::visit(Assignment&)` (722 lines) → 6-8 sub-methods
- `CodeGenerator::visit(FunctionCall&)` (508 lines) → 3-4 sub-methods
- `CodeGenerator::visit(BinaryOperation&)` (444 lines) → 2-3 sub-methods
- `IRBuilder::visit(Assignment&)` → parallel extraction
- `Parser::parseStatement()` (454 lines) → per-statement parsers

**Target:** All visitor methods under 150 lines; all helper methods under 100 lines

---

### 2.2 Unify Symbol and Type Tracking (60 hours)
**Files:** `src/main/CodeGenerator.cpp`, `src/main/CodeGenerator.hpp`  
**Issue:** Scope/type info scattered across 5+ data structures  
**Status:** TODO

**Implementation:**
- Create `src/common/ScopeManager.hpp`
- Extract nested scope tracking from `functionStack_`
- Consolidate `variableTypes`, `globalVariableTypes`, `functionStack_`
- Unify `resolveVarName()` and `lookupVar()` into single interface
- Update CodeGenerator to use ScopeManager

**Benefit:** 200+ lines removed; clearer variable lookup; single symbol table

---

### 2.3 Merge CodeGenerator and IRBuilder (100-150 hours)
**Files:** Most of `src/main/CodeGenerator.cpp` + `src/main/IRBuilder.cpp`  
**Issue:** Two parallel visitors on same AST; 1,500+ lines of duplication  
**Status:** TODO (Largest refactor, requires careful integration testing)

**Implementation Strategy:**
1. Make IRBuilder the canonical code generator
2. Port type-coercion and register-allocation logic from CodeGenerator to IRBuilder
3. Ensure all CodeGenerator tests pass with IRBuilder
4. Remove CodeGenerator entirely
5. Rename IRBuilder to something clearer (or keep as-is)

**Risk:** Medium (requires comprehensive testing; currently CodeGenerator validates IRBuilder)  
**Benefit:** 1,500+ lines removed; single source of truth

---

## Phase 3: Architectural Simplification (Weeks 6-8, ~240 hours, High Risk)

### 3.1 Simplify Register Allocation Architecture (120 hours)
**Files:** `src/main/VRegAllocator.cpp`, `src/main/M65Emitter.cpp`, `src/main/MachineState.cpp`  
**Issue:** Over-engineered for 8 CPU registers (3 layers of indirection)  
**Status:** TODO

**Consolidation:** Merge into unified `RegisterAllocator` with direct physical register tracking

---

### 3.2 Consolidate Optimization Passes (60 hours)
**Files:** `src/main/ConstantFolder.cpp`, `src/main/IROptimizer.cpp`, `src/main/AssemblerOptimizer.cpp`  
**Issue:** Multiple separate optimization passes; can be unified  
**Status:** TODO (dependent on Phase 1.2)

**Goal:** Single IR optimizer; remove assembler-level peephole (fold into IR)

---

### 3.3 Refactor Parser Structure (40 hours)
**Files:** `src/main/Parser.cpp`, `src/main/Parser.hpp`  
**Issue:** Large monolithic parser methods (parse() 565 lines, parseStatement() 454 lines)  
**Status:** TODO (dependent on 2.1)

**Goal:** Cleaner dispatch structure; methods under 100 lines each

---

## Previous Issues & Recommendations (from v1.1 work)

### Unified Lexer Base [DEFERRED]
**Observation:** `Lexer.cpp` and `AssemblerLexer.cpp` share character/position tracking logic  
**Status:** Low priority (minimal duplication, both work well)  
**Recommendation:** Create `LexerBase` if adding new lexers

### Unified Type System [IN PROGRESS]
**Observation:** Types as `std::string typeName + int pointerLevel` leads to repetitive comparisons  
**Status:** Partially addressed by 1.1; full system (Phase 2.2 and later)  
**Recommendation:** Extract to proper `Type` class post-Phase-2

### Refactor cc45_main.cpp [DEFERRED]
**Observation:** Main driver has preprocessing, lexing, parsing, validation in single block  
**Status:** Works but could be cleaner  
**Recommendation:** Extract to `Compiler` class after Phase 1 stabilizes

### Parser Base Class [DEFERRED]
**Observation:** `AssemblerParser` and `Parser` share token matching/error reporting  
**Status:** Would only save ~50 lines; defer to Phase 3  
**Recommendation:** Create `ParserBase` if refactoring Parser in Phase 3

---

## Success Metrics

- **Code Reduction:** 15-25% fewer lines (~8,000+ lines removed)
- **Maintainability:** All methods < 150 lines; single responsibilities clear
- **Test Coverage:** No regressions (500+ tests passing)
- **Performance:** ±1% acceptable variance in generated code size
- **Documentation:** Each phase documented with rationale and metrics

---

## Checklist: Phase 1 Execution

### 1.1: Unify Type Size Calculation
- [x] Create `include/TypeSystem.hpp`
- [x] Create `src/main/TypeSystem.cpp` with unified implementation
- [x] Update `CodeGenerator.cpp` to use TypeSystem via helper method
- [x] Update `IRBuilder.cpp` to use TypeSystem
- [x] Update `ConstantFolder.hpp/cpp` to use TypeSystem
- [x] Run `make -j 12 test` (verified no regressions)
- [x] Remove duplicate implementations
- [x] Commit changes (commit a58380d)

### 1.2: Audit Constant Folding
- [x] Audit ConstantFolder vs IROptimizer
- [x] Document findings (complementary, not redundant)
- [x] Verify no consolidation needed
- [x] Updated simplify.md with findings
- [x] Decision: Focus on real duplication (disk images Phase 1.3)

### 1.3: Begin Disk Image Refactor
- [ ] Design DiskImage template + FormatTraits
- [ ] Create `src/main/DiskImageTemplate.hpp`
- [ ] Migrate D64Image to template
- [ ] Migrate D81Image to template
- [ ] Migrate D71Image to template
- [ ] Migrate D80Image to template
- [ ] Test disk image creation/manipulation
- [ ] Document format traits system
- [ ] Update CHANGELOG.md
- [ ] Note: Remaining formats (D82, GCR, etc.) deferred to future PR

---

## References

- **Analysis Date:** 2026-07-07
- **Analyzer:** Claude Code (Haiku 4.5)
- **Codebase Size:** ~35,000 lines (estimate)
- **Target Reduction:** 15-25% (~8,000 lines)
- **Test Coverage:** 500+ unit tests

---

## Notes for Developers

- **Incremental Integration:** Merge Phase 1 as independent PRs (1.1 → 1.2 → 1.3)
- **Regression Detection:** Run full test suite after each PR
- **Code Review:** Recommend peer review for any refactors touching CodeGenerator/IRBuilder
- **Feature Parity:** Ensure generated code metrics (size, speed) remain equivalent

