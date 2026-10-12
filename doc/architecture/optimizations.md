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
     folding, CSE, loop-invariant code motion, copy propagation, address fusion.
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
| Unused static function removal (`IRBuilder::generate`) | Removes `static` functions that nothing in the file calls or takes the address of (repeats until stable, so chains go too). Non-static functions are always kept | Always on |
| Devirtualization (`IRBuilder::generate`) | A virtual call through a vtable slot that has only one implementation **among the vtables of this file** becomes a direct `CALL`; reports `note: devirtualized N virtual call(s)`. A subclass defined in another file that overrides the method is not seen, so its override is bypassed: use `-fno-devirtualization` in a file whose classes are extended elsewhere | On at every level; `-fno-devirtualization` |
| IR passes (`IROptimizer.cpp`) | See [IR passes](#ir-passes) | `-O1`+; `-f[no-]<pass>`; `#pragma cc45 optimize(...)` |
| Parameter narrowing advisory | For `static` functions: `note: parameter N of 'f' could be 'unsigned char' (all call sites pass 0-255)` (or `unsigned int`). Advisory only, code is not changed | Always on |
| Vector-extension unrolling | Element-wise vector operations are unrolled when the size estimate fits the limit, otherwise a loop is emitted | `-fmax-unroll-bytes=N` / `#pragma cc45 max_unroll_bytes(N)` |
| `repeat(N) { ... }` | Body is emitted N times | Always unrolled (language construct) |
| Static Allocation Convention (SAC) | Locals/params of non-recursive functions in static BSS records instead of a stack frame | `-fstaticalloc` (default) / `-fno-staticalloc`; see `calling-conventions.md` |
| ZP calling convention | Parameters in ZP `$10-$2F` | `-fzpcall` (default) / `-fno-zpcall` |
| Code generation (`IRCodeGen`) | Store-fused 16-bit add/sub, single-bit `AND` + branch → `BBS`/`BBR`, fused compare-and-branch, CONST suppression, constant-address direct stores, per-function `.reg_clobbers`/`.flag_clobbers`/`leaf` metadata | Always on, not tied to `-O` |
| Assembler peephole (`ca45`) | Branch inversion, JMP→BRA, redundant loads, tail calls, … (table below) | ca45 `-O`/`-P` options; from cc45, five of them through `-f[no-]<name>` |

Ordinary `for`/`while` loops are never unrolled or interchanged (LICM hoists
invariant computations out of them). The "loop optimization" and
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
| `strength-reduction` | `strength` | `x * 2^n` → shift, unsigned `x / 2^n` → shift, unsigned `x % 2^n` → `AND`, `x * 1` → copy (signed division is left alone: it rounds toward zero) |
| `algebraic-simplify` | `algebraic` | `x + 0`, `x - 0`, `x * 1`, `x | 0`, `x ^ 0`, `x << 0`, `x & -1` → `x`; `x * 0`, `x & 0` → 0; `x - x` → 0 |
| `type-narrowing` | `narrow` | An `ADD`/`MUL`/`AND`/`OR`/`XOR`/`SHL`/`NOT` whose result is only used through a truncation to 8 bits is computed in 8 bits |
| `branch-folding` | `branch` | Removes instructions after a block's terminator, folds branches on constants, threads jumps to blocks that only jump, removes unreachable blocks |
| `cse` | `cse` | Within a block, a repeated pure expression reuses the earlier result; copy propagation of temporaries. Forgets an expression when an operand is reassigned, and on stores through pointers, calls and inline asm |
| `loop-invariant-code-motion` | `licm` | Moves pure computations whose operands don't change in the loop to a pre-header. A variable whose address is taken counts as changed when the loop calls a function, stores through a pointer or contains asm |
| `copy-propagation` | `copy` | Replaces uses of a copy of a temporary by the original and removes the dead copies (temporaries defined once only; never variables) |
| `addr-elem-fusion` | `fusion` | An array element address used once, by the next 1/2-byte (or 4-byte load) access of an integer or pointer, is computed inside that load/store (no pointer temporary) |

The IR is not in SSA form (a variable's slot is assigned by `STORE`; a
temporary can be defined on two paths, as in `a && b`), so every pass treats
a value as constant or fixed only when it has exactly one definition.

## Optimization levels (cc45)

Each catalog optimization has a level from which it is on
(`OptimizationCatalog::isEnabledAtLevel`); `-f`/`-fno-` override it.

| Option | Level | Effect |
|---|---|---|
| `-O0` | 0 | No IR passes, no automatic inlining |
| `-O1` | 1 | The eight IR passes |
| `-O2` … `-O9` (default `-O2`) | 2–9 | IR passes and automatic inlining of small functions. Identical output for all of them (the catalog entries for higher levels are not implemented) |
| `-Osize` | 2 | Same as `-O2` |
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
| `CmpElimination` | `cmp-elim` | Removes `CMP #0` when flags already reflect the value |
| `FCmpOpt` | `fcmp-opt` | Float compare result tests (`CMP #$FF` → `BMI`/`BPL`) |
| `RedundantLoad` | `redundant-load` | Removes `LDA/LDX/LDY/LDZ` when the register already holds the value |
| `StoreLoadPair` | `store-load-pair` | Store/load pair simplification on ZP |
| `PreserveXSP`, `TSXRedundant` | `preserve-x-sp`, `tsx-redundant` | Avoid reloading X from SP |
| `TailDedup` | `tail-dedup`, `tail-dedup-suffix` | Merges identical tail sequences |
| `SeqExtract` | `seq-extract` | Extracts repeated sequences into shared routines; **only with `--experimental`** |
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
| `strength-reduction`, `algebraic-simplify`, `type-narrowing`, `branch-folding`, `cse`, `loop-invariant-code-motion`, `copy-propagation`, `addr-elem-fusion` | IR pass ([IR passes](#ir-passes)); also per function by pragma | `-O1` |
| `inline-small-functions` | IRBuilder inlining (`-fno-` also overrides `-finline-functions`) | `-O2` |
| `devirtualization` | IRBuilder | `-O0` |
| `tail-call-optimization`, `branch-inversion`, `jump-optimization`, `redundant-load-elimination`, `dead-store-elimination` | ca45 pass: cc45 passes `-P<x>` / `-PNo<x>` (`TailCall`, `BranchInvert`, `JmpBra`, `RedundantLoad`, `DeadStore`) to ca45; file-wide only | ca45's default (always) |
| `constant-folding`, `dead-code-elimination`, `frame-pointer-optimization` | Part of code generation, always on: `-fno-` warns *"cannot be disabled"* | always |
| `loop-unrolling`, `loop-interchange`, `cross-function-inlining`, `co-optimization`, `procedure-inlining`, `interprocedural-optimization` | **Not implemented**: accepted and ignored; with `--future` cc45 warns *"optimization 'loop-unrolling' is not implemented yet; -floop-unrolling has no effect"* | — |

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
