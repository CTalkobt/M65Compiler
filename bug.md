# Bug: IR CodeGen Frame-Relative Address Overflow

## Status
**Open** — 13 mmemu memory-dump tests fail with IR codegen as default.
30/30 shadow tests (simple programs) pass. Assembly-level tests all pass.

## Symptom
Programs with frame-allocated vRegs produce incorrect results when writing
through pointers. The 4th store in a sequence of `r[N] = val` (where `r` is
`volatile char *r = (char *)0x4000`) writes to the wrong memory address.
First 3 stores succeed; subsequent ones fail.

The program "escapes" the halt loop — PC ends up at $4000+ instead of
staying in the `jmp __halt` loop at $2003.

## Root Cause
`ldax.fp` / `stax.fp` simulated ops compute addresses as:
```
__sp_base + _fp + local_offset, X    (where X = SPL from TSX)
```

With `__sp_base = $0101`, `_fp = 8` (from 4 `phw #0` pushes = 8 bytes frame),
and `local_offset = 0` (first frame slot), the effective address operand is:
```
$0101 + 8 + 0 = $0109
```

After the 4 `phw` pushes + the `jsr` return address (10 bytes total on stack),
SPL = $FF - 10 = $F5. So the actual address accessed is:
```
$0109 + $F5 = $01FE    (wraps within 16-bit, correct stack page)
```

But that's the return address, not the frame data. The intended address for
frame slot 0 is `$01F5 + 1 = $01F6` (just above SP after the frame push).

**The formula `__sp_base + _fp + offset + SPL` double-counts the frame size.**
- `_fp` = 8 (accounts for 8 bytes of frame pushed via `phw`)
- `SPL` = $F5 (ALSO reflects those 8 bytes being on the stack)

The legacy CodeGenerator avoids this because it uses `_fp = 0` for
`void main()` with no locals — it doesn't push a local frame via `phw`.
Instead, it allocates locals using `.var` which bumps `_fp` without
pushing data. The `ldax.fp` formula then works because `_fp` counts
bytes that `.var` reserved (which changes the assembler's tracking of
what's on the stack) rather than bytes actually pushed.

## Key Insight
The IR CodeGen uses `phw #0` to physically push frame slots, then bumps
`_fp` to match. But `ldax.fp` expects `_fp` to represent the assembler's
`.var`-tracked frame size, not physical pushes. Physical pushes change SP
(reflected in SPL via TSX), while `.var` changes the assembler's `_fp`
symbol without modifying SP.

**`phw` changes SP. `.var` changes `_fp`. Using both for the same frame
causes double-counting.**

## Proposed Fix
Stop using `phw #0` for frame allocation. Instead, use the legacy pattern:
- Use `.var _fp = _fp + 2` for each local slot (bumps assembler's _fp)
- Use `stax.fp __vrN` to initialize (which writes to the correct address)
- Don't physically push anything — the `.var` bump tells the assembler
  where the frame extends to

This matches the legacy CodeGenerator's approach and avoids the
double-counting issue.

## Affected Tests (13)
All use `volatile char *r = (char *)0x4000` and write through the pointer:
- test_mmemu_control.c
- test_inline_asm.c
- test_multidim_array.c
- test_array_loop.c
- test_array_init.c
- test_struct_array.c
- test_short.c
- test_struct_return.c
- test_bitfield_mmemu.c
- test_compound_literal.c
- test_long_mmemu.c
- zpCall test
- zpCall mixed convention

## What Works
- 30/30 shadow tests (simple programs with no/small frames)
- All compiler unit tests (assembly output checks)
- All assembler validation tests
- All linker tests
- Programs without frame-allocated vRegs (all vRegs in ZP or AX)
- Programs with small frames where the address doesn't overflow

## Prior Fixes in This Session
1. **Simulated-op refactor** (`cd5ce3c`) — collapsed 3 dispatch chains
2. **IR definition** (`10b8817`) — IR.hpp data structures
3. **Linker convention thunks** (`4824096`) — both-direction thunks
4. **Line-map debug info** (`7f97d32`) — .debug.json sidecar files
5. **IRBuilder complete** (`83e4048`) — all 42 AST visitor methods
6. **IRCodeGen** (`b99029f`) — minimal viable IR→assembly emitter
7. **vReg allocator** (`ef99953`) — liveness analysis, AX/ZP/frame classes
8. **Shadow tests verified** (`3ad08fe`) — 10/10 on mmemu
9. **Expanded to 30 tests** (`248b843`) — all matching legacy
10. **Signed/unsigned tracking** (`699bfca`) — correct cmp ops
11. **IR as default** (`04173fb`) — LOAD/STORE indirect, strings, externs
12. **Dereference stores** (`fc45a78`) — *ptr = val, arr[i] = val patterns
13. **Frame cleanup at return** (`12a732d`) — @__return label before endproc
14. **Calling convention** (`db333c4`) — callee cleanup, ZP safety across calls
15. **Push order** (`72c0cc2`) — push .ax left-to-right, remove param .var
16. **_fp tracking** (`d996f21`) — bump/unbump around function calls
17. **Scratch conflicts** (`3434ceb`) — __zp_scratch2 for indirect addressing
18. **Symbolic frame access** (uncommitted) — ldax.fp __vrN instead of numeric

## Related Files
- `src/main/IRCodeGen.cpp` — the IR→assembly emitter
- `src/main/VRegAllocator.cpp` — vReg liveness + allocation
- `include/IRCodeGen.hpp` — IRCodeGen class definition
- `src/main/AssemblerSimulatedOps.cpp` — ldax.fp/stax.fp expansion
- `src/test/test_mmemu.sh` — failing mmemu tests
- `src/test/test_shadow_ir.sh` — passing shadow comparison tests

## Clarifying Questions (from Gemini) and Answers

### Q1: Stack Pointer Management
> If we stop using `phw #0` for frame allocation, how will we ensure the
> stack pointer is correctly moved to protect the frame data?

**Answer:** We DON'T stop using `phw #0`. The legacy CodeGenerator DOES use
`phw #0` to physically push frame slots. The fix is to **stop bumping `_fp`
after the pushes**. The legacy pattern is:

```asm
proc _add, W#_p_a, W#_p_b
    .var _fp = 0           ; _fp stays at 0!
    phw #0                 ; push 2 bytes (SP -= 2), _fp NOT bumped
    phw #0                 ; push 2 bytes (SP -= 2), _fp NOT bumped
    .local _l_x = 0       ; local at frame offset 0
    .local _l_y = 2       ; local at frame offset 2
    .var _p_b = 6          ; param b at offset 6 (past 4-byte frame + 2-byte ret addr)
    .var _p_a = 8          ; param a at offset 8
```

With `_fp = 0`, `ldax.fp _l_x` computes `__sp_base + 0 + 0 + SPL`, and SPL
correctly reflects the 4 bytes of pushes + 2 bytes of return address. No
double-counting.

### Q2: vReg Initialization
> Will the proposed fix include an explicit initialization step?

**Answer:** `phw #0` already zero-initializes. We keep using `phw #0`. The
only change is removing the `.var _fp = _fp + 2` that follows each `phw`.
So initialization behavior is unchanged.

### Q3: Interrupt Safety
> If we reserve space via `.var` without physically moving the stack pointer,
> the "frame" resides in unallocated stack space.

**Answer:** This concern is moot — we ARE physically moving SP via `phw #0`.
The proposed fix was poorly worded. The actual fix is: use `phw #0` (moves SP)
but DON'T bump `_fp` (keep it at 0). This is exactly what legacy does.

The space IS protected because SP physically moved. Interrupts push below the
current SP and won't corrupt the frame.

### Q4: Local Offset Calculation
> Will offsets need to be reversed or shifted?

**Answer:** No. The VRegAllocator assigns offsets starting from 0. With
`_fp = 0`, `.local __vr0 = 0` means "0 bytes above the frame base." The
formula `__sp_base + _fp + 0 + SPL` = `__sp_base + 0 + SPL` correctly
addresses the first pushed word. Offsets 0, 2, 4, 6 map to the 4 pushed
words in order.

The param `.var` offsets need to account for the frame size + return addr:
`.var _p_first = frameSize + 2 + cumulative`. With `_fp = 0`, this is just
`frameSize + 2` for the first param.

### Q5: Return Path Cleanup
> Do we still need to physically adjust the stack pointer on return?

**Answer:** Yes. The `phw` pushes must be matched by `pla` pops before `rts`.
The legacy uses `taz; pla; pla; ...; tza` to preserve the return value in Z
while popping. The `@__return` label emits this cleanup, then falls through
to `endproc` which emits `rts #N` for param cleanup.

## Corrected Fix
The actual fix is simpler than originally proposed:

**Remove the `.var _fp = _fp + 2` lines after each `phw #0`.**

Keep `phw #0` (physical push). Keep `.local __vrN = offset`. Keep `pla`
cleanup at `@__return`. Just stop telling the assembler that `_fp` grew.

The param `.var` offsets must be computed as `frameSize + 2 + cumulative`
(where frameSize is the total bytes pushed), NOT as `_fp + 2 + cumulative`
(since `_fp` will be 0).
