# Changelog

All notable changes to the cc45 / ca45 suite will be documented in this file.

## [Unreleased] - 2026-05-29

### Added
- **MachineState Register & Memory Tracking Framework**: Unified value tracking for CPU registers (A/X/Y/Z/SP), CPU flags (N/Z/C/V), and three-tier memory (ZP full, stack full, 512-entry ring buffer for absolute addresses). Replaces ad-hoc `xHoldsSP_`, `aEqualsX_`, and string-based `RegState` in the assembler optimizer. Enables constant propagation, flag-derived optimizations, and store-forwarding across the entire toolchain.
  - Phase 1: Register tracking (CONSTANT, SAME_AS_REG, UNKNOWN). All M65Emitter instruction methods update state.
  - Phase 2: Three-tier memory tracking. Stores propagate register values to memory; ZP loads propagate known values back.
  - Phase 3: RANGE/NONZERO value kinds. FlagState derives exact N/Z from known constants. ALU immediate ops propagate constants through and/ora/eor/adc/sbc/asl/lsr/neg/inc/dec. Compare instructions derive exact N/Z/C.
  - Phase 4: AssemblerOptimizer integration. MachineState replaces ad-hoc RegState for redundant load elimination and memory tracking.

### Optimizations
- **CMP #0 Elimination**: Assembler optimizer detects redundant `cmp #0` when N/Z flags already reflect the accumulator from a prior load instruction. Safe when only BEQ/BNE/BMI/BPL follow (not BCC/BCS).
- **Store-Forwarding in IR Codegen**: When a STORE's source vreg was produced by the immediately preceding instruction, skip the redundant ZP reload — the result is still in A:X.
- **CONST Direct Store**: When a CONST is immediately followed by STORE to a ZP local, store constant bytes directly to the local's ZP address without round-tripping through a temp vreg.
- **I16 INC/DEC Peephole**: `local += 1` / `local -= 1` for ZP-allocated I16 locals emits `inc $ZP; bne *+4; inc $ZP+1` (5 bytes) instead of full load + add.16 + store + reload + store (~20 bytes). Handles both immediate operands and CONST vregs.
- **Skip TAX/TAY/TAZ for Duplicate CONST Bytes**: When a CONST instruction has identical high/low bytes (e.g., -1 = 0xFFFF) and the destination is ZP-allocated, skip the transfer instruction since storeVreg uses `sta` via `regsEqual()`. Also use `sta` instead of `sty`/`stz` for I32 ZP stores when Y==A or Z==A.
- **ZP Addressing in Text Mode**: Simulated op text emission uses 2-digit hex for addresses < 256, allowing the assembler to select base-page mode (saves 1 byte per instruction).
- **ZP Addressing in Binary Mode**: Simulated ops converted from hardcoded branch offsets to `emitBranchPlaceholder`/`patchBranchTarget` (20 forward branches). All 141 memory-access calls changed from `_abs` to `_addr`, auto-selecting base-page for ZP-range addresses in binary mode.
- **Constant-Memory Forwarding**: M65Emitter `_addr` read-type methods (lda/ldx/ldy/ldz/adc/sbc/and/ora/eor/cmp) check MachineState for ZP locations holding known constants and emit immediate mode instead of memory read. Assembler generator preserves memory state across non-label statement boundaries to enable cross-statement forwarding.
- **Relocatable Constant Forwarding (RELOC_CONST)**: When `lda #<_symbol; sta $ZP` stores a relocatable symbol's lo/hi byte to a ZP scratch location, subsequent reads from that ZP location (`adc $ZP`) are forwarded to immediate mode (`adc #<_symbol`) with proper R_LOW/R_HIGH relocation records. Eliminates ZP memory round-trip for global address arithmetic in relocatable objects. Enabled by per-instruction MachineState tracking in the assembler generator — no blanket register invalidation at statement boundaries.
- **CPX for Unsigned 16-bit Compare**: `cmp.16` now uses `cpx #hi` for the high byte instead of `txa; cmp #hi`, saving 1 byte per unsigned comparison and preserving A. Applies to both immediate and memory operand paths. Signed `cmp.s16` unchanged (needs EOR #$80).
- **Store-Fused I16 Add/Sub**: When an I16 add/sub result is immediately stored to a ZP local, emit `clc; adc lo; sta $ZP; txa; adc hi; sta $ZP+1` instead of `pha; txa; adc; tax; pla; sta; stx`. Saves 3 bytes per fused operation.
- **Frame INC/DEC Pseudo-Ops**: New `inc.fp`/`dec.fp` (8-bit) and `inc.16f`/`dec.16f` (16-bit) pseudo-ops for direct stack-relative increment/decrement. IRCodeGen I16 INC/DEC peephole extended to IN_FRAME variables. `gen++` on a stack variable: 25 bytes → 9 bytes.
- game_of_life.prg: **5301 → 4985 bytes** (further reduction from MachineState-enabled optimizations).

## [Unreleased] - 2026-05-27

### Added
- **`mega65.h` Hardware Register Header**: VIC-IV register struct overlay (`vic4->border = 0`) mapping all 128 registers ($D000-$D07F). Includes nested sprite position struct, `VREG_*` direct register defines, bit/colour constants, and convenience functions (`vic4_unlock`, `vic4_fast`, `vic4_sprite_pos`).
- **SID Struct Overlay**: 4 SID instances (`sid1`-`sid4` at $D400/$D420/$D440/$D460) with voice struct (frequency, pulse width, waveform, ADSR), filter, volume. `SIDn_VOICE(n)` helpers for voice access. Waveform, filter routing, and filter mode constants.
- **CIA Struct Overlay**: 2 CIA instances (`cia1`/$DC00, `cia2`/$DD00) with ports, data direction, timers A/B, TOD clock, serial data, interrupt control, and control registers. ICR/CRA/CRB bit constants, joystick direction constants, `joy1_read()`/`joy2_read()` convenience functions.
- **DMA Controller Struct**: `dma` at $D700 with control, address, and enhanced trigger registers. DMA command constants.
- **Math Accelerator Structs**: `math_div` ($D760) and `math_mul` ($D770) for hardware 32-bit divide and multiply. `MATH_BUSY`, `HW_RANDOM` direct access defines.
- **Audio Mixer Struct**: `audio_mixer` ($D63C) for SID left/right volume panning. `AUDIO_LEFT()`, `AUDIO_RIGHT()`, `AUDIO_BOTH()` helper macros.
- **Memory Pointers**: `SCREEN_RAM` ($0800) and `COLOUR_RAM` ($D800) convenience pointers.
- **Floppy Controller Struct**: `fdc` ($D080) with track/sector/side/data registers and FDC command/status constants.
- **SD Card Controller Struct**: `sd` ($D680) with sector address, command, buffer address, and read data. SD command/status constants.
- **Ethernet Controller Struct**: `eth` ($D6E0) with control, TX frame size, MAC address. ETH control bit constants.
- **Hypervisor Trap Struct**: `hyper` ($D640) with 64-byte trap register array and common trap constants.
- **`key_pressed()` Keyboard Scanner**: `__regparm` library function for direct CIA1 matrix scan. Supports simultaneous multi-key detection. 66 `KEY_*` constants for the full C64/MEGA65 keyboard matrix. Both stack and ZP calling convention variants in `c45.lib`/`c45_zp.lib`.

### Optimizations
- **Constant-Address Direct Store**: `*(volatile unsigned char *)0xD020 = val` now emits `lda #val; sta $D020` instead of loading the address into a ZP pair and using indirect addressing. Tracks vreg constant values through the IR to detect constant addresses at STORE time.
- **Constant-Arg Function Call Push**: When all arguments are simple (constants, globals), pushes inline with `phw #imm16` (3 bytes) instead of round-tripping through ZP temp slots (~100 bytes → ~15 bytes for `memset(grid, 0, 1000)`). 32-bit constants use two `phw`. Falls back to two-phase ZP temps for complex args.
- **CONST Vreg Suppression**: Pre-scan identifies CONST vregs only used in constant-address STOREs or as args to all-simple CALL sites. Suppresses emission, frame slot allocation, and STORE-to-local-slot. `VREG_BORDER = 0` → `lda #0; sta $D020` (2 instructions). Dead call-arg local slots eliminated.
- **Global Address `phw` in Calls**: `phw #_symbol` for global address args (3 bytes, linker-resolved) instead of `ldax #_symbol; push .ax` (5 bytes).
- **PLZ Frame Cleanup**: Function epilogues use `plz` to pop frame slots instead of `pla`. PLZ doesn't clobber A/X/Y, eliminating return value save/restore (4-6 instructions saved per function). I32 returns save Z to scratch.
- **Stdlib Single-TSX Param Load**: `memset`, `memcpy`, `memmove` load all stack params with a single `tsx` instead of one per param. `pha/plx` for Y→X transfer avoids ZP temp.
- **Static DMA Jobs in CRT**: ZP save/restore uses static 12-byte DMA command blocks in data segment instead of runtime-built jobs (~40 bytes each). `__exit`: 95 → 23 bytes.
- **TSX Caching**: `lda_stack` skips redundant `tsx` when X already holds SP within a simulated op expansion. Saves one `tsx` per `ldax.fp` param load.
- **`ldax.fp` pha/plx**: 16-bit stack param loads use `pha/plx` to transfer hi byte instead of ZP scratch temp `$02`. No ZP clobber.
- **Short-Circuit `&&` / `||`**: Logical AND/OR now use short-circuit branching — LHS evaluated first, RHS skipped if result already determined. Matches C standard semantics.
- **ORA Zero-Test**: `x == 0` / `x != 0` for ZP-allocated 16-bit values uses `lda zp_lo; ora zp_hi` (5 bytes) instead of `cmp.16 .AX, #0` (~12 bytes). Sets Z flag directly.
- **CONST Suppression for CMP Operands**: CONST vregs used only as CMP src2 are suppressed — the comparison already reads the constant value directly.
- **Boolean Materialization Elimination**: `if`/`while` with `&&`/`||`/`!` conditions use `emitConditionBranches()` to chain branches directly to true/false targets without materializing intermediate boolean vregs.
- **Unused Static Function Elimination**: Static functions never called by any surviving function are removed at compile time. Handles transitive call chains.
- game_of_life with mega65.h: **7037 → 5301 bytes (25% total reduction)**.

### Improved
- **Function Declaration `.loc`**: Prologue and parameter loading code attributed to the function declaration line (e.g., `int foo(int a)`) instead of the first statement. Makes `objdump45 -d` source interleaving more readable.

### Fixed
- **Cast-Pointer Dereference Store Width (Issue #83)**: `*(volatile unsigned char *)ADDR = val` was generating 16-bit stores, clobbering the adjacent byte. `IRBuilder::getExprTypeInfo()` now derives the correct pointee type from cast expressions (not just variable references), producing proper 8-bit stores.
- **Nested Struct Array Member Access (Issue #84)**: `ptr->array_member[n].field` was generating a spurious dereference (loading from the address instead of using it) and using wrong element stride. Two fixes in `IRBuilder`: (1) array/struct-typed members now decay to address instead of generating a LOAD; (2) `ArrayAccess` now derives element size from struct member info when the array base is a `MemberAccess`. Enables `vic4->sprite[n].x`, `sid1->voice[n].freq_lo` etc.
- **Empty-Parameter Function-Like Macros**: `#define FOO() body` was not expanding when invoked as `FOO()`. The preprocessor now correctly handles zero-parameter function-like macros in both definition parsing and expansion.
- **`-I` Flag Space-Separated Path**: `cc45 -I dir` was treating `dir` as the source file instead of an include path. Now handles both `-Idir` and `-I dir` forms.
- **debug.json Bloat (Issue #85)**: `.debug.json` line map files now only emit entries when file or line changes, using range-based lookup. ~94% size reduction. Fixed in both `ln45` and `ca45`.
- **Cast-Pointer Subscript Element Size**: `((unsigned char *)ADDR)[n]` was using stride 2 (int) instead of 1 (char). `ArrayAccess` now derives element size from the expression's pointed-to type when the root is not a variable or struct member.
- **Undeclared Identifier Diagnostic**: `map::at` crash on undeclared variables replaced with proper `"file:line: error: use of undeclared identifier 'name'"` message via `lookupVar()` helper in CodeGenerator.
- **Error Line Numbers After `#include`**: Preprocessor now emits `#line` directives after includes; lexer consumes them to restore original source line numbering. Error messages and `.loc` debug info now report original source lines instead of preprocessed line numbers.
- **`.o45` Line Map Offsets (Issue #87)**: Line map entries in `.o45` files were offset by the data segment size, causing wrong source line attribution in `objdump45` and `debug.json` after linking. Fixed by computing offsets relative to the code segment start instead of global base 0.
- **`.loc` Before Function Prologue**: Function prologues (`phw` frame setup) were attributed to the previous function's last source line. Now emits `.loc` before the prologue so `objdump45` shows the correct function entry line.
- **objdump45 TEXT/DATA Boundary**: Disassembly now stops at the TEXT segment end when a map file is available, preventing DATA arrays from being shown as `brk` instructions.
- **objdump45 Source Line Lookup**: Source line interleaving now uses range-based lookup (`upper_bound`) instead of exact match, working correctly with deduplicated debug entries.
- **cc45 Binary Path Resolution**: System include path (`lib/include`) now resolved via `/proc/self/exe` or `realpath()` instead of `argv[0]`, fixing include search when invoked from any directory.
- **Removed `.debug.json` Generation**: `ln45` and `ca45` no longer emit `.debug.json` files. Use `-M` flag for map file output which contains correct line info.
- **`.word` Symbol References (Issue #88)**: `.word` directive now supports symbol operands with relocations, enabling static data tables with linker-resolved addresses.
- **ln45 Unused Symbol Warnings**: Suppressed "unused global symbol" warnings for `__`-prefixed internal symbols (CRT internals like `__exit`, `__init`, `__sp_base`). Only user-facing symbols are reported.

## [Unreleased] - 2026-05-26

### Added
- **`math.h` Integer Functions**: `min(a,b)`, `max(a,b)`, `gcd(a,b)`, `lcm(a,b)` — each in a separate translation unit for selective linking. Both stack and ZP calling convention variants.
- **`abs`/`labs` Macros**: `abs` and `labs` default to inline macros in `stdlib.h`, eliminating call overhead. Define `__NOMACRO_ABS` or `__NOMACRO_LABS` to use the function versions instead.
- **Thunk Warning by Default**: The linker now always warns when generating calling convention bridge thunks, not just with `-Wthunk`. Use `--no-thunks` to error instead.
- **`time.h`**: `clock()`, `time()`, `difftime()`, `CLOCKS_PER_SEC` (60Hz jiffy clock). Both stack and ZP calling convention variants.
- **`#pragma cc45` Namespace**: All compiler pragmas now use the `#pragma cc45 <option>` prefix (e.g., `#pragma cc45 heap`, `#pragma cc45 no_bssinit`). **Breaking change**: `#pragma crt <option>` is no longer accepted. `#pragma once` and `#pragma weak` remain unchanged.
- **Inline Function Expansion**: `inline` keyword now triggers function body inlining at call sites. `-finline-functions` flag enables auto-inlining of small functions (≤20 statements) even without the `inline` keyword. Eliminates JSR/RTS overhead for small leaf functions. Recursive, variadic, interrupt, and naked functions are excluded.
- **`repeat()` Loop Unrolling**: Compile-time loop unrolling via `repeat(N) { body }` and `repeat(type var, N) { body }` with optional loop variable (1..N). Zero loop overhead — body is duplicated N times.
- **DMA Intrinsics**: `__dma_copy(dst, src, len)` and `__dma_fill(dst, len, val)` for MEGA65's F018B DMA controller (~40MB/s). Compiler builtins, no library needed.
- **`__regparm` Function Attribute**: First parameter passed in A (char) or AX (int/pointer) register instead of ZP/stack. Saves 2–4 bytes and 3–6 cycles per call. Remaining parameters use normal convention.
- **`.repeat N` / `.endrepeat` Assembler Directive**: Compile-time loop unrolling in assembly. Supports nesting and expression counts.
- **`-Roptimizer` Flag**: Reports assembler optimizer actions to stderr with source file, line, pass name, and action (for both `ca45` and `cc45`).
- **`-Rcodegen` Flag**: Annotates assembly output with IR codegen reasoning comments.
- **JMP→BRA Peephole**: Converts 3-byte JMP to 2-byte BRA when target within ±127 bytes.
- **JSR→BSR Peephole**: Converts JSR to BSR (relative16) for local symbols, enabling position-independent code.
- **Standalone CRT Return Value**: Main return value preserved across ZP restore via $02/$03.
- **`__interrupt` / `__naked` Function Attributes**: `__interrupt` emits PHA/PHX/PHY/PHZ + RTI; `__naked` suppresses all prologue/epilogue.
- **Case Ranges**: GCC-style `case 'A' ... 'Z':` syntax for matching value ranges in switch statements.
- **Packed Structs by Default**: All structs packed (no alignment padding). `__unpacked` keyword opts into traditional C alignment.
- **`-Rcodegen` Flag**: Annotates assembly output with codegen reasoning comments showing which IR instruction generated each line.
- **objdump45 Enhancements**: Auto-discovers `.map` files for PRG symbol annotation; interleaves source line comments in disassembly; linker map includes Source Lines section.
- **Integer Literal Auto-Promotion**: Literals > 65535 automatically promoted to `long` with compiler warning.

### Optimizations
- **palette_fade.prg: 2151 → 1003 bytes (53% reduction)** across all optimizations below:
- **Fused Compare-and-Branch**: CMP/AND/OR result + BR_COND → direct branch on flags, skipping boolean materialization.
- **BBS/BBR Single-Bit Tests**: `if (var & power_of_2)` on ZP variables emits 3-byte BBS/BBR instruction.
- **Branch Inversion**: `Bcc +2; BRA target` → `B!cc target`, saves 2 bytes per occurrence.
- **Pointer Constant Propagation**: `*p = val` where p holds constant address → `sta $ADDR` directly.
- **Dead Local Variable Elimination**: IR pass removes unread vregs and dead stores.
- **8-bit Char Arithmetic**: Native `and`/`ora`/`eor`/`adc`/`sbc`/`cmp`/`inc a`/`dec a` for char operations.
- **Bidirectional Store/Load Tracking**: Assembler eliminates `lda $ZP` after `sta $ZP` when A still holds the value.
- **Tail-Call Optimization**: `JSR + RTS` → `JMP` in assembler optimizer.
- **32-bit Shift Byte-Shuffle**: Immediate shifts of 8/16/24 bits use register shuffling instead of loops.
- **`while(1)` Constant Loop**: Detected at IR level, emits single unconditional back-branch.
- **Dead Trailing Block Elimination**: Unreachable code after infinite loops removed.
- **Void Call Cleanup**: `CALL_VOID` for void functions prevents spurious return value stores.
- **Return Constant**: `return N` emits immediate load directly, no vreg round-trip.
- **`chknonzero.8` Elimination**: Redundant `cmp #$00` removed when Z flag already set from load.
- **`loadVregA`/`loadOperandA`**: I8-only load paths skip `ldx #0` zero-extension.

### Fixed
- **`rts #N` Stack Mismatch**: `B#` (char) parameters counted as 1 byte for stack cleanup but caller pushes 2 bytes (C promotion). Fixed to use promoted size.
- **`cmp.32` A-Clobber**: 32-bit comparison clobbered A (byte 0) before final byte comparison. Fixed by saving A to ZP scratch.
- **ZEXT I8→I32**: Zero-extension from 8-bit to 32-bit now clears Y and Z registers.
- **Signedness Defaults**: Integer literals and function returns default to unsigned, matching cc45's type system.
- **CRT ZP Save/Restore**: `crt0.o45` and `crt0_zp.o45` now save/restore ZP $08-$FF using `move` pseudo-op.
- **Legacy Codegen Removed**: `--legacy-codegen` flag removed; all compilation uses IR pipeline.

## [Unreleased] - 2026-05-22

### Added
- **CPU and Flag Intrinsics**: High-level access to processor state via `__cpu.R` and `__flags.F`.
    - Supports all physical registers (A, X, Y, Z) and virtual pairs/quads (AX, Q, etc.).
    - Supports processor flags (Carry, Zero, Negative, Overflow).
    - Optimized code generation: `__flags.Carry = 1` emits `SEC`; `__cpu.X = 5` emits `LDX #5`.
    - Integrated safety: compile-time error if taking address of registers/flags.
- **`mod.32` support**: Added 32-bit unsigned modulo support to the assembler.
- **Hardware Register Header**: Centralized MEGA65 math and DMA register definitions in `include/Mega65Registers.hpp`.

### Fixed
- **Math Accelerator**: Corrected register addresses for Quotient and Remainder based on empirical simulator probing.
- **`realloc` memory corruption**: Fixed a bug where simulated operations clobbered scratch registers used by the memory allocator.
- **DMA Buffer Hazards**: Implemented explicit BSS reservation for FILL and MOVE buffers in the assembler.
- **Relocation Signedness**: Improved linker support for signed offsets on external symbols.

## [Unreleased] - 2026-05-17

### Added
- **Compiler (cc45)**:
    - **Zero Page Preservation**: Automatic preservation of Zero Page state ($08-$FF) at program entry/exit. This ensures that compiled C programs do not corrupt the system's BASIC/KERNAL ZP state when they return to the OS. Controlled by the new `saveZP` flag in `ir::Module`.
    - **`#pragma no_zp_save`**: Allows developers to explicitly disable ZP preservation when not needed (e.g., in standalone firmware).
    - **Unused Variable Warnings**: The compiler now warns about unused local variables and function parameters, including filename and line number info. Integrated with `ConstantFolder` to prevent false positives for folded expressions.
- **Assembler (ca45)**:
    - **8-bit Operand Overflow Warnings**: Emits a warning when a 16-bit value is used for an 8-bit addressing mode (like `#immediate` or base-page), preventing silent truncation bugs.
    - **Procedure Scoping Validation**: Enhanced validation of `proc`/`endproc` blocks. Detects and reports errors for nested procedures, `endproc` without a matching `proc`, and unclosed procedures at the end of the file.
- **Linker (ln45)**:
    - **Unused Global Symbol Warnings**: The linker now warns about exported global symbols that are not referenced by any other module, helping identify dead code at the link stage.

### Fixed
- **Compiler (cc45)**:
    - **IR Pipeline Stabilization**: Unified type propagation in `IRBuilder`, fixed 32-bit arithmetic operations, array indexing, and pointer dereference logic. Corrected `IntegerLiteral` handling to use 64-bit values to prevent early truncation.
    - **VReg Allocation Accuracy**: Fixed `VRegAllocator` to properly track all local variables, preventing memory corruption from overlapping ZP or frame slot assignments.
    - **32-bit Memory Operations**: Fixed clobbering of registers during 32-bit indirect loads and stores by ensuring correct usage of scratch ZP registers.
- **Assembler (ca45)**:
    - **M65Emitter Scratch ZP Fix**: Corrected a bug where scratch ZP registers used by simulated operations were hardcoded to $02-$06. They are now correctly initialized relative to the user-specified `zeroPageStart` (e.g., $08). This prevents simulated ops from clobbering BASIC/KERNAL memory.
    - **Improved Error Reporting**: Refactored error handling to ensure each diagnostic is printed only once and correctly results in a non-zero exit code.
- **Linker (ln45)**:
    - **Symbol Resolution Robustness**: Fixed edge cases in symbol resolution and improved internal state management during the multi-pass linking process.

## [v1.0-rc2] - 2026-05-11

### Fixed
- **Assembler (ca45)**:
    - **O45Emitter symbol segment misassignment (Issue #45-2)**: Symbols defined after a `.data` or `.bss` directive could be incorrectly tagged as TEXT segment in the `.o45` output. Root cause: the O45Emitter inferred segment membership from address ranges (`findSegmentForAddress()`), which failed at segment boundaries where the code segment's PC was adjacent to the data segment's start. Fix: use the authoritative `sym->segment` field set at parse time instead of address-range inference. Also fixed missing segment fields on `.array`, `proc`, `__fill_dma_buf`, and `__move_dma_buf` symbol initializers. The `findSegmentForAddress()` function has been removed.
    - **Segment `startAddress` going stale across pass2 iterations (Issue #45)**: When assembling `.o45` files with both `.code` and `.data` sections, forward-reference resolution caused pass2 to run multiple iterations. On the first iteration, unresolved instructions were oversized, inflating the code section. The data segment's `startAddress` was set on this first iteration and never recalculated, creating a gap between TEXT and DATA segments. This caused `findSegmentForAddress()` to fail for data symbols, which then defaulted to `SEG_TEXT` with wrong offsets. After linking, data symbols overlapped with code symbols at the same addresses. Fix: reset segment `startAddress` at the start of each pass2 iteration.
- **Linker (ln45)**:
    - **Cross-segment address collision detection**: The linker now tracks the segment ID of each exported symbol and emits a warning when two symbols from different segments are placed at the same address. This catches assembler segment bugs early instead of producing silently-corrupt binaries.

### Added
- **Testing**:
    - **Segment emission test suite** (`make test-segment-emission`): 55 new tests covering end-to-end segment assignment (data/bss/boundary/multi-segment/proc labels), multi-object linking (no overlap, BSS placement, unique addresses, cross-object relocations), and regression tests for Issue #45-2.

### Changed
- **Assembler (ca45)**:
    - **Bare `.text` no longer switches segments**: Previously, `.text` without arguments was an alias for `.code` (segment switch). This created ambiguity with the `.text "string"` PETSCII data directive. Bare `.text` is no longer recognized as a segment switch. Use `.code` instead. The `.segment "text"` form and `.text "string"` data directive are unaffected.

### Known Limitations
- **Custom segments in `.o45` mode**: User-defined segments (`.segment "myname"`) are mapped to `SEG_TEXT` in `.o45` output. The `.o45` format supports four segment IDs: TEXT, DATA, BSS, and ZP. Custom segment names are preserved via `OPT_SEGATTR` option records, enabling linker-side ordering (e.g., `"init"` before `"code"`). Custom segments work correctly in flat binary mode; in relocatable builds they share the TEXT segment body but retain their name metadata.

## [Unreleased] - 2026-05-09

### Added
- **Linker (ln45) & Object Format (`.o45`)**:
    - **Calling convention enforcement**: Added `FUNC_FLAG_ZP_CONV` bit (0x04) to `O45FuncAttr.flags` to track calling convention per-function. The linker now enforces that ZP-convention callers (compiled with `-fzpcall`) do not call stack-convention callees, catching a class of silent wrong-answer bugs. Enforcement is one-directional (ZP→stack only) to permit intentional `__fastcall__` interop where stack-convention code generates ZP param setup before calling. Hard linker error on mismatch.
    - **`FUNC_FLAG_REENTRANT` semantic clarification**: Renamed the previously-unused `INT_SAFE` flag to `FUNC_FLAG_REENTRANT` (bit 0x02) to capture the more general property: a function that is re-entrant safe (no global mutable state, stack-only locals). This is the superset property of which interrupt-safety is one consequence.
- **Assembler (ca45)**:
    - **`.func_flags` directive**: New directive for hand-written assembly to annotate function calling convention and properties. Syntax: `.func_flags zp_call [, stack_call] [, leaf] [, reentrant]`. Parsed within `proc`/`endproc` blocks. Integrates seamlessly with automatic metadata emission from the compiler.
- **Compiler (cc45)**:
    - **Automatic calling convention metadata emission**: Both ZP and stack code paths now emit `.func_flags zp_call` / `.func_flags stack_call` directives automatically for every function, with `leaf` annotation when no calls are present. Enables linker enforcement without requiring manual annotation in C source.
- **Tools**:
    - **nm45 symbol display**: Updated `nm45 -f` to display calling convention and function flags in symbol listings. Example: `[zp_call leaf uses:- clobbers:- ...]`.
    - **objdump45 symbol table**: Added function attribute display to symbol table output, showing convention and flags for each exported function.
- **Documentation**:
    - **`doc/lib45.md` Section 4.4**: Complete specification of function attribute records: marker byte `$FA`, flags byte (LEAF, REENTRANT, ZP_CONV), register/flag clobber masks, ZP usage/clobber bitmasks. Documents linker enforcement rule (ZP→stack errors, stack→ZP permitted).

## [Unreleased] - 2026-05-08

### Added
- **Compiler (cc45)**:
    - **`_Alignas` support for local variables and struct members (Phase 1)** (d1debd3): Alignment computation and storage reservation implemented. Local variables and struct members can now be declared with `_Alignas(N)` specifiers. The compiler computes required padding between fields to meet alignment constraints and reserves additional frame space as needed. Struct member layout respects alignment boundaries.
    - **Symbolic `.fp` references for local variables (Phase 2)** (6ab142e): Frame-pointer-relative variable references now use symbolic names (e.g., `.fp var_name`) in addition to numeric offsets. The assembler's symbol table supports frame-relative symbol lookup, enabling more readable and maintainable assembly output.
- **Assembler (ca45)**:
    - **Dead code elimination after infinite loops (Issue #26)** (7724cfa): The assembler now detects and removes unreachable code that follows infinite loops (e.g., code after `while(1)` constructs). Instructions after `BRA` loops are recognized as dead code and eliminated, reducing binary size and improving code clarity.
    - **Fix BinaryExpr reentrancy bug in assembler expression evaluator (Issue #35)** (8bb0764): Fixed a critical bug where recursive evaluation of binary expressions in the assembler's expression evaluator could cause incorrect operand values or hangs. The fix ensures that complex nested expressions are evaluated correctly even when the evaluator processes sub-expressions recursively.
    - **3-operand MOVE/FILL test coverage**: Added comprehensive test cases for the 3-operand syntax (`MOVE src, dest, len` and `FILL dest, len`) that was previously undocumented. The syntax allows explicit specification of source, destination, and length operands without requiring preloaded register pairs. Tests include: immediate operands, register pair operands, symbol operands, stack-relative operands, and mixed operand types. All 9 new C++ tests pass; assembly examples in `test_fill_advanced.s` demonstrate practical usage. Parser support for 3-operand syntax was already implemented in `AssemblerSimulatedOps::emitMoveCode()` — tests validate and document this capability.
- **Documentation**: Updated `doc/opcodes.md` with complete syntax documentation for 2-operand and 3-operand forms of `MOVE` and `FILL`, including usage examples.

## [Unreleased] - 2026-05-05

### Added
- **Compiler (cc45)**:
    - **`#pragma once`**: Replaces the non-standard `#pragma include_once` with the widely-supported `#pragma once` for header guard optimization. The old directive has been removed.
    - **Type qualifier ordering**: All orderings of type qualifiers are now accepted across all declaration contexts (globals, locals, parameters, return types, typedefs). For example, `volatile signed int`, `int volatile`, `signed volatile int`, and `unsigned const char` all parse correctly. Previously, qualifiers were required to appear before signedness specifiers.
- **Assembler (ca45)**:
    - **`defined(<symbol>)` in expressions**: Test whether a symbol exists at assembly time (post-pass-1). Returns 1 if the symbol is defined, 0 otherwise. Works in any expression context (e.g., `.if defined(MY_FLAG)`). Accepts both IDENTIFIER and INSTRUCTION tokens as symbol names.
- **Standard Library (stdlib45.lib)**:
    - **`stdio.h`**: `getchar()` — Input a single character via KERNAL `GETIN` ($FFE4). Busy-waits until a key is pressed.
    - **`ctype.h`**: Added 6 PETSCII-aware character classification functions: `isupper` ($C1-$DA), `islower` ($41-$5A), `isxdigit` (digits + hex letters in both PETSCII cases), `ispunct` (printable non-alnum non-space), `isblank` (space $20 or tab $09), `iscntrl` ($00-$1F and $7F-$9F). All implemented in hand-written 45GS02 assembly for both stack and ZP calling conventions.
    - **`string.h`**: Added 6 string functions: `strncat` (concatenate with length limit, assembly), `strstr` (substring search, C), `strpbrk` (find first char from set, C), `strspn` (count leading chars in set, C), `strcspn` (count leading chars not in set, C), `strtok` (tokenize string, C with static state). All implemented for both calling conventions.
    - **`stdlib.h`**: `strtol` (string to signed long) and `strtoul` (string to unsigned long). Support base 0 (auto-detect), 8, 10, 16 with optional `0x`/`0X` prefix. Handle PETSCII letter ranges for hex digits. `strtoul` is the core implementation; `strtol` wraps it with sign handling.

### Fixed
- **Assembler (ca45)**:
    - **O45 relocation for equate-based ZP operands**: Fixed corrupt relocations when assembling instructions with equate-defined zero-page operands (e.g., `ptr = $20; lda ptr`). The O45 emitter was using the stale `stmt->instr.mode` (ABSOLUTE, the parser's default for unresolvable identifiers) instead of the resolved mode (BASE_PAGE, determined by `calculateInstructionSize` in pass 2). This caused 16-bit `R_WORD` relocations to be emitted for 1-byte ZP operands, corrupting both the operand byte and the adjacent opcode during linking. Fixed by re-resolving the addressing mode in the O45 emitter before generating relocations.
- **Compiler (cc45)**:

### Changed
- **Preprocessor (cp45)**:
    - Removed `#pragma include_once` directive. Use `#pragma once` instead (the standard form supported by GCC, Clang, and MSVC).

## [Unreleased] - 2026-05-04

### Added
- **Compiler (cc45)**:
    - **Fine-grained clobber tracking (Phase 1)**: The compiler now tracks which CPU registers (A, X, Y, Z) and processor flags (C, N, Z, V) each function actually modifies during code generation. Per-function `FuncClobberInfo` records are built incrementally as code is emitted. Accurate `.reg_clobbers` and `.flag_clobbers` directives are emitted for both ZP and stack calling conventions (previously only a blanket `A, X, Y, Z` was emitted for zpcall functions, nothing for stack convention). Leaf fastcall functions like `int identity(int x) { return x; }` now correctly report `.reg_clobbers A, X` instead of all four registers. Added `invalidateFromClobbers(regMask, flagMask)` for future Phase 2 selective invalidation at call sites.

### Fixed
- **Assembler (ca45)**:
    - **Assembler simulated op size drift**: Fixed a major source of address drift where simulated opcodes produced different code lengths between pass 1 and pass 2, especially when operands involved forward references. This was addressed by implementing a "conservative sizing" strategy: in pass 1, if an operand's type or value is uncertain, the assembler assumes the largest possible code path (e.g., stack-relative for `ptrstack`). This prevents instructions from growing in size during pass 2, which ensures convergence. Also added `isPass1()` to `AssemblerParser` for this purpose, and refined `isStackRelativeOperand` and similar operand evaluation logic to handle forward references gracefully during pass 1.
- **Compiler (cc45)**:
    - **ConstantFolder type preservation**: Updated `ConstantFolder` to track and propagate C types along with folded values. When a `long` variable is replaced by a folded `IntegerLiteral`, the literal now preserves the original `"long"` type in its `castType` field. This ensures that variadic functions like `printf` correctly push 4 bytes for constant-folded long arguments instead of defaulting to 2-byte `int`.
    - **Branch-aware constant invalidation**: Implemented constant state merging in the `IfStatement` visitor. Constants are now independently tracked through `then` and `else` branches and merged via intersection. A variable is only considered constant after an `if` if it holds the same constant value in all possible paths. Reassignments within branches now correctly invalidate any pre-existing constant assumptions.
    - **CompoundStatement scope leakage**: Removed incorrect state restoration at block end that was causing constants assigned within nested scopes to be incorrectly discarded.
- **Standard Library (stdlib45.lib)**:
    - **ZP Safety & Relocation**: Relocated all standard library assembly functions (`atoi`, `itoa`, `ltoa`, `malloc`, `memcpy`, `strcmp`) to use the hardware stack for ZP preservation. All library functions now push clobbered ZP registers to the hardware stack at entry and restore them via `plz / stz $xx` at exit. This eliminates ZP conflicts between the library and the compiler's temporary register pool ($02-$0A).
    - **Hardware Math Stability**: Added busy-wait loops on `$D7FE` bit 7 in `itoa` and `ltoa` to ensure the MEGA65 hardware divider has finished calculation before results are read. Fixes race conditions that produced empty or corrupted string output in `sprintf` / `%ld`.
    - **Preservation of Return Values**: Refactored restoration logic to use the Z register as a temporary scratch, ensuring that 16-bit (`AX`), 24-bit (`AXY`), and 32-bit (`AXYZ`) return values are fully preserved without clobbering the Accumulator.
    - **Stack Addressing Fixes**: Updated library functions to use correct stack-relative syntax (`(offset, sp), y` and `, s`) after ZP-push shifts.

## [Unreleased] - 2026-05-02

### Added
- **Compiler (cc45)**:
    - **`long` type**: Full support for 32-bit `long` and `unsigned long` integer types. Values use the Q register (AXYZ) (A=byte0, X=byte1, Y=byte2, Z=byte3). Supports global and local variable declarations, function parameters (4-byte push), function return values (hidden-pointer ABI, same as struct returns), arithmetic (`+`, `-`, `*`, `/`, `%`), bitwise (`&`, `|`, `^`, `~`), comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`), shifts (`<<`, `>>`), unary negation (`-`), increment/decrement (`++`/`--` via native `incq`/`decq` for globals), and type casts (widening via `sxt.16`/zero-extend, narrowing by truncation). `sizeof(long)` returns 4. Integer literal suffixes `L`/`l`/`U`/`u` are accepted. Global data emitted via `.dword` directive. Leverages native 45GS02 Q register operations (`ldq`/`stq`, `adcq`/`sbcq`, `andq`/`oraq`/`eorq`, `cmpq`, `aslq`/`lsrq`/`rolq`/`rorq`, `incq`/`decq`) for efficient 32-bit code on absolute addresses.
    - **Compound literals**: Support for C99 compound literals — `(struct Point){1, 2}`, `(int){42}`, `(int[]){1,2,3}`. Creates anonymous temporary values inline. Struct and array compound literals allocate frame temporaries and return their address; scalar compound literals produce the value directly. Works as variable initializers (`struct Point p = (struct Point){10, 20}`), function arguments (`foo(&(struct Point){1, 2})`), and in general expression contexts. New `CompoundLiteral` AST node with full support across Parser, CodeGenerator, ConstantFolder, FrameScanner, and all internal AST visitors.
    - **Struct initializer lists**: `struct Point p = {1, 2}` now initializes struct members in declaration order (by offset). Previously, struct locals with initializer lists only worked for arrays. The frame-local VariableDeclaration handler now detects struct types and initializes each member individually at its correct offset.
    - **Bitfields**: Full support for C-standard bitfield declarations in structs and unions. Syntax: `unsigned char mode : 3;`, `unsigned int counter : 10;`. Storage units are automatically packed — consecutive bitfields of the same type share a byte (for `char`) or word (for `int`), with a new unit started when the type changes or the field won't fit. Layout is conformant with C99 §6.7.2.1 (never spans storage unit boundaries). Read, write, increment/decrement, and arrow-access all work. `&struct.bitfield` is correctly rejected as a compile error.
- **Assembler (ca45)**:
    - **Bitfield pseudo-ops**: Eight new simulated opcodes for bitfield extract and insert:
        - `bfext #bitoff, #width` — extract bitfield from A (8-bit storage unit). Shifts right and masks.
        - `bfext16 #bitoff, #width` — extract from AX (16-bit storage unit). Result in A (with X=0 if ≤8 bits).
        - `bfins addr, #bitoff, #width` — insert A into absolute/ZP storage byte. Uses TRB+TSB for atomic read-modify-write on ZP and absolute targets.
        - `bfins.sp addr, #bitoff, #width` — insert into stack-relative storage byte. Uses shift/mask/ORA sequence.
        - `bfins.ind $zp, #bitoff, #width` — insert via indirect ZP pointer.
        - `bfins16` / `bfins16.sp` / `bfins16.ind` — 16-bit storage unit variants of the above.
    - All `bfins` variants use the 45GS02's TRB (Test and Reset Bits) and TSB (Test and Set Bits) instructions where possible (ZP and absolute addressing modes), providing atomic read-modify-write with fewer instructions than the general shift/mask/ORA approach.
    - **32-bit simulated opcodes**: Sixteen new `.32` and `.s32` pseudo-ops for 32-bit operations on the Q register (AXYZ): `add.32`, `sub.32`, `and.32`, `ora.32`, `eor.32`, `cmp.32`, `cmp.s32`, `neg.32`, `not.32`, `abs.32`, `lsl.32`, `lsr.32`, `rol.32`, `ror.32`, `asr.32`, `sxt.16` (sign-extend AX to AXYZ). Plus signed variants (`add.s32`, `sub.s32`, `neg.s32`, `abs.s32`, `asr.s32`, `lsl.s32`, `lsr.s32`, `rol.s32`, `ror.s32`). All use deferred branch patching (`emitBranchPlaceholder`/`patchBranchTarget`) for correct offset resolution. These complement the native Q register operations for cases requiring register-only or simulated addressing modes.
    - **Q register instruction aliases**: Added `adcq`, `sbcq`, `andq`, `oraq`, `eorq`, `cmpq`, `incq`, `decq`, `aslq`, `lsrq`, `rolq`, `rorq`, `negq` as recognized instruction mnemonics. These are NOP-NOP-prefixed versions of their 8-bit counterparts, operating on the full 32-bit Q register (AXYZ).
    - **`D#` parameter size prefix**: The `proc` directive now accepts `D#` (dword, 4 bytes) in addition to `B#` (byte) and `W#` (word) for parameter size declarations, enabling 32-bit parameter passing.
- **Standard Library (stdlib45.lib)**:
    - **`ltoa(long value, char *str, int base)`**: 32-bit long-to-string conversion. Uses the MEGA65 hardware divider's full 32-bit capability. Supports bases 2–36, signed negation for base 10. Declared in `<stdlib.h>`.
    - **`printf` / `sprintf` long format specifiers**: Added `l` length modifier for 32-bit arguments: `%ld` (signed decimal), `%lu` (unsigned decimal), `%lx` (hexadecimal), `%lo` (octal), `%lb` (binary, sprintf only). When `l` is present, 4 bytes are read from the variadic argument list and converted via `ltoa`.
    - **`sscanf(char *str, char *fmt, ...)`**: Formatted string input parsing. Supports `%d`, `%u`, `%x`, `%o`, `%c`, `%s`, `%%`, and long variants `%ld`, `%lu`, `%lx`, `%lo`. Handles leading whitespace, optional sign for `%d`, hex prefixes (`$`, `0x`) for `%x`, and whitespace-delimited strings for `%s`. Returns the number of successfully matched and assigned items. Compiled as a separate object file (`sscanf.o45`) so it is only linked when referenced. Declared in `<stdio.h>`.

### Fixed
- **Assembler (ca45)**:
    - **`ldq`/`stq` missing Q prefix**: `ldq` and `stq` were being assembled without the NOP-NOP prefix required by the 45GS02 to operate on the Q register, causing them to behave as regular `lda`/`sta`. Both sizing (pass1) and emission (pass2) now correctly emit the 2-byte Q prefix for `ldq`/`stq`.
    - **Q-suffixed instructions in accumulator mode**: Instructions like `lsrq`, `aslq`, `rolq`, `rorq` with no operand were rejected because the parser set IMPLIED mode while the opcode database only had ACCUMULATOR entries. Added fallback from IMPLIED to ACCUMULATOR in opcode lookup.
- **Compiler (cc45)**:
    - **PETSCII/ASCII encoding mismatch**: Character literals (`'A'`, `'d'`, etc.) now apply the same PETSCII case-swap conversion as the assembler's `.text` directive. Lowercase `'a'`–`'z'` map to `$41`–`$5A`, uppercase `'A'`–`'Z'` map to `$61`–`$7A`, matching the bytes emitted for string literals. This fixes comparisons like `*str == 'd'` when `str` points into a PETSCII-encoded string. Previously, char literals used raw ASCII values, causing mismatches.
    - **`-O0` global pointer initializers**: Global variables with non-literal initializers (e.g., `volatile char *r = (char *)0x4000`) now emit correct `.word`/`.byte` values with `-O0`. Previously, `-O0` skipped constant folding, leaving the initializer as a `CastExpression` which `emitData()` couldn't handle, falling back to `.res` (uninitialized). Added `tryEvalConstInt()` — a recursive evaluator that handles `CastExpression`, unary `-`/`~`, and binary arithmetic on constants.

### Testing
- Added `test_compound_literal.c` — mmemu validation test for compound literals (7 sub-tests): struct compound literal as variable initializer, scalar int/char compound literals, struct compound literal passed as pointer argument to function, struct with expression values, zero-init struct. Verified at `$4000`: `1E 2A 07 2C 01 14 00`.
- Added `test_bitfield.c` — compiler test validating bitfield struct declarations, read, write, increment, and 16-bit bitfields compile and assemble.
- Added `test_bitfield_mmemu.c` — mmemu runtime validation test for bitfields. Tests 8-bit bitfield write/read/increment (`active:1`, `mode:3`, `priority:4`) and 16-bit bitfield write/read (`counter:10`, `channel:6`). Verified at `$4000`: `01 05 0C 06 F4 1E`.
- Added `test_long.c` — compiler test validating long type declarations, arithmetic, function params/returns, unary ops, casting, sizeof, and bitwise operations compile and assemble correctly.
- Added `test_long_mmemu.c` — mmemu runtime validation test for long type (12 sub-tests): sizeof(long)=4, function call with 32-bit args and hidden-pointer return, unsigned comparison, low-byte extraction via cast, int↔long casting, 32-bit overflow (incq wrapping), and global-to-global 32-bit addition with full 4-byte verification (100000+200000=300000). Verified at `$4000`: `04 C0 01 A0 2A A0 00 E0 93 04 00 AA`.
- Added `test_32bit_ops.s` — assembler-level mmemu validation test for 32-bit operations: native Q register add (`adcq`), subtract (`sbcq`), bitwise OR (`oraq`), negation (`neg.32`), and sign extension (`sxt.16`). Verified at `$4000`: `E0 93 04 00 A0 86 01 00 E0 8F 03 00 60 79 FE FF` and at `$4010`: `FF FF FF FF`.

### Changed
- **Testing**:
    - Removed `while(1){}` infinite loops from test programs (`test_short.c`, `test_struct_return.c`, `test_inline_asm.c`, `test_mmemu_control.c`). Programs now return cleanly via RTS to the caller, matching standard CRT `exit_rts` behavior.

## [Unreleased] - 2026-05-01

### Added
- **Compiler (cc45)**:
    - **Frame pre-allocation**: Local variables are now pre-scanned and allocated as a single frame in the function prologue, replacing the incremental push-per-declaration model. A new `FrameScanner` AST walker computes `maxFrameSize` and assigns fixed frame offsets to each local variable with scope-aware slot reuse — variables in non-overlapping scopes (e.g. separate `{ }` blocks, if/else branches) share frame slots, reducing stack usage. The entire frame is zeroed at function entry via `PHW #0` instructions. Local variable initialization now writes directly to pre-allocated frame slots using `sta.fp`/`stax.fp` pseudo-ops. Only `_fp` needs bumping when pushing function call arguments — not every local variable individually. Parameters use `.var` at `frameSize + 2 + paramOff` (past frame + return address). Resolves the "scoped variable lifetime" issue from the 1.0 roadmap.
- **Assembler (ca45)**:
    - **`ldxy` / `stxy` simulated ops**: Load/store 16-bit values into the X(lo)/Y(hi) register pair. Supports immediate (`ldxy #$1234`), absolute (`ldxy addr`), and stack-relative (`ldxy var, s`) operands with frame-relative symbol detection. Useful for KERNAL calls that take pointer arguments in X/Y.
    - **`.local` directive**: Declares frame-relative variables with fixed offsets. Syntax: `.local name = offset`. Creates symbols with `isFrameRelative=true` and `frameOffset=offset`. Re-evaluated in pass 2 like `.var`. Used by the compiler's frame pre-allocation to define locals at compile-time-constant frame offsets.
    - **Frame-pointer pseudo-ops**: Six new simulated ops for frame-relative variable access:
        - `lda.fp offset` — 8-bit load from frame slot into A (`TSX; LDA spBase+_fp+offset,X`)
        - `sta.fp offset` — 8-bit store from A to frame slot
        - `ldax.fp offset` — 16-bit load from frame slot into AX (hi→scratch, lo→A, scratch→X)
        - `stax.fp offset` — 16-bit store from AX to frame slot (X→scratch, A→stack, scratch→stack+1)
        - `leax.fp offset` — load effective address of frame slot into AX
        - `move.fp dest, src, len` — DMA block copy between frame-relative addresses
    - All frame ops use direct stack access (`TSX; LDA/STA base+offset,X`), not the `($nn,SP),Y` indirect mode.

- **Compiler (cc45)**:
    - **`short` type**: `short`, `unsigned short`, and `signed short` are now accepted as type specifiers in all contexts: variable declarations, function parameters, return types, cast expressions, `sizeof`, and `_Generic` associations. On this 16-bit target, `short` is an alias for `int` (2 bytes). Added `SHORT` token type, lexer keyword, and parser support across all type-parsing locations.
- **Standard Library (stdlib45.lib)**:
    - **`sprintf(buf, fmt, ...)`**: Variadic formatted string output. Supports `%d` (signed decimal), `%u` (unsigned decimal), `%x`/`%X` (hex), `%s` (string), `%c` (char), `%%` (literal percent), `%o` (octal), `%b` (binary), `%p` (pointer with `$` prefix). Written in C, compiled to .o45, selectively linked. Uses `itoa` for numeric conversions.
    - **`printf(fmt, ...)`**: Variadic formatted output to screen via `putchar`. Same format specifiers as `sprintf`. Does not append newline. Written in C.
- **Headers**:
    - **`stdio.h`**: Added `sprintf` and `printf` declarations.
    - **`cbm.h`**: New header providing inline C wrappers for the standard Commodore KERNAL jump table. 25 functions covering character I/O (`cbm_chrout`, `cbm_chrin`, `cbm_getin`), I/O status (`cbm_readst`), file operations (`cbm_setlfs`, `cbm_setnam`, `cbm_open`, `cbm_close`, `cbm_chkin`, `cbm_chkout`, `cbm_clrchn`, `cbm_clall`), keyboard (`cbm_stop`), screen control (`cbm_gotoxy`, `cbm_wherexy`, `cbm_screen`), system clock (`cbm_rdtim`), and serial bus/IEC (`cbm_listen`, `cbm_unlisten`, `cbm_talk`, `cbm_untalk`, `cbm_second`, `cbm_tksa`, `cbm_ciout`, `cbm_acptr`). All implemented as `static` functions with `__asm__` for zero call overhead. Also defines KERNAL address constants (`KERNAL_CHROUT`, etc.), PETSCII control codes (`CH_RETURN`, `CH_CLEAR`, colors), and device numbers (`DEV_DISK_8`, etc.). Compatible with C64, C128, and MEGA65 KERNAL.
    - **`stdlib.h`**: Added `atos(s)` and `stoa(value, str, base)` convenience macros for `short` string conversion. `atos` casts through `atoi`; `stoa` casts through `itoa`.
    - **Return struct by value**: Functions can now return structs by value (e.g. `struct Point make_point(int x, int y)`). Uses a hidden-pointer ABI: the caller passes the address of the destination as an implicit last parameter (`_p___ret_ptr`); the callee copies the return value through this pointer before returning. The caller computes the destination address via `leax.fp` before pushing regular arguments. Struct-returning function calls used as initializers (`struct Point p = make_point(1,2)`) write directly to the local's frame slot — no intermediate copy. `getExprType` now returns the struct type for calls to struct-returning functions. `FunctionDeclaration` AST node gains `returnPointerLevel` field. New tracking maps `functionReturnTypes` and `structReturningFunctions` in CodeGenerator.

### Fixed
- **Assembler (ca45)**:
    - **BSS architecture rework**: `__bss_start`/`__bss_end` are now linker-defined symbols (injected by `ln45` at the merged BSS segment boundaries), not emitted by individual compilation units. The compiler no longer emits these labels in relocatable mode — each `.o45` module only emits `.res` directives and `.global` exports for its BSS variables. A new `crt_bssinit.o45` CRT module provides `_init_bss` which references the linker-provided `__bss_start`/`__bss_end` via `.extern`. In flat binary mode, the compiler still emits inline `_init_bss` and BSS boundary labels. This eliminates duplicate symbol conflicts when linking C-compiled library modules.
    - Fixed constant propagation not invalidated by address-of — when `&x` is applied to a non-const variable, the ConstantFolder now removes `x` from `knownConstants` and tracks it in `addressEscapedVars` to prevent re-propagation. Previously, `set_val(&x, 42); return x;` would return the pre-call value instead of 42. `const` variables are exempt since they can't be modified through pointers.
    - Fixed O45 reader `$FF` escape byte handling in relocation tables — `parseRelocTable` treated every byte as a `delta, typeSeg` pair, but `$FF` is a skip-254 escape with no type/seg byte following. Large C-compiled modules with relocations >254 bytes apart produced `$FF` escapes that desynchronized the reader, causing "truncated import name" errors and preventing linking. Fixed by skipping the type/seg read when delta is `$FF`.
    - Fixed string literal address loading — `ldax #<STR0` only loaded the low byte (hi byte hardcoded to 0). Changed to `ldax #STR0` for full 16-bit immediate. Any program with string literals in the `.data` segment above page 0 was affected.
    - Fixed expression parser greedily consuming `, s` stack-relative suffix after numeric literals inside binary expressions. `_l_p+0, s` was parsed as `_l_p + (stack_var_0)` instead of `(_l_p+0), s`, causing `stax`/`ldax` to emit absolute stores to address $0000 instead of stack-relative stores. Fix: only consume `, s` after a numeric literal when it is the first token in the expression (not an operand of a binary operator).
    - Fixed `lda_frame`/`sta_frame` in M65Emitter — previously used `($nn,SP),Y` indirect addressing (reads a pointer from the stack then dereferences it), which is wrong for direct stack variable access. Now uses `TSX; LDA/STA spBase+offset,X` (direct stack-relative).
    - Fixed 16-bit frame-relative stores (`stax.fp`, STW frame path) — `TSX` clobbers X before the hi byte can be stored. Now saves X to scratch ZP first, matching the pattern used by non-frame 16-bit stores.
    - Fixed `ptrstack` for frame-relative symbols — previously tried to read a pointer from the stack (indirect). Now computes `SP + spBase + fpOff + yOff` directly (same pattern as stack-relative `ptrstack`).
    - Fixed all `resolveFrameAccess`-based code paths in `emitLDWCode`, `emitSTWCode`, `emitPHWStackCode`, `emitStackIncDecCode`, `emitNegNot16Code`, `emitABS16Code`, and `AssemblerExpression.cpp` to use direct stack access instead of `($nn,SP),Y` indirect.

- **Compiler (cc45)**:
    - **Variadic functions**: Full support for variadic function declarations and calls using `...` syntax. Includes `<stdarg.h>` with `va_list`, `va_start`, `va_arg`, and `va_end`. Variadic calls use right-to-left argument pushing so named parameters have fixed stack offsets regardless of how many variadic arguments are passed. All variadic arguments are promoted to 16-bit (default argument promotions per C standard). `va_start` computes the actual stack memory address of the first variadic argument; `va_arg` reads via ZP-indirect `(ZP),Y` addressing and advances the pointer by 2 bytes. `va_end` is a no-op. New AST nodes `BuiltinVaStart` and `BuiltinVaArg` handle the `__builtin_va_start` and `__builtin_va_arg` intrinsics. Parser accepts `...` after the last named parameter in function declarations and prototypes.
- **Headers**:
    - **`stdarg.h`**: Added `va_list` typedef (`unsigned int`) and macros `va_start`, `va_arg`, `va_end` wrapping compiler builtins.

### Testing
- Added `test_short.c` — mmemu validation test for `short` type (7 sub-tests): arithmetic, signed, sizeof, function params/returns, pointers, arrays. Verified at `$4000`: `1E 05 02 0C 0A C8 AA`.
- Added `test_struct_return.c` — mmemu validation test for struct return by value (6 sub-tests): basic struct return with member access, multiple struct returns, and using returned struct values. Verified via memory dump at `$4000`: `01 02 03 04 0A 14 AA`.
- Added `test_variadic.c` — mmemu validation test for variadic functions (10 sub-tests): sum of 3/1/0/5 values, max finding, multiple named params before `...`, zero variadic args consumed, large values (>255), and result in expressions. Verified via mmemu (A=$00).

## [Unreleased] - 2026-04-30

### Added
- **Compiler (cc45)**:
    - **Array initializer lists**: Support braced initializer lists for array declarations: `int arr[5] = {1, 2, 3};`. Works for both global and local arrays, `char` and `int` element types. Partial initialization zero-fills remaining elements. Empty initializer `= {}` zero-fills the entire array. Trailing commas are allowed. Nested initializer lists are parsed (for future multi-dimensional support). Global arrays emit `.byte`/`.word` directives per element; local arrays push elements onto the stack. Validated via mmemu emulator with global/local char/int arrays, partial initialization, and zero-fill.
    - **`register` storage class**: Support for the `register` keyword on local variable declarations. Register variables are allocated in zero page instead of on the stack, providing faster access (shorter instructions, fewer CPU cycles). The compiler uses the existing zero-page register pool (`allocateZP`), dynamically growing it as needed up to the ZP limit ($FF). When no ZP space is available, the compiler silently falls back to normal stack allocation (per the C standard, `register` is a hint). Register variables bypass the stack frame entirely — they don't consume stack space, don't participate in `.var` offset bumping, and don't require cleanup on function return. Supported in regular declarations and `for` loop initializers (`for (register int i = 0; ...)`). Arrays and structs are excluded. Optimizations like `inc`/`dec`/`inw`/`dew` work naturally with ZP addresses.
    - **`inline` keyword (no-op)**: The `inline`, `__inline`, and `__inline__` function specifiers are now parsed and accepted. Currently treated as a no-op — functions are emitted normally. Actual inlining is planned for a future release.
- **Assembler (ca45)**:
    - **`.array` directive**: Declares multi-dimensional arrays with automatic storage reservation and metadata. Syntax: `.array name, element_size, dim0 [, dim1 [, dim2 ...]]`. Reserves `element_size × product(dims)` bytes and defines compile-time constants for each dimension: `name.__elsize`, `name.__dims`, `name.__dimN`, `name.__strideN`. Strides are computed in row-major order (stride[i] = product of subsequent dimensions × element size). Supports arbitrary dimensions and element sizes (1=byte, 2=word, etc.).
    - **Array indexing in `expr`**: The expression evaluator now supports `name[index]` and `name[i][j][...]` syntax for arrays declared with `.array`. Constant indices are resolved at assembly time to direct `LDA addr` instructions. Runtime indices generate optimized code: bare `.X` register with stride=1 emits a single `LDA base,X`; power-of-2 strides use ASL shifts; arbitrary strides use the MEGA65 hardware multiplier ($D770). Multi-dimensional runtime indexing accumulates partial offsets on the hardware stack with no scratch memory conflicts. Mixed constant/runtime indices are supported (constant dimensions are folded into the base address).
- **Compiler (cc45)**:
    - **`static` keyword**: Full support for the `static` storage class specifier:
        - **Static global variables**: File-scope internal linkage. Storage is allocated normally but the symbol is not exported (no `.global` in relocatable mode).
        - **Static functions**: File-scope internal linkage. Function body is emitted but not exported.
        - **Static local variables**: Persistent storage across function calls. Allocated in the `.data`/`.bss` segment (not on the stack) with mangled names (`_funcname__varname`) to avoid collisions. Initializers are applied once at program load, not on each function entry.
    - **Multi-dimensional array declarations**: Support `int a[3][4]` row-major layout. The parser accepts multiple `[N]` suffixes on variable and struct member declarations. The type system stores dimension vectors (`arrayDims`) and computes correct strides at each indexing level. `sizeof` returns the full array size. `getExprType` adds array dimensions to pointer level for correct type propagation through chained `a[i][j]` access. `emitAddress` walks the `ArrayAccess` chain to determine the dimension depth and computes the appropriate stride (product of remaining dimensions × element size). Validated via mmemu with 1D and 2D array store/read tests.
    - **Struct arrays**: Support `struct point pts[10]` with subscript indexing and member access (`pts[i].x`). Constant and runtime-indexed stores/reads, loop-based initialization, and `sizeof` all work correctly. Validated via mmemu.
    - **CRT `_init_bss` placement**: Moved `_init_bss` (with BSS zeroing code) into the CRT stub area before function code, using a pre-scan of global declarations to determine if BSS zeroing is needed.
    - **`#pragma crt heap`**: Opt-in pragma that links the heap CRT module (`crt_heap.o45`) and calls `_init_heap_crt` during startup (before `_init_features`). Enables `malloc`/`free`/`calloc`/`realloc` from `stdlib.h`.
    - **`#pragma crt stdio`**: Pragma recognized by preprocessor and CodeGenerator (sets `crtStdio` flag). Module not yet implemented.
    - **Unified CRT startup**: The CRT inline stub is now emitted for both relocatable (`-c`) and flat binary modes. In relocatable mode, `__init`, `__sp_base`, `_init_features`, `__bss_start`, and `__bss_end` are exported via `.global`. The startup sequence now supports optional `_init_heap_crt` and `_init_stdio_crt` calls controlled by pragmas.
    - **`M65Emitter::emitDirective` / `emitRaw`**: New methods for emitting assembler directives and raw text lines in TEXT mode.
- **Standard Library (stdlib45.lib)**:
    - **`malloc(size_t size)`**: First-fit allocator with sorted free list and block splitting. Block header is 4 bytes (2-byte size with allocated bit, 2-byte next pointer). Minimum allocation 4 bytes, 2-byte aligned. Auto-initializes heap on first call if `_heap_init` hasn't run.
    - **`free(void *ptr)`**: Returns block to sorted free list with forward and backward coalescing of adjacent free blocks.
    - **`calloc(size_t nmemb, size_t size)`**: Allocates via `malloc(nmemb * size)` then zeros with `memset`.
    - **`realloc(void *ptr, size_t size)`**: Allocates new block, copies min(old, new) bytes via `memcpy`, frees old block. `realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` = `free(p)`.
    - **`_heap_init`**: Internal initializer. Uses weak `__heap_start`/`__heap_end` symbols (defaults to `__bss_end`..`$D000`). Creates single free block spanning the heap.
    - **`abs(int value)`**: Returns 16-bit absolute value via complement-and-add.
    - **`rand(void)`**: Returns pseudo-random int 0-32767 using MEGA65 hardware RNG at `$D7EF`. Busy-waits on `$D7FE` bit 7 for RNG stabilization before each byte read.
    - **`srand(unsigned int seed)`**: No-op stub for C standard compatibility (hardware RNG cannot be seeded).
    - **`atoi` / `itoa`**: Already implemented (previously unlisted in changelog).
- **CRT Archive (crt45.lib)**:
    - New `crt45.lib` archive built from CRT modules (`crt0.o45`, `crt0_mega65.o45`, `crt_heap.o45`). The `crt_heap.o45` module exports `_init_heap_crt` which calls `_heap_init` from `malloc.o45`.
- **Headers**:
    - **`stddef.h`**: Added `size_t` typedef (`unsigned int`).
    - **`stdlib.h`**: Added `malloc`, `free`, `calloc`, `realloc` declarations. Added `abs`, `rand`, `srand` declarations. Now includes `stddef.h` for `size_t`.
- **mmemu (emulator)**:
    - Added MEGA65 hardware RNG registers to the math accelerator device: `$D7EF` (random byte, advances LFSR on read) and `$D7FE` (bit 7 = not-ready, always 0 in emulator). Uses a 32-bit Galois LFSR for deterministic pseudo-random output.

### Fixed
- **Assembler (ca45)**:
    - Fixed infinite hang when assembling files with duplicate `@` local labels across different routines (e.g., `@loop:` in two functions without `proc` scoping). The labels were stored unscoped in the symbol table, causing the second definition to overwrite the first. Pass2's convergence loop then oscillated the symbol value between two addresses indefinitely. Fix: `@` labels are now automatically scoped under the nearest preceding non-`@` label (e.g., `_myfunc:@loop`). The scope is stored per-statement so it is correctly restored during both pass2 address recalculation and code generation (which iterates by segment, not source order). Duplicate `@` labels within the same scope are now detected and reported as an error. Additionally, pass2's convergence loop is now capped at 100 iterations to prevent hangs from any remaining edge cases.
- **Compiler (cc45)**:
    - Fixed `register` variables being passed to functions via `phw.sp` (stack-relative push), which referenced an undefined `.var` symbol. Register variables are now loaded from their ZP address into A:X and pushed via `push .ax`.
    - Fixed `ConstantFolder` not copying the `isRegister` flag when rebuilding `VariableDeclaration` nodes, causing register variables to silently fall back to stack allocation after constant folding.
- **Assembler (ca45)**:
    - Fixed `.var` non-positional evaluation — `.var` ASSIGN directives were not re-evaluated during pass 2, causing all references to see the final pass-1 value regardless of source position. Temporary stack offset bumps (`.var x = x + 1` around a `pha`, then `.var x = x - 1` after `pla`) produced incorrect offsets for references between the bumps. Fix: re-evaluate `.var` ASSIGN expressions in source order during pass 2. The existing variable reset (resetting to `initialValue` before pass 2) prevents the double-evaluation bug that originally motivated removing pass-2 re-evaluation.
    - Fixed `inw`/`dew` increment optimization emitting base-page-only opcodes for global (absolute address) variables. The 45GS02 `INW` instruction only supports base page addressing; for 16-bit globals, the compiler now emits `INC addr; BNE *+5; INC addr+1` (and the inverse for decrement). 8-bit globals correctly use `INC`/`DEC` which support absolute addressing natively.
    - Fixed `sizeof` for struct/pointer arrays — `sizeof(pts)` on a `struct Point pts[4]` returned `pointer_size * count` (8) instead of `struct_size * count` (16). The array-decayed `pointerLevel` was used instead of the raw type's pointer level.
- **Assembler (ca45)**:
    - Fixed `ptrstack` for global variables — now detects global/absolute symbols and emits `LDA #lo; LDX #hi` (4 bytes) instead of the stack-relative `TSX; TXA; CLC; ADC...` sequence (9 bytes). Stack variables (`.var`-defined) continue to use the SP-relative path. This fixes incorrect address computation when `ptrstack` was used on global array/pointer labels.
    - Fixed `ptrstack` missing from pass 1 and pass 2 sizing chains — `PTRSTACK` was omitted from both the initial size calculation and the pass 2 re-sizing loops, causing it to have size 0 in pass 1. This produced cumulative address drift for all labels following any `ptrstack` instruction, leading to BRK crashes in programs with global arrays.
    - Fixed `mul.16`/`div.16` result-reading loop clobbering the low byte when loading the high byte into register destinations (`.AX`, `.AXY`, `.Q`). Any multiply/divide result > 255 had its low byte zeroed. Fixed by reading high bytes first, then low byte last.
    - Fixed `mul.16`/`div.16`/signed variants treating bare addresses as immediates — `mul.16 .ax, $04` was "multiply by 4" instead of "multiply by contents of $04". Added `isImmediate` check requiring `#` prefix, consistent with `add.16`/`sub.16`.
- **Compiler (cc45)**:
    - Fixed `_init_bss` address resolution — moved BSS init routine into the CRT stub area so its address is computed before any function code, avoiding assembler simulated opcode size drift.
    - Fixed `emitAddress` stride multiplication — now uses `mul.16 .ax, #stride` (immediate) instead of storing the index to ZP and passing the ZP address, which was misinterpreted as a literal constant by the assembler's expression parser.
    - Fixed assignment RHS save using `push .ax` which shifted SP, breaking stack-relative index reads inside `emitAddress`. Now saves RHS to an `allocateZP`-managed ZP slot instead, keeping SP unchanged.
    - Fixed `inc a; bne(0x02); inx` carry propagation — branch offset was 1 byte too large, jumping into the middle of the next instruction. Changed to `bne(0x01)` to correctly skip the 1-byte INX.
    - Fixed global integer initializer hex formatting — `int a = 3` emitted `.word $3000` instead of `.word $0003` due to `std::left` stream alignment leaking from a previous output statement. Added explicit `std::right`.

### Testing
- Added `test_multidim_array.c` — mmemu validation test for multi-dimensional arrays. Tests 1D array read (`scores[2]`), 2D constant-index store and read (`grid[1][2]`, `grid[2][3]`, `grid[0][0]`), and `sizeof` for 2D arrays. Verified via memory dump at `$4000`: `03 0C 17 00 18 AA`.
- Added `test_array_loop.c` — mmemu validation test for runtime-indexed global array stores via loops. Tests 1D loop fill (`scores[i] = i+1`), 2D nested loop fill (`grid[i][j] = i*10+j`), and reads of both. Verified via memory dump at `$4000`: `01 05 00 0C 17 AA`.
- Added `test_array.s` — assembler test for `.array` directive and `expr` array indexing (constant and runtime indices, multi-dimensional, stride metadata constants).
- Added `test_pragma_heap.c` — compiler test validating `#pragma crt heap` with `malloc`/`free` usage compiles and assembles.
- Added `test_malloc.c` — compiler test validating `stdlib.h` heap function declarations (`malloc`, `free`, `calloc`, `realloc`) compile and assemble.
- Verified `malloc.s` assembles in relocatable mode (`ca45 -c`) with correct symbol exports (`_malloc`, `_free`, `_calloc`, `_realloc`, `_heap_init`).
- Verified `crt_heap.s` assembles in relocatable mode with `_init_heap_crt` export.
- Verified `lib/Makefile` builds both `crt45.lib` (3 members) and `stdlib45.lib` (28 members) successfully.
- Added `test_register.c` — mmemu validation test for `register` keyword (16 sub-tests): int/char initializers, assignment, loop accumulation, mixed register/stack variables, function argument passing, function return values, large literals, increment/decrement, nested scopes, `for`-loop init, and self-assignment.
- Added `test_register.sh` — 16 assembly-output pattern tests validating ZP allocation comments, `.var` absence, direct ZP store/load patterns, `inw`/`inc` optimizations, stack cleanup reduction, distinct ZP addresses, array/struct fallback, and full pipeline assembly.

## 2026-04-29

### Added
- **Compiler (cc45)**:
    - **`const` type qualifier**: Full support for the `const` keyword on local variables, global variables, function parameters, and struct members. Distinguishes between `const int *p` (pointer to const — prevents `*p = x`), `int *const p` (const pointer — prevents `p = &b`), and `const int *const p` (both). Assignment to `const`-qualified lvalues produces a compile error (`"Assignment to read-only location"`). Increment/decrement (`++`/`--`) of `const` variables and members is also rejected. Casting away const (`(int *)&const_var`) is allowed per C standard. Enforcement occurs in both the ConstantFolder (catches violations before optimization removes them) and the CodeGenerator (catches violations on non-folded paths). The `const` qualifier propagates through `getExprType()` for variable references, member access, array access, pointer dereference, and address-of.
    - **Const-correctness warnings**: Passing a pointer to a `const` variable to a function parameter that lacks `const` (`void f(int *p)` called with `f(&const_var)`) emits a warning: `"discards 'const' qualifier from pointer target type"`. Function parameter types are tracked via `functionParamTypes` for cross-function const checking.
- **Assembler (ca45)**:
    - **`.const` directive**: Defines an immutable named constant (`.const NAME = expr`). Redefinition of a `.const` symbol (via `.const`, equate, or `.var`) is an error. Constant symbols are marked `isConstant` in the symbol table to distinguish them from mutable `.var` symbols and equates.
- **Testing**:
    - Added `test_const_compiler.sh` — 15 compiler tests: assignment to const local/global/struct member/parameter, increment of const, write through const pointer, casting away const, const parameter read-only enforcement, reading through const pointer, const-to-non-const pointer warning, `const int *p` allows pointer reassignment, `int *const p` prevents reassignment, `int *const p` allows write-through, `const int *const p` prevents both, and valid const usage.
    - Added `test_const_assembler.sh` — 4 assembler tests: basic `.const` usage with binary verification, redefinition of `.const`, redefinition via equate, and redefinition via `.var`.
- **Compiler (cc45)**:
    - **Relocatable stack base (`__sp_base`)**: Replaced all hardcoded `$0101` references in `M65Emitter` and `AssemblerSimulatedOps` with the configurable `__sp_base` symbol (default `$0101`). Predefined in both `ca45` and `cc45`; overridable via `-D__sp_base=$NNNN`. In relocatable mode (`-c`), `__sp_base` can be declared `.extern` for linker resolution, with `R_WORD` relocations generated at all stack-access sites. Also fixed a pre-existing bug in `ptrstack` where the high byte was hardcoded to 0 instead of the stack page.
    - **`#pragma crt no_0100_stack`**: Emits `.extern __sp_base` in relocatable mode so the linker can resolve the stack base address for non-page-$01 stacks.
    - **`#pragma crt exit_rts|exit_halt|exit_brk`**: Select program exit behavior after `main` returns. `exit_rts` (default): saves/restores the caller's full 16-bit SP using self-modifying code (SP values stored in the immediate operands of the restore instructions — zero extra bytes). Provides `__exit` label for `exit()`. `exit_halt`: infinite `bra` loop. `exit_brk`: single BRK instruction. Setting one unsets the others.
    - **`#pragma crt no_bssinit`**: Controls BSS zeroing at startup. Default behavior emits `_init_bss` which zeros the BSS segment via a 16-bit pointer loop (`__bss_start` to `__bss_end`). Pragma suppresses the zeroing, emitting only an `rts` stub. Init sequence: `_init_bss` → `_init_features` → `_main`.
    - **Frame pointer removal**: Eliminated the 5-byte `proc` prologue (`TSX; LDA #page; PHA; PHX`) and 2-byte `endproc` epilogue (`PLA; PLA`). Parameters now use direct stack-relative addressing (`TSX; LDA __sp_base+offset,X`) — same as locals — instead of indirect `($nn,SP),Y` via a saved frame pointer. Params are added to `currentVars` and bumped by `.var` alongside locals. `_fp` starts at 0 (was 1). Saves 7 bytes code + 2 bytes stack per function call.
- **Standard Library (stdlib45.lib)**:
    - **`stdlib.h` / `exit()`**: `_Noreturn void exit(int status)` — loads status into `.AX` and jumps to `__exit` (CRT-provided). Works with all exit modes.
    - **`string.h`** — 12 hand-written 45GS02 assembly functions: `strlen`, `strcpy`, `strncpy`, `strcmp`, `strncmp`, `strcat`, `strchr`, `strrchr`, `memcpy`, `memmove`, `memset`, `memcmp`. All use the frame-pointer-free convention (`.var _fp = 0`).
    - **`ctype.h`** — 7 PETSCII-aware character functions in hand-written 45GS02 assembly: `isdigit`, `isalpha`, `isalnum`, `isspace`, `isprint`, `toupper`, `tolower`. All use inline range checks (no lookup table). `toupper`/`tolower` convert between PETSCII lowercase (`$41`–`$5A`) and uppercase (`$C1`–`$DA`) via bit manipulation.
    - Updated existing stdlib modules (`putchar`, `puts`, `strlen`, `memset`) to the new frame-pointer-free convention.
- **Testing**:
    - Added `test_bssinit.c` — mmemu validation test for BSS zeroing. Pre-fills RAM with `$DE` garbage, verifies uninitialized globals start at zero and assigned values are correct.
    - Added `test_strlen.c` — validates `strlen` with 4 cases (empty, 1-char, 5-char, 10-char).
    - Added `test_strcmp.c` — validates `strcmp` with 6 cases (equal, empty, less-than, greater-than, prefix).
    - Added `test_strcpy.c` — validates `strcpy` with `strcmp`/`strlen` verification.
    - Added `test_memcpy.c` — validates `memset`, `memcpy`, `memcmp` with 6 cases.
    - Added `test_strchr.c` — validates `strchr`/`strrchr` with 5 cases (first, last, not-found).

### Optimized
- **Compiler (cc45)**:
    - **Granular Store Optimization**: The compiler now individually tracks the `A` and `X` registers during 16-bit assignments. If either register already contains the correct byte for the target variable, the corresponding `sta` or `stx` instruction is omitted. Standalone 8-bit stores are similarly optimized.
    - **Self-Assignment Elimination**: Standalone `x = x;` statements for non-volatile variables are now completely optimized out.
    - **Improved Register Reuse**: Removed aggressive register tracker invalidation after local variable declarations, allowing registers initialized during variable setup to be reused by subsequent statements.

### Fixed
- **Compiler (cc45)**:
    - **Volatile Correctness**: Fixed a bug where `volatile` variable loads were being optimized away by the register tracker. Volatile variables now strictly result in explicit memory reads and writes as required by the C standard.
    - **Stack-relative LDX/LDA suffix**: Ensured that granular 16-bit loads for local variables correctly use the `, s` stack-relative suffix.

### Changed
- **Assembler (ca45)**:
    - `proc` now emits 0 bytes (was 5). No prologue — it only establishes scope and creates parameter symbols.
    - `endproc` emits only `RTS`/`RTS #n` (was `PLA; PLA; RTS`/`RTS #n`). Size: 1-2 bytes (was 3-4).
    - Parameter symbols created by `proc` are no longer `isFrameRelative`. Offsets start at 2 (past return address) instead of frame-relative Y offset 3.
    - `__sp_base` added as a predefined symbol (`$0101`) in both `ca45` and `cc45`.
    - All simulated stack ops (`lda.sp`, `sta.sp`, `inw.sp`, `dew.sp`, `phw.sp`, `ptrstack`, `neg.16`/`not.16`/`abs.16` on stack, DMA `fill.sp`/`move.sp`) use `M65Emitter::spBase()` instead of hardcoded `$0101`.

## [0.99.dev] - 2026-04-28

### Added
- **Suite-wide**:
    - **`-V` / `--version` flag** on all tools (`cc45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`). Prints `toolname v0.99.dev (githash)` where the git hash is embedded at compile time.
    - `include/Version.hpp` centralizes the version string; Makefile passes `-DGIT_HASH` automatically.
- **Linker (ln45)**:
    - **`-M <file>` — Linker map file**: Writes a detailed map file after linking, showing merged segment layout (address ranges and sizes), per-object segment contributions (which file contributes which address range), and all resolved symbols sorted by address with source file and weak/strong annotations.
- **Utilities**:
    - **`objdump45` — Object File Inspector & Disassembler**: Displays file headers (`-f`), section headers (`-h`), symbol tables (`-t`), relocation entries (`-r`), hex dumps (`-s`), and full 45GS02 disassembly (`-d`) from `.o45` and `.o65` object files. Also supports `.prg` files (auto-detects 2-byte load address) and raw `.bin` files (with `-b ADDR` for base address). Disassembly features symbolic labels at export addresses, branch/call target annotations with symbol names, and support for all 45GS02 addressing modes including `($nn,SP),Y`, `[zp],Z`, 16-bit branches, `PHW #imm16`, and `RTS #n`. Flags can be combined (`-fdt`); `-a` shows all. Built from a reverse lookup of the assembler's opcode database.
- **Testing**:
    - Added `test_objdump45.sh` — 113 assertions validating all `objdump45` display modes: version/help output, error handling (missing files, bad format, no flags), file header fields, section headers, symbol table (imports, exports, weak symbols, type letters), relocation entries, hex dump contents, disassembly (mnemonics, operand formatting, all addressing modes), branch target annotations, symbol labels, combined flags, multi-file output, PRG disassembly (auto-detected load address, `-b` override, hex dump), raw `.bin` disassembly (with `-b` base address, default base), and BASIC upstart PRG handling. Integrated into `make test` and available standalone via `make test-objdump45`.
- **Assembler (ca45)**:
    - **`mul.s16` / `div.s16`** — Signed 16-bit multiply and divide. Wraps the MEGA65 hardware math unit ($D770+/$D760+) with automatic sign correction: saves sign XOR, takes absolute values, performs unsigned operation, negates result if needed. Uses $D76E as sign scratch byte.
    - **`mod.16`** — Unsigned 16-bit modulo. Performs `div.16` then reads the hardware remainder registers ($D770/$D771).
    - **`mod.s16`** — Signed 16-bit modulo. Sign of result follows the dividend (C99 semantics).
- **Testing**:
    - Promoted 19 tests to the mmemu execution-validation suite: `test_ptr_arith`, `test_ptr_ptr`, `test_complex_math`, `test_struct`, `test_opt_struct`, `test_large_struct`, `test_anon_struct`, `test_union`, `test_global_vars`, `test_global_res`, `test_switch_continue`, `test_goto_sizeof`, `test_signed_cc`, `test_sub`, `test_struct_param`, `test_typedef`, `test_generic`, `test_strength_reduction`, `test_volatile`. Total mmemu-validated tests: 47 (up from 28).
    - Added `test_ptr_precedence.c` — validates pointer operator precedence and indirect increment/decrement: `(*p)++`, `++*p`, `(*p)--`, `--*p`, `*p++`, `arr[i]++`, and `char*` variants (14 test cases, mmemu-validated).

### Fixed
- **Compiler (cc45)**:
    - **Indirect increment/decrement**: `++`/`--` on dereferenced pointers (`(*p)++`, `++*p`), array elements (`arr[i]++`), and arrow-accessed members (`p->field++`) now correctly emit load-modify-store sequences through the pointer. Previously, the CodeGenerator's fallback path for non-variable lvalue operands evaluated the expression but discarded the increment/decrement, producing no side effect.
    - **Signed right-shift**: `>>` on signed types now emits `asr.16` (arithmetic shift right, preserving sign bit) instead of `lsr.16` (logical shift right). Affects four code paths: constant-count shifts, dynamic-count shifts in `emitOperation()`, and dynamic-count shifts in `BinaryOperation` visitor.
    - **Signed multiply/divide/modulo**: The compiler now emits `mul.s16`, `div.s16`, `mod.s16` for signed operands and `mod.16` for unsigned modulo (replacing direct `$D770`/`$D771` hardware register reads). Applies in `emitOperation()`, constant-literal paths, and general `BinaryOperation` visitor.
    - **Power-of-2 division optimization**: The shift-right optimization (`/ 4` → `lsr.16` × 2) is now skipped for signed types, which require proper truncation-toward-zero semantics. Signed divisions fall through to `div.s16`.

---

## [Unreleased] - 2026-04-27

### Added
- **Compiler (cc45)**:
    - **Relocatable object mode (`-c`)**: `cc45 -c input.c -o output.o45` compiles C to a `.o45` relocatable object file. The CodeGenerator auto-emits `.global` for defined functions and global variables, `.extern` for called-but-not-defined functions. Skips the `.org $2000` and startup stub in reloc mode. The `-o` flag controls the final `.o45` name; intermediate `.s` file is generated automatically.
    - **`#pragma weak`**: Marks the next function or global variable as a weak export. The preprocessor converts `#pragma weak` to an internal `.weak_next` marker; the CodeGenerator emits `.weak` instead of `.global`. Weak symbols can be overridden by strong definitions at link time.
    - **Frame-pointer-relative parameter access**: Function parameters are now accessed via a saved frame pointer using the 45GS02's native `($nn,SP),Y` addressing mode (opcodes $E2/$82). The `proc` prologue saves SP as a 16-bit LE pointer on the stack (`TSX; LDA #$01; PHA; PHX`). Parameters get fixed Y offsets that never change as locals are pushed, eliminating the need for `.var` offset bumping on parameters.
    - Added `_fp` assembler variable that tracks the frame pointer's stack position, automatically adjusted by `.var` as locals are declared.
    - Added `test_many_params_locals.c` — validates functions with >2 parameters (up to 5) and >2 local variables (up to 6), including mixed char/int params, nested multi-param calls, and computed expression arguments (10 test cases).
    - Added `test_16bit_stack.s` — validates 16-bit stack pointer relocation via `TYS`/`TSY` with push/pull verification on page $40.
- **Assembler (ca45)**:
    - Added `BASE_PAGE_INDIRECT_SP_Y` to `emitInstruction` (text and binary modes) and `calculateInstructionSize` — the `($nn,SP),Y` addressing mode was previously missing from the instruction encoder.
    - Added `lda_frame`/`sta_frame` methods to `M65Emitter` for frame-pointer-relative memory access.
    - Added `FrameAccessInfo` / `resolveFrameAccess()` helper to `AssemblerParser` for detecting frame-relative symbols.
    - **Relocatable object mode (`-c`)**: New `-c` flag produces `.o45` relocatable object files instead of flat binaries. Extracts per-segment bodies, generates `R_WORD` relocation entries for external and cross-segment references, builds import/export symbol tables from `.global`/`.extern` directives. Default output filename is `out.o45`.
    - **`.global` directive**: Marks symbols for export in `.o45` output.
    - **`.extern` directive**: Declares imported symbols (resolved by linker). Creates placeholder symbols with value 0 and assigns zero-based import indices for relocation entries.
    - **`.weak` directive**: Marks symbols as weak exports. Weak symbols use the high bit of the export segment byte (`O45_EXPORT_WEAK = 0x80`). The linker resolves strong-over-weak: two strong = error, strong+weak = strong wins, weak+weak = first wins.
- **Linker infrastructure (`.o45` format)**:
    - `O45Types.hpp` — format constants, enums (`O45Segment`, `O45RelocType`), mode bits, CPU IDs, and helper functions.
    - `O45Writer` — produces complete `.o45` files (header, options, segment bodies, relocation tables, symbol tables).
    - `O45RelocEncoder` — encodes high-level relocation entries into `.o65`/`.o45` delta-offset byte streams.
    - `O45SymbolTable` — manages imports/exports with deduplication, validation, and `applyTo(writer)`.
    - `O45Emitter` — bridge between the assembler and `.o45` format: extracts segments, scans for relocations, packages output.
    - `O45Reader` — parses `.o45` and `.o65` files back into structured data (header, options, segments, relocations, symbol tables). Supports both 16-bit and 32-bit formats.
    - `O45Linker` — links multiple `.o45` objects: segment merging, symbol resolution (with duplicate/undefined detection), relocation decoding and patching (all types: WORD, LOW, HIGH, LINEAR24, LINEAR32, SEGADR), flat binary or PRG output.
    - `O45RelocDecoder` — decodes delta-offset relocation byte streams back into high-level `O45Reloc` entries (inverse of `O45RelocEncoder`).
- **Utilities**:
    - **`ln45` — Linker**: Links multiple `.o45` relocatable objects into a flat binary or PRG. Merges text/data/bss/zp segments, resolves imports against exports, applies relocations. Supports library archives (`.lib`) with selective linking — only members needed to resolve undefined symbols are pulled in, with iterative chain resolution. Flags: `-t`/`-d`/`-b`/`-z` (segment base addresses), `-prg` (PRG output with load header), `-basic` (PRG with BASIC `SYS` stub at `$0801` for auto-run on MEGA65/C64), `-l` (link against library), `-m` (print symbol map).
    - **`ar45` — Archiver**: Creates and manages `.lib` archive files containing `.o45` object members. Commands: `c` (create), `t` (list), `x` (extract), `s` (symbol index), `a` (add members), `d` (delete members). Automatically builds a global symbol index by scanning each member's exports.
    - **`nm45` — Symbol Lister**: Lists exported/imported symbols in `.o45` and `.o65` object files. Traditional `nm` output format: `offset type name` (U=undefined, T=text, D=data, B=bss, Z=zp). Flags: `-u` (undefined only), `-g` (exported only), `-n` (sort by address), `-r` (reverse sort), `-p` (no sort), `-A`/`-o` (prepend filename for grep-friendly output). Multi-file listing.
- **Testing**:
    - Added `test_many_params_locals` to both `test_compiler.sh` and `test_mmemu.sh` validation suites.
    - Added 16-bit stack pointer test to `test_mmemu.sh` — verifies TYS/TSY and push/pull on a relocated stack page.
    - Added `test_global_extern.s` to assembler test suite — validates `.global`/`.extern` directive parsing.
    - Added `test_o45` unit test binary (217 assertions) — validates `.o45` header, segment bodies, relocation encoding, symbol tables, option headers, and segment mapping.

### Changed
- **Compiler (cc45)**:
    - **Symbol naming convention (C linkage)**: Global variables changed from `_g_` prefix to `_` (single underscore). Functions changed from bare names to `_` prefix (e.g., `main` -> `_main`). This follows the traditional C linkage convention for cross-toolchain compatibility (cc65, etc.). Parameters (`_p_`) and locals (`_l_`) are unchanged. The `isGlobal` detection now uses `globalVariableTypes.count(rName)` instead of prefix matching.
    - Parameters (`_p_` variables) are no longer added to `currentVars` and no longer receive `.var` offset bumps when locals are declared.
    - Return cleanup now pops 2 additional bytes for the saved frame pointer.
- **Assembler (ca45)**:
    - `proc` now emits 5 bytes of frame pointer save code (previously 0).
    - `endproc` emits 2 PLA instructions for frame pointer cleanup before RTS.
    - Size adjustment pass updated to use new `proc` (5 bytes) and `endproc` (3-4 bytes) sizes.
    - All simulated stack-access opcodes (`lda.sp`, `sta.sp`, `ldax`, `stax`, `phw.sp`, `ptrstack`, stack inc/dec, `neg.16`/`not.16`/`abs.16` on stack) now check for frame-relative symbols and use `($nn,SP),Y` addressing when appropriate.
    - `VariableNode::emit` in expression evaluator updated for frame-relative variables.
    - `pass2()` now updates segment `pc` fields at the end of each iteration, making segment sizes available to the `.o45` emitter after assembly.

### Fixed
- **Compiler (cc45)**:
    - **Explicit cast expressions**: Support for C-style cast syntax `(type)expr` including `int`, `char`, `void`, `struct`, `union`, `enum`, `signed`/`unsigned`, typedef aliases, and pointer casts. Casts are parsed in `parseUnary()` at the correct precedence level, with constant folding support (e.g., `(char)0x1FF` folds to `0xFF` at compile time).
    - **Implicit narrowing warnings**: The compiler now emits warnings to stderr when implicit conversions lose data — e.g., assigning an `int` to a `char` variable, or a pointer to a `char`. Explicit casts suppress the warning. Warnings also detect constant overflow (e.g., assigning `500` to a `char`).
    - Added `test_struct_param.c` — validates struct pointer parameters with the arrow operator (`p->member`), including nested structs.
    - Added `test_inline_asm.c` — validates inline assembly (`__asm__()`) accessing parameters (`_p_`), locals (`_l_`), and globals (`_`) with mmemu emulator validation.
- **Testing**:
    - Added `test_cast.c` — validates explicit cast expressions: int-to-char narrowing, char-to-int widening, literal casts, nested casts, pointer-to-int casts, and casts in arithmetic expressions (10 test cases).
    - Added `test_narrowing_warn.c` — validates implicit narrowing warning output: 3 expected warnings (int-to-char declaration, int-to-char assignment, pointer-to-char) and 4 expected non-warnings (explicit cast, widening, same-size).
    - Added `test_inline_asm` to `test_mmemu.sh` — validates inline asm variable access (int params, char params, global stores, local read/write) via memory inspection at `$4000`.

### Fixed
- **Compiler (cc45)**:
    - **Arrow operator on struct pointer parameters**: `getExprType()` now checks `_p_` (parameter) prefix before `_l_` (local) in `variableTypes`, matching `resolveVarName()` lookup order. Previously, struct pointer parameters resolved to `int` instead of their declared struct type.
    - **`proc` instruction missing parameter declarations**: Merged parameter declarations (`W#`/`B#`) onto the `proc` instruction line so the assembler creates parameter symbols with correct stack offsets. Previously, parameters were only listed on the no-op `.proc` directive, leaving symbols undefined (offset 0) for functions without local variables.
- **Assembler (ca45)**:
    - **`lda.sp` / `sta.sp` pass-2 code generation**: Added missing `LDA_STACK` and `STA_STACK` dispatch entries in `AssemblerGenerator.cpp`. These simulated opcodes were correctly sized in pass 1 but emitted zero bytes (BRK) during code generation.
    - **`.var` assignment double-evaluation**: Removed the `.var` ASSIGN re-evaluation from the pass-2 generator. Self-referencing expressions like `.var _p_val = _p_val + 2` were evaluated both during parsing and generation, applying the adjustment twice. The parse-time value is now final.

---

## [Unreleased] - 2026-04-26

### Added
- **Compiler (cc45)**:
    - Added `-O0` flag to disable all optimizations (constant folding, propagation, dead variable elimination).
    - Added startup code generation: `jsr main; _halt: bra _halt` when a `main` function is present.
    - Added callee local variable cleanup before function return.
    - Added caller argument cleanup after function calls.
    - Added dead store elimination to ConstantFolder — consecutive constant stores to the same variable are collapsed to the final value.
- **Assembler (ca45)**:
    - Added `RTN` to the instruction lexer (previously silently skipped).
    - Added error reporting for unknown instructions during assembly, with output suppression on errors.
    - Added try/catch around instruction parsing to convert parse failures into error messages.
    - Added `*` (current PC) support in expressions (e.g., `bra *+4`, `lda *-2`).
    - Added `TSY` and `TYS` methods to M65Emitter.
    - Added `lda.sp` / `sta.sp` simulated opcodes for 8-bit stack-relative access (expands to `TSX; LDA/STA $0101+offset,X`).
    - Added `phw.sp` simulated opcode for pushing a 16-bit stack-relative variable onto the stack.
    - Added width-suffixed `mul.8`/`.16`/`.24`/`.32` and `div.8`/`.16`/`.24`/`.32` to the instruction lexer.
- **Testing**:
    - Added comprehensive control flow test (`test_mmemu_control.c`) covering if/else, while, do-while, for with break/continue, switch with fallthrough, and ternary operators.
    - Added simple compiler test (`mmemu_compiler_simple.c`).
    - Added `src/test/test_mmemu.sh` and `src/test-resources/test_mmemu_hello.s` for automated validation using `mmemu-cli`.

### Fixed
- **Assembler (ca45)**:
    - **Opcode database**: Corrected `STA` ($82) and `LDA` ($E2) stack-relative entries from `STACK_RELATIVE` (direct) to `BASE_PAGE_INDIRECT_SP_Y` (indirect, matching actual 45GS02 hardware). Removed duplicate `STACK_RELATIVE` entries that shadowed `BASE_PAGE_INDIRECT_Z` opcodes.
    - **Stack access synthesis**: Rewrote `lda_stack`/`sta_stack`/`stz_stack` to use `TSX` + absolute,X indexed addressing (e.g., `TSX; STA $0103,X`) since the 45GS02 has no native direct stack-relative mode. Previous opcodes ($82/$E2) are indirect and were treated as NOPs by the emulator.
    - **Simulated op immediate detection**: Fixed `add.16`, `cmp.16`, `cmp.s16`, `and.16`/`ora.16`/`eor.16`, and `ldw` to correctly distinguish `#immediate` operands from memory address operands. Previously, bare numeric operands like `$02` were incorrectly treated as immediates via `isConstant()`.
    - **CMP16/CMP_S16 expression parsing**: Fixed `#` token not being skipped before expression parsing, causing null AST and zero-length instruction emission.
    - **BRA text output**: Fixed relative branch offset in TEXT mode — the emitter now correctly converts branch offsets to target addresses (adding instruction size) for `*+N` syntax.
    - **Branch target evaluation**: Changed branch target resolution in the generator from `resolveSymbol` (string lookup) to `evaluateExpressionAt` (full expression evaluation), enabling `*+N` and computed targets.
    - **Segment PC tracking**: Fixed pass2 segment PC initialization to use sentinel values, preventing data segment labels from resolving to address $0000.
    - **STAX/LDAX stack simulated ops**: Replaced `PHA`/`PLA` sequences (which shift SP and corrupt TSX-based offsets) with ZP $00 temp storage for preserving register values during multi-byte stack access.
- **Compiler (cc45)**:
    - **ZP temp clobber across function calls**: Reordered evaluation in simple and compound assignment paths to evaluate the RHS first and push to the hardware stack before computing the destination address into ZP. Prevents callee functions from clobbering ZP temporaries.
    - **Dead variable elimination removing live variables**: Fixed `VariableUseChecker` to count assignment targets as uses, preventing variable declarations from being eliminated while dead stores still reference them.
    - **Global struct member access with `, s` suffix**: Fixed MemberAccess visitor that unconditionally appended stack-relative suffix to global variable member loads.
    - **8-bit local variable access**: Changed CodeGenerator to emit `lda.sp`/`sta.sp` simulated opcodes for char-sized local variable access instead of bare `lda`/`sta` with `, s` (which the 45GS02 doesn't support as a native addressing mode).
    - **Function argument passing**: Changed CodeGenerator to emit `phw.sp` instead of `phw.s` for pushing stack-relative variables as function arguments.
    - **ConstantFolder switch cases**: Clear `knownConstants` at each `case`/`default` label to prevent cross-case constant propagation (e.g., `result=20` from case 2 leaking into case 3's `result+5`).
    - **ConstantFolder switch exit**: Clear `knownConstants` after switch body to prevent stale pre-switch values from propagating.
    - **Register cache after stw**: Invalidate register cache after `stw` to global variables (the simulated op clobbers A with the high byte).
    - **emitAddress resultNeeded**: Set `resultNeeded=true` before visiting array base expression in `emitAddress`, preventing stale register values from being used for pointer-indexed accesses.
    - **Local variable stack offset**: Changed `.var` offset for new local variables from `size` to `0` (correct for `$0101+offset` convention where SP+1 = top of stack).

## [Unreleased] - 2026-04-23

### Added
- **Compiler (cc45)**:
    - Added support for **Compound Assignment Operators**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`.
    - Added support for the **Modulo Operator** (`%`) for integer remainder.
    - Added support for **`for` loop declarations**: variables can now be declared in the initializer clause (e.g., `for (int i = 0; ...)`).
    - Implemented the **`typedef` keyword** for creating type aliases, including support for pointer and struct/union typedefs.
    - Added support for **`enum` types** and enumerator constants.
- **Assembler (ca45)**:
    - Added support for generating **`.prg` files**: If the output filename ends in `.prg`, a 2-byte load address header (inferred from the first `.org`) is prepended to the binary.
    - Improved **`.basicUpstart` directive**:
        - Automatically defaults to `.org $2001` if no start address is specified.
        - Fixed size at 12 bytes across all passes for more reliable label resolution.

### Fixed
- **Compiler (cc45)**:
    - Fixed a critical bug in `BinaryOperation` where operands were being reversed for non-commutative operators (subtraction, division, shifts).
    - Improved constant folding to be more robust across control flow (clearing constants when entering loops).
- **Assembler (ca45)**:
    - Standardized `.basicUpstart` size to 12 bytes to match the emitted code.

## [Unreleased] - 2026-04-18

### Added
- **Compiler (cc45)**:
    - Added support for the `volatile` keyword for variable declarations and function parameters. `volatile` variables are now explicitly prevented from being optimized away by dead store elimination.
    - Added `-E` flag to run only the preprocessor and output to `stdout` or a file specified by `-o`.
    - Improved multi-call support: if the binary is invoked as `cp45`, it automatically runs in preprocessing-only mode.
- **Preprocessor (cp45)**:
    - Added `bin/cp45` as a new build target (symlink to `cc45`).
    - Defaults to `stdout` output when running in standalone mode.
    - Added support for function-like and **variadic macros** (`__VA_ARGS__`).
    - Implemented **comma removal extension** (`##__VA_ARGS__`) for empty variadic arguments.
    - Added support for the **`_Pragma` operator**.
    - Implemented macro operators: `#` (stringification) and `##` (token pasting).
    - Added macro expansion support within `#include` directives.
    - Implemented header guard optimization via `#pragma once`.
    - Added support for `#if` and `#elif` expression evaluation, including `defined()`, arithmetic, and logical operators.
    - Added support for line continuation using the backslash (`\`) character.
    - Added support for standard predefined macros: `__STDC__`, `__STDC_VERSION__`, and `__STDC_HOSTED__`.
    - Added support for `#undef`, `#line`, `#error`, `#warning`, and `#pragma` directives.
    - Implemented expansion of standard predefined macros: `__FILE__`, `__LINE__`, `__DATE__`, and `__TIME__`.
- **Documentation**:
    - Renamed preprocessor documentation to `doc/cp45.md` to align with toolchain naming conventions.
- **Assembler (ca45)**:
    - Implemented a suite of high-level simulated opcodes:
        - `ldax / lday / ldaz`: 16-bit word loads with support for immediate (`#`), stack-relative (`offset, s`), zero page, and absolute addressing.
        - `stax / stay / staz`: 16-bit word stores with support for stack-relative, zero page, and absolute addressing.
        - `phw`: 16-bit word push, now supports stack-relative `offset, s` using an optimized `tsx` sequence.
        - `add.16 / sub.16`: 16-bit addition/subtraction on the `.ax` register pair.
        - `and.16 / ora.16 / eor.16`: 16-bit bitwise logic on the `.ax` register pair.
        - `neg.16 / not.16`: 16-bit negation and bitwise NOT, now supports `.ax`, stack-relative, and memory operands.
        - `cpw`: 16-bit word comparison on the `.ax` register pair.
        - `branch.16`: High-level 16-bit zero-check branching (`beq`, `bne`).
        - `chkzero.8 / .16`: Truthiness check returning a boolean in `.a`.
        - `ptrstack`: Efficiently calculates and loads a stack variable address into `.ax`.
        - `ptrderef`: De-references a 16-bit pointer stored in Zero Page.
        - `ldw.f / stw.f / inc.f / dec.f`: Linear 28-bit memory access (using hardware `eom` prefixes).
        - `swap / zero`: Multi-register manipulation helpers.
    - Added `@` prefix for "don't care" labels to define local optimization windows.

### Changed
- **Compiler (cc45)**:
    - Updated all code generation to utilize the new simulated opcodes, resulting in significantly more compact assembly.
    - Implemented **Parameter Prefixing**: All function parameters are now prefixed with `_p_` to avoid collisions with CPU registers or flags.
    - Implemented **Local Variable Prefixing**: All local variables are now prefixed with `_l_`.
    - *Note*: Global variables were initially prefixed with `_g_`; later changed to `_` (single underscore, traditional C linkage convention).
    - Integrated `ldax #value` for more efficient 16-bit constant and string address loading.
    - Changed default `cc45.zeroPageAvail` to `9`.
- **Assembler (ca45)**:
    - Renamed legacy `ldw .ax` syntax to preferred `ldax` mnemonic.

### Optimized
- **Compiler (cc45)**:
    - **Strength Reduction**: Converts multiplication by a power of 2 into equivalent bit shifts (`ASL`, `LSR`), and modulus by a power of 2 into bitwise AND (`AND`).
    - **Constant Propagation**: Substitutes variables with known constant literal values into expressions, enabling further compile-time folding.
    - **Dead Variable Elimination**: Removes stack allocation and initialization for non-volatile local variables that are initialized with constants and not subsequently used.
    - **Constant Initialization**: Uses native `phw #value` for declaring local variables with constant initializers, saving 3 instructions and 3 bytes per declaration.
    - **Constant Assignment**: Optimized 16-bit constant assignments to stack variables to use a "lda twice" sequence, avoiding clobbering the `x` register and reducing code size.
    - **Zero Optimization**: Integrated `zero a, x` simulated opcode for more efficient handling of zero literals.
    - **Surgical Loading**: Improved `VariableReference` loading to surgically update only the required registers if part of the word is already in the correct state.
- **Assembler (ca45)**:
    - **Stack-relative Logic**: Extended `stz`, `stx`, and `sty` with `tsx` absolute indexed addressing simulations where native stack-relative variants are missing.
    - **Word Load Tracking**: Enhanced the optimizer to track register states across high-level word load/store operations.

## [Unreleased] - 2026-04-18

### Added
- **Compiler (cc45)**:
    - Added support for pointers to pointers (multiple levels of indirection).
    - Implemented dereference (`*`) and address-of (`&`) unary operators.
    - Updated assignment logic to support arbitrary targets (e.g., `*ptr = value`).
- **Assembler (ca45)**:
    - Added support for `FLAT_INDIRECT_Z` (`[zp],Z`) addressing mode for `ADC`, `AND`, `CMP`, `EOR`, `LDA`, `ORA`, `SBC`, and `STA`.
    - Added support for `IDENTIFIER = expression` equates in `pass1`.
    - Improved `ACCUMULATOR` mode detection to correctly distinguish between registers and labels.

### Fixed
- **Assembler (ca45)**:
    - Fixed branch instruction emission order (opcode now correctly precedes the relative offset).
    - Fixed `SEE` instruction opcode (corrected to `0x03`).
    - Fixed `BSR` 16-bit relative offset calculation.
    - Fixed `pass1` re-evaluation loop to correctly handle `org` and `* =` directives, ensuring accurate address calculation for subsequent labels.
    - Fixed object file mismatch issues by ensuring clean builds.
- **Compiler (cc45)**:
    - Fixed 16-bit pointer arithmetic scaling for `char` and `int` pointer types.
    - Improved hex formatting for ZP scratchpad addresses in generated assembly.

## [Unreleased] - 2026-04-14
...
### Added
- **Compiler (cc45)**:
    - Added support for `do-while` loops.
    - Added support for inline assembly using `asm("...")` or `__asm__("...")`.
    - Added support for `if` and `else` statements.
    - Added support for `while` and `for` loops.
    - Added support for the `char` (8-bit unsigned) type.
    - Implemented full set of comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`.
    - Added automated regression testing suite in `src/test/`.
- **Assembler (ca45)**:
    - Added support for `.cpu _45gs02` directive.
    - Added support for `* = $addr` ORG syntax (KickAssembler compatibility).
    - Added support for `//` and `/* ... */` style comments.
    - Added tiered branch strategy: automatically selects between 8-bit and 16-bit relative branches.
    - Implemented dead code removal: strips instructions following `RTS`, `RTN`, or `RTI` within a procedure.
    - Added optimization for `RTN #0` to `RTS` (`0x60`).
    - Added support for 16-bit relative branch opcodes (LBEQ, LBNE, etc. on 45GS02).

### Changed
- **Assembler (ca45)**: Hex literals ($) and binary literals (%) now retain their prefixes in token values for improved parsing.
- **Types**: All basic types (`int`, `char`) are now treated as unsigned by default.
- **Arithmetic**: Improved 16-bit unsigned arithmetic (addition and subtraction) using carry flag sequences.
- **Code Generation**: Variable references and assignments now correctly handle 16-bit stack-relative offsets.
- **Project Structure**: 
    - Moved `test_compiler.sh` to `src/test/`.
    - Renamed `src/test/resources` to `src/test-resources`.
- **Build System**: Updated `Makefile` with improved `test` and `clean` targets.

### Optimized
- `+1` and `-1` operations now use `INC A`, `DEC A`, `INX`, `DEX` when appropriate.
- Function returns with no parameters now use a single-byte `RTS` instead of `RTN #0`.
