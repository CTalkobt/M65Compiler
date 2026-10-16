# Standard Library Implementation Notes (Performance)

How the performance-relevant parts of the C library are implemented, their known limits, and open work. For the API see [stdlib.md](stdlib.md).

No cycle counts are given: none have been measured on hardware or xemu for the current library. `src/test-resources/bench_stdlib_critical.c` contains micro-benchmarks for these functions, but it is not built by any target and times with a byte read from `$D711`, which is not a jiffy counter; it needs a real timer (CIA timer or `clock()`) before its numbers mean anything.

## Assembly Routines (`lib/stdlib_asm/`, ZP convention)

### malloc / free / calloc / realloc — `malloc.s45` (718 lines)

- Sorted singly-linked free list, first fit, block splitting, coalescing with both neighbours on `free`.
- Header: 2-byte size with bit 0 = allocated; free blocks also hold a 2-byte next pointer.
- Lazy initialization on the first `malloc`; bounds from `__heap_start`/`__heap_end` (0 = automatic, see [stdlib.md](stdlib.md#4-general-utilities-stdlibh)).
- `calloc` = `malloc` + `memset`; `realloc` always allocates, copies and frees (it never grows in place).
- Cost of `malloc` grows with the length of the free list (linear search).

### memcpy / memset / memmove / memcmp

Byte loops over `(zp),y` with a 16-bit count; the pointer high bytes are incremented every 256 bytes. `memmove` copies backwards when `dest > src`. No unrolling and no DMA: for large blocks, `dma_copy`/`dma_fill` (`dma.h`, inline F018B jobs) are much faster.

### strlen, strcpy, strcmp, strcat, strchr, …

Byte loops with an 8-bit `Y` index. `strlen` (and `puts`) stop when `Y` wraps, so they only handle strings shorter than 256 bytes — `strlen` of a 256-byte string returns 0.

### rand / srand — `rand.s45`

Until `srand`: two reads of the hardware RNG (`$D7EF`), each after waiting for `$D7FE` bit 7 to clear; bit 15 is cleared. After `srand`: C's reference generator, `next * 1103515245` by the hardware multiplier (the low 32 bits of the product) `+ 12345`, result bits 16–30 (114 bytes; the C version was 336).

### clock — `clock.s45`

Reads the KERNAL's 24-bit frame counter (`$110C`–`$110E`) until no byte changed since, and negates it (`negq`): 32 bytes (the C version was 222).

### strtoul / strtol — `strtoul.s45`

A digit loop with the product `result * base` from the hardware multiplier (32 × 32 bits: any bit above the low 32, or a carry adding the digit, is an overflow); `strtol` checks its sign, calls `strtoul` and limits the range. 677 bytes for both (the C versions were 1733).

### strtok / strtok_r — `strtok.s45`

One routine: `strtok` passes its static save pointer to `strtok_r` (136 bytes; 584 in C).

### qsort / bsearch — `sort.s45`, `stdlib/qsort.c`

- Not recursive: median of three (element 0, the middle and the last) as the pivot, the smaller part of each partition sorted next and the larger one on a 16-entry stack (enough for any 16-bit count), insertion sort for 8 elements or fewer. An element's address is `base + index * size` by the hardware multiplier; the swap is a byte loop over `(zp),y`.
- The comparison function is called through `__sort_cmp` (`stdlib/qsort.c`), compiled in each library's calling convention: it calls the program's function through the pointer in that convention (ln45 bridges the assembly's call of `__sort_cmp` in `c45_stack.lib`, not a call through a pointer). The comparator may change any zero-page byte, so the state is in BSS: `qsort` is not reentrant (a comparator must not call `qsort` or `bsearch`).
- 1.2 KB with the C part (the C versions were 2.2 KB); 20 sorts of 300 ints: 58 → 49 frames (the comparator calls dominate).

### itoa / ltoa / utoa

Repeated division with the MEGA65 hardware divider (`$D770`…), digits pushed on the stack and popped in order.

## C Routines (`lib/stdlib/`)

### memcpy_optimized — `memcpy_fast.c`

Exported as `memcpy_optimized` (not declared in any header). For `n > 4` with both pointers 4-byte aligned it runs a C loop copying 4 bytes per iteration, otherwise it calls `memcpy`. It is not used by anything in the library, and no measurement shows it beating the assembly `memcpy`.

### strtof — `strtof.c`

The digits gathered as a whole number (up to 10 significant digits, the rest only moving the exponent), then scaled at the end by powers of ten from a table of 10^0–10^9 (multiplied or divided in steps of up to 10^9), so short decimals are exact (0.25 is 25 / 100).

### Float math — `float_rom_fns.c`, `floatmath.c`, `src/float_rom.s45`

- `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf` copy the argument into `__float_a` and call the BASIC 65 ROM routine through `float_rom.s45` (which switches to base page 0 around the ROM call); there are no tables or series in the library.
- `powf` = `expf(y * logf(|x|))`; `log10f`/`log2f` scale `logf`; `ldexpf` multiplies by 2 in a loop (|n| iterations); `frexpf` halves/doubles in a loop; `truncf` goes through `long` for |x| < 2^31.
- Speed is bounded by the ROM routines.

### printf family — `stdlib_asm/__vformat.s45`

One formatter for all `printf`/`sprintf`/`snprintf`/`fprintf` variants, writing through a per-call `put` callback (`__fastcall__`). It is hand-written assembly (1.8 KB; the C version compiled to 5.5 KB) with its state in BSS. Integer digits come from the `__fmt_utoa`/`__fmt_ultoa` hooks, linked only as the program's format strings need them; float formatting (`printf_float.c`/`sprintf_float.c`) is only linked when a float is passed to a variadic call. A `sprintf` of `%d` links about 3.3 KB instead of 7 KB.

## Open Work

| Item | Notes |
|------|-------|
| Benchmarks | Fix `bench_stdlib_critical.c` timing and add a target that runs it on xemu |
| `strlen`/`puts` length | Use a 16-bit index (current limit: < 256 bytes) |
| `memcpy`/`memset` | Unroll, or use DMA above a size threshold |
| `malloc` | Size-class free lists for small blocks; in-place `realloc` growth |
| `qsort` | Word/long fast paths in `swap`; iterate on the larger partition |
