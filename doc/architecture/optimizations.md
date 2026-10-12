# cc45 Optimization Guide

This page describes what the toolchain actually optimizes today, which options
control it, and which documented-looking options have no effect. Everything
here was checked against the sources (`src/main/cc45_main.cpp`,
`CompilationPipeline`, `OptimizeStage`, `CodegenStage`, `IRBuilder`,
`IRCodeGen`, `AssemblerOptimizer`, `ca45_main.cpp`) and by compiling test files
with `./bin/cc45` (see [Checking it yourself](#checking-it-yourself)).

## Summary

- A C file goes through three places where code is improved:
  1. **cc45 front end / IR builder**: AST constant folding, function inlining,
     removal of unused `static` functions, devirtualization, SAC.
  2. **cc45 code generator** (`IRCodeGen`): instruction selection with built-in
     fusions/suppressions. Always on, no options.
  3. **ca45 peephole optimizer** (`AssemblerOptimizer`): runs on the generated
     assembly when cc45 builds a `.o45` or `.prg`. cc45 always runs it at
     ca45's default level (`-O2`).
- **`-O` levels in cc45 only switch automatic inlining.** `-O0` and `-O1` give
  the same output; `-O2` through `-O9` give the same output (the default is `-O2`).
- **There are no `-f<pass>` / `-fno-<pass>` switches for individual passes in cc45**,
  and no `#pragma cc45 optimize(...)`. Names such as `-fconstant-folding`,
  `-floop-unrolling`, `-fcse`, `-fno-dead-store-elimination` are not recognized
  (see [Options that do nothing](#options-that-do-nothing)).
- The IR passes in `src/main/IROptimizer.cpp` (CSE, copy chains, LICM, algebraic
  simplification, strength reduction, type narrowing, branch folding, ADDR_ELEM
  fusion) **are never called** and are not linked into `bin/cc45` (the fused
  ADDR_ELEM+LOAD/STORE code in `IRCodeGen` is therefore unreachable). Multiply/divide
  by a power of two, for example, still goes through the hardware
  multiplier/divider, and `x * 1` is still a multiply.

## Where optimization happens

| Stage | What it does | Controlled by |
|---|---|---|
| AST constant folding (`ConstantFolder`) | Folds constant sub-expressions (`2 * 4` → `8`). Variables are never replaced by their initial values | Always on, at every `-O` level |
| Inlining (`IRBuilder`) | Expands calls to inline candidates at the call site | `-O2`+, `-finline-functions`, `inline`, attributes, size limit (below) |
| Unused static function removal (`IRBuilder::generate`) | Removes `static` functions that nothing in the file calls or takes the address of (repeats until stable, so chains go too). Non-static functions are always kept | Always on |
| Devirtualization (`IRBuilder::generate`) | A virtual call through a vtable slot that has only one implementation **among the vtables of this file** becomes a direct `CALL`; reports `note: devirtualized N virtual call(s)`. A subclass defined in another file that overrides the method is not seen, so its override is bypassed | Always on, cannot be disabled |
| Parameter narrowing advisory | For `static` functions: `note: parameter N of 'f' could be 'unsigned char' (all call sites pass 0-255)` (or `unsigned int`). Advisory only, code is not changed | Always on |
| Vector-extension unrolling | Element-wise vector operations are unrolled when the size estimate fits the limit, otherwise a loop is emitted | `-fmax-unroll-bytes=N` / `#pragma cc45 max_unroll_bytes(N)` |
| `repeat(N) { ... }` | Body is emitted N times | Always unrolled (language construct) |
| Static Allocation Convention (SAC) | Locals/params of non-recursive functions in static BSS records instead of a stack frame | `-fstaticalloc` (default) / `-fno-staticalloc`; see `calling-conventions.md` |
| ZP calling convention | Parameters in ZP `$10-$2F` | `-fzpcall` (default) / `-fno-zpcall` |
| Code generation (`IRCodeGen`) | Store-fused 16-bit add/sub, single-bit `AND` + branch → `BBS`/`BBR`, fused compare-and-branch, CONST suppression, constant-address direct stores, per-function `.reg_clobbers`/`.flag_clobbers`/`leaf` metadata | Always on, not tied to `-O` |
| Assembler peephole (`ca45`) | Branch inversion, JMP→BRA, redundant loads, … (table below) | ca45 `-O`/`-P` options; cc45 always uses ca45's default `-O2` |

Ordinary `for`/`while` loops are never unrolled, interchanged or hoisted. The
"loop optimization" and "cross-module optimization" steps in `OptimizeStage` and
`CodegenStage` (`performLoopOptimizations`, `performCrossModuleOptimization`,
`applyIPOSpecialization`, `optimizeIR`, `applyInlining`, `eliminateDeadCode`)
only count functions and print messages with `-vv`; they change nothing.

## Optimization levels (cc45)

The level is read in `cc45_main.cpp` and used in exactly one place that
changes code: `builder.inlineFunctions = inlineFunctions || optimizationLevel >= 2`.

| Option | Level | Effect |
|---|---|---|
| `-O0`, `-O1` | 0, 1 | No automatic inlining. Identical output |
| `-O2` … `-O9` (default `-O2`) | 2–9 | Automatic inlining of small functions. Identical output for all of them |
| `-Osize` | 2 | Same as `-O2` |
| `-Ospeed` | 3 | Same as `-O2` |
| `-Os`, `-O` (no digit), any other `-O<text>` | 0 | **Same as `-O0`** (not `-O2`) |

Only the first digit counts (`-O10` is `-O1`). The level is **not** passed on to
ca45.

The `-O` level does not turn constant folding, dead static removal,
devirtualization, SAC or the ca45 peephole optimizer on or off.

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

Inlining usually makes the program **larger** on this target: for
`src/test-resources/test_phase91_validation.c` the one-step PRG is 526 bytes at
`-O0` and 571 bytes at `-O2`; `test_phase91_tuning.c` is 1249 vs 1610 bytes.
Use `-O0`/`-O1`, or `-fmax-inline-ir-size=0`, when size matters
(`-O2 -fmax-inline-ir-size=0` produced output identical to `-O0` for the test
file used here). The limit also applies to `inline` functions; only
`always_inline` bypasses it.

## Options that control optimization

| Option / pragma | Default | Effect |
|---|---|---|
| `-O<n>` | `-O2` | See above |
| `-finline-functions` | off | Automatic inlining even at `-O0`/`-O1` (there is no `-fno-inline-functions`) |
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
| `TailCall` | `tail-call` | `JSR x` + `RTS` → `JMP x`. Does not fire on cc45 output in practice: the `RTS` is labelled `@__return:` |
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

## Options that do nothing

| Option | What happens |
|---|---|
| `-fconstant-folding`, `-fdead-code-elimination`, `-finline-small-functions`, `-ftail-call-optimization`, `-fstrength-reduction`, `-falgebraic-simplify`, `-floop-unrolling`, `-floop-interchange`, `-floop-invariant-code-motion`, `-fcross-function-inlining`, `-fdevirtualization`, `-fcse`, `-fcopy-propagation`, `-fbranch-inversion`, `-fbranch-folding`, `-fjump-optimization`, `-fredundant-load-elimination`, `-fdead-store-elimination`, `-fframe-pointer-optimization`, `-fco-optimization`, `-fprocedure-inlining`, `-finterprocedural-optimization`, their `-fno-` forms, and any other unknown option | Not recognized. cc45 treats an unknown argument as the input file name: placed **before** the source file it is silently overwritten (no effect); placed **after** it, compilation fails with `Failed to open input file: -f...` |
| `#pragma cc45 optimize(...)` (any argument: `none`, `speed`, `space`, `default`, pass names) | Dropped by the preprocessor, no diagnostic |
| `#pragma cc45 unroll [N]`, `#pragma cc45 unroll_default N` | Turned into a directive that only the unused `CodeGenerator` reads; no effect |
| `--help-optimizations` | Not an option (treated as an input file name) |
| `--emit-ir`, `-Rcodegen` | Parsed, never used. Use `CC45_DUMP_IR=1` for IR |
| `--pragma <p>` | Parsed into `cliPragmas`, never used: the pragma is not injected |

`src/main/OptimizationCatalog.cpp` / `OptimizationController.cpp` hold a
catalog of 24 named optimizations with levels; nothing in cc45 uses it.

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
- Unknown options are ignored the same way (see above).

Example:

```
# ~/.config/m65/cc45.conf
-O1
-fmax-unroll-bytes=64
```

## Checking it yourself

```bash
cc45 -S -O0 t.c -o t0.s ; cc45 -S -O2 t.c -o t2.s ; diff t0.s t2.s   # only inlining differs
cc45 -S -O2 t.c -o a.s ; cc45 -S -O9 t.c -o b.s ; cmp a.s b.s        # identical
cc45 -c -O2 t.c -o t.o45 ; nm45 -s t.o45                              # segment sizes
nm45 -f t.o45                                                         # zp_call / leaf / clobbers per function
objdump45 -d t.o45                                                    # final code after ca45
cc45 -S t.c -o t.s ; ca45 -c -Roptimizer t.s -o /dev/null             # ca45 peephole report
CC45_DUMP_IR=1 cc45 -S t.c -o /dev/null                               # IR after IR generation
```

A personal `~/.config/m65/cc45.conf` changes the default level; run with
`HOME=/some/empty/dir` to compare against the built-in defaults. For files with
at most three functions cc45 currently prints a `[DCE] Functions: ...` debug
line to stderr; it does not affect the output.
