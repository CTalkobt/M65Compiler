# Bug #186: asctime day/month name strings garbled when linked from library

**Status:** Open (partially fixed — 4 of 5 sub-issues resolved)
**Labels:** bugfix, assembler, linker
**Affects:** asctime(), any `ldax #relocatable_symbol` in .o45 objects

---

## Summary

`asctime()` returns a buffer where numeric fields (day, time, year) are correct but day/month NAME strings are zeros or garbage. The root cause is a chain of 5 issues, 4 of which are now fixed. The remaining issue is that `emitLDWCode` (which handles `ldax #symbol`) does not record symbol relocations for immediate-mode loads.

---

## Sub-Issues (Investigation History)

### 1. R_HIGH extra field — FIXED (commit 4cf4fa9)

**Problem:** `emitLoadAddrConst()` in `AssemblerSimulatedOps.cpp:3277` passed `offset & 0xFF` (always 0 from caller) instead of `total & 0xFF` (baseVal + offset) as the R_HIGH `extra` byte. The linker uses `extra` for carry correction when relocating split R_LOW/R_HIGH pairs.

**Fix:** Changed line 3277 from:
```cpp
e.recordSymbolRelocHi(name, offset & 0xFF);
```
to:
```cpp
e.recordSymbolRelocHi(name, total & 0xFF);
```

**Verification:** `test_2arr.c` (inline pointer arrays) produces correct R_HIGH extras ($19, $27) and both day/month names display correctly.

**Files:** `src/main/AssemblerSimulatedOps.cpp:3277`

---

### 2. Data/BSS segment interleaving — FIXED (commit 4cf4fa9)

**Problem:** When globals alternate between initialized (DATA) and uninitialized (BSS), `emitGlobals()` in IRCodeGen only emitted `.segment "data"` ONCE (on first init global). After switching to `.segment "bss"` for uninit globals, subsequent init globals (like `_day_names`, `_mon_names`) remained in BSS.

In rtc.c, the global layout is:
```
_days_in_month  → initialized (DATA)
___tm_buf       → uninitialized (BSS)
___asc_buf      → uninitialized (BSS)
_day_names      → initialized (DATA) ← was going to BSS!
_mon_names      → initialized (DATA) ← was going to BSS!
```

**Fix:** In `IRCodeGen::emitGlobals()`, re-emit `.segment "data"` when an initialized global follows uninit globals:
```cpp
if (!hasData || hasBss) {
    if (relocMode) {
        emit(".segment \"data\"");
        if (!hasData) emit(".byte 0"); // pad byte
    }
    hasData = true;
    hasBss = false;
}
```

**Verification:** `rtc.o45` now shows R_HIGH DATA (not BSS) for `_day_names` and `_mon_names` references. BSS extras shifted by +1 correctly.

**Files:** `src/main/IRCodeGen.cpp:570-585`

---

### 3. Data segment pad byte — FIXED (commit 4cf4fa9)

**Problem:** The .o65 relocation encoding uses delta byte $00 as end-of-table. Relocations at segment offset 0 produce delta=0 and are silently dropped by `O45RelocEncoder::encode()`.

**Fix:** Emit a 1-byte pad (`.byte 0`) at the start of the data segment in relocatable mode. All data relocations are at offset >= 1.

**Verification:** All 19 DATA section relocations (for `.word __str_N` entries) now appear in the .o45 file.

**Files:** `src/main/IRCodeGen.cpp:574-579`, `src/main/O45Writer.cpp:40-43`

---

### 4. addr_elem.16 frame access check — FIXED (commit 4cf4fa9)

**Problem:** `dispatch_AddrElem()` called `resolveFrameAccess(idxBaseStart)` even for immediate bases (`#_day_names`). When `baseFa.isFrame` returned true (incorrectly), it used `emitSpBaseAddrCalc16` instead of `emitLoadAddrConst`, producing wrong relocations.

**Fix:** Skip frame check when base is immediate:
```cpp
if (baseFa.isFrame && !baseIsImmediate) {
    emitSpBaseAddrCalc16(...);
}
```

**Verification:** `test_2arr.c` (linked from library) shows correct day AND month names.

**Files:** `src/main/AssemblerSimulatedOps.cpp:3441`

---

### 5. ldax #symbol missing relocations — OPEN

**Problem:** `emitLDWCode()` in `AssemblerSimulatedOps.cpp:442-444` handles immediate-mode loads (`ldax #___asc_buf`) by emitting `LDA #lo; LDX #hi` but does NOT record R_LOW/R_HIGH symbol relocations. The linker cannot fix up these addresses when linking.

This causes TWO separate `ldax #___asc_buf` references in the same function to produce DIFFERENT linked addresses (one at $43C2, other at $443F), because the assembly-time addresses are baked in and not relocated.

**Current code (line 442-444):**
```cpp
if (isImm) {
    uint32_t val = srcAst->getValue(parser);
    e.lda_imm(val & 0xFF);
    uint8_t val2 = (val >> 8) & 0xFF;
    if (reg2 == 'X') e.ldx_imm(val2);
    else if (reg2 == 'Y') e.ldy_imm(val2);
    else if (reg2 == 'Z') e.ldz_imm(val2);
}
```

No calls to `e.recordSymbolRelocLo()` or `e.recordSymbolRelocHi()`.

**Attempt 1: Inline relocation recording**
```cpp
if (isImm) {
    uint32_t val = srcAst->getValue(parser);
    std::string srcName = parser->tokens[tokenIndex + 1].value;
    bool isReloc = parser->isRelocatableSymbol(srcName);
    if (isReloc) e.recordSymbolRelocLo(srcName);
    e.lda_imm(val & 0xFF);
    if (isReloc) e.recordSymbolRelocHi(srcName, val & 0xFF);
    uint8_t val2 = (val >> 8) & 0xFF;
    if (reg2 == 'X') e.ldx_imm(val2); ...
}
```

**Result:** Program crashes before main(). No output at $4000 (not even pre-asctime checkpoints). Tests pass but linked programs with library rtc.o45 fail.

**Likely cause:** The `tokens[tokenIndex + 1]` assumption may be wrong for some token layouts, OR the `isRelocatableSymbol` call during the sizing pass has side effects on parser state.

**Attempt 2: Delegate to emitLoadAddrConst**
```cpp
if (isImm) {
    std::string srcName = parser->tokens[tokenIndex + 1].value;
    emitLoadAddrConst(parser, e, srcName, 0, reg2);
}
```

**Result:** Same crash. The `emitLoadAddrConst` function calls `resolveSymbol()` and `isRelocatableSymbol()` which may interact with the parser's expression evaluation state during the sizing pass. The sizing pass runs `emitFn` with a temporary M65Emitter to compute instruction sizes. If `resolveSymbol` or `parseExprAST` advances token indices as a side effect, the sizing pass leaves the parser in a different state than expected.

**Key observation:** `emitLoadAddrConst` works correctly when called from `dispatch_AddrElem` (for `addr_elem.16 #_day_names`), but crashes when called from `emitLDWCode` (for `ldax #___asc_buf`). The difference may be in how the token indices are set up or in the parser's current scope state.

---

## Relevant Code Locations

| File | Line | Function | Purpose |
|------|------|----------|---------|
| `src/main/AssemblerSimulatedOps.cpp` | 404 | `emitLDWCode` | Handles `ldax/lday/ldaz` simulated ops |
| `src/main/AssemblerSimulatedOps.cpp` | 442 | (immediate path) | **BUG: no relocation recording** |
| `src/main/AssemblerSimulatedOps.cpp` | 3260 | `emitLoadAddrConst` | Correct implementation with relocs |
| `src/main/AssemblerSimulatedOps.cpp` | 3387 | `dispatch_AddrElem` | Calls emitLoadAddrConst for #symbol |
| `src/main/O45Emitter.cpp` | 79-88 | (Step 1 reloc scan) | Calls emitFn to collect symbol relocs |
| `src/main/O45Emitter.cpp` | 492 | (symbol reloc scan) | Builds relocations from M65Emitter |
| `src/main/M65Emitter.cpp` | 405-411 | `recordSymbolRelocLo/Hi` | Records relocation metadata |
| `src/main/O45Writer.cpp` | 19 | `O45RelocEncoder::encode` | Encodes relocations to byte stream |
| `src/main/O45Linker.cpp` | 467-512 | `applyRelocs` (internal) | Applies R_LOW/R_HIGH relocations |
| `lib/stdlib/rtc.c` | 116-147 | `asctime` | Uses `ldax #___asc_buf` (2 sites) |

---

## Key Assumptions

1. **`int` defaults to unsigned** — all stdlib uses `signed int` explicitly
2. **45GS02 STZ stores Z register** (not zero like 65C02)
3. **cc45 invokes ca45 as a separate process** via `std::system()` (line 791 of cc45_main.cpp). Both must be rebuilt together.
4. **Stale installed binaries can shadow local builds** — `/usr/local/bin/ca45` or `~/.local/bin/ca45` may be used instead of `./bin/ca45`. Use `make uninstall_local` to clean.
5. **.o65 relocation delta $00 = end-of-table** — relocations at segment offset 0 are unrepresentable. Data segments have a 1-byte pad.
6. **Segment switching in assembly** — `.segment "data"` / `.segment "bss"` must be re-emitted when switching back. The assembler saves/restores per-segment PCs.
7. **O45Emitter sizing pass** — runs each `emitFn` with a throwaway M65Emitter to compute byte sizes. Must produce the EXACT same byte count as the final emit pass. Side effects from `resolveSymbol`/`parseExprAST` during sizing can corrupt parser state.

---

## Reproduction

```c
#include <time.h>
volatile char *r = (char *)0x4000;
void main() {
    struct tm t;
    t.tm_wday = 4; t.tm_mon = 0; t.tm_mday = 15;
    t.tm_hour = 9; t.tm_min = 5; t.tm_sec = 30;
    t.tm_year = 126; t.tm_yday = 14; t.tm_isdst = 0;
    r[0] = 0x42; // checkpoint
    char *s = asctime(&t);
    r[1] = s ? s[0] : 0xFF;  // s[0] should be 't' ($74), gets $00
    r[2] = 0xAA; // sentinel
}
```

Compile and link: `cc45 -c test.c -o test.o45 -Ilib/include && ln45 test.o45 c45.lib -o test.prg`

Expected: `42 74 AA`
Actual: `42 00 AA` (s[0]=0 because __asc_buf address is wrong)

---

## Next Steps

1. **Investigate sizing pass side effects**: Check if `emitLoadAddrConst` or the inline reloc recording changes the parser's token position state during the sizing pass (O45Emitter line 79-88). The `parseExprAST` call in `emitLDWCode` line 440 may advance `idx` differently between passes.

2. **Compare token indices**: Verify that `parser->tokens[tokenIndex + 1]` correctly identifies the symbol name for ALL token layouts of `ldax #symbol`. Edge cases: scoped symbols (`@_l_name`), expressions (`#symbol+offset`).

3. **Alternative approach**: Instead of modifying `emitLDWCode`, modify the O45Emitter's instruction-level relocation scanner to detect `LDA #imm; LDX #imm` patterns that reference relocatable symbols and generate R_LOW/R_HIGH there.

---

## Related Issues

- #183: bsearch store-forwarding (FIXED)
- #185: *p++ codegen with DEREF opcode (FIXED)
- Pointer array element size and string literal init (FIXED)
- Unused variable warning for address-of expressions (FIXED)
