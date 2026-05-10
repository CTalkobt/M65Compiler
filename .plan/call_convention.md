# Calling Convention Enforcement via Bit Flag in .o45 Format

**Status**: Approved for implementation  
**Target**: v1.0 release  
**Complexity**: Medium — touches 7 files, 1 new bit, straightforward changes

---

## Problem Statement

The MEGA65 C compiler supports two calling conventions:
- **Stack-based** (default): parameters on stack, return in AXYZ
- **ZP-based** (`-fzpcall`): parameters in zero page, faster, per-function clobber tracking

Linking objects compiled with mismatched conventions is a silent wrong-answer bug. Example:
```c
// a.c compiled with -fzpcall
extern void foo(int x);
void caller() { foo(42); }  // sets up ZP param

// b.c compiled without -fzpcall
void foo(int x) { /* reads x from stack */ }  // x is garbage (or on ZP if fastcall)
```

The linker must detect and reject this.

---

## Solution Design

### Flags Byte in `.o45` Format

The `O45FuncAttr.flags` byte gains one new bit and one renamed bit:

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | `FUNC_FLAG_LEAF` | Makes no calls (existing, never emitted) |
| 1 | `FUNC_FLAG_REENTRANT` | Re-entrant safe: no global mutable state, stack-only locals (rename from INT_SAFE which was never emitted) |
| 2 | `FUNC_FLAG_ZP_CONV` | ZP calling convention; 0 = stack-based (NEW) |
| 3–7 | — | Reserved, must zero |

**Convention Known**: When `hasFuncAttr=true` on an export, convention is known (bit 2 determines which). When `hasFuncAttr=false`, convention is unknown (old object).

### Enforcement Rule

**Only one direction is enforced at link time**: ZP→stack calls are errors. Stack→ZP is not enforced because:
- ZP→stack is always catastrophically wrong (ZP code has no way to pass stack params)
- Stack→ZP may be intentional (fastcall: compiler generates ZP setup at each call site)
- Enforcing only ZP→stack catches the most dangerous case without false positives

### Compiler Emission

Both ZP and stack codegen paths emit `.func_flags`:
- ZP path (line ~1262 in CodeGenerator.cpp): `.func_flags zp_call [, leaf]`
- Stack path (line ~1371 in CodeGenerator.cpp): `.func_flags stack_call [, leaf]`

This happens for every function (variadic functions use stack convention, so they get `stack_call`).

### Assembler Parsing

The `.func_flags` directive accepts space/comma-separated flags:
```asm
.func_flags zp_call, leaf
.func_flags stack_call
.func_flags reentrant
```

Parsed by existing token-stream mechanism (like `.reg_clobbers`, `.flag_clobbers`).

### Linker Enforcement

In `O45Linker::emitDiagnostics()`, for each call-graph edge (caller → callee):
- If both have `hasFuncAttr=true` AND caller has `ZP_CONV` AND callee lacks `ZP_CONV` → linker error
- Otherwise → pass (includes unknown callees for backward compat, fastcall callees)

Error becomes a hard linker failure (alongside duplicate symbols, undefined imports, etc.).

### Tools

- **nm45 -f**: displays convention + leaf/reentrant in symbol listing
- **objdump45 -t**: displays convention in symbol table (requires shared `formatFuncAttr()` in new header)

---

## Implementation Outline

### 1. `include/O45Types.hpp`
```cpp
constexpr uint8_t FUNC_FLAG_LEAF      = 0x01;
constexpr uint8_t FUNC_FLAG_REENTRANT = 0x02;
constexpr uint8_t FUNC_FLAG_ZP_CONV   = 0x04;
```

### 2. `src/main/AssemblerParser.cpp` (~line 594)
Add `.func_flags` directive handler using existing token-stream pattern:
```cpp
else if (stmt->dir.name == "func_flags") {
    // parse: zp_call | stack_call | leaf | reentrant
    // set bits accordingly, mark hasFuncAttrs = true
}
```

### 3. `src/main/CodeGenerator.cpp`
- ZP path (~1262): `emit(".func_flags zp_call" + (isLeaf ? ", leaf" : ""));`
- Stack path (~1371): add CallCollector for `isLeaf`, then same emission

### 4. `src/main/O45Linker.cpp`
- `O45Linker.hpp`: add `std::vector<std::string> convErrors_;`
- `emitDiagnostics()`: walk callGraph, check ZP→stack mismatch, populate `convErrors_`
- `link()`: after `emitDiagnostics()`, if errors, return false with message

### 5. `src/main/nm45_main.cpp`
- `formatFuncAttr()`: prepend convention + leaf/reentrant before existing `uses:` output

### 6. `src/main/objdump45_main.cpp` + new `include/O45FormatUtil.hpp`
- Extract `formatFuncAttr()` from nm45 to shared inline header
- `printSymbolTable()`: print funcAttr after each symbol if `hasFuncAttr=true`

### 7. `doc/lib45.md`
- Document flags byte table
- Explain hasFuncAttr as "convention known" signal
- Note ZP→stack enforcement only

---

## Testing Strategy

1. **Compile without change** (`make test`): verify all existing tests pass (old objects have `hasFuncAttr=false`)
2. **Mismatch detection**: 
   - Create `test_zp_to_stack.c`: compile with `-fzpcall`, call extern `target_stack()`
   - Create `test_stack_caller.c`: compile without flag, define `target_stack()`
   - Link them: expect linker error mentioning convention mismatch
3. **Tools verification**:
   - `nm45 -f test_zp.o45`: verify `zp_call` appears in output
   - `objdump45 -t test_zp.o45`: verify convention shown in symbol table
4. **Backward compat**:
   - Ensure old .o45 files (no funcAttr) link without error

---

## Bits Reserved for Future

Bits 3–7 in `O45FuncAttr.flags` remain reserved. No changes to `O45_FUNCATTR_SIZE` (remains 16 bytes).

---

## Related Issues / Follow-up

- **Per-file convention checking**: Could emit compiler warnings when calling unknown-convention externs from ZP-convention code (optional, post-v1.0).
- **Fastcall attributes in headers**: Could extend C syntax to mark library functions as `__fastcall__` in headers, enabling per-call site checking (post-v1.0).
- **Leaf detection for stack paths**: Currently not always computed; could optimize further.

---

## References

- `doc/lib45.md` — .o45 format specification
- `CLAUDE.md` — Architecture & conventions
- `src/main/CodeGenerator.cpp` — where calling conventions are chosen and metadata emitted
- `src/main/O45Linker.cpp` — where enforcement will be added
