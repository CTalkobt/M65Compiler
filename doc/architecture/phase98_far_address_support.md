# Phase 98: Far Address Support (24-Bit Addressing)

**Status**: Starting 2026-08-21  
**Target Version**: v1.0.6  
**Complexity**: Medium (extends Phase 97)

## Overview

Phase 98 implements code generation for the `__far` address space qualifier, enabling full 24-bit addressing on MEGA65 for accessing extended memory beyond the standard 64K address space.

### The Problem

MEGA65 extends the 6502 with:
- **32-bit accumulator (A/X/Y/Z)** in Phase 97
- **24-bit address space** (bank switching via MAP register)
- **Extended RAM** (up to 256MB with page banking)

Currently:
- `__zp` works (8-bit zero page) ✅
- `__abs` works (16-bit absolute) ✅
- `__far` is parsed but not code-generated ❌

### The Solution

Phase 98 adds:
1. **24-bit pointer tracking** in IR
2. **Bank-aware code generation** (MAP register setup)
3. **Far memory operations** (lda/sta with 24-bit addressing)
4. **Far pointer arithmetic** (24-bit addition/subtraction)

### Example Usage

```c
// Extended RAM buffer (beyond 64K)
__far unsigned char large_buffer[65536];  // 64KB @ address $010000+

// Far pointer
__far int* far_array;                     // 24-bit pointer to far memory

// Far struct
__far struct {
    int data[1024];
    char flags[256];
} far_config;

// Far function access
void process_far_data(void) {
    // Compiler generates bank-switching setup
    large_buffer[0] = 42;  // Automatic MAP register configuration
    
    // Far pointer dereference
    int value = *far_array;  // 24-bit address dereference
}
```

## Phase 98 Sub-Phases

### Phase 98.1: IR Extensions
- Add `AddressSpace::Far` to IR type system
- Track 24-bit pointer types through IR
- Generate bank selection code in IR

### Phase 98.2: Code Generation
- Emit MAP register setup for far memory access
- Generate 24-bit load/store sequences
- Optimize bank-aware addressing

### Phase 98.3: Linker Integration
- Place `__far` variables in extended memory banks
- Generate relocation entries for 24-bit addresses
- Update .o45 format for bank information

### Phase 98.4: Validation & Benchmarking
- Multi-bank test programs
- Verify bank switching correctness
- Benchmark access latency vs. optimization

## Implementation Architecture

```
Phase 97 (Address Space Qualifiers)
    ↓
Phase 98.1: IR Extensions
    - Add Far type to AST/IR
    - Track 24-bit pointers
    - Generate MAP register sequences
    ↓
Phase 98.2: Code Generation
    - Emit bank-aware load/store
    - Optimize repeated bank selections
    - Handle pointer arithmetic
    ↓
Phase 98.3: Linker Integration
    - Extended memory layout
    - Bank allocation strategy
    - Relocation generation
    ↓
Phase 98.4: Validation
    - Test programs with far memory
    - Benchmarking & optimization
    - Documentation
```

## Key Components

### 1. IR Far Type System
```cpp
// In IRType.hpp
enum class AddressSpace {
    Stack,      // Stack-relative (default)
    ZeroPage,   // __zp (8-bit addressing)
    Absolute,   // __abs (16-bit addressing)
    Far,        // __far (24-bit addressing)
};

// Far pointer type
struct FarPointerType : IRType {
    IRType* pointee;
    unsigned bankHint;  // Suggested bank for allocation
};
```

### 2. MAP Register Management
```cpp
// In CodeGenerator
// MAP register layout:
// Bits 7-4: Banks for A00000-CFFFFF
// Bits 3-0: Banks for 00000-3FFFF (lower 256KB)

void emitMapSetup(unsigned bank) {
    // lda #$XX        ; Bank value
    // sta $FFF8       ; MAP register
    // (Wait for CPU pipeline flush if needed)
}
```

### 3. Far Memory Access
```asm
; Load byte from far memory (24-bit address in __far pointer)
; Pointer in $20-$22 (24-bit little-endian)
    lda $20         ; Get bank byte
    sta $FFF8       ; Set MAP register
    lda ($21)       ; Access at bank-relative address
    ; Result in A register

; Store byte to far memory
    lda $20         ; Get bank byte
    sta $FFF8       ; Set MAP
    lda #value
    sta ($21)       ; Store at bank-relative address
```

## Addressing Modes

### 8-Bit Zero Page (__zp - Phase 97)
```asm
lda.zp $20      ; Direct ZP addressing
```

### 16-Bit Absolute (__abs - Phase 97)
```asm
lda $1234       ; Standard absolute addressing
```

### 24-Bit Far (__far - Phase 98)
```asm
; Setup bank
lda $high_byte_of_address
sta $FFF8

; Access at bank-relative offset
lda ($low_16_bits), Y
```

## Performance Characteristics

### Access Latency
| Mode | Cycles | Notes |
|------|--------|-------|
| ZP | 3-4 | Fastest, limited to $00-$FF |
| Absolute | 4-5 | Standard, 64K space |
| Far | 6-8 | Bank setup + access |
| Far (cached) | 4-5 | If bank already set |

### Code Size
| Operation | Code Size | Notes |
|-----------|-----------|-------|
| ZP load | 2 bytes | lda.zp $20 |
| Abs load | 3 bytes | lda $1234 |
| Far load | 8 bytes | MAP setup + load |
| Far load (opt) | 4 bytes | If bank cached |

### Optimization Opportunities
- **Bank caching**: Keep frequently-accessed bank in MAP register
- **Bank locality**: Allocate related data in same bank
- **Prefetch**: Load bank before tight loops
- **Lazy MAP**: Defer bank setup until actually needed

## Integration Points

### Phase 97 (Address Space Qualifiers)
- Reuse `__far` keyword parsing
- Share qualifier validation framework
- Extend type system with Far pointers

### Phase 91 (Cross-Module Optimization)
- Bank-aware function specialization
- Cache-locality optimization
- Extended memory inlining

### Phase 96.5 (Field Caching)
- Far pointer field optimization
- Cross-bank access patterns
- Extended memory structure layout

### Linker (ln45)
- Bank allocation strategy
- Extended memory layout
- Relocation table updates

## Known Constraints

1. **MEGA65 Hardware**
   - MAP register affects entire address space
   - Context switches must save/restore MAP
   - 64KB bank size (addresses $00000-$0FFFF per bank)

2. **Compiler Strategy**
   - Conservative bank setup (safety over speed)
   - Optional aggressive caching with pragma
   - Per-function bank state tracking

3. **ABI Compatibility**
   - Far pointers are 3 bytes (24-bit)
   - Stack still uses 16-bit addressing
   - Function pointers remain 16-bit (in current bank)

## Test Strategy

### Unit Tests
- Far pointer declaration parsing
- Far variable allocation
- Bank-aware code generation

### Integration Tests
- Multi-bank programs
- Far pointer dereference
- Far function calls

### Benchmarks
- Bank switching overhead
- Cache optimization impact
- Memory access latency

## Success Criteria

✅ Phase 98.1: IR extensions compile and pass tests  
✅ Phase 98.2: Far code generation produces valid assembly  
✅ Phase 98.3: Linker places far variables correctly  
✅ Phase 98.4: Multi-bank test programs execute correctly  
✅ Zero regressions on Phases 91-97

## Timeline

- **Phase 98.1**: 1-2 hours (IR extensions)
- **Phase 98.2**: 2-3 hours (code generation)
- **Phase 98.3**: 1-2 hours (linker integration)
- **Phase 98.4**: 1-2 hours (validation)
- **Total**: 5-9 hours

## References

- **MEGA65 Hardware**: https://github.com/MEGA65/mega65-core
- **MAP Register**: Banking and memory configuration
- **Phase 97**: Address Space Qualifier foundation
- **45GS02 CPU**: Extended 6502 with 32-bit registers

---

**Next Step**: Phase 98.1 - IR Extensions Implementation

