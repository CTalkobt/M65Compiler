# Code Cleanup Plan - All Projects (Phases C1-C6)

**Status**: Phase C1 (Categorization) IN PROGRESS  
**Total TODOs**: 73 items  
**Files with TODOs**: 17  
**Scope**: cc45, ca45, ln45, all tools  

---

## Phase C1: Document & Categorize All TODOs

### Category 1: Incomplete Optimization Passes (35 TODOs)

These are partially-implemented optimization passes with stubs:

#### BitManipulationReduction (8 TODOs)
- **File**: src/main/BitManipulationReduction.cpp
- **Status**: ~30% complete (placeholder structure exists)
- **Tasks**:
  1. Walk AST for binary operations
  2. Detect AND, OR, XOR, shift operations with constants
  3. Check if cheaper instruction exists
  4. Walk IR module for bit operations
  5. Extract operand patterns
  6. Replace operations with reduced forms
  7. Transform IR operations
  8. Update bytesReduced_ and patternsOptimized_
- **Cleanup Decision**: 
  - [ ] Complete implementation (feature work)
  - [ ] Mark as abandoned/remove
  - [ ] Document as "future optimization"

#### ConditionalChainFusion (5 TODOs)
- **File**: src/main/ConditionalChainFusion.cpp
- **Status**: ~20% complete
- **Tasks**:
  1. Walk AST and identify if/else chains with same condition
  2. Detect sequences of BR_COND with same condition vreg
  3. Implement chainability analysis
  4. Implement fusion logic and label management
- **Cleanup Decision**:
  - [ ] Complete implementation
  - [ ] Remove/deprecate
  - [ ] Document status

#### AddressTemplateDetector (3 TODOs)
- **File**: src/main/AddressTemplateDetector.cpp
- **Status**: ~40% complete
- **Tasks**:
  1. Implement cumulative stride detection
  2. Profile access patterns
  3. Extract usage frequency from access patterns
- **Cleanup Decision**:
  - [ ] Complete
  - [ ] Remove
  - [ ] Document

#### DMA Optimization Passes (6 TODOs)
- **Files**: Various (DMACallInliner, DMALoopFusion, etc.)
- **Tasks**:
  1. Replace memcpy calls with __dma_copy
  2. Replace memset calls with __dma_fill
  3. Replace inline loops with DMA
  4. Detect memcpy, memset, memmove calls
  5. Detect FOR_LOOP IR nodes with 8-bit counter vreg
  6. Analyze inline copy loops
- **Status**: Partially stubbed
- **Cleanup Decision**:
  - [ ] Complete DMA optimization suite
  - [ ] Remove/merge with existing code
  - [ ] Document phase status

#### Zero-Page Cache Optimization (8 TODOs)
- **Files**: ZeroPageCacheOptimizer, etc.
- **Tasks**:
  1. Allocate ZP cache slots for hot pointers
  2. Create cache allocation hints for code generator
  3. Count zero-page allocations
  4. Set vreg allocation hint to X register
  5. Allocate counter directly to X register
  6. Count dereferencing frequency per loop
  7. Analyze loop body for other X register uses
  8. Generate corrected addressing code
- **Status**: ~35% complete
- **Cleanup Decision**:
  - [ ] Complete implementation
  - [ ] Remove incomplete code
  - [ ] Document design

#### Switch Table Optimization (3 TODOs)
- **Files**: SwitchTableOptimizer, etc.
- **Tasks**:
  1. Identify SWITCH ops with dense case ranges
  2. Replace SWITCH with table lookup + indirect JMP
  3. Generate jump table data
- **Status**: Stubbed
- **Cleanup Decision**:
  - [ ] Implement
  - [ ] Remove
  - [ ] Document

#### Other Optimization Stubs (2 TODOs)
- ZeroArgCallDetector: "Implement complete ZeroArgCallDetector visitor methods"
- Phase 95.5: "Implementation" (context unclear)

### Category 2: Integration Points (1 TODO)

#### CompilerHookIntegrator
- **File**: src/main/CompilerHookIntegrator.cpp
- **TODO**: "Consult online learner for optimization decision when OnlineLearner is fully integrated"
- **Status**: Phase 113 COMPLETE - can now be implemented
- **Action**: 
  - [x] Remove TODO (Phase 113 provides learner integration)
  - [ ] Implement learner consultation logic

### Category 3: Configuration & Feature TODOs (remaining)

- "Implement configuration loading from file"
- "Add binary diff mode (-D) for comparing two .prg/.o45 files"
- Various IR analysis tasks

---

## Phase C2: Remove/Resolve Low-Priority TODOs

**Criteria for Removal**:
- No active development in last 6 months
- Marked as "future implementation"
- Superceded by newer phases (e.g., Phase 113 replaces learner TODO)
- No dependency from other code

**Current Candidates**:
- [ ] BitManipulationReduction (if not scheduled)
- [ ] ConditionalChainFusion (if not scheduled)
- [ ] Other abandoned passes

---

## Phase C3: Fix Unused Parameters & Dead Code

**Current Issues**:
- 1 unused parameter in FarAddressCodeGenerator.cpp (already handled with (void) cast)
- Commented-out code blocks in optimization passes
- Dead visitor methods in incomplete passes

**Actions**:
- [ ] Add [[maybe_unused]] attributes to intentional unused parameters
- [ ] Remove commented-out code blocks
- [ ] Remove stub implementations that won't be completed

---

## Phase C4: Remove Dead Code & Commented Sections

**Areas to Review**:
- Old/commented optimization implementations
- Disabled feature flags
- Old constant definitions
- Incomplete branches

---

## Phase C5: Split Oversized Files

**Files > 1000 lines**:
- IRCodeGen.cpp (~2000 lines) - consider splitting by optimization phase
- CodeGenerator.cpp (~1500 lines) - split by code generation type
- Various optimizer passes (~800-1200 lines each)

---

## Phase C6: Standardize Naming & Documentation

**Standards to Apply**:
- Consistent visitor method naming
- Consistent optimization pass structure
- Consistent error message formatting
- Uniform documentation for optimization passes

---

## Summary Table

| Phase | Category | Items | Est. Time | Priority |
|-------|----------|-------|-----------|----------|
| C1 | Documentation | 73 | 1-2 hrs | HIGH |
| C2 | Remove TODOs | 35-45 | 2-3 hrs | MEDIUM |
| C3 | Code Quality | 10-15 | 1-2 hrs | MEDIUM |
| C4 | Dead Code | 20-30 | 2-3 hrs | LOW |
| C5 | File Organization | 10-15 | 3-4 hrs | LOW |
| C6 | Standardization | Full codebase | 4-6 hrs | LOW |

**Total Estimated Cleanup**: 13-21 hours  
**Recommended Approach**: Phases C1-C3 (4-7 hours) for immediate quality improvement

---

## Action Items

### Immediate (Session 1):
- [ ] Phase C1: Complete TODO categorization
- [ ] Phase C2: Remove 20-30 abandoned TODOs
- [ ] Phase C3: Fix unused parameters

### Follow-up (Session 2):
- [ ] Phase C4: Remove dead code
- [ ] Phase C5: Split large files
- [ ] Phase C6: Standardization pass

---

**Next Step**: Execute Phase C2 (remove low-priority TODOs) or Phase C3 (code quality fixes)?
