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
     parameter narrowing, strength reduction, algebraic simplification, type
     narrowing, branch folding, CSE, loop-invariant code motion, copy
     propagation, address fusion, post-increment sinking.
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
| String builtins (`IRBuilder::foldStringCall`, #78) | `strlen`, `strcmp`, `strncmp` of string literals are constants (`strlen("hello")` is 5; `strcmp` the library's -1 / 0 / 1 of the bytes as stored, the encoding applied); `strcmp(x, "lit") == 0` / `!= 0` / `!strcmp(...)` / a condition, and `strncmp(x, "lit", n)` so used, are byte compares when `x` is a simple expression and at most 1 byte is compared (`strcmp(s, "") == 0` is `s[0] == 0`: each byte compared is 8 to 12 bytes of code, more for an array in a stack frame, the call about 20), at `-O3` and above at most 8 (for speed); at `-O3` and above `strcpy(d, "lit")` is `memcpy(d, "lit", length + 1)` (no test for the end, 8 bytes more a call). `stridx(s, "a", "b", ...)` (#79) with literal candidates is `__stridx_table(s, table)` (one data block, no variadic call), with a literal `s` the index; at `-O3` a trie of up to 6 nodes is inline compares (more: about 45 bytes a node, larger than the table). A literal reached through `const char s[] = "..."`, `const char *const p = "..."` or a local `const char *p = "..."` the function never changes counts. Not in a file that defines a function of that name | On at every level; `-fno-builtin` |
| IR passes (`IROptimizer.cpp`) | See [IR passes](#ir-passes) | `-O1`+; `-f[no-]<pass>`; `#pragma cc45 optimize(...)` |
| Parameter narrowing advisory | For `static` functions: `note: parameter N of 'f' could be 'unsigned char' (all call sites pass 0-255)` (or `unsigned int`). The note changes nothing; the `param-narrowing` IR pass narrows the parameters for which it pays ([IR passes](#ir-passes)) | Always on |
| Vector-extension unrolling | Element-wise vector operations are unrolled when the size estimate fits the limit, otherwise a loop is emitted | `-fmax-unroll-bytes=N` / `#pragma cc45 max_unroll_bytes(N)` |
| Loop unrolling (`IROptimizer.cpp`) | Counted `for` loops fully unrolled ([Loop unrolling](#loop-unrolling)) | `-O4`+, `-f[no-]loop-unrolling`, the same size limit |
| `repeat(N) { ... }` | Body is emitted N times | Always unrolled (language construct) |
| Static Allocation Convention (SAC) | Locals/params of non-recursive functions in static BSS records instead of a stack frame | `-fstaticalloc` (default) / `-fno-staticalloc`; see `calling-conventions.md` |
| ZP calling convention | Parameters in ZP `$10-$2F` | `-fzpcall` (default) / `-fno-zpcall` |
| Code generation (`IRCodeGen`) | Add/sub straight into the destination when the operands and the result are in memory (ZP, static-allocation storage) or constants (`lda a; clc; adc b; sta d; …`, or `clc; ldq a; adcq b; stq d` for 32 bits), static-allocation variables used directly as operands (not copied to scratch first), loads through a pointer without stack shuffling (high byte first; `ldz #0; ldq (p),z` for a 32-bit load at offset 0), 32-bit stores of a value in memory through a pointer byte by byte, `p->f += v` / `p->f++` / `*p -= v` (a load, an add/sub of a constant or a variable in memory, and a store to the same address) done in place (`ldy #o; clc; lda (p),y; adc #c; sta (p),y; iny; …`, `inc a` for a byte), a load of a value right after it was stored is dropped (the registers still hold it; `ora #0`/`inx`+`dex` restores the flags before a branch that needs them; `CC45_NO_STORECACHE=1` turns it off), single-bit `AND` + branch → `BBS`/`BBR`, fused compare-and-branch, CONST suppression (also a constant used only as values stored through pointers or to globals: loaded as an immediate by the store), constant-address direct stores and loads (`*(volatile char *)0xD012` is `lda $D012`; four hex digits keep an address below `$100` absolute, not the program's base page), a value put together from bytes of others (`a | b << 8 | (long)c << 16`, with `ZEXT`/`SHL`/`SHR` by multiples of 8, byte masks and truncations in between) built byte by byte from where each byte lives (`ldx b; lda a`; the sources are kept live up to the `OR`; not when two sources' bytes overlap), a pointer `+ c` or copy used only as load/store addresses folded into them (`ldy #c; lda (p),y`), a byte cut from a variable (`*p = c` with `int c`, `g = (char)l`) stored from the variable's low byte, a byte variable or parameter (or a temporary in ZP with only `NOP`s up to the use) zero-extended for one 16-bit `ADD`/`SUB`/`AND`/`XOR` or comparison read where it lives, with 0 for its high byte (`lda x; clc; adc c; bcc *+3; inx`, `cpx #0; bne +; cmp c`, no 16-bit temporary), a value loaded from a global of the program (a byte, 16 or 32 bits; not on the base page, not a parameter's storage) whose one use follows with only register code between (an add/sub, a compare, a store of it, or for a byte a widening) read by that use where it is (`lda g` there, no temporary), `x = x ± 1` on a 16-bit or pointer variable in ZP as `inw`/`dew`, a 16-bit add/sub whose result goes to ZP or static storage computed into it (`clc; adc m; sta d; txa; adc m+1; sta d+1`, not ca45's `add.16` with `pha`/`pla`), an element of a global array read or written as `name+off,y` with no address built (`ldy i; lda name,y`; also `x[i]--`, `x[i] += c` and several accesses at one address): a byte element at a byte index of any array (the index in the base page; a byte widened only for the index is read as the byte), or elements of 1, 2 or 4 bytes of an array of at most 256 bytes (Y = index * size), a byte through a pointer in the base page at a byte index likewise as `(p),y` (`ldy i; lda (p),y`: the pointer and the index unchanged up to the access; fused into the load or store, or not), an element address with a runtime index otherwise built straight into its destination (`clc; adc #<base; sta d; txa; adc #>base; sta d+1`), large stack frames pushed in a loop and dropped by setting SP, division and remainder as calls of the library's `__divs16` … `__modu32` (`lib/stdlib_asm/divmod.s45`: divisor in `__zp_scratch2`, dividend in A/X(/Y/Z); inline only without a library, `-S`), a signed multiply as the unsigned one (the low bits of a product do not depend on the signs), a signed `x < 0` / `x >= 0` as a test of the sign bit (`lda hi; eor #$80; asl a`, not `cmp.s16`), other signed 16-bit `<` / `>=` by subtraction (`cmp lo; txa; sbc hi; bvs *+4; eor #$80; asl a`: C = not less, 10 bytes instead of 13–19; `<=` / `>` against a constant as `<` / `>=` with the next one), multiplications outside loops as calls of `__mul16` / `__mul32` (an element address's index times its size too; inside loops inline; `-fmul-call` / `-Os` every one, `-fno-mul-call` none; a constant factor an immediate, not a register; a byte constant factor `ldy #c; jsr __mul16y`, 5 bytes), per-function `.reg_clobbers`/`.flag_clobbers`/`leaf` metadata, and stores to vreg pool registers (`$30`–`$7F`) that nothing in the function reads are left out (a temporary stored although every use took it from A/X; not in functions with inline asm, that index a pool register or take its address (`lda $30,x`, `#$30`: a float copied byte by byte), or in files with nested functions; `CC45_NO_POOL_PRUNE=1` keeps them) | Always on, not tied to `-O` |
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
| `ipa-pure-const` | `purity` | Runs first: a function of the module that reads only its parameters and own variables, writes only its own variables, has no loops or recursion and calls only `const` / `__pure` functions (declared, or inferred: a fixed point) is `const` (pure when it calls pure ones), as if declared so; the passes below then merge, drop and hoist its calls. It never reads memory itself (the IR does not mark `volatile` loads) |
| `loop-unrolling` (from `-O4`) | `unroll` | See [Loop unrolling](#loop-unrolling). Runs first, so the other passes fold its constants |
| `param-narrowing` | `param` | An `int` parameter of a `static` function that is called only directly in its file, never assigned and never has its address taken, and that every call passes a value of 0-255 (a constant, an `unsigned char`, `x & 0xFF`, `x % 256`), becomes a byte: each call passes the byte, each use in the function reads the parameter zero-extended. Only when every use costs nothing that way: a byte use (a truncation to 8 bits), or a 16-bit `ADD`/`SUB`/`AND`/`XOR` or comparison (not signed `<=` / `>`), which the code generator computes from the byte where it lives; a parameter that is multiplied, divided, shifted, stored, returned or passed on keeps its type. Repeats while narrowing a function makes more arguments fit (up to 8 rounds) |
| `strength-reduction` | `strength` | `x * 2^n` → shift, unsigned `x / 2^n` → shift, unsigned `x % 2^n` → `AND`, `x * 1` → copy; signed `x / 2^n` → `(x + b) >>a n` and `x % 2^n` → `x - ((x + b) & -2^n)` with the bias `b = (x >>a (w-1)) & (2^n - 1)` (C rounds toward zero); it runs again after the other passes (a promoted `char` divisor is a constant only then) |
| `algebraic-simplify` | `algebraic` | `x + 0`, `x - 0`, `x * 1`, `x | 0`, `x ^ 0`, `x << 0`, `x & -1` → `x`; `x * 0`, `x & 0` → 0; `x - x` → 0; an element address with a constant index on a global or pointer base → base + constant; an operation on two constants (also a variable assigned a constant once: `c + 1` with `c = (char *)0x110C`) → the constant, for a vreg with one definition |
| `type-narrowing` | `narrow` | An `ADD`/`SUB`/`NEG`/`MUL`/`AND`/`OR`/`XOR`/`SHL`/`NOT` whose result is only used through a truncation to 8 bits, or by 8-bit operations whose low byte depends only on their operands' (`ADD`/`SUB`/`MUL`/`AND`/`OR`/`XOR`/`NOT`/`NEG`/`SHL`, a copy), is computed in 8 bits, and so is `x >> n` of a zero-extended byte (#342: `(char)(v << 4 \| v >> 4)`); repeated until nothing more narrows; a vreg of several definitions (a `?:` result) is narrowed as a whole when each definition can be (a result used wider, such as a char difference compared as an int, keeps 16 bits). It runs again after copy propagation; its operands zero- or sign-extended from a byte in the same block read the byte itself, when nothing in between can change it, so the extensions go. Forward: a `ZEXT`, `SEXT` or `TRUNC` of a constant becomes a constant of the new width (so a constant argument, `f('d', 'l', n)`, is an immediate stored straight into the parameter block); `AND`/`OR`/`XOR` and comparisons of zero-extended bytes (or a byte and a constant that fits) are done in 8 bits; a branch on a zero-extended byte tests the byte; a single-definition constant vreg becomes an immediate operand of `ADD`/`SUB`/`AND`/`OR`/`XOR`/shifts/comparisons. Unsigned comparisons with 0 or 1 become tests for zero (`x > 0`, `0 < x`, `x >= 1` → `x != 0`; `x <= 0`, `x < 1` → `x == 0`; `x >= 0` → 1, `x < 0` → 0), which codegen does with `lda`/`ora` and the Z flag. Ends by reducing the immediate operands of 8-bit `ADD`/`SUB`/`MUL`/`AND`/`OR`/`XOR` to their low byte (`p & ~0x80` is `and #127`, not `and #-129`) |
| `branch-folding` | `branch` | Removes instructions after a block's terminator, folds branches on constants, threads jumps to blocks that only jump, removes unreachable blocks. A test whose outcome is known where it runs becomes a plain branch: an edge of `br_cond %v` to a block that only tests `%v` again goes straight to that test's target, and a forward dataflow carries `%v != 0` / `%v == 0` from the edges of a test of `%v` until `%v` is assigned (variables whose address is taken, and anything after inline asm, are never known): `if (!block && !len) … else if (!block \|\| …)` tests `block` once on each path |
| `cse` | `cse` | Within a block, a repeated pure expression reuses the earlier result (a constant operand compares by its value); a costly one (a multiply, a division, an element address, a call of a `__pure` / `const` function) computed in a block that dominates another is reused there too, when nothing on the paths between (nor the block, in a loop) changes its operands or, for a load or a pure call, writes memory. Cheap ones are not carried between blocks: measured on vim2 and the library, their longer life costs more than it saves; copy propagation of temporaries. Forgets an expression when an operand is reassigned; on stores through pointers, calls and inline asm, those that read a variable whose address is taken (`&x`, an array or struct used through its address, `volatile`) -- every variable in a function with inline asm, `setjmp` or a computed goto, and in a file with nested functions (the static chain reaches the parent's variables); a variable whose address is never taken cannot change behind the code's back (library 3,890 bytes smaller in both conventions, the single-file examples 1,396; test `test_cse_slots_xemu.c`); a call of a `__pure` / `const` function with the arguments of an earlier one reuses its result (a pure one's only while nothing writes memory in between), and does not make the pass forget what it knows. Loads of globals (#320): a load from a global not declared `volatile` (its address `@g`, `addr_global @g`, an element of `@g`, or a pointer added to one of those, through temporaries defined once) is reused by a later load of the same address until something may write that global: a store into the same global, a store through a pointer of unknown target, a call (not of a pure function), inline asm, or any instruction that is not known to leave memory alone; a store into another global or into a local leaves it; a load whose address the block stores to later (a read-modify-write: `x--`, `x += k`) loads afresh, so the update stays in place (`dec`). Across blocks too, from a dominating block, when nothing on the paths between writes that global or unknown memory (`?:` diamonds). Distinct objects do not overlap in C; a load through a pointer that is not known to point into a global, such as `*(volatile char *)0xD012`, is never reused. Struct members are not known to be `volatile` (the parser does not record it), so a global struct with `volatile` members must be declared `volatile` itself. pfest: 958 bytes smaller, vim2's code 446; test `test_load_reuse_xemu.c`. **`restrict` parameters** (#77): a `restrict` pointer parameter the function never assigns and whose address it never takes points to an object of its own: a load through it (or a pointer made from it: `p + k`, `&p[i]`) is reused until a store through it, through an unknown pointer, a call or asm, not across stores into globals or through other `restrict` pointers (and a load of a global survives a store through a `restrict` pointer); the parameter itself counts as unchanged by stores through pointers. Test `test_restrict_xemu.c` |
| `loop-invariant-code-motion` | `licm` | Moves pure computations whose operands don't change in the loop to a pre-header. A variable whose address is taken counts as changed when the loop calls a function, stores through a pointer or contains asm; constants, global addresses (`&g + c`) and the zero extension of a variable or parameter stay where they are used (they cost nothing there; hoisted, a 16-bit copy would be live across the loop); a call of a `const` function (or of a `__pure` one in a loop that writes no memory) with invariant arguments, from the loop's header only (it runs whenever the loop is entered); a load through a `restrict` parameter (#77) at an invariant address when the loop writes that parameter's object neither through it (or a pointer made from it) nor through an unknown pointer, and has no call or asm (stores into globals and through other `restrict` pointers do not count): `out[i] = in[i] * *k` loads `*k` once. Loads of globals are not hoisted |
| `loop-strength-reduction` | `lsr` | After LICM: an element address `g[i]` whose element size is not a power of 2 (a row of a 2D array: `grid[r]`, 80 bytes) or a product `i * K` (K not a power of 2) in a loop where `i` changes only by `i = i ± c` becomes a new variable: set before the loop (the multiply there once, a constant when `i` was just set to one), stepped by `c * size` right after `i`'s update, read where the multiply was. `i` is a 16-bit variable whose address is never taken, or an `unsigned char` stepped by 1 whose loop is entered at its header only while `i < N` (it never wraps there); the base a global or a temporary the loop does not change (not a local variable: as an element address's base that can be the local array itself); not in functions with inline asm or nested functions. Test `test_lsr_xemu.c` |
| `copy-propagation` | `copy` | Replaces uses of a copy of a temporary by the original and removes the dead copies (temporaries defined once only; never variables). Backward: `%t = op …; %d = copy %t`, the copy right after and `%t` used only there, computes into `%d` (the arms of `x ? a - b : c` into the result, not into temporaries copied to it; not for extensions, which the code generator reads as their source byte) |
| `addr-elem-fusion` | `fusion` | An array element address used once, by the next 1/2-byte (or 4-byte load) access of an integer or pointer, is computed inside that load/store (no pointer temporary) |
| `post-increment-sink` | `postinc` | `*p++` / `*p--` on a pointer variable: the IR copies the old pointer to a temporary, updates `p`, then accesses through the copy. When nothing between the update and the last use of the copy reads or writes `p` (or calls, or is asm), the update moves after that use and the copy becomes `p + 0`, which the code generator folds into the access: `lda ($p),y` with `p` in ZP, then `inw $p` / `dew $p` for a step of 1. Runs after the other passes and after the leaf-function pointer allocation ([Pointers in zero page](#pointers-in-zero-page-leaf-functions)). The same pass (tag `incdec`) updates other variables in place: `n--` (`%t = deref n; %u = sub %t, 1; store %u, n`) subtracts from `n` itself, and after `--n` (`%u = sub n, 1; store %u, n`) the uses of `%u` read `n` (until it is written again); the code generator then emits `inc`/`dec` (a byte), `inw`/`dew` (ZP), `inc`/`bne`/`inc` (static storage) or ca45's `inc.16f`/`dec.16f` (stack frame). Likewise a call's result stored into a variable of its type (`c = getc()`): its later uses read the variable, and the code generator stores it from A/X without a temporary |
| `rotate-loops` | `rotate` | Loop rotation (#138): a `for` / `while` loop's test (one block, branching to the body after it and out of the loop) moves after the loop's last block, which then falls into it; the block before the loop branches to the test once. The same code (the branch back becomes the entry's), one branch less an iteration; ca45 starts the body with the registers the test left (see `RedundantLoad`). Runs in code generation, after `reorder-blocks`, and is kept only where it does not cost bytes: cc45 assembles the file both ways (two ca45 runs at once), keeps a function's rotation when its code is no larger, and the result when the file's code is no larger as a whole (its object is the output: a third run only when some functions keep it and others not). `-Roptir` reports `[rotate] f: kept (a -> b bytes)` / `undone`. `-frotate-loops` (or `#pragma cc45 optimize(rotate-loops)`) rotates without the check; `-Os` (and `-fno-rotate-loops`) not at all. Examples: kept in 136 functions, undone in 36; every example the same size or smaller (315 bytes over all), `run_bench.sh` 1.6% faster (772 → 760 ms), loops 3–7% |
| `reorder-blocks` | `reorder` | The blocks of the branch that `__unlikely` / `__likely` (or `__builtin_expect` with a constant) says is not expected to run (an `if`'s then branch, or its else after `__likely`) move after all the others, in their order; a block that fell into the next one first gets a branch to it. The code generator emits the epilogue (`@__return`, the frame cleanup, `rts`) between the expected blocks and the cold ones, so the expected path falls through into it. Runs last |

After the passes, pure instructions whose temporary result nothing uses are
removed (`removeDeadTemps`; part of dead-code elimination, always on), and
calls of `__pure` / `const` functions whose result is unused.

### Pointers and variables in zero page (leaf functions)

A pointer dereference needs the pointer in zero page (`lda ($zp),y`); a
pointer kept in static or stack storage is copied to scratch first on every
access. In a **leaf function** (no `CALL`/`CALL_VOID`/`CALL_INDIRECT`, no
inline asm), so nothing can clobber the register pool between uses:

- `IRCodeGen` allocates named pointer, `char` and `int` locals, and far
  pointers used as far addresses (#348: `[$30],z` on the local itself) (not arrays,
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

After a function's code is generated, two cleanups run on its text: an
`ldx` just before a ZP-convention call that clobbers X anyway (an 8-bit
value's `ldx #0`; the arguments are in `$10`-`$2F`) is removed
(`pruneDeadLdx`, #296), and the `ora #0` / `inx / dex` that restores the
N/Z flags of a dropped load on the fall-through into a label is removed
when the code there sets N/Z before reading them (`pruneDeadFlagRestores`,
#297).

Other code generation for loops: an element address with a stride of 2, 4
or 8 shifts the index (`asl; pha; txa; rol; tax; pla` per doubling, 6 bytes)
instead of using the hardware multiplier (22 bytes); a store of a constant or
of a value in ZP to an element (`a[i] = 0`) computes the address first and
loads the value after it (no `pha`/`pla`); and `removeDeadTemps` runs again
after `post-increment-sink`, which drops the unused old value of `x++` as a
statement (`for (...; i++)`: only `inw $30` remains). Speed is measured with
`src/test/bench/run_bench.sh` (eight benchmarks on xemu, timed by CIA2;
not part of `make test`): 1344 ms at v1.0.6, 1041 ms after these (the first
six). `life` (a 2D grid with `int` counters) and `rows` (`y * 80 + x` with
`unsigned char` counters) were added with loop strength reduction: 102 → 94
and 109 → 103 ms for those loops against the code before it (October 2026).

A stack frame of 8 to 255 bytes is made by moving SP (`tsx; txa; sec; sbc #n;
tax; txs`, the low byte only, right with either stack) instead of pushing
zeros; when the low byte would wrap, the push loop runs instead (C locals
start undefined). ca45's redundant-load elimination keeps what the registers
hold across a local label nothing refers to (branch inversion leaves the loop
body's label so: `bcc @body; bra @end; @body:` becomes `bcs @end`), so a
loop body does not reload the counter the test just loaded (a label reached
only by one branch, not falling in, starts with the registers at that branch:
a forward branch's, or a backward one's from the previous sweep when a label
that resets everything lies between them, as in a rotated loop); `cmp.16` is known
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

`foldFarOffset` (`IROptimizer.cpp`, after `foldGlobalOffsetAccess`; tag
`faroff`; under `addr-elem-fusion`'s switch; #348) gives a far load or store
whose address is `p + c` (a 32-bit add of a constant, or a copy of `p`,
earlier in the block with `p` unchanged since) the address `p` and the byte
offset `c` (`elemOffset`: Z starts there, `ldz #c`, then `inz`), so
`p[0]`, `p[1]` use one far pointer. For `c >= 256` the address is
`p + (c & $FF00)`, computed once for the accesses of a block (`p[128]`,
`p[129]`); an access whose bytes would carry Z past 255, and a 4-byte load
(`ldq [p],z` takes no Z offset), keep the whole offset in the address. With
it: a 32-bit add copies the low bytes where the constant has 0s and stores
the constant's own bytes where the source is a zero-extension's 0
(`base + index` for a base `$xxxx0000`), and an `int`'s `x + c` stored
through a far pointer next (its only use) is added byte by byte as it is
stored. A game's map accessors: 89 → 70 and 278 → 170 bytes
(`test_compiler.sh`; runtime `test_far_offsets_xemu.c`).

**Known bits** (#350, `KnownBits.cpp`, computed per function by
`IRCodeGen`; `CC45_NO_KNOWN_BITS=1` turns it off): for each vreg defined
once, and each local assigned once (one store, its address not taken, not
a parameter: the stored value's bits; a read before the store reads an
undefined value), the bits known to be 0, from constants,
zero-extensions, shifts and `and`/`or`/`xor`, and the bounds of adds and
multiplications (`(unsigned)y << 9` has a zero low byte, `x << 2` of a char
fits 10 bits, `k * 4` has its low two bits clear). Code generation uses it
for a 16-bit `and`/`or`/`xor` with a byte known 0 on one side (the other
side's byte, or 0, built in A/X), for adds into memory (a byte where one
side is 0 and no carry can arrive is copied; bits that cannot overlap are
`ora`ed; `adc` only where a carry can come; #348's `+ $08300000` to a
zero-extended index is a case of it) and for `x + c` stored through a far
pointer (`ora #c` when `c` fits in `x`'s known-zero low bits). Test:
`test_known_bits_xemu.c`. Examples: 351 bytes (0.24 %); the map accessors of
#348: 70 → 63 and 170 → 163.

`orderCommutativeOperands` (`IROptimizer.cpp`, last of the late passes; tag
`commute`; under `algebraic-simplify`'s switch; #350): an `add`, `and`,
`or`, `xor` or `mul` whose second operand is the result of the instruction
just before (the first is not) takes it as its first, which code
generation loads into A/X: it is still there, so ca45 drops its store and
reload. Examples: 346 bytes more; the accessors 60 and 160 bytes.

**A result in its dying operand's bytes** (#350, `VRegAllocator`;
`CC45_NO_INPLACE=1` turns it off): `x + c` / `x - c` whose `x` (a temporary,
or a local kept in the register pool; not a parameter or a fixed slot) is
not used after it takes `x`'s base-page slot, which passes to it. The add
is then in place: bytes it does not change are left alone, and
`+ (1 << 8k)` is `inc` (with `bne` past the higher bytes): #348's `p + 256`
is `inc p+1; bne; inc p+2; bne; inc p+3`. Examples: 284 bytes more; the
accessors 59 and 147 bytes. (It showed a ca45 bug: the lexer names `inq` /
`deq` `incq` / `decq`, which the optimizer's register tracking did not
know, so a load after an in-place `inq $nn` was dropped as A's old value.)

**Base-page liveness** (ca45 Pass 1h, `AssemblerOptimizer.cpp`, under
`RedundantLoad`; `CA45_NO_SPILL_REG=1` turns it off; #350): in a cc45 proc
(and a proc's `.zar` block) the liveness of the 256 base-page bytes over its
code, backwards to a fixed point: a statement reads the bytes its operand
names (any statement it does not understand: all of them), a plain store
writes its own; branches and jumps go to their labels (one not in the proc,
a jump through a pointer, or a `jmp $0000` whose operand a computed goto
modifies: everything live); `rts`/`rti` and a tail call leave the proc (the
register pool and the block are dead there), except in a proc holding a
routine another proc calls (SeqExtract's shared `bsr __seqx_N`: its `rts`
returns into the caller) or with `bsr`s of its own (an `rts` a `bsr`'s
target reaches returns after any of them; one only the proc's own code
reaches is its exit); a call reads none of them. Then: a plain store to a
pool / block byte that is not live after it goes (`dead-store`), and
`sta zp ... lda zp` with A overwritten between becomes `tay ... tya` (or
`taz ... tza`) when the byte is dead after the load, Y (Z) is untouched
between and written before it is read after, nothing between is a label,
branch or call, and N / Z after the store are not read (`spill-reg`; 2 bytes
instead of 4). Labels are kept as `proc:name` and named `name` by operands.
(Pass 1e now leaves a proc that holds such a shared routine alone: the
caller reads what the routine stores.) Examples: 3,387 bytes (2.3 %); the
accessors 57 and 145 bytes. Test: `test_assembler.sh` (shared routine,
merge label, computed goto kept; a byte in Y; a dead store on every path).

The register tracking also keeps, at a local label reached by falling in
and by one earlier branch, what both paths agree a register holds (the
same constant, or a base-page byte equal to it on both paths: stored from
it, or holding the same constant; a `?:` or `&&`/`||` result stored on both
paths needs no reload; the label is still a reset point for the snapshots
taken after it); a call to a C function (`jsr _name`) counts as not
reading N / Z (the runtime's `__helpers` still do); and a load of a byte
stored from a register holding a constant becomes a transfer from another
register holding the same constant (`ldx #0; stx $35 ... lda $35` is
`txa`). Examples: 933 bytes (#350).

**Byte division** (#352): an unsigned 16-bit `/` or `%` whose operands both
have a known-zero high byte (an `unsigned char` promoted: `k / 3`, `k % 3`)
calls `__divmodu8` (`stdlib_asm/divmod.s45`: 8 iterations, the quotient in
A, the remainder in X, Y untouched) instead of the 16-bit routine.
`optimizeShareDivMod` (after CSE; tag `divmod`; under `strength-reduction`'s
switch) computes `x / c` and `x % c` of the same `x` and constant `c` in a
block with one divide, before both, and the remainder as `x - (x / c) * c`
(C's truncating division: signed values too); a byte times a constant is
shifts and adds in A. The issue's `a = k % 3; b = k / 3;`: 55 → 41 bytes,
one call. Examples: 311 bytes. Test: `test_divmod8_xemu.c`.

**Constant chains** (`optimizeAlgebraic`, #351, #353): `(x + c1) + c2` and
the forms with subtracts are `x + (c1 + c2)`, the constant wrapped to the
type (modular arithmetic: the same result at every width). Only when the
inner operation is defined once and its own operand is a global, a constant
or a value defined once (a local's slot could change between them).
`pl.items[7]` (`@_pl + 11 + 7`) is then `@_pl + 18`, which
`foldGlobalOffsetAccess` makes the load's address: `lda _pl+18` (22 bytes
→ 7); `x - 25 + 23` is `x - 2`.

**What a function's return carries** (#351): cc45 marks a function whose
caller may read the flags after it (`.var cc45.retnz = 1`: a `char` result,
tested by them) and one whose result has X (`.var cc45.retx = 1`: wider than
a byte), and any with inline asm, `__cpu` / `__flags` writes or an
interrupt's `rti` both ways. ca45 takes the `rts` of any other cc45 proc as
reading neither: a reload kept for its N / Z before a `void` / `int`
function's return goes, and so does a dead `ldx #0` (the dead-LDX pass).
Examples: 737 bytes.

`foldGlobalOffsetAccess` (`IROptimizer.cpp`, after `foldElemIndexOffset`;
tag `globaloff`; under `addr-elem-fusion`'s switch) gives a load or store
through `%p = add ptr @g, c` (`%p` defined once and used only as such
addresses) the address `@g+c` itself: a member of a global struct, an
element at a constant index (`g.y = 5` is `sta _g+2`, not the address
computed into a register pair and an indirect store). Only this file's
globals in the default address space (not `__zp`, whose base-page
addressing an absolute operand would miss, nor `__far`); where a `__noescape`
object goes counts `@g+c` as a use of `g`. The examples: 3.9 %
less code with the changes below (150,159 → 144,291 bytes over 42
objects). Validation: `test_peephole_xemu.c`.

The same changes in the code generator and ca45:

- **Sign extension** (`sxt.8`, ca45): `ldx #0`, `ora #0`, `bpl`, `dex` (7
  bytes, no stack; was `pha` … `pla`, 10), A kept, N / Z from A.
- **Constant masks byte by byte** (`and.16`/`ora.16`/`eor.16` and the
  `.32` forms with `#imm`, ca45): a byte the constant leaves as it is (`and
  #$FF`, `ora` / `eor #$00`) is skipped, one it sets (`and #$00`, `ora
  #$FF`) loaded (`ldx #0` …), N / Z still A's (`x & $00FF` is `ldx #0` `and
  #$FF`, 4 bytes, was 8).
- **32-bit shifts by a constant over 8** (not a multiple of 8, cc45): the
  bytes moved first (the 8 / 16 / 24 forms), then the remaining 1–7 bits;
  after 24 only one byte is left, shifted with the 1-byte `lsr` / `asr` /
  `asl` (`u >> 28`: 11 bytes, was 28 `lsrq`, 84). Shifts by 1–7 stay
  single-bit shifts (a byte move and back would lose bits).
- ca45's `CmpElimination` and `RedundantLoad` passes below: flag tests
  and store-reloads.

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
times, up to 16 bytes (`-fzp-locals-max=N`; `c45.lib` is built with 8: there
8 bytes per function cover 70% of the static-storage accesses, 16 cover 87%,
but 16 makes deep stdio chains overflow the area; in programs 16 is 537 bytes
smaller over the examples than 8 without overflowing). ca45
relocates those operands against `<f>__zar` and ln45 places the blocks in
`$80`–`$E9` so that functions active together never share bytes
(`computeZarOverlay`, the SAC overlay's call graph plus edges from functions
that call through pointers to address-taken functions; callers placed before
callees). The register pool ends at
`$7F`. Blocks that do not fit share bytes, and ln45 calls the functions on
the inside of such pairs through save/restore wrappers (a warning).
`-fno-zp-locals` turns it off; validation: `test_zp_locals_xemu.c` (two
modules) and the overflow program in `test_xemu.sh` (both conventions, a
wrapped function also called through a pointer). The choice can instead be
made for the whole program by a two-pass build: `ln45 --zp-plan-out=FILE`,
then `cc45 -fzp-plan=FILE` (#304,
[ln45: the program-wide plan](../bin/ln45.md#the-program-wide-base-page-plan);
vim2's code 6,716 bytes smaller, no wrapper).

The rest of a static-allocation function's locals and temporaries have
slots in its SAC block (`<f>__local_<id>`), and those never live at the
same time share their storage (`-fsac-slot-sharing`, on from `-O1`, #324;
`SacSlotShare.cpp`). The pass works on the function's finished assembly,
which shows what really reads and writes each slot: a liveness of every
byte of the slots over the code between `proc` and `endproc`, following its
labels and branches (a store writes the bytes it names, a load or
read-modify-write reads them; an instruction it does not know, or an indexed
or indirect operand, reads every byte of the slots it names; a jump through
a pointer or out of the function, or data in the code, has every slot live).
Two slots clash when one is written while the other is live, or both are
live at the entry; the others are grouped, largest first, and a group's
labels share one `.res`. A slot whose address is taken (`#<f>__local_N`) or
that is named outside its function keeps its own storage, and a function
that calls `setjmp` is left as it is. Slots of values the code never stores
(kept in registers or folded away) take no storage. `cc45.sacsize.<f>` gives
the new size, so ln45's SAC overlay shares less: in vim2 the overlay went
from 2,749 to 2,347 bytes (the blocks before sharing from 18,130 to
12,768), in pfest from 957 to 474; the code is unchanged. Test:
`test_sac_share_xemu.c` (two modules; with `-fno-zp-locals` too).

A ZP-convention leaf function goes further: its parameters stay in the ZP
parameter block where the caller put them (`$10`…), with no copy at the entry
and no `leafPointerParamsToZp` copy ([calling conventions](calling-conventions.md);
`CC45_NO_ZP_PARAMS=1` turns it off, `test_zp_params_xemu.c`).

The IR is not in SSA form (a variable's slot is assigned by `STORE`; a
temporary can be defined on two paths, as in `a && b`), so every pass treats
a value as constant or fixed only when it has exactly one definition. The constants the passes see (`singleDefConsts`): a
temporary defined once by `CONST`; a `zext`/`sext`/`trunc` of one (the
converted value, sign-extended from its width); and a variable (a local's or
parameter's slot) stored exactly once with a constant, in the entry block
before any read there, when nothing branches to the entry block, its address
is never taken, it is not volatile, not reached by a nested function
(`memoryVregs`), not a `register` variable, and the function has no inline
assembly. Commutative operations (`+ * & | ^`) get a constant first operand
moved second before strength reduction and the algebraic simplification
look at them, which runs again after type narrowing.

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
| `-O1` | 1 | The IR passes |
| `-O2`, `-O3` (default `-O2`) | 2, 3 | IR passes and automatic inlining of small functions. Identical output |
| `-O4` … `-O9` | 4–9 | Adds loop interchange and loop unrolling. Identical output for all of them (the catalog entries for higher levels are not implemented) |
| `-Os`, `-Osize` | 2 | `-O2` with `-fmul-call` (multiplications as library calls) and `-fno-rotate-loops` (unless `-frotate-loops`) |
| `-Ospeed` | 3 | Same as `-O2` |
| `-O` (no digit), any other `-O<text>` | 0 | **Same as `-O0`** (not `-O2`) |

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
| `#pragma cc45 optimize(off)` / `optimize(on)` | — | `#pragma cc45 optimize(off)` … `optimize(on)`: the functions defined between them compiled as written, to read their code before and after optimizing or to step through it: no IR pass (nor dead-temporary removal, loop interchange), no inlining of them into callers or of calls inside them, and ca45's passes (redundant loads, branch inversion, tail calls, tail merging, sequence extraction, …) leave their code alone (cc45 puts it between `.noopt_start` and `.noopt_end`). Inside a function body, for that function only. What code generation always does (constant folding of the source, its choice of instructions) stays; the other functions are optimized as usual (`test_noopt_xemu.c`). |
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
| `CmpElimination` | `cmp-repeat` | A `CMP`/`CPX`/`CPY`/`CPZ` at a label that only one conditional branch reaches, that branch right after the same compare, with no fall into the label: the flags are still that compare's (#341) |
| `RedundantLoad` | `dead-store` (base page) | A store into the proc's own base-page block (`.zar`), or in a cc45 proc into the register pool, that nothing in the proc reads is dropped (#341), and one that the straight path from it overwrites or leaves the proc by `rts`/`rti` before reading (#348; not in a proc with subroutines of its own, `bsr __seqx_N`: their `rts` returns into it). 16/32-bit operations (`ora.16 .AX, $34`) read their operands. A labelled store stays (its label may start a SeqExtract routine). Registers known at a branch include a byte they were stored to (`sta k` then `bne @l`: at `@l`, A is `k`) |
| `JmpBra` | `jump-rts`, `jump-chain`, `dead-rts` | Jump threading (#340): a `BRA`/`JMP` to an `RTS` becomes the `RTS`; a branch to a `BRA`/`JMP` goes to its target (a relative one when in range); an `RTS` right after a `JMP`/`BRA`/`RTS` whose labels are local and named nowhere is dropped (not in a proc that branches by `*+n`). `endproc` adds no `RTS` after a final `JMP`/`BRA` either |
| `JSRRelocate` | `jsr-bsr` | `JSR` → `BSR` when the target is a resolved, non-relocatable label within ±32 KB (rare in `-c` objects) |
| `DeadStore` | `dead-store` | Removes dead `STW.SP` stores |
| `CmpElimination` | `cmp-elim`, `flag-test` | Removes `CMP #0` when flags already reflect the value — not when the carry it sets may be read: forward past what leaves C alone (loads, stores, transfers, `INX` …, `BEQ`/`BNE`/`BMI`/`BPL`/`BVS`/`BVC`) to what reads it (`BCC`/`BCS`, `ADC`/`SBC`/`ROL`/`ROR`/`PHP`: kept; `lda $lo` / `ldx $hi` / `cmp #0` / `txa` / `sbc #1` is a 16-bit compare) or sets it (`CLC`, `CMP`, `ASL` …: removed); a label, call or simulated instruction counts as a read (it looked at only the first instruction after the branches). Also removes a flags test that changes nothing (`ORA #0`, `EOR #0`, `AND #$FF`: N / Z only) when the flags reflect A; when an `LDX`/`LDY`/`LDZ #imm` came between A's `LDA` and the test (a byte widened before it is tested, only stores between them and the test), the load moves before the `LDA` (not indexed by that register) and the test goes |
| `FCmpOpt` | `fcmp-opt` | Float compare result tests (`CMP #$FF` → `BMI`/`BPL`) |
| `RedundantLoad` | `redundant-load`, `store-transfer` | A load of a zero-page slot that another register holds the value of (stored from it, both as they were since) becomes a transfer (`TAX` … `TZA`, 1 byte, the same N / Z); when the value stored from a register changed since, the transfer goes right after the store and the load disappears, if the register loaded is neither read nor written between (no label, call, branch or simulated instruction, no write through a pointer or an index) and N / Z are not read after it (or nothing between sets them). Removes `LDA/LDX/LDY/LDZ` when the register already holds the value (what A/X/Y/Z hold carries into a local label reached only by one branch: from a forward branch, or from a backward one when a referenced label lies between them, #138), unless its N/Z may be read (the flags reflect another value and a branch, `PHP`, jump, return or call may read them). Also removes an `LDX #imm` (cc45's zero extension of a byte used as a byte) when neither X nor the N/Z flags it sets are read on any path before they are written again: a liveness of X and N/Z over the file's statements, following branches and jumps to its labels; a call, return, simulated instruction (but `ldax #v`), Q instruction (but `ldq`), X-indexed operand, jump through a pointer or to a label not in the file, data or `.segment` counts as reading both (#321; pfest: 214 fewer, 434 bytes) |
| `StoreLoadPair` | `store-load-pair` | Store/load pair simplification on ZP |
| `PreserveXSP`, `TSXRedundant` | `preserve-x-sp`, `tsx-redundant` | Avoid reloading X from SP |
| `TailDedup` | `tail-dedup`, `tail-dedup-suffix` | Merges identical tail sequences. A base-page operand of a proc with a `.zar` block is relocated against that proc's own block, so it matches only tails of the same proc (#327: a merged tail read another function's base-page bytes) |
| `SeqExtract` | `seq-extract` | Extracts a 4-instruction run repeated in one proc into a routine before its `endproc` (`BSR` calls); cc45 `-fno-seq-extract` turns it off (benchmark: 98 bytes smaller, ~0.5% slower) |
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
| `param-narrowing`, `strength-reduction`, `algebraic-simplify`, `type-narrowing`, `branch-folding`, `cse`, `loop-invariant-code-motion`, `loop-strength-reduction`, `copy-propagation`, `addr-elem-fusion`, `post-increment-sink`, `rotate-loops`, `reorder-blocks` | IR pass ([IR passes](#ir-passes)); also per function by pragma | `-O1` |
| `inline-small-functions` | IRBuilder inlining (`-fno-` also overrides `-finline-functions`) | `-O2` |
| `devirtualization` | IRBuilder | `-O0` |
| `sac-slot-sharing` | Code generation: SAC locals never live together share storage ([IR passes](#ir-passes)); file-wide only | `-O1` |
| `tail-call-optimization`, `branch-inversion`, `jump-optimization`, `redundant-load-elimination`, `dead-store-elimination`, `seq-extract` | ca45 pass: cc45 passes `-P<x>` / `-PNo<x>` (`TailCall`, `BranchInvert`, `JmpBra`, `RedundantLoad`, `DeadStore`, `SeqExtract`) to ca45; file-wide only | ca45's default (always) |
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
| `#pragma cc45 unroll [N]`, `#pragma cc45 unroll_default N` | Turned into a directive that nothing reads; no effect |
| `--emit-ir` | Parsed, never used. Use `CC45_DUMP_IR=1` for IR (`-Rcodegen` annotates the assembly: [cc45.md](../bin/cc45.md)) |
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
