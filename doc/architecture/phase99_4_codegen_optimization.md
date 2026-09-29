# Phase 99.4: Code Generation Optimization - Bank Layout Integration

**Status**: Starting 2026-08-21  
**Target Version**: v1.0.6  
**Complexity**: High (integrates with CodeGenerator + loop optimizer)

## Overview

Phase 99.4 integrates the Phase 99.1-99.3 bank layout analysis into the code generation pipeline, optimizing bank setup instruction placement and enabling bank caching within loops.

### The Problem

Phase 98-99.3 allocates and analyzes `__far` variables optimally, but:
- Bank setup instructions placed naively (every access)
- Repeated bank switches in loops
- No awareness of loop structure
- No register caching of frequently-accessed banks

### Example

**Before (Phase 98)**:
```asm
; Loop iteration 1
lda #$01          ; Bank 1 setup
sta $FFF8
lda palette[0]    ; Access palette

lda #$02          ; Bank 2 setup  ← unnecessary!
sta $FFF8
lda vertices[0]   ; Access vertices (same bank as before loop)

; Loop iteration 2
lda #$01          ; Bank 1 setup  ← repeated!
sta $FFF8
lda palette[1]
```

**After (Phase 99.4)**:
```asm
; Setup before loop
lda #$01
sta $FFF8         ; Set bank once

loop:
; No bank switch - both in same bank
lda palette[i]
lda vertices[i]

; Loop continues without bank switches
jmp loop
```

## Phase 99.4 Sub-Phases

### Phase 99.4.1: Bank Setup Optimizer
- Analyze loop structure for `__far` access patterns
- Hoist bank setup outside loops when possible
- Track bank state through loop iterations
- Detect repeated bank switches for elimination

### Phase 99.4.2: Register Caching Strategy
- Cache current bank in register (e.g., temp reg)
- Skip repeated bank setup if already loaded
- Maintain cache state through control flow
- Handle nested loops and branches

### Phase 99.4.3: CodeGenerator Integration
- Connect BankLayoutIntegrator to CodeGenerator
- Update far variable access code generation
- Insert optimal bank setup instructions
- Apply caching strategies

### Phase 99.4.4: Loop Optimizer Integration
- Coordinate with existing LoopOptimizer
- Analyze loop-based access patterns
- Determine co-location benefits
- Suggest loop unrolling opportunities

### Phase 99.4.5: Validation & Benchmarking
- Test loop optimization with far variables
- Measure bank switch reduction
- Benchmark code size and speed
- Performance report

## Architecture

### Component Hierarchy

```
CodeGenerator (existing)
    ↓
Phase99BankLayoutCodeGenAdapter (new, Phase 99.4.3)
    ├─ receiveBankLayout() → Accept BankLayout
    ├─ optimizeBankSetup() → Place bank setup optimally
    ├─ cacheHotBanks() → Register caching strategy
    └─ emitOptimalCode() → Generate optimized assembly
    ↓
Phase99LinkerIntegrator (Phase 99.3)
    ↓
Phase99BankAssignmentEngine (Phase 99.2)
    ↓
Phase99CrossModuleDatabase (Phase 99.1)
    ↓
M65Emitter (existing)
```

### Integration Points

1. **CodeGenerator**: Receives BankLayout from linker hints
2. **LoopOptimizer**: Analyzes far access patterns in loops
3. **M65Emitter**: Emits optimized bank setup sequences
4. **IRCodeGen**: Tracks variable allocation decisions

## Key Concepts

### Bank Setup Hoisting

Variables accessed in loop with same bank:
```c
__far int palette[256];
__far int mesh[256];  // Both in Bank 1

void render_loop() {
    for (int i = 0; i < 256; i++) {
        draw(palette[i], mesh[i]);  // Both same bank!
    }
}
```

Optimal code:
```asm
lda #$01
sta $FFF8         ; Setup once before loop
ldx #0
loop:
lda palette,x     ; Offset addressing
lda mesh,x        ; Same bank, no setup needed
inx
bne loop
```

### Bank Caching Strategy

Track bank state in register:
```asm
lda #0            ; currentBank = 0
sta Z_currentBank

lda #$01          ; Want bank 1
cmp Z_currentBank ; Already loaded?
beq skip_setup
sta $FFF8         ; Setup bank 1
lda #$01
sta Z_currentBank
skip_setup:
lda var,x         ; Access in bank 1
```

### Nested Loop Optimization

```c
for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 256; j++) {
        access palette[i][j];  // Bank setup outside inner loop
    }
}
```

Result:
```asm
loop_i:
lda #$01          ; Setup bank (outer loop)
sta $FFF8

loop_j:
lda palette,y     ; Inner loop: no setup
iny
cpy #256
bne loop_j

; Outer loop iteration
inc
cmp #10
bne loop_i
```

## Implementation Strategy

### Phase 99.4.1: Bank Setup Optimizer

```cpp
struct BankSetupDecision {
    bool hoistOutsideLoop = false;
    int suggestedBank = -1;
    int registerCacheSlot = -1;
};

class BankSetupOptimizer {
public:
    std::map<std::string, BankSetupDecision> 
        analyzeBankSetupPlacement(const IRFunction& func,
                                   const BankLayout& layout);
    
    bool canHoistBankSetup(const IRNode& node,
                           const BankLayout& layout) const;
    
    bool couldBenefitFromCaching(const IRNode& node) const;
};
```

### Phase 99.4.2: Register Caching

```cpp
class BankCacheManager {
public:
    void startFunction(const IRFunction& func);
    void recordBankSetup(int bank);
    void recordBankAccess(int bank);
    
    bool isCurrentBankCached(int bank) const;
    int getCacheRegister(int bank) const;
    
    void updateLoopContext(const LoopNode& loop);
    std::vector<std::string> generateCacheCode();
};
```

### Phase 99.4.3: CodeGenerator Integration

```cpp
class Phase99BankLayoutCodeGenAdapter {
public:
    explicit Phase99BankLayoutCodeGenAdapter(
        CodeGenerator& codeGen,
        const BankLayout& layout);
    
    void optimizeBankSetup();
    void cacheHotBanks();
    void emitOptimalCode();
    
    std::string getBankForVariable(const std::string& varName) const;
};
```

## Performance Targets

| Metric | Baseline | Phase 99.4 | Improvement |
|--------|----------|-----------|-------------|
| Bank switches/loop | 2-3 | 0-1 | 50-100% |
| Code size | Base | -5% | 5% reduction |
| Execution speed | Base | +15% | 15% faster |
| Bank switches total | Base | -40% | 40% reduction |

## Success Criteria

✅ Phase 99.4.1: Bank setup optimizer working  
✅ Phase 99.4.2: Register caching implemented  
✅ Phase 99.4.3: CodeGenerator integration working  
✅ Phase 99.4.4: Loop optimizer coordination  
✅ Phase 99.4.5: All validation tests passing  
✅ Zero regressions on existing tests  
✅ 15%+ code size improvement measured  

## Timeline

- **Phase 99.4.1**: 1-2 hours (optimizer)
- **Phase 99.4.2**: 1-2 hours (caching)
- **Phase 99.4.3**: 1-2 hours (integration)
- **Phase 99.4.4**: 1 hour (coordination)
- **Phase 99.4.5**: 1-2 hours (validation)
- **Total**: 5-9 hours

## References

- **Phase 99.1-99.3**: Cross-module database, bank layout, linker integration
- **Phase 98**: Far address support (base operations)
- **Phase 91**: Cross-module optimization (dispatcher patterns)
- **LoopOptimizer**: Existing loop analysis framework

---

**Next Step**: Phase 99.4.1 - Bank Setup Optimizer Implementation

