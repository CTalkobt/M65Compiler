# Register & Memory Value Tracking Framework — Design Plan

## Problem

Multiple optimization opportunities share the same underlying need: knowing what values registers and memory locations hold at any point during code emission. Currently, each optimization (tsx caching, aEqualsX_, constant suppression) is ad-hoc with its own tracking flag. This doesn't scale and creates fragile interactions.

## Proposed Framework: `MachineState`

A single state object tracks what is known about all registers and recently-accessed memory locations during code emission (IRCodeGen or assembler generator).

### Core Data Model

```cpp
struct ValueInfo {
    enum Kind {
        UNKNOWN,        // no information
        CONSTANT,       // known exact value
        SAME_AS_REG,    // mirrors another register (e.g., A==X after tax)
        SAME_AS_MEM,    // mirrors a memory location (e.g., A == [$20] after lda $20)
        RANGE,          // bounded range (e.g., 0..24 for a loop counter)
        NONZERO,        // known to be != 0
        ZERO,           // known to be == 0
    };
    Kind kind = UNKNOWN;
    int64_t constVal = 0;           // for CONSTANT
    uint8_t mirrorReg = 0;          // for SAME_AS_REG (register ID)
    uint16_t mirrorAddr = 0;        // for SAME_AS_MEM (ZP or absolute)
    int64_t rangeLo = 0, rangeHi = 0; // for RANGE
};
```

### Register State (6 entries)

```cpp
struct MachineState {
    ValueInfo reg[6];  // A, X, Y, Z, SP, Flags
    // Indexed by enum: REG_A=0, REG_X=1, REG_Y=2, REG_Z=3, REG_SP=4, REG_FLAGS=5
};
```

### Memory Cache (ring buffer)

Track the N most recently accessed memory locations:

```cpp
struct MemEntry {
    uint16_t addr;      // memory address (ZP or absolute)
    ValueInfo info;     // what's known about this location
    uint32_t age;       // access counter for LRU eviction
};

// Ring buffer of ~32 entries — covers typical ZP working set
std::array<MemEntry, 32> memCache;
```

### State Transitions

Each emitted instruction updates the state:

| Instruction | Effect |
|---|---|
| `lda #N` | A = CONSTANT(N) |
| `lda $ZP` | A = SAME_AS_MEM($ZP); if mem[$ZP] known, A inherits |
| `ldx #N` | X = CONSTANT(N); clear A==X mirror |
| `tax` | X = SAME_AS_REG(A); if A is CONSTANT, X = same CONSTANT |
| `sta $ZP` | mem[$ZP] = SAME_AS_REG(A); if A is CONSTANT, mem = CONSTANT |
| `tsx` | X = SAME_AS_REG(SP) |
| `pha/phx/phy/phz` | SP = modified; X mirror of SP invalidated |
| `pla` | A = UNKNOWN; SP = modified |
| `ora $ZP` | A = UNKNOWN (but FLAGS = derived from result) |
| `jsr` | SP modified; A/X/Y/Z = UNKNOWN (callee may clobber) |
| label/branch target | all state = UNKNOWN (conservative at merge points) |

### Invalidation Rules

- **Any write to register R**: clear all `SAME_AS_REG(R)` mirrors in other registers and memCache
- **Any write to memory addr M**: clear memCache entry for M; clear all `SAME_AS_MEM(M)` mirrors in registers
- **Branch/label**: invalidate everything (conservative; could refine with dataflow analysis later)
- **JSR**: invalidate A/X/Y/Z per callee's `.reg_clobbers` if known; invalidate SP

### Optimization Opportunities This Enables

| Current Ad-Hoc | Framework Equivalent |
|---|---|
| `xHoldsSP_` (tsx caching) | `reg[X].kind == SAME_AS_REG && reg[X].mirrorReg == SP` |
| `aEqualsX_` (tax elision) | `reg[A].kind == CONSTANT && reg[X].kind == CONSTANT && same value` |
| `vregConstVal_` (const address store) | `reg[A].kind == CONSTANT` at STORE time |
| Redundant load elimination (`lda $ZP` after `sta $ZP`) | `reg[A].kind == SAME_AS_MEM && addr matches` → skip load |
| `stx` → `sta` when A==X | `reg[A] == reg[X]` → use sta |
| Skip `ldx #0` after `lda #0; tax` | X already CONSTANT(0) → skip |
| `stz` awareness | Z register tracked, emit `stz` when `reg[Z].kind == CONSTANT(0)` |
| Dead store detection | mem[$ZP] already has the value being stored → skip store |
| `cmp #0` elimination | if `reg[FLAGS]` already set from the value → skip cmp |

### New Optimizations It Would Enable

- **Register forwarding**: `sta $20; ... lda $20` → skip lda if A unchanged and mem[$20] unchanged
- **Cross-instruction constant propagation**: `lda #5; sta $20; ... lda $20` → `lda #5` (if $20 not modified between)
- **Redundant tax/tay elimination**: if A already equals X, skip tax
- **Smart stz usage**: track Z register; when Z==0, use `stz` instead of `lda #0; sta`
- **Store forwarding**: `sta $20; stx $21; lda $20` → A already holds the value
- **Compare elimination**: after `lda $ZP`, Z flag already set for the loaded value

### Integration Points

1. **IRCodeGen**: replace `aEqualsX_`, `vregConstVal_`, tsx caching with MachineState queries
2. **AssemblerOptimizer**: use MachineState for peephole passes (redundant load/store elimination)
3. **Simulated ops**: M65Emitter methods update MachineState instead of ad-hoc flags
4. **Per-block**: state resets at block boundaries; could extend with block-level merging later

### Implementation Phases

1. **Phase 1**: Core MachineState struct + register tracking (CONSTANT, SAME_AS_REG, UNKNOWN). Replace `xHoldsSP_` and `aEqualsX_`.
2. **Phase 2**: Memory cache (ring buffer). Enable redundant load elimination and store forwarding.
3. **Phase 3**: Range tracking and NONZERO/ZERO states. Enable compare elimination and smart branching.
4. **Phase 4**: Integration with AssemblerOptimizer for cross-statement peephole optimization.
