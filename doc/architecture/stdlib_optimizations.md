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

### rand

Two reads of the hardware RNG (`$D7EF`), each after waiting for `$D7FE` bit 7 to clear; bit 15 is cleared. `srand` is a no-op.

### itoa / ltoa / utoa

Repeated division with the MEGA65 hardware divider (`$D770`…), digits pushed on the stack and popped in order.

## C Routines (`lib/stdlib/`)

### qsort — `qsort.c`

- Median-of-three pivot, insertion sort for partitions of ≤ 8 elements, recursion into both partitions (smaller first).
- `swap`, `do_cmp`, `cmp_positive` are `always_inline` helpers; `swap` is a byte loop for every element size.
- Comparator and element size are kept in globals (`qs_cmp`, `qs_size`), so `qsort` is not reentrant.
- `qs_impl` is recursive, so it uses a stack frame rather than static allocation.
- The `#pragma cc45 optimize(...)` lines inside the functions name passes (`constant-folding`, `dead-code-elimination`, `branch-inversion`, `cross-function-inlining`, `tail-call-optimization`, `cse`); they enable those passes for the function but do not change the algorithm. In `-O2` output cc45 does not turn tail calls into jumps (see [../ref/V1_1_OPTIMIZATION_ROADMAP.md](../ref/V1_1_OPTIMIZATION_ROADMAP.md)), so both recursive calls are real calls.

### bsearch — `bsearch.c`

Iterative binary search with `mid = lo + (hi - lo) / 2`, early return on a match, comparator through an `always_inline` wrapper and a global (`bs_cmp`).

### memcpy_optimized — `memcpy_fast.c`

Exported as `memcpy_optimized` (not declared in any header). For `n > 4` with both pointers 4-byte aligned it runs a C loop copying 4 bytes per iteration, otherwise it calls `memcpy`. It is not used by anything in the library, and no measurement shows it beating the assembly `memcpy`.

### strtol / strtoul / strtof — `strtol.c`, `strtoul.c`, `strtof.c`

Straightforward digit loops (`result = result * base + digit` on `unsigned long`); no unrolling or base-specific fast paths. `strtol` delegates to `strtoul`.

### Float math — `float_rom_fns.c`, `floatmath.c`, `src/float_rom.s45`

- `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf` copy the argument into `__float_a` and call the BASIC 65 ROM routine through `float_rom.s45` (which switches to base page 0 around the ROM call); there are no tables or series in the library.
- `powf` = `expf(y * logf(|x|))`; `log10f`/`log2f` scale `logf`; `ldexpf` multiplies by 2 in a loop (|n| iterations); `frexpf` halves/doubles in a loop; `truncf` goes through `long` for |x| < 2^31.
- Speed is bounded by the ROM routines.

### printf family — `__vformat.c`

One formatter for all `printf`/`sprintf`/`snprintf`/`fprintf` variants, writing through a per-call `put` callback. Float formatting (`printf_float.c`/`sprintf_float.c`) is only linked when a float is passed to a variadic call, so integer-only programs do not carry it.

## Open Work

| Item | Notes |
|------|-------|
| Benchmarks | Fix `bench_stdlib_critical.c` timing and add a target that runs it on xemu |
| `strlen`/`puts` length | Use a 16-bit index (current limit: < 256 bytes) |
| `memcpy`/`memset` | Unroll, or use DMA above a size threshold |
| `malloc` | Size-class free lists for small blocks; in-place `realloc` growth |
| `qsort` | Word/long fast paths in `swap`; iterate on the larger partition |
