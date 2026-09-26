# Phase 92 IPO: .callsite Directive Specification

**Date:** 2026-08-20  
**Version:** 1.0  
**Status:** Specification Complete

---

## Overview

The `.callsite` directive enables Phase 92 IPO cross-module function specialization by recording function calls with constant argument patterns in relocatable object files (`.o45`).

---

## Syntax

```asm
.callsite FUNCTION_NAME paramMask=0xHH values=0xVV,0xVV,... [line=NNN]
```

### Parameters

| Parameter | Required | Type | Description |
|-----------|----------|------|-------------|
| `FUNCTION_NAME` | Yes | identifier | Function being called (e.g., `_multiply_by_n`) |
| `paramMask` | Yes | hex | Bitmask indicating which parameters are constants (bit i = param[i] is const) |
| `values` | Yes | hex list | Comma-separated constant values for each parameter (max 4) |
| `line` | No | decimal | Source line number (for debugging) |

---

## Examples

### Example 1: Single Constant Parameter

```c
// Source: multiply_by_n(x, 4);
```

Emitted directive:
```asm
.callsite _multiply_by_n paramMask=0x2 values=0x4 line=42
```

**Explanation:**
- `paramMask=0x2` → binary 0010 → param[1] is constant
- `values=0x4` → param[1] value is 4
- Line 42 is source location

### Example 2: Multiple Constant Parameters

```c
// Source: helper_add(x, 10, 5);
```

Emitted directive:
```asm
.callsite _helper_add paramMask=0x6 values=0xa,0x5 line=55
```

**Explanation:**
- `paramMask=0x6` → binary 0110 → params[1] and [2] are constant
- `values=0xa,0x5` → param[1]=10, param[2]=5

### Example 3: Large Constant (32-bit)

```c
// Source: load_address(ptr, 0x12345678);
```

Emitted directive:
```asm
.callsite _load_address paramMask=0x2 values=0x12345678 line=88
```

---

## Semantics

### When to Emit

The compiler emits `.callsite` directives when:

1. **Function call** (`CALL` or `CALL_VOID` IR operation)
2. **Arguments detected as constants** (either:
   - Integer literal passed directly: `func(42)`
   - Variable with constant initialization: `const int N = 4; func(N)`
   - Expression that folds to constant: `func(2+2)`)
3. **Call is not optimized away** (function not inlined)

### When NOT to Emit

- Generic function calls with all non-constant arguments
- Inlined function calls (handled separately, no CALL instruction)
- Indirect calls through function pointers
- Calls to built-in functions (__builtin_*, etc.)

---

## Object File Storage

The `.callsite` directives are stored in the `.o45` relocatable object file as:

1. **Metadata Section** (`.callsite_metadata`)
   ```
   Offset  | Content
   --------|----------
   0       | Number of entries (N)
   4       | Entry 1: function name length
   5       | Entry 1: function name (string)
   ...     | Entry 1: paramMask, values[], line
   ...     | Entry 2...N
   ```

2. **Index** (`.callsite_index`)
   - Maps function name → offsets in metadata section
   - Enables O(1) lookup during linking

---

## Linker Processing (Phase 92.2)

### During Linking

1. **Parse .callsite directives** from all `.o45` input files
2. **Build cross-module call graph:**
   ```
   _multiply_by_n:
     pattern[1]=4: count=2, modules=[a.o45, b.o45]
     pattern[1]=8: count=1, modules=[a.o45]
   ```

3. **Identify specialization candidates:**
   - If pattern appears in 2+ modules: specialize
   - If pattern only in 1 module: keep per-module variant
   - If ROI (code_saved > routing_overhead): specialize

4. **Generate routing stubs** (Phase 92.2)
   - Route calls based on constant pattern
   - Dispatch to specialized variant

---

## Performance Considerations

### Metadata Size Overhead

Per call site:
- Function name: ~10-20 bytes (typical identifier)
- Constant values: 8 bytes (max 2 constants × 4 bytes each)
- Metadata fields (mask, line): 4 bytes
- **Total per call site: ~20-30 bytes**

**Typical Program:**
- 100 call sites with specialization potential
- Metadata overhead: 2-3 KB
- Easily amortized by 8-15% code reduction

### Linker Performance

Call graph construction:
- Parse all `.o45` files: O(N) where N = #modules
- Build pattern map: O(M log M) where M = #unique patterns
- Identify candidates: O(M)
- **Total: O(N + M log M)** — linear in file count

---

## Examples in Assembly Output

### With Phase 92.1 Enabled

```asm
; Function: multiply_by_n
proc _multiply_by_n
    ; ... function body ...
    rts
endproc

; Call sites with constant patterns
.callsite _multiply_by_n paramMask=0x2 values=0x4 line=42
.callsite _multiply_by_n paramMask=0x2 values=0x8 line=55
.callsite _multiply_by_n paramMask=0x0 line=70  ; no constants
```

### Object File (.o45) Structure

```
.o45 relocatable object
├── .text
│   └── Code section (including JSR instructions)
├── .data
│   └── Initialized data
├── .bss
│   └── Uninitialized data
├── .callsite_metadata       ← NEW in Phase 92.1
│   └── Call site patterns with constants
├── .callsite_index          ← NEW in Phase 92.1
│   └── Fast lookup index
├── .symtab
│   └── Symbol table
└── .reloc
    └── Relocations
```

---

## Compatibility

### With Existing Tools

- **Old assemblers (ca45 v1.0.7):** Ignore unknown `.callsite` directives
- **Old linkers (ln45 v1.0.7):** Ignore `.callsite_metadata` sections
- **Backward compatible:** Binaries produced identical to Phase 91

### With Calling Conventions

Works with all three conventions:
- Stack convention: Parameter values tracked normally
- ZP convention: Constant parameters in ZP slots
- SAC convention: Static AR parameters

---

## Future Extensions

**Phase 92.3+: Enhanced Patterns**
- Array indices: `.callsite func paramMask=0x2 arraySize=256`
- Bitfield patterns: `.callsite func paramMask=0x2 bitMask=0xFF`
- Value ranges: `.callsite func paramMask=0x2 valueRange=0x0-0x10`

---

## Implementation Checklist

- ✅ Directive syntax defined
- ✅ Semantics specified
- ✅ Object file format defined
- ✅ Linker processing algorithm documented
- ⏳ Compiler implementation (Phase 92.1)
- ⏳ Assembler support (ca45 enhancement)
- ⏳ Linker support (ln45 enhancement)

---

