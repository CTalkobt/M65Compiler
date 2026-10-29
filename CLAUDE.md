# MEGA65 C Compiler Suite — Codebase Guide

**Version:** 1.1.0.dev (`include/Version.hpp`, `SUITE_VERSION`)
**Maintainer:** Craig Taylor (CTalkobt)

This file is the overview for working in the repository. The validated
reference documentation is in `doc/bin/*.md` (one page per tool) and
`doc/architecture/` (calling conventions, object format, IR, optimizations,
stdlib, opcodes). When this file and those docs disagree, the docs (checked
against the source on 2026-10-11) win — and the source wins over both.

---

## Project Overview

A toolchain for the MEGA65 (45GS02 CPU), written in C++17 with no external
dependencies (SQLite and FUSE only for disk45):

- **cc45** — C compiler (one input file per run; `-S`, `-c`, or a one-step build to a `.prg`)
- **cp45** — the C preprocessor (a link to cc45, selected by program name)
- **ca45** — 45GS02 assembler (flat binaries or relocatable `.o45` objects with `-c`)
- **ln45** — linker (`.o45` objects + `.lib` archives → PRG or flat binary)
- **ar45** — archiver for `.lib` static libraries
- **nm45** — symbol/attribute/relocation inspector for `.o45` and `.lib` (and ln45's `.ovd`/`.ov1` overlay files)
- **objdump45** — object/PRG/binary/`.ovd`/`.ov1` inspector and disassembler (also BASIC listing with `-bas`)
- **basic45** — BASIC 65 program generator (label-based source, PETSCII tokenizer, preprocessor,
  long variable names, block IF / proc, `--link` C functions, `--decompile`, `--disk`, `--emulator`)
- **disk45** — CBM disk/tape/archive image utility (25 formats, FUSE mount, SQLite catalog)
- **cvt_asm** — assembler source converter between 7 dialects
- **pack45** — packs data into a stream the library's `unpack()` (`<unpack.h>`) unpacks by DMA to any 28-bit
  address, in place if loaded inside the area (`include/Pack45.hpp`, format 1); `ln45 --pack-overlays` packs the
  overlay files with it (#345)
- **tune45** — music for the tune player (`<audio_tune.h>`): text (pfest's format) or MIDI → data;
  plays on the PC (reSIDfp, `third_party/libresidfp`), in xemu or on a MEGA65; composes (`create`)
- **dbg45** — source-level debugger in xemu (#202): the program's debug file (`cc45 -g` / `ln45 -g`:
  `<prog>.dbg`, JSON), breakpoints by line / function, stepping, the stack, variables by type; drives
  xemu's monitor socket (needs its `k` breakpoint list: CTalkobt/xemu `cc45-dev`, which the Docker images build);
  `--dap` for VS Code (`tools/ide/vscode-extension`, debug type `cc45`)

## Architecture

### Compilation Pipeline

```
C source
  → cp45 preprocessor (integrated; #pragma cc45 … become asm(".directive"))
  → Parser → AST (constant folding, function analysis)
  → IRBuilder (IR generation, inlining, static allocation, register allocation)
  → IRCodeGen (ca45 assembly: proc/endproc, .loc, per-function metadata)
  → ca45 -c  → .o45
  → ln45 -basic (or -prg -t <base>) with c45.lib / c45_stack.lib → .prg
```

The live compiler path is **IRBuilder → IRCodeGen** (the older AST-level
`CodeGenerator` and the code only it used were removed, #92). Code from
earlier "phases" (cross-module IPO specialization, field caching, bank
assignment, link-time coordination, compiler hooks, adaptive tuning,
benchmarking, DWARF output, O45Linker's analysis passes) is in
`lib45-phases.a`, which `make all` builds but no tool links — see
`doc/architecture/cross-module-optimization.md`. Code nothing calls belongs
there (a `*Phases.cpp` file for a live class's dead methods), not in the
archives the tools link.

`cc45 file.c -o file.prg` builds in one step: relocatable code, `ca45 -c`,
then `ln45 -basic` with the standard library of the calling convention
(`bin/../lib/build/` or an installed `bin/../lib/cc45/`, else `-l` via
`CC45_LIB`). The library's `crt0` member is the startup code. `-S` alone
writes a standalone assembly form with its own startup stub.

### Code Overlays (details: `doc/bin/ln45.md#code-overlays`)

A file with `#pragma cc45 overlay(name)` / `-foverlay=name` (ca45 `.overlay`)
has its code in that overlay: ln45 links all overlays to run at one region
below the stack's share (`__ovl_region`; the heap ends there) and writes
them to `<prog>.ov1` (on the disk as `<PROG>.OV1`, `--overlay-name`), which
crt0's `__ovl_init` loads into bank 5 (`--overlay-store`; bank 1 is BASIC's; further
ranges take what does not fit, a file per 64 KB bank: `<prog>.ov2`, …; ranges in attic
RAM `$8000000`+ hold copied overlays, loaded through `--overlay-staging` bank 5 and
copied up by DMA; `far_attic_init` leaves them out).
Calls from outside an overlay (and addresses taken) go through 6-byte
resident stubs (`jsr __ovl_calli` + id + address; 21 bytes with ln45
`--overlay-fast-stubs`) to `__ovl_call` (`lib/stdlib_asm/ovl.s45`), which copies the
overlay in by DMA and swaps the return address (stack layout unchanged).
Copied by default; `--overlay-map` maps a window of `$4000`-`$7FFF` to each
overlay instead (only the lower half of MAP: the KERNAL is mapped at $E000;
the program ends below the window, the heap starts above it; the KERNAL's
current map at `$011C` is kept in step). Data/BSS stay resident (but
`__noescape` constants and `#pragma cc45 noescape_strings` literals are kept
with the code: in the overlay, or a `__far` function's chunk); no
`__interrupt` in an overlay. ln45 describes the overlays and their symbols in
`<prog>.ovd` (`include/O45OverlayFile.hpp`; never loaded), which nm45 and
objdump45 read; the `-M` map shows each overlay's bank and its symbols and
lines per overlay. Examples `examples/c/overlays` and
`examples/c/mapped_overlays` (checks MAP really runs the code in bank 5),
test `src/test-resources/overlay/`. **`__far` functions**: cc45 emits them in the
text sub-segment `far`; ln45 packs each object's `far` chunk into automatic
mapped overlays `far1`, `far2`, … (8 KB, 16 KB if a chunk needs it; per-chunk
overlay: `TextRemap::ovl`, `InputObject::ovlAt`); test `src/test-resources/far/`. **`__bank(n)`** (#69): a
variable in bank 4/5 or attic RAM (text sub-segment `bank.n`: a data overlay `bankN`, never run, its symbols
at their far addresses, reached in C as a far object through a hidden `.dword`) or a `__far` function whose
overlay is kept in bank n (`far.n`); placed by `placeOverlayStore`; test `src/test-resources/bank/`.

### Calling Conventions (details: `doc/architecture/calling-conventions.md`)

- **ZP convention** (default, `-fzpcall`, library `c45.lib`): arguments packed
  left to right into the ZP parameter block `$10`–`$2F` (`char`/`_Bool` a
  2-byte slot, `int`/pointer 2, `long` 4, `float` 5). Variadic functions and functions with
  more than 32 bytes of parameters use the stack convention instead.
  `__fastcall__` makes a single function ZP in a stack-mode file.
- **Stack convention** (`-fno-zpcall`, library `c45_stack.lib`): arguments
  pushed right to left (at least 2 bytes each); the caller removes them.
- **Static allocation (SAC)** (default; `-fno-staticalloc` disables):
  non-recursive functions keep parameters and locals in static BSS
  (`_f__param_x`, `_f__local_N`) and have a second entry `_f__sac`. Variadic
  functions too (their `...` stays on the stack: `va_start` reads `_f__va`, set by
  the entry thunk). Functions on a call cycle within the file, `__interrupt`
  and runtime-`alloca` functions get stack frames; `#pragma cc45 recurse` forces a frame for the
  next function. ln45 rejects SAC functions on a cross-module call cycle and
  SAC functions reachable from an `__interrupt` handler, and overlays the
  storage (`_f__ar` blocks) of SAC functions never active at the same time
  (a call through a pointer counts as a call to every address-taken
  function with the pointer's parameter slots: cc45 `cc45.isig.<f>.<sig>` /
  `cc45.fsig.<g>.<sig>`; to every one through an unprototyped pointer, a
  cast or inline asm, `cc45.icall.<f>`; library asm vouches with
  `cc45.icallinfo`).
- **Returns**: `char` A; `int`/pointer A/X; `long` A/X/Y/Z (Q). Structs,
  unions, floats and `long long` are returned through a hidden first parameter
  `__sret` that points into the caller's storage; the callee copies the value
  there before returning, so returning a local struct by value is safe.
- **Frame pointer** `$FD/$FE`: set up only for functions that need a stack frame.
- **Mixing conventions**: ln45 checks every cross-object `JSR` against the
  callee's `.func_flags` record and generates a bridge thunk on a mismatch, in
  **either direction** (`-Wthunk` is accepted; `--no-thunks` makes a mismatch a
  "calling convention mismatch" error). Indirect calls are not bridged.
- **ZP map**: `$06` static chain, `$08`–`$0F` scratch (`__zp_scratch*`),
  `$10`–`$2F` parameters, `$30`–`$7F` register pool, `$80`–`$F1` functions'
  base-page blocks (the most used variables of SAC functions that make calls,
  up to 16 bytes (the library 8), `-fzp-locals-max=N`; ca45 `.zar`, placed by ln45 like the SAC
  overlay; `-fno-zp-locals`; a two-pass build chooses them program-wide:
  `ln45 --zp-plan-out=FILE`, then `cc45 -fzp-plan=FILE`, #304; `__zp` globals take the top of that area; the
  area starts at the end of the pool the program uses: ca45 exports
  `cc45.bptop` per object, `.bptop`/`.bpindex` for hand-written asm),
  `$F2`–`$F5` 32-bit temp, `$F6`–`$FC` library temps, `$FD/$FE` frame pointer.

### Relocated Base Page (the program's private zero page)

- crt0 moves the base page register (`TAB`) to `__zp_page`, a page-aligned
  256-byte block ln45 places at the end of BSS, so the program never touches
  the ROM's page 0. `__exit` sets B back to 0.
- **ROM calls** expect B=0: call sites bracket them with `jsr __bp_rom` /
  `jsr __bp_prog` (crt0). Used by `putchar`, `getchar`, `puts`
  and the float runtime (`float_rom.s45`). The KERNAL's **disk** code also
  needs the page-1 stack: `<cbm.h>` wrappers (and so `fopen`/`cbm_*` file
  I/O) call through `__rom_call` (`stdlib_asm/rom_call.s45`, linked only when
  used), which switches base page and stack.
- **Inline asm can't name a parameter portably**: `@_p_x` is a ZP slot in the
  ZP convention, `lda.sp @_p_x` only works for stack frames. `__jsr(addr, .A = v, .X -> x)`
  calls an address with registers (the values through `stdlib_asm/cbm_args.s45`'s bytes;
  a ROM address via `__rom_calli`); `<cbm.h>`'s wrappers are `__jsr` calls.
- **Interrupts**: the ROM IRQ handler saves B and sets it to 0. A C
  `__interrupt` handler saves B and switches to `__zp_page_isr` (allocated only
  when the program has an `__interrupt` function) and increments
  `__isr_depth`, so `__bp_prog` returns to the handler's page after ROM calls.
- **Pitfalls** (ca45 warns "has no base-page mode"): instructions without a
  base-page form ignore B — `ldz $xx` (use `ldq`, or `lda`+`taz`),
  `lda/sta $xx,y`, `jmp ($xx)`, `phw $00xx`. `.extern` ZP symbols assemble as
  absolute, so library `.s45` files define ZP temps as constants.
- `#pragma cc45 no_zp_save` / `set_bp <decimal>` affect only `-S` (standalone) output.

### CPU Stack

16-bit CPU stack by default: crt0 does `CLE` and sets SP to `__stack_top`
(`$BFFF`; bank 0 `$C000`–`$CFFF` reads as ROM in C65 mode). `-fno-stack16` /
`#pragma cc45 no_stack16` keep the 8-bit page-1 stack (the main module then
defines `__stack_top = 0`); `-fstack-top=ADDR` / `#pragma cc45 stack_top(ADDR)`
move the top. Hand-written asm must read the stack through a pointer
(`tsx; tsy; stx $08; sty $09; lda ($08),y`), never `$0100,x`.

### Object Format

`.o45` relocatable objects (`doc/architecture/lib45.md`): segments, exports
(weak bit `$80`), imports, relocations (R_LOW/R_HIGH/R_ADDR16/R_ADDR24…),
function attribute records (`$F9`: ZP-convention, leaf, static-alloc, ISR,
zero-alloc flags; register/flag clobbers; base-page use masks as address ranges, #360; parameter and frame
sizes), line info, debug types and variables (`OPT_DEBUG_TYPES`/`OPT_DEBUG_VARS`, for a debugger) and alignment markers (`cc45.align.data/bss`). A
`.func_flags` record attaches only to the global symbol named by
`proc <name>`; ca45 warns when `.func_flags` is in an unnamed `proc`.

### Key Files and Directories

```
src/main/            all tool sources (cc45_main, Parser, IRBuilder, IRCodeGen,
                     AssemblerParser, O45Emitter, O45Linker, ln45_main, disk45, basic45…)
include/             C++ headers (IR.hpp, IRBuilder.hpp, AST.hpp, Version.hpp…)
lib/include/         C headers (stdio.h, stdlib.h, stdint.h, intwide.h, mega65.h…)
lib/stdlib/*.c       C part of the standard library (built into c45.lib/c45_stack.lib)
lib/stdlib_asm/*.s45 hand-written asm part of the library (malloc, string…)
lib/crt0*.s45        startup code (crt0_zp / crt0 = ZP / stack), crt_bssinit.s45 (never linked)
lib/src/             not built by lib/Makefile (except float_rom.s45); many of
                     its sources do not compile
lib/build/           c45.lib (ZP), c45_zp.lib (copy), c45_stack.lib, crt0*.o45
src/test-resources/  test programs (*_xemu.c runtime tests, gte/ GCC torture tests)
src/test/            test drivers (test_xemu.sh, test_cc_interop.sh, run_gte_all.sh,
                     known_failures/, known_ignore/, check_docs.py)
doc/bin/             one reference page per tool
doc/architecture/    reference docs; doc/archive and doc/architecture/bug are history
doc/examples/        the examples with screenshots (examples/capture_screens.py)
```

## Build System

- `make` / `make all` — build all tools (bin/)
- `make lib` — build the standard libraries (lib/build/)
- `make test` — compiler/assembler unit tests, validation units and examples: the steps of
  `src/test/test_steps.txt` in parallel (`run_parallel.sh`, `TEST_JOBS`), then the examples
  (parallel too, `EXAMPLE_JOBS`); about 6 minutes
- `make test-xemu` — the runtime tests in xemu (`test_xemu.sh`, parallel runs: `XEMU_JOBS`)
- `make test-validation` — `bin/test_validation`, the C++ validation tests in one program (#96): a
  category per `src/test/test_validation_<x>.cpp` (and `test_segment_emission`, `test_move_fill`),
  their checks from `src/test/test_harness.hpp`; `bin/test_validation parser linker` runs some,
  `-l` lists them, `TV_VERBOSE=1` shows the PASS lines; `make test-validation-<x>` runs one
- `make clean`, `make install` (PREFIX=/usr/local: `bin/`, `lib/cc45/`, `include/cc45/`), `make install_local`, `make man`
- `make coverage`, `make test-o45`, `make docker` (Alpine `src/Docker/Dockerfile`: `make install PREFIX=/app`, entrypoint cc45; `src/Docker/docker-aliases.sh`), `make docker-runtime` (`src/Docker/Dockerfile.runtime`: the toolchain and xemu, `m65run`; #99); the root `Dockerfile` is Ubuntu, with the source tree for `make test` and xemu for `make test-xemu`. The images' xemu is CTalkobt/xemu `cc45-dev` (pinned); the MEGA65 ROM is mounted at `/rom/MEGA65.ROM` (`doc/architecture/DOCKER.md`)
- `bench-optimizer` (a size benchmark) still uses mmemu — **runtime tests are run on xemu-xmega65, never mmemu**.

The tools find headers and libraries relative to their own location, in the
build tree and in an installed one: headers `CC45_INCLUDE`, then
`bin/../lib/include` or `bin/../include/cc45`; libraries (cc45, ln45 `-l`)
`bin/../lib/build` or `bin/../lib/cc45` (ln45 after `CC45_LIB` and `-L`).
`src/test/test_install.sh` (`make test`) installs into `build/tmp` and builds with it.

## Testing

Every test script and the library build pass an explicit `-O2` (cc45's
default level), so a personal `~/.config/m65/cc45.conf` (e.g. `-O1`) does
not change what is tested; a test's own later `-O` still wins. Use an
explicit `-O<n>` in ad-hoc test commands too.

- `make test` — unit tests (no emulator).
- `bash src/test/test_xemu.sh` (`make test-xemu`) — runtime tests on xemu (766 pass, about 1.5 minutes): each
  `*_xemu.c` writes results to `$C0C0` (01 = pass, `$AA` sentinel), built in
  both conventions. New runtime tests follow this pattern and are registered
  in `test_xemu.sh`, most as one line: `xemu_case "FAIL message" <compile_link_test
  args> -- <run_xemu_test args>` (compiled, linked and run as a background job).
  Runs go in the background (`XEMU_JOBS`) on copies of the program and its `-8`
  disk, each with an xemu preferences directory of its own (`src/test/xemu_pref.sh`:
  instances starting together race on rewriting its template file), one
  disk-using run at a time (`flock`: xemu instances using disks at once
  interfere); `XEMU_SYNC=1 run_xemu_test` for a test that reads what the program
  wrote to its disk afterwards. A new test script step (`test_steps.txt`) must
  not glob files other steps may write.
- GCC torture tests: `src/test/run_gte_all.sh` — 551 pass with the ZP
  convention, 551 with `-fno-zpcall` (GTE uses `-fsigned-default`), none
  failing. Failures that could be fixed go in `src/test/known_failures/gte.txt`
  (empty); tests not reasonable on the platform (a 32-bit `int`, too large,
  stack frames over 64 KB, `mmap`) or of features cc45 will not implement
  (`long long` bitfields, `-finstrument-functions`) are skipped with their
  reason from `src/test/known_ignore/gte.txt`.
- `bash src/test/test_cc_interop.sh` — calling-convention interop (13 checks).
- **xemu runs**: `timeout 15 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem out.mem -prg file.prg`.
  xemu exits at BASIC's `READY.`; the exit status is at `$FD/$FE` in the dump
  (0 = pass; `$FFFE` after `abort()`; `$FFFF` = `__exit` never reached).
  xemu's RAM is zero at boot: `test_bss_fill_xemu.c` uses `-fbss-fill=$FF`
  to show the BSS fill runs.

## Configuration

Only these read configuration (`doc/bin/CONFIGURATION.md`):
- cc45/cp45: `~/.config/m65/cc45.conf` / `cp45.conf` (options before the command line; `#` comment lines)
- ca45: `~/.config/m65/ca45.conf`, `./.ca45rc`, `CA45_OPTIMIZE`, `CA45_WARNINGS`
- cvt_asm: `.cvtasmrc`

ln45, ar45, nm45, objdump45, disk45 and tune45 have no config files. Environment:
`CC45_INCLUDE`, `CC45_LIB` (ln45 library search), `CC45_DUMP_IR=1` (IR to stderr),
`CC45_OPMARK=1` (IR operation markers in the assembly; `src/test/opsize.py`
totals code bytes by IR operation).

## Language (details: `doc/bin/cc45.md`)

- **Types**: `char`(1), `short`/`int`(2), `long`(4), `float`/`double`/`long double`
  (CBM 40-bit, 5 bytes, computed by the BASIC 65 ROM), `_Bool`, `__bit` (file-scope booleans packed 8 to a byte;
  `__zp __bit` in the base page: smb/rmb/bbs), `__fixed(I, F)` (fixed point, I + F = 16 or 32; `__fixed8_8`, `__fixed16_16`; #72), pointers (2), far pointers (`T __far *p`, 4 bytes, a 28-bit address read and
  written with `[bp],z`; cc45's scalar type `"__farp T"`, `IRBuilderFar.cpp`, IR `far_load`/`far_store`; #68), near
  pointers (`T __near *p`, 1 byte, a base-page offset, `$00,x`; `"__nearp T"`, IR `near_load`/`near_store`/`near_addr`),
  structs/unions/enums/bitfields, `_Complex`, GCC vectors, `long long` /
  `int64_t` / `uint64_t` = `struct __int64`, `__int128`, `__int(N)` (64 and 128 defined;
  `__int(8/16/32)` are the native char/int/long).
- **Signedness**: plain `char`/`short`/`int`/`long` are **unsigned** unless
  declared `signed` (deliberate). `-fsigned-default` / `#pragma cc45 signed_default`
  give standard C. `<stdint.h>`'s signed types are declared `signed`.
- **Structs are packed** (`_Alignof(int) == 1`); `_Alignas` / `aligned(N)` work
  for globals, statics, SAC locals, members and struct types (not stack locals).
- **Wide integers** (`<intwide.h>`, auto-included when a unit's code names a
  64-bit or `__int(N)` type): operator-overloaded `struct __intN { char bytes[N]; }`
  with the `__intN_*` runtime; implicit conversion from/to integers, signed
  comparisons (`__intN_cmp_s`), `>>`/`/`/`%` signed too, `LL` literals keep 64
  bits. Struct methods are emitted weak so several units can include the same
  struct, and only those the unit calls.
- **Extensions**: struct methods, single inheritance, `virtual`/`final`,
  operator overloading, GCC nested functions (static chain, trampolines),
  statement expressions, computed goto, case ranges, `continue <value>` in a switch, `break N`, `repeat(N)`, `__likely`/`__unlikely`, `__noescape`, `__pure`, `__cpu`/`__flags`, `__bit`/`__bits`, `__jsr`/`__jsr_raw`/`__jmp`, `__section` (`#pragma section("NAME", addr)`: variables and functions placed by ln45, `--section`, cc45 `-Wl,`),
  `__dma_copy`/`__dma_fill`, `__interrupt`, `__naked`, `__regparm`, `__zp` globals
  (in the private base page, placed by ln45 at the top of `$80`–`$F1`, with ca45 `.segment "zp"` variables below them), file-scope
  `asm("...")` (module-level assembly), string encodings
  (`@"…"`, `p"…"`, `s"…"`, `a"…"`, `#pragma encoding`), GCC vector extension (`-fmax-unroll-bytes`, default 128).
- **Optimization** (`doc/architecture/optimizations.md`): `-O1`+ runs the twelve
  IR passes of `IROptimizer.cpp` (parameter narrowing, strength reduction, algebraic simplification,
  type narrowing, branch folding, CSE, LICM, loop strength reduction, copy propagation, address fusion,
  post-increment sinking, block reordering;
  `CodegenStage::optimizeIR`), then loop rotation in `generateAssembly` (#138:
  kept per function only when no larger, the file assembled both ways;
  `-frotate-loops` forces it, `-Os` turns it off); `-O2`+ adds automatic inlining
  (`-fmax-inline-ir-size`, default 11); `-O4`+ adds loop interchange (an AST
  pass, `LoopInterchange.cpp`, run by `OptimizeStage`) and loop unrolling
  (`optimizeLoopUnroll`, limited by `-fmax-unroll-bytes`). Every entry of `OptimizationCatalog`
  has `-f<name>`/`-fno-<name>`: IR passes (also per function with
  `#pragma cc45 optimize(...)`), IRBuilder switches (inline-small-functions,
  devirtualization), ca45 passes forwarded as `-P` (tail calls, branch
  inversion, jump optimization, redundant loads, dead stores, sequence
  extraction), always-on ones
  (`-fno-` warns); unimplemented names are ignored, `--future` warns.
  `-Roptir` traces the IR passes. The IR is not SSA (slot vregs are assigned
  by `store`, temps can have several definitions): passes must treat a vreg
  as constant/fixed only with exactly one definition. Validation:
  `test_iropt_xemu.c` (xemu) and `src/test/test_optimizations.sh` (`make test`).
  The `CC45_IPO`/`CC45_LTCO`/`CC45_BANK_ANALYSIS` variables do not exist.
- **Working pragmas**: `once`, `push_macro`/`pop_macro`, `weak`, `cc45 recurse`,
  `signed_default`/`unsigned_default`, `no_stack16`, `stack_top`, `stack_size`,
  `max_inline_ir_size`, `max_unroll_bytes`, `optimize(...)` (`optimize(off)` / `optimize(on)`: functions compiled as written, ca45 `.noopt_start`/`.noopt_end`), `encoding`, `overlay(name)`, `section("NAME", addr)`; `no_zp_save`/`set_bp`
  (only `-S`). Accepted without effect: `exit_rts`/`exit_halt`/`exit_brk`,
  `no_bssinit`, `heap`, `stdio`, `no_0100_stack`, `unroll`, `no_static_alloc`
  (use `recurse`), `no_striped`.
- **`__far` functions**: kept in far memory (see Code Overlays); `__far`
  on a pointer makes a far pointer; on another variable it is still reserved.
- **Reserved keywords** (parsed, no effect yet; kept for future expansion):
  `__striped` (layout unchanged), `__far` on non-pointer variables (plain 16-bit access).
  `__attribute__((weak))` is ignored (use `#pragma weak`).

## Program Startup and Exit (crt0)

1. Save the caller's SP and flags; enable MEGA65 I/O (`$47 $53` → `$D02F`).
2. Exit status at ROM `$FD/$FE` := `$FFFF`.
3. Base page → `__zp_page`; 16-bit stack at `__stack_top` (unless disabled);
   BSS filled with `__bss_fill` (DMA).
4. `jsr _init_features` (weak `rts`), then `main(argc, argv)`: crt0's weak
   `__mainargs` gives 0/NULL; when `main` takes parameters cc45 links
   `stdlib_asm/mainargs.s45`, which parses the BASIC line at `$0200`
   (`RUN ["NAME"] : [REM] a, b`; keywords need `:REM`).
5. `__exit` (from `main`'s return or `exit()`): B = 0, status → `$FD/$FE`
   (`$FFFE` after `abort()`), restore the caller's stack and flags, `rts`.
   A BASIC caller reads `PEEK(253)+256*PEEK(254)`.

`__basic_call` (`stdlib_asm/basic_call.s45` with crt0's start-up; linked
only when used) runs one C function from a BASIC `SYS` (basic45
`--link` thunks set `___bc_target`): same base page / stack setup, SYS A/X/Y
as the first three parameters (`$10`, `$12`, `$14`), result A/X back to
BASIC (`RREG`); BSS is filled on the first call only.

crt0 fills BSS (with the private base page) with `__bss_fill` by a DMA fill
job after the stack setup: 0, or cc45 `-fbss-fill=V` / ln45 `--bss-fill=V`
(`$FF` finds code relying on zeroed globals). `exit()` and a return from
`main` run the `atexit` handlers (`abort`/`_exit` do not). Heap: `__bss_end` to `__stack_size` bytes below the stack top
(default 4096, so `$B000`; cc45 `-fstack-size=N` / `#pragma cc45 stack_size(N)`,
ln45 `--stack-size=N`; `__heap_start`/`__heap_end` override), initialized on
the first `malloc`. ln45 warns when a program using `malloc` leaves no room
for it (a program ending above `$B000` needs a smaller stack share).

## Standard Library (details: `doc/architecture/stdlib.md`)

stdio (printf family; `%f`/`%e`/`%g` through the weak/strong float printf, digits from the
BASIC 65 ROM's FOUT, full float range), stdlib,
string, ctype, math (ROM-backed float functions; domain/range errors set
`errno` instead of reaching the ROM), time (frame clock, RTC calendar), errno, setjmp,
assert, alloca, conio, CBM file I/O (`cbm_load_far`: a file to any chip RAM bank), `palette.h` (all 256
entries), `seam.h` (two-byte cell screens: full-colour characters among text), stepped DMA
(`dma_copy_far_step`), a tune (song) player (`audio_tune.h`, assembly, callable
from an `__interrupt` handler; `tune45` makes its data), `intwide.h`, `mega65.h` register
overlays, `dma.h` (16-bit builtins; `dma_copy_far`/`dma_fill_far` at 28-bit
addresses), and on those far pointers (`far_ptr.h`/`far_ops.h`, an allocator
in banks 4-5 with `far_reserve`d ranges (its core assembly, `stdlib_asm/far_heap.s45`); attic RAM when the program enables
it, `far_attic_init`/`far_attic_enable`: a buddy allocator in `far_attic.c`,
power-of-two blocks aligned to their size, `far_malloc_long` over 64 KB) and
`dma_controller.h`, `rom_ram.h` (the unused C64 ROM space, `$2A000`-`$2BFFF` and `$2E000`-`$2FFFF`, as RAM: the hypervisor's `$D641` write protection lifted; `__exit` protects it again). `conio.h` has a full-colour mode (a background colour per character:
glyphs made from the font, cached in bank 4/5). `tgi.h` is cc65's TGI (320x200, 256 colours, one or two
pages of VIC-IV full-colour characters in bank 4/5; 640x200, 16 colours, nibble-colour characters;
`stdlib/tgi*.c`, `stdlib_asm/tgi_*.s45`). Many other headers (graphics,
sprites, hw_*, uart) declare functions that no library implements.

## Known Bugs (found in the 2026-10-11 doc audit; not yet fixed unless noted)

- **ca45**: fixed (2026-10-12): expressions/literals, data directives,
  `#include`, `.org` expressions, the simulated ops (`test_ca45_simops_xemu.s45`).
- **cc45**: none known (2026-10-12).
- **ln45**: none known (2026-10-12).
- **basic45**: fixed (2026-10-12): line links, numbering, labels, operator tokens,
  escapes, `--minify` (`test_basic45_xemu.bas`); see its Known Problems for
  the placeholder options.
- **disk45**: fixed (2026-10-12): REL files on D81/D65 (super side sector) and
  D71, a new entry in a directory sector's first slot cut the directory, D80
  remove; NIB/Zipcode are read-only (changes refused); G71
  side 2 and real (headered) NIB files decode; CMD images use the real
  D2M-DNP.TXT layout.

## Development Guidelines

- C++17, no external dependencies (except disk45's SQLite/FUSE).
- Language feature: lexer/parser (`Parser.cpp`, `AST.hpp`) → IR (`IRBuilder.cpp`,
  `IR.hpp`) → codegen (`IRCodeGen.cpp`, `M65Emitter`) → a runtime test
  `src/test-resources/test_<feature>_xemu.c` registered in `src/test/test_xemu.sh`
  (both conventions) → `doc/bin/cc45.md`.
- Constant expressions: `ConstEval` (`ConstEval.hpp`) is the one evaluator
  (the folder's literal arithmetic, static initializers, case labels, array
  sizes, enum values, bit-field widths, `_Static_assert`, `_Alignas`); a
  caller gives it a `ConstEvalContext` (sizes, alignments, enum constants).
  A folded literal's value is `ConstValue::ofLiteral(l).intValue()` (its type
  applied), not `l.value`. Unit test `src/test/test_const_eval.cpp`.
- AST passes: derive from `ASTWalker` (`AST.hpp`), which walks into every
  child node, and override only the nodes the pass acts on (call
  `ASTWalker::visit(node)` to keep walking below one); a new node type gets
  its children walked there (`src/test/test_ast_walker.cpp`).
- Stdlib function: `lib/stdlib/<f>.c` or `lib/stdlib_asm/<f>.s45`, declare in
  `lib/include/<header>.h`, rebuild with `make lib`, document in
  `doc/architecture/stdlib.md`.
- Diagnostics: every tool logs through `include/Logger.hpp` (`LOG_INFO("cc45", …)`,
  `Logger::stream(LogLevel::DEBUG)`; `-v` … `-vvvv` on stderr as `[tool:LEVEL] …`; tools
  with positional arguments `Logger::takeFlags`), never `std::cout`; the level must not
  change the output (`src/test/test_logging.sh`).
- Docs: `python3 src/test/check_docs.py` validates the doc tree layout.

## Debugging Tips

```bash
./bin/cc45 -S input.c -o input.s45       # standalone assembly (inspection)
./bin/cc45 -c input.c -o input.o45       # relocatable object
CC45_DUMP_IR=1 ./bin/cc45 -c input.c     # optimized IR on stderr
./bin/cc45 -Rcodegen -S input.c -o x.s45 # why each instruction ("; [CONST → v2(...)]")
./bin/ca45 -c input.s45 -o input.o45
./bin/nm45 -f input.o45                  # symbols + function attributes
./bin/nm45 -r input.o45                  # relocations
./bin/objdump45 -d input.o45             # disassemble
./bin/ln45 -basic -m -o p.prg a.o45 lib/build/c45.lib   # link with a map
```

## Disk Image Utility (disk45)

25 formats chosen by extension (`doc/bin/disk45.md`; layouts per
https://ist.uwaterloo.ca/~schepers/formats.html): read/write D64, D71,
D81 (recommended), D65, D80, D82, D90/D60 (VICE's layout), X64, G64/G71 (unchanged tracks keep their
GCR), T64, ARK, ARC/SDA, LNX, P00, transparent `.gz` and `.zip` (`x.d81.zip`,
`archive.zip:member.d64`), CMD D1M/D2M/D4M/DNP (`disk.d2m:N` opens partition N, native or emulated 1541/1571/1581; native `DIR` subdirectories; `create --partitions`);
read-only NIB/NBZ, Zipcode, TAP, CVT. CBM subdirectories (`DIR/NAME` paths; disk45's own layout, not 1581 partitions), FUSE mount (subdirectories too; GEOS files as `<name>.cvt`), SQLite catalog, batch
scripts, PETSCII/ASCII filters (`-a2p`, `-p2a`). GEOS files (VLIR and
sequential) on disk images: `extract` writes and `add` takes the Convert
(CVT) form, `copy`/`convert` keep them, `create --geos`.

`disk45 sd` handles MEGA65 SD card images (`SdCardImage`): `create` lays out
MBR + FAT32 + system partition ($41) as the MEGA65's FDISK does; files are
stored contiguously (the MEGA65 mounts only unfragmented D81s), with VFAT
long names. The hypervisor reads its configuration sector from **card sector
1** (not the system partition, despite `MEGA65_System_Partition.md`);
`--onboarded` skips the first-boot onboarding. xemu needs `-defd81fromsd` to
mount the card's `MEGA65.D81`. Any disk45 command opens a disk image on a card as
`card.img:/DIR/X.D81`; `disk45 catalog build` indexes the disk images in
zips and on SD card images as `<container>:<member>`.

## References

- MEGA65 hardware: https://github.com/MEGA65/mega65-core
- CBM file and disk formats (D64, D71, D81, D80/D82, REL, G64, T64...):
  https://ist.uwaterloo.ca/~schepers/formats.html — the layouts disk45 writes
- The MEGA65 Book (BASIC 65, DOS, PETSCII/escape codes; a local copy is in
  `../refs/mega65-book.txt`, outside the repository)
- `doc/architecture/calling-conventions.md`, `lib45.md`, `ir.md`, `optimizations.md`, `opcodes.md`, `stdlib.md`
- CHANGELOG.md, ROADMAP.md
