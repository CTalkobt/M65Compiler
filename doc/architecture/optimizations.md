# cc45 Optimization Guide

This page describes what the toolchain actually optimizes today, which options
control it, and which documented-looking options have no effect. Everything
here was checked against the sources (`src/main/cc45_main.cpp`,
`CompilationPipeline`, `OptimizeStage`, `CodegenStage`, `IRBuilder`,
`IRCodeGen`, `AssemblerOptimizer`, `ca45_main.cpp`) and by compiling test files
with `./bin/cc45` (see [Checking it yourself](#checking-it-yourself)).

## Summary

- A C file goes through four places where code is improved:
  1. **cc45 front end / IR builder**: AST constant folding, function inlining,
     removal of unused `static` functions, devirtualization, SAC.
  2. **cc45 IR passes** (`src/main/IROptimizer.cpp`, run by `CodegenStage::optimizeIR`):
     strength reduction, algebraic simplification, type narrowing, branch
     folding, CSE, loop-invariant code motion, copy propagation, address fusion,
     post-increment sinking.
     All on from `-O1`.
  3. **cc45 code generator** (`IRCodeGen`): instruction selection with built-in
     fusions/suppressions. Always on.
  4. **ca45 peephole optimizer** (`AssemblerOptimizer`): runs on the generated
     assembly when cc45 builds a `.o45` or `.prg`, at ca45's default level.
- Every optimization of the catalog (`src/main/OptimizationCatalog.cpp`) has a
  `-f<name>` / `-fno-<name>` switch, and the IR passes can be switched per
  function with `#pragma cc45 optimize(...)`. Names that are not implemented
  are accepted and ignored; `--future` warns about them
  (see [Optimization switches](#optimization-switches)).

## Where optimization happens

| Stage | What it does | Controlled by |
|---|---|---|
| AST constant folding (`ConstantFolder`) | Folds constant sub-expressions (`2 * 4` → `8`). Variables are never replaced by their initial values | Always on, at every `-O` level (`constant-folding`) |
| Inlining (`IRBuilder`) | Expands calls to inline candidates at the call site | `-O2`+, `-finline-functions`, `-f[no-]inline-small-functions`, `inline`, attributes, size limit (below) |
| printf format strings (`IRBuilder::noteFormatNeeds`) | The formatter's integer digit hooks are linked only as the literal formats of `printf`-family calls need them (`%d` → 16-bit, `%ld` → 32-bit, a non-literal format → 32-bit, which serves both), through `.ref` from the calling function; see stdlib.md | Always on |
| `printf` of plain text (`IRBuilder::simplifyPrintfCall`) | A `printf("text")` statement without `%` becomes `puts("text")` (text ending in `\n`) or `putstr("text")`, so the formatter is not linked | Always on, when `<stdio.h>` declares the replacement and the unit does not define `printf` |
| Unused static function removal (`IRBuilder::generate`) | Removes `static` functions that nothing in the file calls or takes the address of (repeats until stable, so chains go too). Non-static functions are always kept | Always on |
| Devirtualization (`IRBuilder::generate`) | A virtual call through a vtable slot that has only one implementation **among the vtables of this file** becomes a direct `CALL`; reports `note: devirtualized N virtual call(s)`. A subclass defined in another file that overrides the method is not seen, so its override is bypassed: use `-fno-devirtualization` in a file whose classes are extended elsewhere | On at every level; `-fno-devirtualization` |
| IR passes (`IROptimizer.cpp`) | See [IR passes](#ir-passes) | `-O1`+; `-f[no-]<pass>`; `#pragma cc45 optimize(...)` |
| Parameter narrowing advisory | For `static` functions: `note: parameter N of 'f' could be 'unsigned char' (all call sites pass 0-255)` (or `unsigned int`). Advisory only, code is not changed | Always on |
| Vector-extension unrolling | Element-wise vector operations are unrolled when the size estimate fits the limit, otherwise a loop is emitted | `-fmax-unroll-bytes=N` / `#pragma cc45 max_unroll_bytes(N)` |
| Loop unrolling (`IROptimizer.cpp`) | Counted `for` loops fully unrolled ([Loop unrolling](#loop-unrolling)) | `-O4`+, `-f[no-]loop-unrolling`, the same size limit |
| `repeat(N) { ... }` | Body is emitted N times | Always unrolled (language construct) |
| Static Allocation Convention (SAC) | Locals/params of non-recursive functions in static BSS records instead of a stack frame | `-fstaticalloc` (default) / `-fno-staticalloc`; see `calling-conventions.md` |
| ZP calling convention | Parameters in ZP `$10-$2F` | `-fzpcall` (default) / `-fno-zpcall` |
| Code generation (`IRCodeGen`) | Add/sub straight into the destination when the operands and the result are in memory (ZP, static-allocation storage) or constants (`lda a; clc; adc b; sta d; …`, or `clc; ldq a; adcq b; stq d` for 32 bits), static-allocation variables used directly as operands (not copied to scratch first), loads through a pointer without stack shuffling (high byte first; `ldz #0; ldq (p),z` for a 32-bit load at offset 0), 32-bit stores of a value in memory through a pointer byte by byte, `p->f += v` / `p->f++` / `*p -= v` (a load, an add/sub of a constant or a variable in memory, and a store to the same address) done in place (`ldy #o; clc; lda (p),y; adc #c; sta (p),y; iny; …`, `inc a` for a byte), a load of a value right after it was stored is dropped (the registers still hold it; `ora #0`/`inx`+`dex` restores the flags before a branch that needs them; `CC45_NO_STORECACHE=1` turns it off), single-bit `AND` + branch → `BBS`/`BBR`, fused compare-and-branch, CONST suppression (also a constant used only as values stored through pointers or to globals: loaded as an immediate by the store), constant-address direct stores, a pointer `+ c` or copy used only as load/store addresses folded into them (`ldy #c; lda (p),y`), a byte cut from a variable (`*p = c` with `int c`, `g = (char)l`) stored from the variable's low byte, `x = x ± 1` on a 16-bit or pointer variable in ZP as `inw`/`dew`, a 16-bit add/sub whose result goes to ZP or static storage computed into it (`clc; adc m; sta d; txa; adc m+1; sta d+1`, not ca45's `add.16` with `pha`/`pla`), an element address with a runtime index built straight into its destination (`clc; adc #<base; sta d; txa; adc #>base; sta d+1`), large stack frames pushed in a loop and dropped by setting SP, division and remainder as calls of the library's `__divs16` … `__modu32` (`lib/stdlib_asm/divmod.s45`: divisor in `__zp_scratch2`, dividend in A/X(/Y/Z); inline only without a library, `-S`), a signed multiply as the unsigned one (the low bits of a product do not depend on the signs), a signed `x < 0` / `x >= 0` as a test of the sign bit (`lda hi; eor #$80; asl a`, not `cmp.s16`), other signed 16-bit `<` / `>=` by subtraction (`cmp lo; txa; sbc hi; bvs *+4; eor #$80; asl a`: C = not less, 10 bytes instead of 13–19; `<=` / `>` against a constant as `<` / `>=` with the next one), with `-fmul-call` / `-Os` multiplications as calls of `__mul16` / `__mul32`, per-function `.reg_clobbers`/`.flag_clobbers`/`leaf` metadata | Always on, not tied to `-O` |
| Assembler peephole (`ca45`) | Branch inversion, JMP→BRA, redundant loads, tail calls, … (table below) | ca45 `-O`/`-P` options; from cc45, five of them through `-f[no-]<name>` |

From `-O4`, counted `for` loops are unrolled and suitable nests interchanged;
LICM hoists invariant computations out of loops from `-O1`. The "loop optimization" and
"cross-module optimization" steps in `OptimizeStage` and `CodegenStage`
(`performLoopOptimizations`, `performCrossModuleOptimization`,
`applyIPOSpecialization`, `applyInlining`, `eliminateDeadCode`) only count
functions and print messages with `-vv`; they change nothing.

## IR passes

`CodegenStage::optimizeIR` runs these after IR generation, in this order, on
every function for which the pass is on (`-O1` and up, unless switched off).
`-Roptir` reports each change on stderr as `[tag] function (file:line): …`.

| Pass (`-f` name) | Tag | What it does |
|---|---|---|
| `loop-unrolling` (from `-O4`) | `unroll` | See [Loop unrolling](#loop-unrolling). Runs first, so the other passes fold its constants |
| `strength-reduction` | `strength` | `x * 2^n` → shift, unsigned `x / 2^n` → shift, unsigned `x % 2^n` → `AND`, `x * 1` → copy (signed division is left alone: it rounds toward zero) |
| `algebraic-simplify` | `algebraic` | `x + 0`, `x - 0`, `x * 1`, `x | 0`, `x ^ 0`, `x << 0`, `x & -1` → `x`; `x * 0`, `x & 0` → 0; `x - x` → 0; an element address with a constant index on a global or pointer base → base + constant |
| `type-narrowing` | `narrow` | An `ADD`/`MUL`/`AND`/`OR`/`XOR`/`SHL`/`NOT` whose result is only used through a truncation to 8 bits is computed in 8 bits. Forward: a `ZEXT` of a constant becomes the constant; `AND`/`OR`/`XOR` and comparisons of zero-extended bytes (or a byte and a constant that fits) are done in 8 bits; a branch on a zero-extended byte tests the byte; a single-definition constant vreg becomes an immediate operand of `ADD`/`SUB`/`AND`/`OR`/`XOR`/shifts/comparisons. Unsigned comparisons with 0 or 1 become tests for zero (`x > 0`, `0 < x`, `x >= 1` → `x != 0`; `x <= 0`, `x < 1` → `x == 0`; `x >= 0` → 1, `x < 0` → 0), which codegen does with `lda`/`ora` and the Z flag |
| `branch-folding` | `branch` | Removes instructions after a block's terminator, folds branches on constants, threads jumps to blocks that only jump, removes unreachable blocks |
| `cse` | `cse` | Within a block, a repeated pure expression reuses the earlier result; copy propagation of temporaries. Forgets an expression when an operand is reassigned, and on stores through pointers, calls and inline asm |
| `loop-invariant-code-motion` | `licm` | Moves pure computations whose operands don't change in the loop to a pre-header. A variable whose address is taken counts as changed when the loop calls a function, stores through a pointer or contains asm |
| `copy-propagation` | `copy` | Replaces uses of a copy of a temporary by the original and removes the dead copies (temporaries defined once only; never variables) |
| `addr-elem-fusion` | `fusion` | An array element address used once, by the next 1/2-byte (or 4-byte load) access of an integer or pointer, is computed inside that load/store (no pointer temporary) |
| `post-increment-sink` | `postinc` | `*p++` / `*p--` on a pointer variable: the IR copies the old pointer to a temporary, updates `p`, then accesses through the copy. When nothing between the update and the last use of the copy reads or writes `p` (or calls, or is asm), the update moves after that use and the copy becomes `p + 0`, which the code generator folds into the access: `lda ($p),y` with `p` in ZP, then `inw $p` / `dew $p` for a step of 1. Runs after the other passes and after the leaf-function pointer allocation ([Pointers in zero page](#pointers-in-zero-page-leaf-functions)). The same pass (tag `incdec`) updates other variables in place: `n--` (`%t = deref n; %u = sub %t, 1; store %u, n`) subtracts from `n` itself, and after `--n` (`%u = sub n, 1; store %u, n`) the uses of `%u` read `n` (until it is written again); the code generator then emits `inc`/`dec` (a byte), `inw`/`dew` (ZP), `inc`/`bne`/`inc` (static storage) or ca45's `inc.16f`/`dec.16f` (stack frame). Likewise a call's result stored into a variable of its type (`c = getc()`): its later uses read the variable, and the code generator stores it from A/X without a temporary |

After the passes, pure instructions whose temporary result nothing uses are
removed (`removeDeadTemps`; part of dead-code elimination, always on).

### Pointers and variables in zero page (leaf functions)

A pointer dereference needs the pointer in zero page (`lda ($zp),y`); a
pointer kept in static or stack storage is copied to scratch first on every
access. In a **leaf function** (no `CALL`/`CALL_VOID`/`CALL_INDIRECT`, no
inline asm), so nothing can clobber the register pool between uses:

- `IRCodeGen` allocates named pointer, `char` and `int` locals (not arrays,
  not structs or variables whose address is used (`addr_local`), not
  `__cpu` registers) in the ZP register pool like `register` variables: a
  dereference uses the pointer in place and a loop counter is a ZP access
  (`inw $30`, `lda $30`) instead of an absolute one.
- `leafPointerParamsToZp` (`IROptimizer.cpp`, after the passes and
  `removeDeadTemps`; tag `zp`) gives a pointer parameter used twice or more
  a new register local, stored from the parameter at the start of the entry
  block, and points every other use at it. Both entries of a SAC function
  (`_f` and `_f__sac`) reach that store. A function with a branch to its entry
  block is skipped (the copy must run once), as are address-taken and
  `register` parameters and `__naked` functions.

It is not a `-f` switch: `CC45_NO_ZP_POINTERS=1` turns both parts off (for
comparing code). Validation: `test_zp_pointers_xemu.c`.

Other code generation for loops: an element address with a stride of 2, 4
or 8 shifts the index (`asl; pha; txa; rol; tax; pla` per doubling, 6 bytes)
instead of using the hardware multiplier (24 bytes); a store of a constant or
of a value in ZP to an element (`a[i] = 0`) computes the address first and
loads the value after it (no `pha`/`pla`); and `removeDeadTemps` runs again
after `post-increment-sink`, which drops the unused old value of `x++` as a
statement (`for (...; i++)`: only `inw $30` remains). Speed is measured with
`src/test/bench/run_bench.sh` (six benchmarks on xemu, timed by CIA2;
not part of `make test`): 1344 ms at v1.0.6, 1041 ms after these.

A stack frame of 8 to 255 bytes is made by moving SP (`tsx; txa; sec; sbc #n;
tax; txs`, the low byte only, right with either stack) instead of pushing
zeros; when the low byte would wrap, the push loop runs instead (C locals
start undefined). ca45's redundant-load elimination keeps what the registers
hold across a local label nothing refers to (branch inversion leaves the loop
body's label so: `bcc @body; bra @end; @body:` becomes `bcs @end`), so a
loop body does not reload the counter the test just loaded; `cmp.16` is known
to leave A and X alone. A byte constant stored to an element loads only A.
Benchmark: 987 ms.

`foldElemIndexOffset` (`IROptimizer.cpp`, after the passes and
`removeDeadTemps`; tag `elemoff`; `CC45_NO_ELEM_OFFSET=1` turns it off) turns
an element access `a[x + c]` (`a[x - c]`) of a global array into element `x`
of `a + c*stride` (`Inst::elemOffset`; the code generator writes the base as
`#<(_a+2)`), when `x + c` is a 16-bit add defined once in the same block
before the access and `x` is not written in between: the add disappears and
`a[j]` / `a[j + 1]` scale the same index. It runs last because CSE does not
compare offsets. Validation: `test_elem_offset_xemu.c`. Benchmark: 904 ms.

A fused element load leaves `__zp_scratch` pointing at base + offset +
index*stride; the next fused load of the same global array and index
variable (no other instruction between, no label) reads through it with Y
moved by the offset difference (`a[j]` then `a[j + 1]`: `ldy #3` instead of
the address computed again). A load into the index variable itself
(`i = n[i]`) forgets it. Fused element stores use it too, and it survives
the instructions that cannot touch `__zp_scratch` or the index: a store of a
value to a variable in ZP, a load or store through a pointer in ZP (bubble
sort's swap: `a[j + 1] = t` reuses the address of the `a[j + 1]` load before
it). It survives a compare and a conditional branch of bytes and words whose
code does not use `__zp_scratch` (a simulated instruction other than
`cmp.16` counts as using it: `cmp.s16` does), and carries into a block whose
only predecessor is the conditional branch that ends the block before it (no
inline asm in the function, the label not address-taken). An `addr_elem`
the cache holds (`&a[j]` kept in ZP for a load and a store) copies it from
`__zp_scratch` instead of computing it: bubble sort's `if (a[j] > a[j + 1])
{ t = a[j]; a[j] = a[j + 1]; a[j + 1] = t; }` computes one address per
iteration. Benchmark: 759 ms.

A static-allocation function that makes calls cannot use the pool across
them; its most used variables go in a base-page block of its own, `$80+k`,
declared with `.zar $80, N`: pointers first, then `char`/`int`/`long` variables
(parameters too in the ZP convention) by number of uses, loop uses counted 4
times, up to 8 bytes (`-fzp-locals-max=N`; in `c45.lib` 8 bytes per function
cover 70% of the static-storage accesses, 16 cover 87%, but 16 makes deep
stdio chains overflow the area). ca45
relocates those operands against `<f>__zar` and ln45 places the blocks in
`$80`–`$E9` so that functions active together never share bytes
(`computeZarOverlay`, the SAC overlay's call graph plus edges from functions
that call through pointers to address-taken functions; callers placed before
callees). The register pool ends at
`$7F`. Blocks that do not fit share bytes, and ln45 calls the functions on
the inside of such pairs through save/restore wrappers (a warning).
`-fno-zp-locals` turns it off; validation: `test_zp_locals_xemu.c` (two
modules) and the overflow program in `test_xemu.sh` (both conventions, a
wrapped function also called through a pointer).

A ZP-convention leaf function goes further: its parameters stay in the ZP
parameter block where the caller put them (`$10`…), with no copy at the entry
and no `leafPointerParamsToZp` copy ([calling conventions](calling-conventions.md);
`CC45_NO_ZP_PARAMS=1` turns it off, `test_zp_params_xemu.c`).

The IR is not in SSA form (a variable's slot is assigned by `STORE`; a
temporary can be defined on two paths, as in `a && b`), so every pass treats
a value as constant or fixed only when it has exactly one definition.

## Loop unrolling

`-floop-unrolling` (on from `-O4`) fully unrolls a `for` loop whose variable
starts at a constant, is compared with a constant and changes by a constant
(`i++`, `i--`, `i += 3`, …), when the trip count is at most 255 and the
unrolled code is estimated (about 4 bytes per IR instruction) to fit
`-fmax-unroll-bytes` (default 128; `#pragma cc45 max_unroll_bytes(N)`, the
last one in the file). Inner loops are unrolled first.

Each copy of the body reads the loop variable's value for that iteration as
a constant, so indexes and conditions fold (`a[i]` becomes a fixed element
address); `continue` goes to the next copy and `break` to the end, and the
variable has its final value after the loop (and, when the body can break
out, in each copy). For a 4-iteration `a[i] = a[i] + k` loop run 60000 times
the run time on xemu dropped from 0.38 s to 0.20 s (startup subtracted;
2026-10-11), and the program grew from 345 to 434 bytes.

Not unrolled: loops whose body assigns the variable or takes its address,
contains inline asm, computed-goto labels or labels jumped to from outside;
variables that are volatile or address-taken; values that would cross the
sign bit or wrap; bounds that are not constants. `#pragma cc45
optimize(no-loop-unrolling)` turns it off for a function. `-Roptir` reports
`[unroll] _f: loop at for_condN unrolled K times`; `test_unroll_xemu.c`
checks the results.

## Loop interchange

`-floop-interchange` (on from `-O4`; `src/main/LoopInterchange.cpp`, run by
`OptimizeStage` on the AST before IR generation) swaps a perfectly nested pair
of `for` loops when the array subscripts put the inner loop's variable in an
earlier dimension than the outer one's, as in

```c
for (int i = 0; i < C; i++)
    for (int j = 0; j < R; j++)
        b[j][i] = a[j][i] + 1;      /* becomes: j outer, i inner */
```

The 45GS02 has no cache; the gain is that after the swap the row address
`a[j]` does not change in the inner loop, so LICM computes it once per row.
For a 16×16 `int` copy like the one above the loop ran about 35% faster on
xemu (3000 repetitions: about 1.5 s against 2.3 s, startup subtracted;
measured 2026-10-11). Only the two loop headers are exchanged; the body is
not copied, so the code size does not change.

It is done only when it cannot change behaviour (checked by name, conservatively):

- a perfect nest (the outer body is the inner `for` alone) with rectangular,
  side-effect-free headers: each loop's initializer, condition and `++`/`--`/`+=`/`-=`
  increment use its own variable and names the nest does not assign;
- a body without `break`, `continue`, `goto`, `return`, labels, `switch`,
  `asm`, `repeat`, inner loops, calls, `&`, volatile objects, or declarations
  that shadow a variable of the nest;
- stores only to elements of declared (non-pointer, non-parameter) arrays whose
  every access in the body uses the same subscripts, which mention both loop
  variables; scalars only as integer `+=`, `-=`, `^=`, `|=`, `&=` reductions
  not otherwise used, or as temporaries declared in the body; and, when
  anything is stored, no reads through pointers;
- loop variables declared by their `for` statements, or not used elsewhere
  in the function (their values after the nest would differ).

`#pragma cc45 optimize(no-loop-interchange)` turns it off for the function
it is in (or, at file scope, for the functions after it), and
`optimize(loop-interchange)` turns it on below `-O4`. `-Roptir` reports each
swap as `[interchange] _f (line N): loops over i and j interchanged`.
`test_interchange_xemu.c` checks the results of nests it swaps and of nests
it must leave alone.

## Optimization levels (cc45)

Each catalog optimization has a level from which it is on
(`OptimizationCatalog::isEnabledAtLevel`); `-f`/`-fno-` override it.

| Option | Level | Effect |
|---|---|---|
| `-O0` | 0 | No IR passes, no automatic inlining |
| `-O1` | 1 | The nine IR passes |
| `-O2`, `-O3` (default `-O2`) | 2, 3 | IR passes and automatic inlining of small functions. Identical output |
| `-O4` … `-O9` | 4–9 | Adds loop interchange and loop unrolling. Identical output for all of them (the catalog entries for higher levels are not implemented) |
| `-Os`, `-Osize` | 2 | `-O2` with `-fmul-call` (multiplications as library calls) |
| `-Ospeed` | 3 | Same as `-O2` |
| `-Os`, `-O` (no digit), any other `-O<text>` | 0 | **Same as `-O0`** (not `-O2`) |

Only the first digit counts (`-O10` is `-O1`). The level is **not** passed on to
ca45, and does not turn constant folding, dead static removal,
devirtualization or SAC on or off.

## Inlining

A function is an inline candidate when it is not variadic, `__interrupt` or
`__naked`, does not return a struct/union/float through the hidden `__sret`
pointer, takes no aggregate by value, and one of these is true:

- automatic inlining is on (`-O2`+ or `-finline-functions`),
- it is declared `inline` (or `__inline`, `__inline__`),
- it has `__attribute__((always_inline))` / `__forceinline`,
- it is a struct method with at most 3 statements.

Unless it is `always_inline`, a function is never inlined when its body
contains inline asm (which may name the parameters, `@_p_x`), labels or
`goto`, nested function definitions, `static` locals or variable-length
arrays, when a parameter is a variable-length array (`int a[2][n]`), or when
it takes the address of a parameter (`&x`): each of these needs the
function's own frame, symbols or a single copy. The inlined `return` value
has the inlined function's return type, the inlined body sees its own
parameters and locals and the globals (never the caller's locals), and a
parameter the body assigns becomes a local variable of the caller.

It also needs at most 20 body statements (`INLINE_MAX_STMTS`) and, after IR
generation, at most `max_inline_ir_size` IR instructions (default **11**).
`always_inline` skips both size limits; `__attribute__((noinline))` always
wins (and with both, cc45 warns and ignores `always_inline`). A recursive
`always_inline` function warns and is called normally. Once all calls to a
`static` function are inlined, the function body is removed.

Inlining can make the program larger or smaller. One-step PRG sizes (bytes;
toolchain of 2026-10-11, `HOME` without a cc45.conf):

| File | `-O0` | `-O1` (IR passes) | `-O2` (IR passes + inlining) |
|---|---|---|---|
| `src/test-resources/test_phase91_validation.c` | 526 | 508 | 448 |
| `src/test-resources/test_phase91_tuning.c` | 1249 | 1231 | 1464 |

Use `-O1`, `-fno-inline-small-functions` or `-fmax-inline-ir-size=0` when
inlining costs space (`-O2 -fmax-inline-ir-size=0` produced output identical
to `-O1` for `test_phase91_validation.c`). The limit also applies to `inline`
functions; only `always_inline` bypasses it.

## Options that control optimization

| Option / pragma | Default | Effect |
|---|---|---|
| `-O<n>` | `-O2` | See above |
| `-f<name>` / `-fno-<name>` | per level | Switch one optimization (see [Optimization switches](#optimization-switches)) |
| `#pragma cc45 optimize(<name>, no-<name>, …)` | — | Switch IR passes for the functions defined after it |
| `--future` | off | Warn when an optimization that is not implemented is requested |
| `-Roptir` | off | Report the IR passes' changes on stderr |
| `-finline-functions` | off | Automatic inlining even at `-O0`/`-O1` (same as `-finline-small-functions`) |
| `-fmax-inline-ir-size=N`, `#pragma cc45 max_inline_ir_size(N)` | 11 | Largest inline candidate in IR instructions |
| `-fmax-unroll-bytes=N`, `#pragma cc45 max_unroll_bytes(N)` | 128 | Largest estimated unrolled code for vector-extension operations |
| `__attribute__((always_inline))`, `__forceinline` | — | Inline regardless of size limits |
| `__attribute__((noinline))` | — | Never inline |
| `-fstaticalloc` / `-fno-staticalloc` | on | SAC on/off |
| `#pragma cc45 recurse` | — | Next function uses a stack frame instead of SAC |
| `-fzpcall` / `-fno-zpcall` | ZP | Calling convention |
| `CC45_DUMP_IR=1` (environment) | — | Prints the final IR to stderr (debugging aid) |

The pragma forms accept `max_inline_ir_size(N)` and `max_inline_ir_size N`
(same for `max_unroll_bytes`) and apply from that point in the file.

## Assembler optimizer (ca45)

`cc45 -c` and the one-step build write the assembly to a temporary file and run
`ca45 temp.s45 -c -o out.o45` without any optimization option, so ca45 uses
`-O2`. `cc45 -S` output is **before** this pass.

ca45 levels: `-O0` disables all passes; `-O1`, `-O2` and `-O3` enable the same
set (`OptimizationFlags::fromLevel` only distinguishes level 0). Individual
passes: `-P<Name>` / `-PNo<Name>` (case-sensitive, applied in command-line
order after `-O`). `-Roptimizer` prints every change
(`opt: file:line: [tag] ...`).

| `-P` name | Tag in `-Roptimizer` | What it does |
|---|---|---|
| `TailCall` | `tail-call` | `JSR x` + `RTS` → `JMP x`. When the `RTS` is a branch target (cc45's `@__return:`) it is kept and only the `JSR` becomes a `JMP` |
| `NoOpBra` | `no-op-bra` | Removes `BRA` to the next instruction |
| `BranchInvert` | `branch-invert` | `Bcc skip / BRA target / skip:` → inverted branch |
| `JmpBra` | `jmp-bra` | `JMP` → `BRA` when in range |
| `JSRRelocate` | `jsr-bsr` | `JSR` → `BSR` when the target is a resolved, non-relocatable label within ±32 KB (rare in `-c` objects) |
| `DeadStore` | `dead-store` | Removes dead `STW.SP` stores |
| `CmpElimination` | `cmp-elim` | Removes `CMP #0` when flags already reflect the value — not when the carry it sets is read (`BCC`/`BCS` in the branches that follow, or `ADC`/`SBC`/`ROL`/`ROR`/`PHP` after them) |
| `FCmpOpt` | `fcmp-opt` | Float compare result tests (`CMP #$FF` → `BMI`/`BPL`) |
| `RedundantLoad` | `redundant-load` | Removes `LDA/LDX/LDY/LDZ` when the register already holds the value |
| `StoreLoadPair` | `store-load-pair` | Store/load pair simplification on ZP |
| `PreserveXSP`, `TSXRedundant` | `preserve-x-sp`, `tsx-redundant` | Avoid reloading X from SP |
| `TailDedup` | `tail-dedup`, `tail-dedup-suffix` | Merges identical tail sequences |
| `SeqExtract` | `seq-extract` | Extracts a 4-instruction run repeated in one proc into a routine before its `endproc` (`BSR` calls); **only with `--experimental`** |
| `StrengthReduction`, `AlgebraicSimplify`, `TypeNarrowing`, `BranchFold`, `CSE`, `LICM`, `CopyChains`, `AddrElemFusion` | — | Accepted, **no effect** (IR-level flags; ca45 has no IR) |

At `-O2` and above ca45 also runs the field-striped array passes
(`fieldStripedOpt`, `fieldDeadCode`, `variableSizeOpt`); they have no `-P` names.

There is no working way to change the ca45 level from cc45: `CA45_OPTIMIZE`
(and any option in `~/.config/m65/ca45.conf` or `.ca45rc`) breaks ca45's `-o`
handling, so `CA45_OPTIMIZE=0 cc45 -c x.c -o x.o45` fails with
`Failed to open input file: x.o45`. To assemble with other ca45 options, run
the steps yourself:

```bash
cc45 -S -O2 prog.c -o prog.s
ca45 -O0 -c prog.s -o prog.o45          # or: ca45 -PNoRedundantLoad -c ...
ln45 -basic -o prog.prg prog.o45 lib/build/c45.lib
```

## Optimization switches

Every optimization of the catalog has a switch `-f<name>` / `-fno-<name>`; the
name may also be written with underscores. Later switches win; a switch wins
over the `-O` level. What a switch does depends on how the optimization is
implemented:

| Name | Implemented as | On from |
|---|---|---|
| `strength-reduction`, `algebraic-simplify`, `type-narrowing`, `branch-folding`, `cse`, `loop-invariant-code-motion`, `copy-propagation`, `addr-elem-fusion`, `post-increment-sink` | IR pass ([IR passes](#ir-passes)); also per function by pragma | `-O1` |
| `inline-small-functions` | IRBuilder inlining (`-fno-` also overrides `-finline-functions`) | `-O2` |
| `devirtualization` | IRBuilder | `-O0` |
| `tail-call-optimization`, `branch-inversion`, `jump-optimization`, `redundant-load-elimination`, `dead-store-elimination` | ca45 pass: cc45 passes `-P<x>` / `-PNo<x>` (`TailCall`, `BranchInvert`, `JmpBra`, `RedundantLoad`, `DeadStore`) to ca45; file-wide only | ca45's default (always) |
| `constant-folding`, `dead-code-elimination`, `frame-pointer-optimization` | Part of code generation, always on: `-fno-` warns *"cannot be disabled"* | always |
| `loop-interchange` | AST pass before IR generation ([Loop interchange](#loop-interchange)); also per function by pragma | `-O4` |
| `loop-unrolling` | IR pass ([Loop unrolling](#loop-unrolling)); also per function by pragma | `-O4` |
| `cross-function-inlining`, `co-optimization`, `procedure-inlining`, `interprocedural-optimization` | **Not implemented**: accepted and ignored; with `--future` cc45 warns *"optimization 'procedure-inlining' is not implemented yet; -fprocedure-inlining has no effect"* | — |

`#pragma cc45 optimize(no-licm)` (or `optimize(cse, no-copy_propagation)`,
`optimize cse`) overrides the command line for the IR passes it names. Inside
a function body it applies to that function; at file scope it applies to the
functions defined after it, until another `optimize` pragma changes the same
pass. Only IR passes can be switched per function. Naming any other
implemented optimization is accepted when it asks for it to be on (it is on
anyway); `no-` of a ca45 or IRBuilder one warns that it applies to the whole
file, `no-` of an always-on one warns that it cannot be disabled, an unknown
name warns, and an unimplemented one warns with `--future`.

Any other unknown `-f…` (or other `-…`) option gives *"cc45: warning: unknown
option '-f…' ignored"*.

## Options that do nothing

| Option | What happens |
|---|---|
| `#pragma cc45 unroll [N]`, `#pragma cc45 unroll_default N` | Turned into a directive that only the unused `CodeGenerator` reads; no effect |
| `--emit-ir`, `-Rcodegen` | Parsed, never used. Use `CC45_DUMP_IR=1` for IR |
| `--pragma <p>` | Parsed into `cliPragmas`, never used: the pragma is not injected |

## Configuration file

cc45 reads `~/.config/m65/cc45.conf` (`$HOME/.config/m65/<program>.conf`, so
`cp45` reads `cp45.conf`) and puts its tokens **before** the command-line
arguments, so the command line wins (`-O0` in the file, `-O2` on the command
line → `-O2`).

- A line whose **first character** is `#` is a comment; blank lines are skipped.
- A `#` later in a line is not a comment: `-O0  # note` yields the tokens
  `-O0`, `#`, `note`. The extra tokens become a (later overwritten) input file
  name, so they are silently ignored as long as the command line names a source file.
- Tokens are split on spaces/tabs; `'...'` and `"..."` group.
- Unknown options starting with `-` warn and are ignored.

Example:

```
# ~/.config/m65/cc45.conf
-O1
-fmax-unroll-bytes=64
```

## Checking it yourself

```bash
cc45 -S -Roptir t.c -o t.s                                           # what the IR passes did
cc45 -S -O1 t.c -o t1.s ; cc45 -S -O2 t.c -o t2.s ; diff t1.s t2.s   # only inlining differs
cc45 -S -O2 t.c -o a.s ; cc45 -S -O9 t.c -o b.s ; cmp a.s b.s        # identical
cc45 -c -O2 t.c -o t.o45 ; nm45 -s t.o45                              # segment sizes
nm45 -f t.o45                                                         # zp_call / leaf / clobbers per function
objdump45 -d t.o45                                                    # final code after ca45
cc45 -S t.c -o t.s ; ca45 -c -Roptimizer t.s -o /dev/null             # ca45 peephole report
CC45_DUMP_IR=1 cc45 -S t.c -o /dev/null                               # IR after the IR passes
```

A personal `~/.config/m65/cc45.conf` changes the default level; run with
`HOME=/some/empty/dir` to compare against the built-in defaults. For files with
at most three functions cc45 currently prints a `[DCE] Functions: ...` debug
line to stderr; it does not affect the output.
