# Bug Investigation: game_of_life / memset_screen crash

**Date:** 2026-06-02
**Branch:** dev_v1.0.3
**Status:** In progress — crashes on real MEGA65 hardware, works in mmemu emulator

## Symptoms

### memset_screen.prg (simple test program)
- Fills ~half the screen with '@' (char 0) — consistent with 1000 bytes on 80-column MEGA65 screen
- Crashes after completing all 256 fill iterations (ch=0 through ch=255)
- Monitor dump: `PC:$2064 SR:B0 A:$FF X:$E5 Y:$E8 Z:$03 SP:$01E5 B:1`
- B:1 flag indicates BRK ($00) opcode was executed at $2064
- $2064 should contain `LDX #$08` ($A2 $08) — the code was overwritten with $00
- Crash occurs on the SECOND pass of the `while(1)` loop (first 256 fills succeed)

### game_of_life (life.prg)
- Shows black screen on real MEGA65 (presumably crashes early)

### Both programs work perfectly in the mmemu emulator
- SP fully unwinds, all iterations complete, correct output

## Bugs Found and Fixed (committed)

### 1. Proc parameter offset assignment reversed (commit c3145a5)
**File:** `AssemblerParser.cpp:1042`

The `proc` directive's parameter offset loop iterated backwards, assigning the lowest stack offset to the last declared parameter. With right-to-left push, the first declared param should get the lowest offset.

**Impact:** `memset(dest, val, count)` read `count` as `dest` and vice versa — filled 10832 bytes from $03E8, overwriting the entire code segment.

**Fix:** Changed loop from `for(i=size-1; i>=0; --i)` to `for(i=0; i<size; ++i)`.

### 2. memset loop termination bug (commit 703fd60)
**File:** `lib/stdlib/memset.s45:32` (also `lib/stdlib_zp/memset.s45:23`)

Used `cpy $05` (compare Y with count_lo) to detect loop end. Y tracks page offset while count decrements independently — they diverge. When count reaches 0, Y ≠ count_lo, so the loop overflows by ~65000 bytes.

**Fix:** Changed `cpy $05` to `ldx $05` (check if count_lo is zero).

### 3. Optimizer tail-dedup BRK corruption (commit c3145a5, cherry-pick of 8514aed)
**Files:** `AssemblerOptimizer.cpp`, `AssemblerGenerator.cpp`, `AssemblerParser.cpp`

The tail-dedup optimizer (Pass 3) created BRA/label/RTS statements with empty `segmentName`. The generator filters by segment name, so these were silently skipped, leaving BRK gap bytes ($00) where the replacement code should be. This corrupted step()'s PLZ frame cleanup sequence.

### 4. memcpy return value offset (commit c3145a5)
**File:** `lib/stdlib/memcpy.s45:56-58`

After PLZ/STZ restored saved ZP bytes, the return value loading still used +2 offsets for the (now-popped) PHA saves. Changed to `__sp_base+_p_dest` (no +2).

### 5. Caller-side stack cleanup — avoid RTS #N (commit b7eba20)
**Files:** `IRCodeGen.cpp`, `AssemblerGenerator.cpp`, `AssemblerParser.cpp`, `memcpy.s45`

`RTS #N` (opcode $62) appeared unreliable on real MEGA65 hardware — each call leaked N bytes on the stack. Switched entire calling convention to caller-side cleanup: the compiler now emits PLZ×N after each stack-convention JSR to pop arguments.

- `endproc` always emits plain `RTS` ($60)
- `rtn #0` remains plain `RTS` (no auto-add of procParamSize)
- Compiler emits PLZ instructions after every stack-convention call

## Current Open Issue

### Code at $2064 overwritten with $00 on real MEGA65

**What we know:**
- The binary is correct (verified via objdump — $2064 contains $A2 = LDX)
- Caller-side PLZ cleanup is present and executes (Z=$03 confirms last PLZ ran)
- The crash happens on the 2nd pass of the while(1) loop (A=$FF = ch 255 from 1st pass completion)
- SP=$01E5 — about 16 bytes deeper than expected ($01F5 frame level)
- Both memset_screen and game_of_life work perfectly in the mmemu emulator

**What we've ruled out:**
- Proc parameter offsets — fixed and verified
- memset termination overflow — fixed (ldx $05)
- Optimizer BRK corruption — fixed (cherry-pick 8514aed)
- RTS #N opcode — eliminated entirely (caller-side cleanup)
- DMA BSS init — DMA job parameters verified correct, don't touch code area
- KERNAL IRQ clobbering ZP $02-$06 — KERNAL IRQ handler uses $A0+ workspace, not $02-$06

## Hypotheses to investigate:
1. **Monitor breakpoint artifact** — The MEGA65 monitor may have a stale breakpoint at $2064 from a previous session. B:1 in SR is consistent with BRK. The user should clear all breakpoints and try again.

2. **Memory banking / MAP** — The MEGA65's MAP instruction or memory banking could cause writes to unexpected physical addresses. If the CPU's memory map has code and data overlapping in physical RAM, a write to $0800 could alias to $2064 under certain banking configurations.

3. **Screen RAM overlap** — On MEGA65, screen RAM location depends on VIC-IV configuration. If the VIC-IV screen pointer is configured such that screen RAM overlaps with program code in physical memory, writes to the "screen" would corrupt code.

4. **C65 vs C64 memory model** — The MEGA65 boots in C65/MEGA65 mode with different memory mapping than the emulator's raw mode. The program might be loaded into a different physical address range.

5. **Stack page / SPH issues** — The 16-bit stack pointer (SPH via TSY/TYS) might behave differently on real hardware vs emulator, causing stack operations to read/write unexpected memory.

## Failed Attempt (2026-06-02)

### Changes Tested:
1.  **Ensured `CLE` and `SEI`**: Verified that the hardware is in 8-bit stack mode and interrupts are disabled to prevent KERNAL/Monitor clobbering ZP.
2.  **ZP Relocation**: Moved the compiler scratch area from `$02` to `$20` to avoid KERNAL ZP usage.
3.  **DMA Fix/Removal**: Identified that `crt0.s45` was triggering DMA using the wrong registers and before I/O enable. Replaced DMA-based ZP save/restore with simple assembly loops for maximum safety.
4.  **Scratch Area Separation**: Discovered and fixed a conflict where the compiler and simulated opcodes (like `ldax.fp`) were sharing the same ZP scratch slot, causing pointer corruption during function call setup.
5.  **Instruction Compatibility**: Replaced 45GS02-specific `PLZ`/`PHZ` with standard `PLA`/`PLY` sequences, and removed `PLX` from library return paths to ensure compatibility with all FPGA cores.

### Result:
-   **No Change**: The program still crashes with a `BRK` at `$2075` (formerly `$2064`) on the second pass of the main loop.
-   **Observations**: The fact that it always crashes at the same location (the start of `memset` call setup) despite significant changes to ZP layout and stack handling suggests a deeper memory mapping or aliasing issue on the real hardware.

## Files Modified (uncommitted)


- `lib/stdlib/memset.s` — currently has SEI/CLI around fill loop (should revert — IRQ hypothesis was wrong)
- `lib/stdlib/memcpy.s` — has SEI/CLI added (should revert)

## Test Results

All test suites pass with the committed fixes:
- Compiler unit tests: all passing
- Assembler validation: all passing
- mmemu execution tests: all passing
- objdump tests: all passing

## Reproduction

```bash
# Build toolchain
make

# Build stdlib
cd lib && make clean && make && cd ..

# Build and test memset_screen
cd examples/c/memset_screen
make clean && make

# Run in emulator (works)
# Transfer memset_screen.prg to real MEGA65 and RUN (crashes)
```
