# Link-Time ZP Calling Convention Promotion

**Status:** Design Proposal
**Author:** Craig Taylor (CTalkobt)
**Date:** 2026-08-27

---

## Summary

Replace the current dual-library approach (`c45.lib` + `c45_zp.lib`) with a single
stack-convention library and a link-time optimization pass that selectively promotes
hot functions to ZP parameter passing. This eliminates the maintenance burden of
duplicate libraries while producing better results — the optimizer has whole-program
visibility to make optimal ZP allocation decisions.

## Motivation

### Current State

The toolchain maintains two parallel standard libraries:

- `c45.lib` — stack calling convention (102 members)
- `c45_zp.lib` — ZP calling convention (96 members)

Every stdlib function must be compiled, tested, and maintained in both variants.
The `-fzpcall` flag selects the convention at compile time, and the linker generates
thunks for mismatches. Users must choose a convention before seeing the full program.

### Problems

1. **Maintenance cost**: Every bug fix applied twice. Every new function added twice.
2. **Blind decisions**: The user/compiler chooses ZP convention without knowing which
   functions are hot, how deep the call graph is, or how much ZP pressure exists.
3. **All-or-nothing**: Every function gets the same convention. A program with one
   hot inner-loop function and fifty cold utility functions pays ZP overhead everywhere.
4. **Convention mismatches**: Mixing conventions requires thunks, adding code size
   and complexity for no performance gain.
5. **Wasted ZP range**: $20-$2A reserved for ZP parameters even if the function
   only takes one byte parameter.

### Proposed Solution

Compile everything with stack convention. At link time, analyze the full program and
selectively promote functions to ZP parameter passing where it provides measurable
benefit. Reclaim the fixed ZP parameter region ($20-$2A) for the optimizer to use
as general-purpose scratch/register-resident storage.

## Design

### Phase 1: Call Graph Analysis (in ln45)

After linking all objects, build the complete call graph:

```
struct FunctionProfile {
    std::string name;
    int paramCount;
    int paramBytes;          // total parameter size
    int callSiteCount;       // how many places call this function
    int codeSize;            // function body size in bytes
    bool isLeaf;             // makes no calls
    bool isRecursive;        // direct or indirect recursion
    int loopNestDepth;       // max loop depth of call sites (0 = not in loop)
    int estimatedCallFreq;   // static estimate from loop depth
};
```

Most of this data is already available:
- Phase 91 (IPO) collects function profiles, call site tracking, leaf detection
- `.o45` metadata includes `.func_flags leaf`, `.reg_clobbers`, parameter counts
- The linker already builds a symbol dependency graph for dead code elimination

### Phase 2: ZP Budget and Candidate Selection

Available ZP budget:

```
Total ZP:       $08-$FF (248 bytes)
Reserved:       $08-$0F (8 bytes, scratch/static_chain)
Frame pointer:  $FD-$FE (2 bytes)
Available:      $10-$FC (237 bytes, ~118 two-byte slots)
```

The optimizer already uses ZP for vreg allocation. Link-time ZP promotion draws
from the same pool but with global visibility. The allocator partitions ZP into:

- **Optimizer pool**: ZP slots for vreg spills, loop-invariant hoisting, register
  pressure relief. Allocated per-function during code generation.
- **Promotion pool**: ZP slots for promoted parameter passing. Allocated at link time.

Candidate scoring:

```
score(fn) = callSiteCount * paramBytes * loopNestBonus
            - codeRewriteCost

loopNestBonus:
    0 (not in loop)    = 1
    1 (single loop)    = 8
    2 (nested loop)    = 64
    3+ (deep nesting)  = 512
```

Functions are ranked by score. The allocator greedily assigns ZP slots to the
highest-scoring candidates until the promotion pool is exhausted.

**Exclusions:**
- Recursive functions (ZP slots would be clobbered on re-entry)
- Variadic functions (unknown parameter count)
- Functions with > 10 bytes of parameters (diminishing returns)
- Functions with only 1 call site outside any loop (inlining is better)

### Phase 3: Instruction Rewriting

For each promoted function, rewrite the call sites and prologue.

#### Call Site Rewrite (caller side)

**Before (stack convention):**
```asm
; Push 2-byte parameter 'x'
lda _vreg5        ; load param low byte
pha               ; push to stack
lda _vreg5+1      ; load param high byte
pha               ; push to stack
jsr _target_func
; Clean up stack (2 bytes)
pla
pla
```

**After (ZP promotion):**
```asm
; Store 2-byte parameter 'x' to ZP slot
lda _vreg5
sta $20           ; ZP param slot (assigned by allocator)
lda _vreg5+1
sta $21
jsr _target_func
; No stack cleanup needed
```

**Savings per call site:** 4 bytes code, ~12 cycles (eliminates PHA/PLA pairs + stack cleanup)

#### Prologue Rewrite (callee side)

**Before (stack convention):**
```asm
proc _target_func
    ; Read param from stack via frame pointer
    ldy #$02
    lda ($FD),y     ; param low byte
    sta _local_x
    iny
    lda ($FD),y     ; param high byte
    sta _local_x+1
```

**After (ZP promotion):**
```asm
proc _target_func
    ; Param already in ZP — use directly or copy to local
    lda $20         ; ZP param slot
    sta _local_x
    lda $21
    sta _local_x+1
```

**Savings in prologue:** 2-4 bytes, ~6 cycles (eliminates indirect indexed load)

#### Optimization: Direct ZP Use

For leaf functions where the parameter is not modified, the rewriter can eliminate
the copy entirely and access the ZP slot directly throughout the function body:

```asm
; Instead of: lda _local_x → lda $20 (ZP direct, 2 bytes, 3 cycles)
; Instead of: lda ($FD),y  → lda $20 (ZP direct, saves indirect overhead)
```

### Phase 4: Pattern Recognition

The rewriter must identify the instruction sequences to transform. The code
generator emits well-defined patterns for stack parameter passing:

**Stack push pattern (at call site):**
```
; I8 parameter:  lda <src>; pha
; I16 parameter: lda <src>; pha; lda <src+1>; pha
; I32 parameter: lda <src>; pha; lda <src+1>; pha; lda <src+2>; pha; lda <src+3>; pha
; Followed by: jsr <target>
; Followed by: pla (repeated for each pushed byte, stack cleanup)
```

**Stack read pattern (in prologue):**
```
; ldy #<offset>; lda ($FD),y; sta <local>
; iny;           lda ($FD),y; sta <local+1>
```

These patterns are deterministic because they are emitted by `IRCodeGen::emitInst()`
for `CALL`/`CALL_VOID` ops. The rewriter matches against these known sequences
rather than attempting general pattern matching.

### Phase 5: ZP Slot Allocation Strategy

ZP slots for promoted parameters are allocated globally across all promoted
functions. Two strategies:

**A. Exclusive allocation (safe, simple):**
Each promoted function gets dedicated ZP slots. No two promoted functions share
parameter ZP space. Maximum safety but limited by available ZP bytes.

```
_func_a: params at $20-$23 (4 bytes, 2 params)
_func_b: params at $24-$25 (2 bytes, 1 param)
_func_c: params at $26-$29 (4 bytes, 2 params)
```

**B. Interference-graph allocation (optimal, more complex):**
Functions that never appear in the same call chain can share ZP slots. Build
an interference graph from the call graph and color it:

```
_func_a calls _func_b:  a and b interfere (cannot share)
_func_c never calls a or b: c can share with a or b
```

This is equivalent to register allocation via graph coloring — well-understood
algorithms exist. Start with strategy A; migrate to B when ZP pressure demands it.

## Integration Points

### Existing Infrastructure Used

| Component | What It Provides |
|-----------|-----------------|
| Phase 91 (IPO) | Function profiles, call site tracking, leaf detection |
| Phase 99 (Bank Analysis) | Variable co-access patterns, loop nest depth |
| Phase 100 (LTCO) | Hint coordination framework, synergy estimation |
| `.o45` metadata | `.func_flags`, `.reg_clobbers`, parameter counts |
| `O45Linker` | Symbol resolution, call graph, dead code elimination |
| `AssemblerOptimizer` | MachineState tracking for post-rewrite peephole |

### New Components

| Component | Purpose |
|-----------|---------|
| `ZPPromotionAnalyzer` | Score functions, select candidates, allocate ZP slots |
| `ZPCallRewriter` | Transform call sites and prologues in linked binary |
| `ZPSlotAllocator` | Manage ZP slot assignment with interference analysis |

### Linker Integration

The promotion pass runs after symbol resolution and relocation but before final
binary emission:

```
ln45 pipeline:
  1. Load .o45 objects and .lib archives
  2. Resolve symbols, apply relocations
  3. Dead code elimination
  4. [NEW] ZP calling convention promotion
     a. Build call graph profiles
     b. Score and select candidates
     c. Allocate ZP slots
     d. Rewrite call sites and prologues
  5. Emit final PRG binary
```

### Compiler Changes

- Remove `-fzpcall` flag from `cc45` (deprecated, no longer needed)
- Remove ZP calling convention code paths from `IRCodeGen` (simplify)
- Expand ZP range available to vreg allocator ($10-$FC instead of $10-$1F + $2B-$FC)
- Add linker flag: `-flink-zp-promote` (enable, default on at -O2+)
- Add linker flag: `-fno-link-zp-promote` (disable)
- Add linker flag: `--zp-promote-budget=N` (limit ZP bytes for promotion)

### Library Changes

- Remove `c45_zp.lib` build from `lib/Makefile`
- Remove all `stdlib_zp/` source files
- Remove `crt0_zp.o45` variant
- Single `c45.lib` with stack convention only

## Rollout Plan

### Step 1: Remove c45_zp.lib (immediate)

Delete the ZP library, ZP source variants, and `-fzpcall` compiler flag.
All code uses stack convention. This is a simplification with no functional loss
(ZP convention was an optimization, not a correctness requirement).

Update examples to remove `-fzpcall` and `c45_zp.lib` references.

### Step 2: Expand optimizer ZP range (immediate)

With $20-$2A no longer reserved for ZP calling convention, expand the vreg
allocator's available ZP pool. This provides an immediate optimization win:
more ZP slots for loop-invariant hoisting, register-resident variables, and
reduced spill pressure.

### Step 3: Link-time promotion analysis (Phase 101)

Implement `ZPPromotionAnalyzer` in the linker. Score functions, log candidates.
No rewriting yet — analysis and reporting only. Validate scoring against
hand-optimized programs.

### Step 4: Call site rewriting (Phase 102)

Implement `ZPCallRewriter`. Transform call sites from stack push to ZP store.
Start with leaf functions only (simplest case, no re-entry concerns).

### Step 5: Prologue rewriting (Phase 103)

Rewrite function prologues to read parameters from ZP instead of stack frame.
Enable direct ZP access optimization for unmodified parameters in leaf functions.

### Step 6: Interference-graph allocation (Phase 104)

Replace exclusive ZP allocation with graph-coloring allocation for better
ZP utilization when many functions are promoted.

## Expected Impact

### Code Size

- **Call sites**: -4 bytes per promoted 2-byte parameter (PHA/PLA elimination)
- **Prologues**: -2 to -4 bytes per promoted parameter (direct ZP vs indirect)
- **Library**: -380KB (c45_zp.lib eliminated entirely)
- **Net**: 5-15% code reduction for call-heavy programs

### Performance

- **Call overhead**: -12 cycles per promoted 2-byte parameter
- **Parameter access**: -3 cycles per access (ZP direct vs frame-relative indirect)
- **Inner loops calling promoted functions**: 20-40% faster (multiplicative savings)
- **Optimizer benefit**: Expanded ZP range improves all functions, not just promoted ones

### Maintenance

- **Library objects**: 96 fewer files to maintain
- **Test matrix**: Halved (no convention variant testing)
- **Makefile complexity**: Simplified (single lib target)
- **User-facing complexity**: No convention choice required

## Risks and Mitigations

| Risk | Mitigation |
|------|-----------|
| ZP pressure from promotion starves optimizer | Budget cap (`--zp-promote-budget`), optimizer pool takes priority |
| Recursive functions promoted incorrectly | Call graph analysis excludes recursive functions |
| Indirect calls (function pointers) can't be rewritten | Only promote functions with all call sites statically known |
| Binary patching introduces bugs | Pattern matching against known code generator output only; validation pass compares before/after behavior |
| Performance regression for ZP-heavy programs | Benchmark before/after; provide `-fno-link-zp-promote` escape hatch |

## References

- `doc/architecture/calling-conventions.md` — Stack and ZP calling convention details
- `src/main/IRCodeGen.cpp:4070-4180` — Current call site code generation
- `src/main/IRCodeGen.cpp:675-694` — ZP allocator initialization
- `src/main/O45Linker.cpp:2425-2895` — Thunk generation infrastructure
- `src/main/CrossModuleOptimizer.cpp` — Cross-module analysis framework
- `.plan/machine_state_framework.md` — MachineState design (for post-rewrite peephole)
