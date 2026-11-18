# M65 Compiler Suite Roadmap: v1.0 → v2.0

**Current Status:** v1.1.0 released; v1.2.0 in preparation (everything below it is on `main`)
**Last Updated:** 2026-11-18
**Maintainer:** Craig Taylor (CTalkobt)

> The milestones below (v1.1 to v1.4, v2.0) group features; they shipped
> in the 1.0.x releases as they were done. Releases: CHANGELOG.md.

---

## v1.2.0 Release (in preparation)

What has landed since v1.1.0 (the entries, with their issues, are CHANGELOG.md's `[Unreleased]` section).

**New tools**

- [x] **cl45** and **al45**, the C and assembly linters (`mega65-*` rules, `.cc45-lint.yml`, `NOLINT`, text / JSON / SARIF; `make lint-lib` keeps the library clean), and the language server running them on the text being edited (#64, #199)
- [x] **dbg45**, a source-level debugger in xemu (`cc45 -g` / `ln45 -g`, breakpoints, stepping, variables by type; `--dap` for VS Code) (#202)
- [x] **deploy45**: a program or disk image to a MEGA65 or an Ultimate 64 (#145)
- [x] **font45** and `<font.h>`: BDF, PSF, TrueType, images and C64 charsets to charsets and proportional fonts (#466)
- [x] **tune45** and the tune player (`<audio_tune.h>`): text, MIDI, MOD / XM / S3M / IT and C64 `.sid` to music data; filter, wave tables, digi voices; plays on the PC (#314, #393)
- [x] **pack45**, `<unpack.h>` and `ln45 --pack-overlays`: packed data unpacked by DMA; sound files to samples (#345)
- [x] **libca45**: ca45's assembler as a C++ library (#117); `cvt_asm` is now **asmCvt45**, with BSA and conversions checked against ca65, ACME and KickAssembler (#223, #224, #371)

**cc45: the language**

- [x] Far pointers (`T __far *`, 28-bit) and near pointers (`T __near *`, base page) (#68); `__bank(n)` variables and functions (#69)
- [x] Fixed-point types `__fixed(I, F)` (#72); the `__bit` type and `__bit` / `__bits` (#73, #204); `restrict` (#77)
- [x] `#pragma section` / `__section`, placed by ln45 (#70); `#embed` and `#include binary` (#91)
- [x] `__jsr` / `__jsr_raw` / `__jmp`, with `<cbm.h>` made of them; `#pragma cc45 optimize(off)`
- [x] The preprocessor expands macros as the C standard has it (#233); `#if` evaluates the whole expression grammar (#375); the comma operator everywhere (#572)
- [x] One constant-expression evaluator (#93), one AST walker (#94); the legacy AST-level code generator removed (#92)
- [x] `-fsigned-default`: `?:` operands typed as standard C has them (#612)

**cc45 / ca45: code**

- [x] Smaller and faster code throughout: loop rotation (#138), loop strength reduction, the program-wide base-page plan (#304), SAC locals sharing storage (#324), `memcpy` / `memset` by DMA (#325), known bits (#350), many peephole and data-flow changes
- [x] Floats: RAM routines with `-ffast-float=1` / `-Ospeed`; the ROM's routines called by the library's own map switch (#614); fewer copies (#485)
- [x] ca45: `.macro`, `.include`, `.assert`, `--cpu 6502` output (#384), the MEGA65 Book's quad instructions

**ln45, objdump45, nm45**

- [x] Code overlays in attic RAM (#315), up to 255 of them (#313), 6-byte stubs (#332), several objects per overlay (#328)
- [x] The call graph counts tail calls (#615); calls through prototyped pointers reach only matching functions
- [x] objdump45: `--html` / `--tree` / `--json` inspector (#201), `--source`, `--cycles`, `--vice`, `--cpu` 6502 / 6510 / 65CE02, ROM and I/O symbol files (#144)

**basic45**

- [x] `--compile`: BASIC 65 programs to machine code through cc45's code generator, the whole language but the statements on the program's own text (#369); `--speed`, `--bank=N` (#580), `--checks`, `--line-numbers`
- [x] `--convert`: BASIC 3.5, Simons' BASIC, BASIC 7.0, BASIC 8 programs as BASIC 65 (#368, #449)
- [x] `--map`, `--minify` of structured programs, `--data-loader`, numbered programs

**Library**

- [x] cc65's conio (in assembly, with a full-colour mode), TGI graphics (`<tgi.h>`, 320x200x256 and 640x200x16), `joystick.h`, `serial.h`, the `cbm_k_` functions
- [x] `sprites.h`, `palette.h`, `seam.h`, `rom_ram.h`, `load.h`, stepped DMA, containers (arrays, maps, sets, ring buffers, lists), the far heap in assembly

**disk45**

- [x] TAP images decoded as the KERNAL writes them (#142); `mount` takes new files named without a type suffix (#626) and says what is missing where FUSE is not there

**Build, packaging, tests**

- [x] `make` needs only a C++17 compiler and GNU make: zlib bundled (#620), SQLite and FUSE 3 optional, `make check-deps` (#619); builds with GCC 14-16, clang, musl and on macOS
- [x] Prebuilt archives for Linux x86_64 / arm64 (static) and macOS arm64, made by the release workflow (#621)
- [x] Docker images with xemu (#99)
- [x] Benchmarks against the other 6502 C compilers (`make bench`, #129)
- [x] Tests: 1092 xemu runtime tests, GCC torture 551 in each convention, the validation tests in one program (#96), every runtime test on xemu

**Before the release** (#622): the Docker images on a registry (#625), the version, the changelog heading, the final validation, the tag and its notes.

---

## v1.1.0 Release (2026-10-16)

- [x] Code overlays: `#pragma cc45 overlay(name)` / `-foverlay`, copied into a region or mapped into a window (`--overlay-map`), stubs, `__ovl_call` runtime, overlay file loaded into bank 5
- [x] `__far` functions: automatic mapped overlays packed by the call graph
- [x] Overlay safety: interrupt handlers may not reach overlay code (link error); `setjmp`/`longjmp` across overlay calls; `far_malloc` spares the overlay store
- [x] nm45 / objdump45 read `.ovl` files; the linker map shows banks, stubs, overlay symbols and lines
- [x] Standard library parts rewritten in assembly (conio, scanf engine, strtol/strtoul, strtok, qsort/bsearch, clock, rand)
- [x] Examples: overlays, mapped_overlays, far_functions; invaders title screen; `-O2` in every example Makefile
- [x] Tests: 295 xemu runtime tests, GCC torture 552 in each convention

---

## Queued

| Issue | Task | Category | Status |
|-------|------|----------|--------|
| #625 | The Docker images on a registry (`docker pull`) | release | in progress (for v1.2.0) |
| #622 | Release housekeeping for v1.2.0 | release | open |
| #611 | al45: style rules (`mega65-style-*`) and the `af45` formatter | linter | open |
| #577 | basic45: mixed programs (some procedures compiled, the rest interpreted) | basic45 | open |
| #248 | Pluggable allocation with interprocedural liveness analysis | codegen | open |
| #143 | The C64IDE AI integration on Linux | IDE | open |
| #65 | Graph-colouring allocation for locals | codegen | open (v2.0) |
| #59 | Inter-TU optimization (Phase 4) | optimizer | open (v2.0) |

Done since v1.1.0 and formerly listed here: `__zp` globals (#67), `break N` (#71), `__likely` / `__unlikely` (#75), `__pure` / `__const` (#76).

---

## Language Extension Candidates

Extensions that would make C convert to shorter or faster 45GS02 code. Not assigned to a release; in the order they would pay.

| Extension | What it gives | Notes |
|-----------|---------------|-------|
| `__striped` arrays | An array of structs or 16-bit values stored as one byte array per byte of the element: `a[i].hp` is `ldx i` / `lda a_hp,x`, with no multiply and no pointer | The keyword is reserved (no effect yet); design in `doc/architecture/striped-arrays.md`. An element has no plain address (`&a[i]`, `memcpy` over the array); arrays over 256 elements need a second scheme |
| `__assume(expr)` and value ranges | A fact the compiler cannot derive (`__assume(i < 40)`): an index kept in X or Y, no high-byte arithmetic, no carry propagation | Feeds the known-bits tracking (#350) and type narrowing; makes `__striped` more useful |
| Compile-time generated tables | A `static const` array written as a formula of its index, evaluated by the compiler: the lookup tables that replace multiply, divide, sine and row addresses | A loop around `ConstEval` in an initializer |
| Results in flags and registers | A function that returns a status in the carry flag, or two small values in A and X, as the ROM and hand-written assembly do | Touches the calling convention, the function records of `.o45` and ln45's convention thunks |
| A BCD type | Decimal-mode arithmetic for scores and counters: no division by ten to print | Small and self-contained |

Less conventional ones, which fit what cc45 already has (static frames, ln45's call graph, the measured cycle table). None has been checked against the source for parts that exist.

| Extension | What it gives | Notes |
|-----------|---------------|-------|
| Coroutines on static frames (`yield`) | A function that suspends and resumes: game entities, music drivers and scripted sequences as straight-line code instead of hand-written state machines | A non-recursive function's locals are already in fixed storage (SAC): suspending costs two bytes for the resume address |
| Cycle budgets (`__budget(cycles)`) | The build fails when a function can take more than a stated number of cycles: interrupt handlers, per-frame routines | ln45's call graph and `include/CpuCycles.hpp`; loops need bounds (`__assume`) |
| Cycle-exact blocks (`__cycles(n)`) | A block padded to an exact length, or an error when it cannot be: raster effects without counting by hand | The same cycle table |
| Compile-time functions | A C function evaluated by the compiler when its arguments are constant | Generalises the generated tables; `ConstEval` |
| Variables in instruction operands | A variable kept in the immediate operand of the instruction that reads it (`lda #imm`, no storage of its own): the hand-coder's trick for inner loops | Safe for non-recursive functions, whose every read and write the compiler knows; not for code run from ROM |
| Atomic blocks (`__atomic { ... }`) | Interrupts off around a block and the state before restored, nesting correctly | Replaces inline `sei` / `cli` |
| Encoding-typed characters | PETSCII, screen codes and ASCII as distinct character types: a screen-code string passed to a PETSCII routine is a compile error | The string prefixes exist (`p"…"`, `s"…"`, `a"…"`); the encoding is lost once the literal has its type |

---

## v1.0 Release (Target: 2026-05-31)

**Status:** All blockers and should-fix items complete. In final QA phase (2026-05-30).

### Readiness Checklist

- [x] All v1.0 blockers resolved
- [x] All v1.0 should-fix items completed
- [x] IR pipeline as default codegen (legacy codegen deprecated and removed in v0.99.dev)
- [x] Test suite: 176 assembler validation tests, 55 segment emission tests, 131 objdump tests, mmemu integration tests
- [x] Code coverage: 80.1% line, 77.1% function, 47.4% branch (measured with `make coverage`)
- [x] All stdlib headers complete (stdio.h, stdlib.h, string.h, ctype.h, math.h, time.h, dma.h, mega65.h, cbm.h, etc.)
- [x] cbm.h KERNAL wrappers validated with IR pipeline (Issue #46)
- [x] mega65.h hardware register overlays complete (VIC-IV, SID, CIA, DMA, math accel, audio mixer, FDC, SD, Ethernet, Hypervisor)
- [x] Calling convention enforcement implemented (FUNC_FLAG_ZP_CONV bit, linker safety checks, thunk generation)
- [x] _Alignas support Phase 1 complete (alignment computation, .align directives)
- [ ] Final QA on real MEGA65 hardware (the runtime tests run on xemu)
- [x] Release notes finalized (CHANGELOG.md)
- [x] Man pages generated (`make man`, requires pandoc)

### Recent fixes (rc3)

- Fixed `cbm.h` inline asm parameter scoping for IR pipeline (`@_p_` convention)
- Fixed `select` simulated op segfault on immediate operands
- Added `make coverage` target for gcov/gcovr code coverage reporting

---

## v1.0.1 Patch (Target: 2026-06-30)

| Task | Category | Status |
|------|----------|--------|
| Regression test validation (diverse 45GS02 patterns) | test | todo |
| Performance microbenchmarks | test | **done** (`make bench`, #129) |
| Documentation cleanup (remove stale .tmp/, consolidate todo.md) | doc | todo |

---

## v1.1 Minor Release (Target: 2026-07-31)

**Completed (shipped in v1.0-rc3):**

- [x] `inline` keyword expansion (Phase 1, ≤20 statement functions)
- [x] `-finline-functions` flag for auto-inlining
- [x] CPU/flag intrinsics (`__cpu.A`, `__cpu.AX`, `__cpu.Q`, `__flags.Carry`, `__flags.Zero`, etc.)
- [x] `math.h` integer ops (`min`, `max`, `gcd`, `lcm`) — all implemented and linked selectively
- [x] `time.h` minimal (clock, time, difftime, CLOCKS_PER_SEC jiffy clock at 60Hz)
- [x] `repeat()` compile-time loop unrolling
- [x] DMA intrinsics (`__dma_copy`, `__dma_fill`)
- [x] mega65.h hardware register overlays (complete VIC-IV, SID x4, CIA x2, DMA, math accel, audio mixer, FDC, SD, Ethernet, Hypervisor, keyboard scanner)
- [x] cbm.h KERNAL wrappers (full ROM interface)
- [x] Flexible Array Members (C99)
- [x] `objdump45` symbolic disassembly with .map file integration

**Remaining for v1.1:**

| Task | Category | Status |
|------|----------|--------|
| Parameter narrowing advisory (note: int param could be char) | optimizer | **done** (v1.0-rc3) |
| CONST vreg suppression (unused const vregs not emitted) | optimizer | **done** (v1.0-rc3) |
| Assembler inter-instruction MachineState tracking | optimizer | **done** (v1.0-rc3) |

---

## v1.2 Register Optimization (Target: 2026-08-31)

| Task | Category | Status |
|------|----------|--------|
| Phase 1: Per-function clobber tracking in IR pipeline | codegen | **done** |
| Phase 2: Selective register invalidation at call sites | optimizer | **done** |
| Phase 3: Emit `.func_flags leaf` directives | codegen | **done** |
| Assembler optimizer case fix + correct instruction semantics | asm | **done** |
| Assembler-level register tracking | asm | **done** |

> Note: All v1.2 items completed ahead of schedule. Phase 1 (IR clobber tracking) and Phase 2 (selective JSR invalidation) were implemented together. The assembler optimizer was rewritten with correct per-instruction register semantics, fixing a pre-existing case-mismatch bug that made the entire register tracker dead code.

---

## v1.3 Calling Convention Safety (Target: 2026-09-30)

| Task | Category | Status |
|------|----------|--------|
| Per-function convention attributes in `.o45` | linker, codegen | **done** |
| Convention mismatch detection | linker | **done** |
| Automatic thunk generation (stack↔ZP) | linker | **done** |
| `--no-thunks` flag and `-Wthunk` warning | linker | **done** |
| Separate ZP-convention stdlib (`c45_zp.lib`) | stdlib | **done** |

> Note: Core thunk infrastructure (s2z, z2s) was implemented ahead of schedule as part of the linker test coverage work. The linker now detects mismatches, generates bridge thunks in THUNK_AUTO mode, and supports THUNK_WARN and THUNK_ERROR modes.

---

## v1.4 Dynamic Linking & Banking (Target: 2026-11-30)

| Task | Category | Status |
|------|----------|--------|
| Banked segment support | linker | **done** (as `__far` functions: mapped overlays in bank 5 behind resident stubs; the Phase 99 code is not connected) |
| Position-Independent Code (PIC) | codegen, linker | **n/a** (45GS02 lacks PC-relative data modes; `.o45` relocation covers practical needs) |
| `.import binary` / `.incbin` assembler directive | asm | **done** |
| Code overlays (copied, mapped) and `__far` functions | linker, codegen | **done** (v1.1.0) |

---

## v2.0 (Target: 2027-01-31)

| Task | Category | Status |
|------|----------|--------|
| `long long` / `unsigned long long` (64-bit) | parser, codegen | **done** (maps to `struct __int64` + `__intN_*` runtime) |
| Variable-Length Arrays (VLA) | parser, codegen | **done** (stack allocation through `alloca`) |
| Inter-TU optimization (Phase 4) | optimizer | **open** (#59: the Phase 91/96.5/99/100 code is in `lib45-phases.a`, linked by no tool; live across objects: ln45's convention thunks, SAC checks and storage overlaying) |
| Assembler peephole optimizer (Phase 5) | asm | **done** (AssemblerOptimizer + MachineState tracking) |
| `mega65.h` platform library | stdlib | **done** (VIC-IV, SID, CIA, DMA, math accel, audio, FDC, SD, Ethernet, Hypervisor, keyboard) |
| File I/O wrapper (`disk.h`) | stdlib | **done** (FILE I/O + CBM file handle I/O in `stdio.h`) |
| Comprehensive validation suite (50+ test programs) | test | **done** (320 test programs) |

---

## Post-v2.0 (Deferred)

- ~~Floating-point support~~ — **done** (CBM 40-bit via BASIC 65 ROM; 27 math functions)
- Lambda expressions
- Link-Time Code Generation (LTO) — the Phase 100 LTCO code exists but is not connected (`doc/architecture/cross-module-optimization.md`)
- Static analysis & sanitizers — static analysis **done** as the linters cl45 and al45 (v1.2.0); sanitizers open

### Optimizer — Remaining Items

| Task | Category | Status |
|------|----------|--------|
| Automatic parameter narrowing (generate I8 code path for `int` params when all call sites pass 0-255) | IR optimizer | **done** (v1.1: when every use is free zero-extended) |
| Binary-mode dead store elimination (remove dead `sta $ZP` after RELOC_CONST forwarding) | asm optimizer | **done** (v1.1, in cc45: stores to vreg pool registers the function never reads are not emitted) |
| Array indexing pseudo-op (`addr_elem.16 dest, base, index, stride` — single expansion replacing mul.16 + add.16) | asm codegen | **done** (an element address is built straight into its destination, no `add.16`; only the stride multiply remains) |

---

## Completed v1.0 Items

| ID | Task | Completed |
|----|------|-----------|
| V1.0-001 | `getchar()` implementation | 2026-05-05 |
| V1.0-002 | `sprintf` %ld ZP conflict fix | 2026-05-02 |
| V1.0-003 | ConstantFolder branch elimination | 2026-05-02 |
| V1.0-004 | Designated Initializers | 2026-05-02 |
| V1.0-005 | Type qualifier ordering | 2026-05-05 |
| V1.0-006 | Local `_Alignas` | 2026-05-01 |
| V1.0-007 | Address-of register vars | 2026-04-30 |
| V1.0-008 | Integer literal overflow warnings | 2026-04-30 |
| V1.0-009 | `ctype.h` completion | 2026-05-05 |
| V1.0-010 | `string.h` completion | 2026-05-05 |
| V1.0-011 | `stdlib.h` extensions | 2026-05-05 |
| V1.0-012 | `#pragma once` support | 2026-05-05 |
| V1.0-013 | `_Pragma()` operator | 2026-04-30 |
| V0.99-A | Core compilation pipeline + IR backend | 2026-05-06 |
| V0.99-B | Register clobber tracking (Phase 1) | 2026-05-06 |
| V0.99-C | Basic standard library | 2026-05-06 |
