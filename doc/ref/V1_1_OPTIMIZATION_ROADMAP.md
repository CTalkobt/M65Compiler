# v1.1 Optimization Roadmap — Code Size & Performance

**Written:** 2026-07-19 (planning) · **Status reviewed:** 2026-10-11 against the current compiler

This plan targeted frame-pointer overhead in the **stack calling convention**. Since it was written, the defaults changed: cc45 now uses the ZP calling convention (`-fzpcall`) and static allocation (SAC, `-fstaticalloc`) by default, so most functions have no stack frame and no frame-pointer setup at all. The items below say what the compiler does today; the original percentage estimates were never measured and have been removed.

How the status was checked: small test functions compiled with `bin/cc45 -S -O2` (default mode) and `bin/cc45 -S -O2 -fno-zpcall -fno-staticalloc` (stack frames), reading the `.s` output and the `objdump45 -d` of the `-c` object.

## Status Summary

| # | Item | Status |
|---|------|--------|
| 1 | Lazy frame-pointer setup | **Done** (FP set up only for functions with a stack frame) |
| 2 | No FP recalculation after calls | **Done / not an issue** (FP set up once per function) |
| 3 | Frame layout without alignment gaps | **Not applicable** (stack locals are not aligned) |
| 4 | Bitfield code generation | **Open** |
| 5 | Calling-convention improvements | **Done** (ZP default, SAC, linker thunks) |

## 1. Lazy Frame Pointer Initialization — Done

A stack-frame function sets up the frame pointer in `$FD/$FE` with

```asm
tsy
tsx
inx
bne @fp_no_carry
iny
@fp_no_carry:
stx $FD
sty $FE
```

after saving the caller's `$FD/$FE`. Today this happens only when the function has a stack frame. Functions without frame storage (for example a `void f(void)` that only writes globals) get no FP code even with `-fno-zpcall -fno-staticalloc`, and in the default mode (ZP + SAC) non-recursive functions keep parameters and locals in ZP/static storage, so none of the test functions had any FP code.

Leaf detection exists: leaf functions are marked `leaf` in `.func_flags`. A leaf that has stack parameters in `-fno-zpcall -fno-staticalloc` mode still sets up the FP, because it addresses its parameters through it — there is no SP-relative addressing path that would let it skip the setup. That remains a possible refinement for stack-convention code only.

## 2. Smart Frame Pointer Recalculation — Done / not an issue

In the stack-frame output, the FP is computed once in the prologue and reused after `JSR`s; there is no "FP setup → JSR → FP setup" pattern to remove.

## 3. Frame Allocation Optimization — Not applicable

The plan assumed natural alignment gaps between differently sized locals. cc45 does not align stack-frame locals (`_Alignas`/`aligned` apply to globals, statics and SAC locals only), so there are no gaps to remove by sorting.

## 4. Bitfield-Specific Optimizations — Open

A bitfield store in a global struct is emitted as the `bfins16.ind` pseudo-op, which the assembler expands to a 16-bit read-modify-write. For example `gs.a = 1; gs.c = 0; gs.b = 5;` on a global `struct { unsigned a:1, b:3, c:1; }` at `-O2` produces, per field, an address computation, a 16-bit mask/shift sequence, a byte read-modify-write and a 2-byte store back — no `TSB`/`TRB`, and no merging of consecutive stores to the same storage unit. Opportunities still open:

- single-bit constant stores → `TSB`/`TRB` (or `SMB`/`RMB` for ZP);
- combine consecutive stores to the same storage unit;
- avoid recomputing the field address and the 16-bit write-back when the field fits in one byte.

## 5. Calling Convention Optimization — Done

- The ZP convention (parameters in `$10`–`$2F`) is the default; variadic functions and parameter lists over 32 bytes fall back to the stack.
- SAC (static activation records) is the default for non-recursive functions.
- ln45 generates stack↔ZP bridge thunks automatically (`-Wthunk` warns, `--no-thunks` makes a mismatch an error).

See [calling-conventions.md](../architecture/calling-conventions.md).

## Remaining Work

1. Bitfield code generation (item 4).
2. Optional: SP-relative parameter access for stack-convention leaf functions (item 1 refinement).
3. Measure before claiming gains: build a fixed set of programs (for example `examples/c/*` and `src/test-resources/bench_optimizer.c`) at the same flags before and after a change and compare `.prg` sizes; see [PERFORMANCE_VALIDATION.md](../architecture/PERFORMANCE_VALIDATION.md).

## Future Enhancements (status today)

| Item | Status |
|------|--------|
| Inlining | Available: `-finline-functions`, `-fmax-inline-ir-size=N`, `always_inline`/`noinline` |
| Loop unrolling | `repeat(N)` and vector operations unroll (`-fmax-unroll-bytes=N`) |
| Loop-invariant code motion | Not observed: a loop-invariant `k * 3` in a simple `for` loop is recomputed each iteration at `-O2` |
| Tail-call optimization | Not observed: `return g(x + 1);` compiles to `jsr _g` … `rts` at `-O2` |
| Dead code elimination | Unused `static` functions are removed; exported functions are kept for the linker |
| Variable packing | Not applicable (see item 3) |
