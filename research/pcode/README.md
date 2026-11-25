# P-code for cc45: research notes (2026-11-24)

Can cc45 save space by compiling functions to a compact p-code that an interpreter runs, instead of 45GS02 code,
and what does it cost? This directory has the measurements and the prototypes: an interpreter, a translator
that turns p-code into native code the first time it runs (and keeps it), and both reading their p-code from far
memory, and a p-code back end in cc45 that runs a whole example (musicgen) as p-code. Nothing here is part of the
toolchain: cc45's changes act only with `CC45_PCODE_STATS` (it measures) or `CC45_PCODE` (the back end) set.

## Summary

- **Size.** The IR encoded as a byte-oriented p-code is 28% smaller than cc45's native code for the library and
  17% for the examples (scheme A); with combined instructions (scheme B) **40% and 30%**. Choosing per function
  (p-code only where smaller, functions with inline asm native): 39% and 31%. Functions under 32 native bytes grow
  (a fixed 6-byte entry), functions of 128 bytes and more shrink by 34-47%.
- **Speed, interpreted.** A loop (`sum`) runs **4.8x slower** than native with the second interpreter (8.7x with
  the first), 6.0x when the p-code is in far memory.
- **Speed, translated.** Translated once into a code cache (templates, variables in the base page, chained
  branches, one peephole), the loop runs **1.07x native** on later runs and 1.29x including the translation on the
  first, wherever the p-code is: bank 0, bank 4 or attic RAM.
- **Far memory.** P-code is data, so it can live anywhere in the 28-bit space (`lda [ip],z`), attic RAM included,
  with no overlay stubs, copies, maps or grouping. Bank 0 keeps the interpreter, the translator, the cache, the
  code that must stay native, and the data. This may be the larger gain than the bytes saved.
- **Cost.** An interpreter of the whole instruction set about 3-3.5 KB, a translator 2-3 KB more, the cache a fixed
  few KB. Against the per-function saving the interpreter pays for itself in a program with about 9-10 KB of C
  code, interpreter and translator at about 15-20 KB.
- **A real program.** cc45's first p-code back end (`CC45_PCODE`) runs examples/musicgen with 47 of its 49
  functions as p-code: it draws its screen and answers keys as the native build does. Its `.prg` is **20,141 bytes
  against 17,034 native (+18%)**: the back end is the plain encoding (no scheme B forms) and the runtime is 2.8 KB.
  An operation-by-operation test against native code found only native bugs (#794, #795); the work found a ca45
  bug too (#796).

## How big would p-code be?

`CC45_PCODE_STATS=<file> cc45 -c ...` (`CodegenStage::writePcodeStats`) appends, for each function, its native
size (the assembled text, from the size trial) and the bytes of its final IR in two encodings:

- **Scheme A** mirrors the IR: an opcode byte (operation and type), a byte per virtual register (2 if a function
  has more than 256), an immediate in 1 byte when it fits, 2-byte globals and branch targets (none to the next
  block), call argument counts, switch tables; 6 bytes a function (a 3-byte `jsr` into the interpreter, a 3-byte
  header).
- **Scheme B** is A with the combined instructions, where a temporary is used only by the next instruction:
  `const` + `store` -> `SET`; an operation + `store` -> in place (`ADD2`), `INC` / `DEC`, or into the variable;
  compare + `br_cond` -> one compare-and-branch; `addr_elem` + `load` / `store` -> an element access (`LDE`);
  pointer + constant + `load` / `store` -> an access with an offset; calls of 0-3 arguments with the count in the
  opcode; an `ADV` every 240 bytes.

`tools/collect.sh` runs it over `lib/stdlib/*.c` (with `lib/Makefile`'s flags) and `examples/c/*/*.c` (six of
them need generated headers and are left out) and prints:

| | native | scheme A | scheme B | B per function (asm native) |
|---|---|---|---|---|
| library (892 functions) | 134,733 | 97,034 (-28%) | 81,352 (-40%) | 82,351 (-39%, 639 as p-code) |
| examples (417 functions) | 119,186 | 99,493 (-17%) | 83,738 (-30%) | 81,820 (-31%, 327 as p-code) |

| native function size | library A | library B | examples A | examples B |
|---|---|---|---|---|
| 0-32 bytes | 130% | 122% | 183% | 159% |
| 32-128 | 76% | 65% | 92% | 77% |
| 128-512 | 72% | 60% | 79% | 66% |
| 512+ | 65% | 53% | 84% | 71% |

The instruction set used: 48,155 instructions, 125 distinct opcodes (operation and type; about 130 with the float
operations the IR printer names `unknown`), 59 operations. 42 opcodes cover 90% of the instructions, 85 cover 99%:
a byte opcode has room for the combined forms, the rare ones can take an escape. The most frequent: `br_cond` 9.2%,
`store` i8 9.1%, `load` i8 7.1%, `const` i8 6.3%, `const` i16 4.6%, `call_void` 4.4%, `br` 4.2%. Scheme B has 18%
fewer instructions than A (39,585). 92 functions have inline assembly (native, or a native escape). Basic blocks
are short: 3.8 IR instructions (about 15 p-code bytes) in the library, 4.2 in the examples.

## The interpreter (`pcode_vm.s45`, `pcode_vm_v1.s45`)

- **Registers**: IP (the p-code, read as `(IP),Z`) and FP (the frame, `(FP),Y`) in the base page; Z indexes the
  stream and Y the frame, so neither is saved and restored. Operands are frame byte offsets.
- **Dispatch**: `lda (IP),z; inz; asl; tax; jmp (handlers,x)`. Bit 7 of the opcode, the width (byte / word), is
  shifted into the carry, which `tax` and `jmp` leave alone: one handler serves both widths (a `bcs` to its word
  half) and the table has 128 entries. Field-structured opcodes help in this way (handlers shared), not by decoding
  bits at run time, which costs more than the one table jump.
- **Version 1**: `LD`, `ST`, `ADD`, `BGEU`, `SET`, `BR`, `RET`; a shared dispatch loop with a check of Z (IP moved
  up at `$E0`). 324 bytes of code + the 256-byte table.
- **Version 2**: adds `ADD2` (in place), `INC`, `LDE` (an element), `BLTU`, `ADV`; the dispatch copied into every
  handler and Z never checked (the compiler puts an `ADV` in straight-line code before Z could wrap). 659 bytes of
  code + the table for 12 operations; a full set about 3-3.5 KB (rare handlers could jump to a shared dispatch).

Cycles of each instruction, its dispatch included (`tools/cycles.py` walks `objdump45 --cycles` along each
branch outcome):

| instruction | v1 | v2 |
|---|---|---|
| `ADD.w d,a,b` | 131 | 122 |
| `ADD2.w d,a` | - | 99 |
| `INC.w` (no carry / carry) | - | 46 / 59 |
| `LD.w` | 108 | 99 |
| `LDE.w b[i]` (v1: 2x `ADD` + `LD`, 352) | - | 172 |
| `BGEU.w` not taken / taken | 102 / 122 | 93 / 117 |
| `BR` | 46 | 41 |
| `SET.w` | 68 | 59 |

An operand costs about 8 cycles to fetch and 7 to reach in the frame; a three-operand `ADD.w` spends about 45
cycles on its operands. Fewer, larger instructions save both bytes and time.

## The translator (`pcode_jit.s45`, `pcode_jit_abs.s45`)

The first time a block (up to its branch or `RET`) runs, each instruction is copied from a native template into a
1 KB cache with its operands filled in; a branch leaves through a 5-byte stub (`jsr jit_miss` + the target's
p-code address) that `jit_miss` makes a `jmp` to the target's block once it is translated (chaining): a loop that
runs again runs from the cache alone. Blocks are found in a 32-entry table; a full cache is emptied.

- **The frame at a fixed address**: the cache's code is right only for the frame it was translated with: a
  function's static block (SAC), which is the same at every call.
- **`pcode_jit_abs.s45`**: variables at absolute addresses (FP + offset).
- **`pcode_jit.s45`**: the frame in the base page: while the cached code runs, B is the frame's page, a variable is
  the base-page byte FP.lo + its offset (the frame within one page; offsets 14-15 the templates' element pointer),
  `INC.w` is one `inw`; `jit_miss` and `RET` put the program's B back. And one peephole: `LDE d` followed by
  `ADD2 x, d` adds the element's bytes to x as they are loaded (d still stored: it may be read later).
- Size: 976 bytes of code for 8 operations (853 without the peephole), 1,161 bytes of RAM (the cache, the table).
  `sum`'s 32 bytes of p-code take 106 bytes in the cache (4.3x the p-code): the cache holds the hot code only.

`sum`'s loop as translated (base page, peephole), 98 cycles:

```asm
loop: lda i / asl / tax / lda i+1 / rol / tay / txa / clc    ; &p[i] -> EP
      adc p / sta EP / tya / adc p+1 / sta EP+1
      ldy #0 / lda (EP),y / sta v / clc / adc s / sta s      ; v = p[i]; s += v
      iny    / lda (EP),y / sta v+1 / adc s+1 / sta s+1
      inw i
      lda i / cmp n / lda i+1 / sbc n+1 / bcs out
      jmp loop                                               ; the stub, chained
```

The rest of the gap: the stores of `v`, dead here (about 6 cycles); the compiler knows `v` is used once and could
say so (an `ADDE x, b, i, s` instruction, or frame offsets kept for such temporaries).

## Far memory (`pcode_vmf.s45`, `pcode_jitf.s45`)

The same interpreter and translator with IP 32 bits (`[IP],Z`, an `EOM` prefix: 13 cycles a fetch, not 7); a
branch target is the 16-bit address in IP's bank; a translated block is known by its bank too. `test_far_xemu.c`
copies `sum` to the same 16-bit address in bank 4 and in attic RAM, wipes the bank-0 copy (a far run reads far),
and runs it from each.

## `sum` measured (`test_jit_xemu.c`, `test_far_xemu.c`)

`unsigned sum(const unsigned *p, unsigned n)` over 200 elements, timed with CIA 2's timer B (1 tick = 40 cycles at
40 MHz, interrupts off):

| | ticks | cycles / iteration | vs native |
|---|---|---|---|
| native (cc45 -O2) | 440 | 88 | 1.00x |
| interpreter v1 (static count) | - | 780 | 8.7x |
| interpreter v2 | 2,107 | 421 | 4.79x |
| translated, absolute, again | 598-629 | 120-126 | 1.36-1.43x |
| translated, base page, again | 535 | 107 | 1.21x |
| translated, base page + peephole, first run | 566 | 113 | 1.29x |
| translated, base page + peephole, again | 471 | 94 | 1.07x |
| far interpreter: bank 0, bank 4, attic RAM | 2,641 | 528 | 6.00x |
| far translator: bank 0, bank 4, attic RAM, first / again | 566 / 472 | 113 / 94 | 1.29x / 1.07x |

`sum`'s sizes: 67 bytes native; 53 bytes of p-code (scheme A), 32 (B).

- Translating costs about 2,500 cycles for `sum`'s 3 blocks: about 6 interpreted iterations.
- Interpreting far p-code costs 25% (every fetch 6 cycles more); translated code does not care where its p-code was.
- xemu does not model attic RAM's latency: on a MEGA65 interpreting from attic RAM will be slower than here; the
  translator reads the p-code once, so hot code should not be. To be measured on the hardware.

## The p-code back end (`CC45_PCODE`; `src/main/PcodeEmit.cpp`, `pcode_rt.s45`)

cc45 translates the functions named by `CC45_PCODE` (`all`, or a comma list) from their optimized IR to p-code;
`CC45_PCODE_REPORT=1` names each one chosen or left native, and why (an operation it does not translate, inline
asm). The program is linked with the runtime, `ca45 -c pcode_rt.s45`.

- **A function** is its frame, `<f>__pf` (static: the function is marked recursive so it gets no SAC block of its
  own; parameters, locals and the IR's temporaries, temporaries of different blocks sharing bytes, at most 250
  bytes), a native entry and its p-code. The entry copies the ZP parameters into the frame and jumps to the
  interpreter with the p-code's address in A/X and the frame's in Y/Z (`jmp __pc_enter`). It declares
  `.zp_clobbers $08-$FF`, so callers keep nothing in the base page across it. RET leaves the result in A/X/Y/Z and
  returns to the native caller.
- **The encoding**: an opcode byte (bits 5-0 the operation, bit 7 the word form, bit 6 the long form), then frame
  offsets, an immediate of the operation's width, or a 2-byte address. 47 opcodes: CONST, MOV, LDG/STG (globals),
  LD/ST (through a pointer), LEAL, ELEM (base + index * stride), the ALU operations with immediate forms, DIV/MOD
  signed and unsigned, NEG, NOT, CMP/CMPI (a condition code), BR/BNZ/BZ, CALL (ZP convention: native, or p-code
  through its entry), RET/RETV, ZEXT/SEXT, and ADV (a run of over 240 bytes). Left native: a function with an
  operation the back end does not translate (far and near access, floats and the like), inline asm, calls through
  a pointer or to stack-convention or `__regparm` functions, the stack convention itself, variadic, `__interrupt`,
  `__naked`, nested and recursive functions.
- **The interpreter** (`pcode_rt.s45`, about 2.8 KB): one generic handler per operation working on two
  accumulators of the operation's width (1, 2 or 4 bytes), dispatched by `asl / tax / jmp (handlers,x)`; its state
  in the base page (IP `$08`, FP `$0A`, accumulators at `$F3` and `$F7`). CALL saves IP and FP on the stack.

### Correctness (`test_pcops_xemu.c`, `run_pcops.sh`)

Every operation at 8, 16 and 32 bits, signed and unsigned, over a spread of values, element access of several
strides, calls with each argument and result width, and branches on each width; built native and as p-code, a
checksum per call compared. All the differences found were native cc45's: signed 16-bit `%` (#794) and 32-bit
`$FFFFFFFF / 1` and `% 1` (#795); the p-code gives the C result.

### musicgen

Every file of `examples/c/musicgen` compiled with `CC45_PCODE=all` and linked with the runtime: 47 functions as
p-code, `main` and `video_on` native (inline asm). Driven headless in xemu with virtual keys (`$D615`), it draws the
whole screen and keys change the style, the drums and the tempo as in the native build (the songs differ: they
are random).

| | native | p-code |
|---|---:|---:|
| `.prg`, interactive build | 17,034 | 20,141 |
| `.prg`, `MG_TEST` build | 8,674 | 10,676 |
| BSS | 3,191 | 4,416 |

The p-code is larger because this back end is the plain encoding: none of scheme B's combined instructions, every
global or constant operand without an immediate form first copied into the frame (a MOV or CONST); plus the 2.8 KB
runtime and frames that ln45 does not overlay as it does SAC blocks (hence the BSS). Scheme B's forms and
overlaid frames are the next steps; the measurements above put the plain encoding's saving at 17% for the
examples before the runtime, so musicgen (about 13 KB of its own code) is near the break-even point.

### Bugs found on the way

- **The startup that returned at once to `READY.`** was ca45's tail deduplication (#796): the first entries ended
  `jsr __pc_enter` + `.word` p-code + `.word` frame + `rts`; ca45 compared only the instructions, found the tails
  the same and made most entries jump into another function's entry, which then ran that function's p-code. The
  entry now passes the addresses in registers.
- Branches taken the wrong way (BR_COND's targets read in the wrong order), and callers keeping arguments in
  `$10` across a call into p-code (the entry did not declare what the interpreter changes): both in the back end,
  fixed.

## What a design would have to settle

- The p-code instruction set: scheme B's forms, the rare operations as escapes, a native escape for inline asm.
- Frames: the static block (SAC) as the frame (no frame setup; fixed addresses for the translator); within a page
  for the base-page frame; ln45 places and aligns them.
- Calls: p-code to p-code in the interpreter; native to p-code through a bank-0 entry (`jsr __pcode` + the 4-byte
  address, about 7 bytes); p-code to native with B switched back (about 6 cycles).
- Which functions: per function (pragma, or by size and loops); the measurements favour p-code for large functions
  and code that runs once, native for small functions, loops that run once, interrupt handlers.
- The translator's cache: its size, a better lookup than a linear table, its emptying, overlays.
- Not worth it: caching decoded operations only (most of the cost is the operands and the frame), and entering
  p-code through BRK (the vector is the KERNAL's and IRQ's; `jsr` with the p-code after it, as SWEET16 does, is
  smaller to handle and far p-code needs an address anyway).
- dbg45 for p-code; interrupt handlers stay native (the interpreter's state is not reentrant).

## Files

| file | |
|---|---|
| `pcode_vm.s45` | the interpreter, version 2 (`pcode_run`) |
| `pcode_vm_v1.s45` | version 1 (`pcode_run1`) |
| `pcode_vmf.s45` | version 2 with far p-code (`pcode_runf`) |
| `pcode_jit.s45` | the translator: the frame in the base page, the peephole (`pcode_jit`) |
| `pcode_jit_abs.s45` | the translator, variables at absolute addresses (`pcode_jita`) |
| `pcode_jitf.s45` | `pcode_jit.s45` with far p-code (`pcode_jitf`) |
| `pcode_rt.s45` | the runtime of cc45's p-code back end (`__pc_enter`) |
| `test_pcops_xemu.c`, `run_pcops.sh` | every p-code operation against native code, in xemu |
| `../../src/main/PcodeEmit.cpp` | the back end (`CC45_PCODE`, `CC45_PCODE_REPORT`) |
| `pcode_progs.s45` | the p-code programs, written by hand |
| `test_pcode_xemu.c`, `test_jit_xemu.c`, `test_far_xemu.c` | the xemu tests (`run.sh` builds and runs them) |
| `tools/collect.sh` | `CC45_PCODE_STATS` over the library and the examples, and the tables |
| `tools/stats_a.py`, `tools/stats_ab.py` | the tables from the statistics files |
| `tools/cycles.py` | an instruction's cycles along a path through an interpreter's disassembly |
