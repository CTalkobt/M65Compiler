# Plan: Fine-Grained Register & Flag Invalidation

## Problem

Every function call (`jsr`) triggers `invalidateRegs()` which wipes all 4 CPU
registers (A, X, Y, Z) and all flags (C, Z, N, V). This forces redundant
reloads after calls even when the callee only touches A and X.

The infrastructure for per-function metadata already exists:
- `O45FuncAttr` stores `regClobbers` and `flagClobbers` bitmasks in `.o45` files
- `.reg_clobbers` / `.flag_clobbers` directives are parsed by the assembler
- But the compiler emits only a blanket `.reg_clobbers A, X, Y, Z` (zpcall only)
  and nothing at all for stack-convention functions

## Scope

Two layers of optimization:

| Layer | Scope | Info Source |
|-------|-------|-------------|
| **Intra-TU** | Calls within same compilation unit | Compiler-collected clobber sets |
| **Inter-TU** | Calls to functions in other `.o45` files | `.o45` function attribute records |

This plan covers **Intra-TU** first (phases 1-3), then **Inter-TU** (phase 4).

---

## Phase 1: Collect Per-Function Clobber Sets in CodeGenerator — DONE

**Status:** Implemented 2026-05-04. All 256 tests pass.

**Goal:** Track which registers and flags each function actually modifies.

### Data Structures

Add to `CodeGenerator.hpp`:

```cpp
struct FuncClobberInfo {
    uint8_t regMask = 0;    // bit 0=A, 1=X, 2=Y, 3=Z
    uint8_t flagMask = 0;   // bit 0=C, 1=N, 2=Z, 3=V
    bool isLeaf = true;     // no calls to other functions
    bool complete = false;  // true once function body is fully visited
};

std::map<std::string, FuncClobberInfo> funcClobbers_;
FuncClobberInfo* currentClobbers_ = nullptr;  // points into funcClobbers_
```

### Collection Points

Instrument existing emission methods to OR in the appropriate bits:

| Emission | Register Bits | Flag Bits |
|----------|--------------|-----------|
| `emit("lda ...")` / `sta`, `adc`, `sbc`, etc. | A (0x01) | C,Z,N as appropriate |
| `emit("ldx ...")` / `stx`, `inx`, `dex` | X (0x02) | Z,N |
| `emit("ldy ...")` / `sty`, `iny`, `dey` | Y (0x04) | Z,N |
| `emit("ldz ...")` / `stz`, `inz`, `dez` | Z (0x08) | Z,N |
| `emit("clc")` / `sec` | — | C |
| `emit("jsr ...")` | mark `isLeaf = false` | — |

**Approach:** Rather than instrumenting every `emit()` string, add a
parallel tracking path in the high-level emitter methods on `M65Emitter`.
The emitter already knows which instructions it's generating. Add a
`clobberReg(uint8_t mask)` / `clobberFlag(uint8_t mask)` callback that
`CodeGenerator` provides.

Alternatively (simpler): since the compiler already tracks register state
via `updateRegA()` etc., the clobber set is the union of all registers
that were written during the function body. Add `currentClobbers_->regMask
|= 0x01` inside each `updateRegA/X/Y/Z` call, and `|= flag bits` inside
`updateFlags()` / `updateZNFlags()`.

**Simulated ops caveat:** Many simulated opcodes (e.g., `ldax`, `stax`,
`mul.16`, `add.16`) clobber registers internally. These already call
`invalidateRegs()` in the compiler, which doesn't help tracking. But since
the compiler emits these as assembly text, the *assembler* is the one that
knows the real clobber set. Phase 3 handles assembler-level collection.

### What This Phase Produces

`funcClobbers_["main"] = { regMask=0x0F, flagMask=0x07, isLeaf=false }`
etc., populated after visiting each FunctionDeclaration.

### Implementation Notes

- `clobberReg()`/`clobberFlag()` added to `updateReg*`, `updateFlags`,
  `updateZNFlags`, `invalidateRegs`, `invalidateFlags`, `transferRegs`
- Post-return `invalidateRegs()` in `visit(ReturnStatement)` suppressed
  from clobber recording (function has already returned at that point)
- `+1`/`-1` BinaryOperation path refined: uses targeted `regA/regX`
  invalidation instead of blanket `invalidateRegs()`
- `.reg_clobbers`/`.flag_clobbers` now emitted for both zpcall and stack
  calling conventions (moved outside `if (zpParamTotalBytes_ > 0)` gate)
- Stack-convention functions still report full clobber sets because
  simulated ops (`ldax ... , s`, `add.16`, etc.) call `invalidateRegs()`
  internally — refining these is lower priority since the main win is
  for fastcall leaf functions

---

## Phase 2: Selective Invalidation at Call Sites (Intra-TU)

**Goal:** Replace `invalidateRegs()` after `jsr` with selective invalidation
using the callee's known clobber set.

### New Method

```cpp
void CodeGenerator::invalidateFromClobbers(uint8_t regMask, uint8_t flagMask) {
    if (regMask & 0x01) { regA.known = false; /* clear fields */ }
    if (regMask & 0x02) { regX.known = false; }
    if (regMask & 0x04) { regY.known = false; }
    if (regMask & 0x08) { regZ.known = false; }
    if (flagMask & 0x01) flags.carry = TriState::UNKNOWN;
    if (flagMask & 0x02) flags.negative = TriState::UNKNOWN;
    if (flagMask & 0x04) flags.zero = TriState::UNKNOWN;
    if (flagMask & 0x08) flags.overflow = TriState::UNKNOWN;
    if (flagMask & 0x06) flags.znSource = FlagSource::NONE;  // Z or N unknown
}
```

### Call Sites to Modify

In `visit(FunctionCall&)`, replace `invalidateRegs()` with:

```cpp
auto it = funcClobbers_.find(node.name);
if (it != funcClobbers_.end() && it->second.complete) {
    invalidateFromClobbers(it->second.regMask, it->second.flagMask);
} else {
    invalidateRegs();  // unknown function or forward reference — conservative
}
```

### Forward References / Recursion

Functions may be called before they are defined (forward declarations,
mutual recursion). Two strategies:

1. **Conservative fallback** (simplest): If `funcClobbers_[name].complete
   == false`, use `invalidateRegs()`. Most C code defines helper functions
   before the caller.

2. **Two-pass approach** (better): First pass walks all function bodies to
   collect clobber sets, second pass does code generation. This is heavier
   but handles all orderings. Can be deferred to a later iteration.

**Recommendation:** Start with strategy 1. The common case (helpers defined
before `main`) benefits immediately. Add a `--clobber-analysis` flag for
the two-pass mode later.

### Transitive Clobbers

If function `A` calls function `B`, then `A` transitively clobbers
everything `B` clobbers. When `B.complete == true` at the time we process
`A`'s call to `B`, merge:

```cpp
currentClobbers_->regMask |= calleeClobbers.regMask;
currentClobbers_->flagMask |= calleeClobbers.flagMask;
currentClobbers_->isLeaf = false;
```

This gives callers of `A` an accurate picture.

### Indirect Calls / Variadic / extern

These remain `invalidateRegs()` (conservative). The callee is unknown at
compile time. Phase 4 can improve this for indirect calls where the set of
possible targets is known.

---

## Phase 3: Emit Accurate `.reg_clobbers` / `.flag_clobbers` Directives

**Goal:** Replace the blanket `.reg_clobbers A, X, Y, Z` with the actual
clobber set computed in Phase 1.

### Changes

At `endproc` emission (CodeGenerator.cpp:1176), replace:

```cpp
emit(".reg_clobbers A, X, Y, Z");
```

With:

```cpp
auto& ci = funcClobbers_[currentFunctionName];
// Emit .reg_clobbers
std::string regs;
if (ci.regMask & 0x01) regs += "A, ";
if (ci.regMask & 0x02) regs += "X, ";
if (ci.regMask & 0x04) regs += "Y, ";
if (ci.regMask & 0x08) regs += "Z, ";
if (!regs.empty()) {
    regs.pop_back(); regs.pop_back(); // trim ", "
    emit(".reg_clobbers " + regs);
}
// Emit .flag_clobbers
std::string fl;
if (ci.flagMask & 0x01) fl += "C, ";
if (ci.flagMask & 0x02) fl += "N, ";
if (ci.flagMask & 0x04) fl += "Z, ";
if (ci.flagMask & 0x08) fl += "V, ";
if (!fl.empty()) {
    fl.pop_back(); fl.pop_back();
    emit(".flag_clobbers " + fl);
}
```

Also emit for stack-convention functions (currently nothing is emitted).

### Leaf Function Flag

```cpp
if (ci.isLeaf) emit(".func_flags leaf");
```

(Requires adding `.func_flags` directive to the assembler, writing to
`ProcContext::funcFlags` bit 0.)

---

## Phase 4: Inter-TU Optimization via `.o45` Metadata

**Goal:** When calling an external function (`.extern`), use its
`O45FuncAttr` from the linked `.o45` to avoid full invalidation.

### Approach A: Header Files (Simplest)

Define a convention for declaring clobber sets in C headers:

```c
__attribute__((reg_clobbers(A, X)))
__attribute__((flag_clobbers(C, Z, N)))
int add(int a, int b);
```

Or via pragma:

```c
#pragma clobbers reg(A, X) flag(C, Z, N)
int add(int a, int b);
```

The compiler records these in `funcClobbers_` at parse time, enabling
Phase 2 optimizations for cross-TU calls.

### Approach B: Linker-Assisted (More Powerful)

The linker already reads `O45FuncAttr` records. It could:

1. Build a call graph from relocation entries (every `R_WORD` to a function
   symbol is a call edge)
2. Compute transitive clobber sets
3. Emit an optimized binary where caller save/restore is minimized

This is a larger project and can be deferred. The `.o45` format already
stores the needed data.

### Approach C: Import Metadata Sidecar

`ln45 -emit-clobbers lib.o45 > lib_clobbers.h`

Generates a header with `#pragma clobbers` for each exported function.
Projects include this auto-generated header for cross-TU optimization.

**Recommendation:** Start with Approach A (explicit header annotations).
It's zero-infrastructure, works today with Phases 1-3, and the common case
(stdlib functions) can ship with annotated headers.

---

## Phase 5: Assembler-Level Fine-Grained Tracking

**Goal:** The assembler optimizer (`AssemblerOptimizer.cpp`) currently
invalidates all registers at `JSR`. Make it clobber-aware too.

### Current State

`AssemblerOptimizer::optimize()` (line 64-73) invalidates all register
state on seeing `JSR`, `RTS`, `CALL`, etc.

### Change

When the optimizer sees `JSR _funcname`, look up the function's
`ProcContext` (if in the same assembly file) or the `.o45` import metadata
to find `regClobbersMask`. Only invalidate matching registers.

This is lower priority — the assembler optimizer does redundant-load
elimination which is a smaller win than the compiler-level tracking. But
it's a natural extension once the data flows through.

---

## Implementation Order

| Phase | Effort | Impact | Dependencies |
|-------|--------|--------|--------------|
| 1 | Small | Foundation | None |
| 2 | Medium | Main optimization win | Phase 1 |
| 3 | Small | Enables inter-TU | Phase 1 |
| 4A | Small | Cross-TU via headers | Phases 2-3 |
| 5 | Medium | Assembler-level gains | Phase 3 |
| 4B/C | Large | Linker-level IPO | Phases 3, ln45 changes |

## Risks and Mitigations

**Correctness risk:** Under-reporting clobbers causes wrong code (stale
register values used after call). Mitigations:
- Conservative default: unknown/extern functions clobber everything
- Transitive closure: a caller inherits all callee clobbers
- Test suite: existing tests catch regressions; add targeted tests for
  "value survives across call" patterns

**Simulated opcodes:** The compiler emits pseudo-ops like `ldax`, `add.16`
that expand to multi-instruction sequences in the assembler. The compiler
may not know the exact register clobber set of these expansions. Mitigation:
the compiler already calls `invalidateRegs()` after emitting most simulated
ops; this invalidation is *within* the function body and contributes to the
function's clobber set correctly.

**Inline assembly:** `__asm__()` already triggers `invalidateRegs()`. The
clobber tracking should treat inline asm as clobbering A, X, Y, Z, all
flags (conservative). Future: `__asm__` with explicit clobber lists like
GCC's extended asm.

## Quick Win: Leaf Functions

Many small utility functions (getters, simple math) don't call other
functions and only touch A and X. These are leaf functions. Phase 1+2
immediately helps callers of leaf functions preserve Y and Z register
values across calls. This is the most common optimization opportunity.

## Example

```c
char get_flag(void) {
    return *(volatile char*)0xD610;  // touches A only
}

void process(void) {
    register int i;
    for (i = 0; i < 10; i++) {
        char f = get_flag();   // currently: invalidates i's ZP tracking
                                // after: only A clobbered, X/Y/Z preserved
        // ... use f and i ...
    }
}
```

After Phase 2, `get_flag` has `regMask = 0x01` (A only), `flagMask =
0x06` (Z, N). The call invalidates only A and Z/N flags. If `i` is tracked
in X or Y, that tracking survives.
