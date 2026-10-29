# C Standard Library (stdlib) for MEGA65/45GS02

This document describes the C library shipped with `cc45`: the headers in `lib/include/`, the runtime archives built from `lib/`, and what each function actually does on the MEGA65.

## 1. Libraries, Building and Linking

### Archives

`make` at the project root (or `make -C lib`) builds three archives in `lib/build/`:

| Archive | Calling convention | Built from |
|---------|--------------------|------------|
| `c45.lib` | ZP (`-fzpcall`, cc45's default) | `crt0_zp.s45`, `crt0_mega65_zp.s45`, `crt_heap_zp.s45`, `crt_bssinit_zp.s45`, `sac_debug.s45`; `stdlib/*.c` compiled with `-fzpcall`; `stdlib_asm/*.s45`; `src/float_rom.s45` |
| `c45_zp.lib` | ZP | a copy of `c45.lib` under its earlier name |
| `c45_stack.lib` | stack (`-fno-zpcall`) | `crt0.s45`, `crt0_mega65.s45`, `crt_heap.s45`, `crt_bssinit.s45`, `sac_debug.s45`; `stdlib/*.c` compiled with `-fno-zpcall`; the same `stdlib_asm/*.s45` and `src/float_rom.s45` objects |

Details (`lib/Makefile`):

- The library C sources are compiled with `-fimplicit-function-declaration -fsigned-default -funsigned-char`: they are written as standard C (signed `int`/`long`), unlike cc45's unsigned-by-default user code.
- They are also compiled with `-ffunction-sections`: every function, variable, string literal and static-allocation block is a section, and ln45 drops what the program does not reach (one `conio` call no longer links all of `conio`, nor `cprintf`'s `vsprintf`, nor the unused functions' storage). The hand-written assembly (`stdlib_asm/*.s45`, `src/float_rom.s45`) is assembled with `ca45 -ffunction-sections`: a section per `proc`, and `float_rom.s45` marks each entry with `.split` (routines that branch into one another stay together). Over the runtime tests, the memory a program uses up to the end of BSS is 10.5% smaller than without dropping; crt0 stays whole.
- The hand-written assembly in `lib/stdlib_asm/*.s45` uses the ZP convention and goes into both archives; stack-convention callers reach it through ln45's stack→ZP bridge thunks. A `stdlib/X.c` that has a `stdlib_asm/X.s45` is not compiled.
- Compiler runtime in `stdlib_asm/divmod.s45`: `__divs16`, `__divu16`, `__mods16`, `__modu16` and the `32` forms, which cc45 calls for `/` and `%` (dividend in A/X or A/X/Y/Z, divisor in `__zp_scratch2`, result in the same registers), and `__mul16` / `__mul32` for `*` outside loops (the same way; `__mul16y` takes a byte factor in Y). Plain labels, so ZP- and stack-convention code call them directly. It declares `cc45.icallinfo` (no calls through pointers) for ln45's call graph.
- Only `.s45` files are assembled. The `.s` files in `lib/stdlib/` and `lib/stdlib_asm/` (`strcspn`, `strdup`, `strlcpy`, `strnlen`, `strpbrk`, `strspn`, `strstr`, `uart`) are not built; those functions come from the C versions — except `uart`, which has no built implementation (see [§12](#12-other-library-backed-headers)).
- `lib/src/*.c` (graphics, sprites, audio, hardware helpers, …) is **not** part of any archive. Headers for those modules exist in `lib/include/`, but their functions are not in the library (see [§14](#14-headers-without-a-library-implementation)).

Linking is selective: only the archive members that resolve a referenced symbol are pulled in.

### Using the library

`cc45` searches `<dir of cc45>/../lib/include` for `<...>` headers (in the build tree that is `lib/include/`), then `CC45_INCLUDE` (colon-separated) and `-I` paths. A one-step build links the right archive automatically — `c45.lib` or, with `-fno-zpcall`, `c45_stack.lib` — from `<dir of cc45>/../lib/build/` or `<dir of cc45>/../lib/cc45/`:

```bash
cc45 prog.c -o prog.prg                 # compile, assemble, link with c45.lib
cc45 -fno-zpcall prog.c -o prog.prg     # ... with c45_stack.lib
```

Linking by hand:

```bash
cc45 -c main.c -o main.o45
ln45 -basic main.o45 -l lib/build/c45.lib -o program.prg       # BASIC SYS stub at $2001
ln45 -prg main.o45 -L lib/build -l c45.lib -o program.prg      # plain PRG (-L or CC45_LIB give search dirs)
```

`make install` copies the archives to `PREFIX/lib/cc45` and the headers to `PREFIX/include/cc45`. An installed `cc45` finds the archives there, but it looks for headers in `PREFIX/lib/include`, so set `CC45_INCLUDE=PREFIX/include/cc45` or pass `-I`. (`lib/include/sys/` is not installed.)

### `restrict` in the library

The C99 prototypes take `restrict` where the standard has it (`<string.h>`: `memcpy`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strtok`, `strlcpy`; `<stdio.h>`: the `printf` / `scanf` families, `fopen`, `freopen`, `fgets`, `fputs`, `fread`, `fwrite`, `fgetpos`, `setbuf`, `setvbuf`; `<stdlib.h>`: `strtol`, `strtoul`, `strtof`). That documents the contract (arguments that overlap are undefined); cc45 uses `restrict` only inside the function that declares it, so a call compiles the same. The C functions where it shrinks the code have it on their definitions: `fputs`, `strspn`, `strcspn`, `strpbrk`, `strlcpy`, `fcm_cputs`, `midi_parse`, `midi_synth_message`, `midi_synth_active`, `__stridx_table`, `__intN_cmp_u`, `__intN_shr_u` / `_s`, the scanf source's `__scan_getc` / `__scan_ungetc` and the memory streams' writer (339 bytes over both archives; no function grows). The assembly routines (`memcpy`, `strcpy`, ...) are not compiled from C.

## 2. Header Overview

| Header | Contents | Implementation |
|--------|----------|----------------|
| `stddef.h`, `stdint.h`, `stdbool.h`, `limits.h`, `float.h`, `stdarg.h`, `sys/types.h` | types and limits | header only |
| `stdlib.h` | conversions, `rand`, heap, `exit`/`abort`/`atexit`, `qsort`/`bsearch` | library |
| `string.h` | string and memory functions | library (mostly assembly) |
| `ctype.h` | PETSCII character classes | `static inline` in the header |
| `math.h` | integer helpers, float math | library |
| `stdio.h` | console output, `printf` family, `FILE` streams, CBM file API | library |
| `errno.h` | `errno`, `_errnoc`, error codes | library |
| `setjmp.h`, `assert.h`, `alloca.h` | non-local jumps, assertions, stack allocation | library / compiler |
| `time.h` | jiffy clock, MEGA65 RTC | library |
| `cbm.h` | KERNAL wrappers ([stdcbm.md](stdcbm.md)) | `static` functions in the header |
| `mega65.h` | hardware register overlays | header (+ `key_pressed` in the library) |
| `dma.h` | `dma_copy`/`dma_fill` (16-bit, bank 0); `dma_copy_far`/`dma_fill_far` (28-bit); `dma_copy_far_step`/`dma_fill_far_step` (with a step) | compiler builtins; `lib/stdlib_asm/dma_far.s45`, `dma_step.s45` |
| `palette.h` | the VIC-IV's 256-entry palette: set, get, save, restore, rotate ([§13](#13-mega65-hardware-mega65h-and-dma-dmah)) | library (`stdlib/palette.c`) |
| `seam.h` | screens of two-byte cells: full-colour characters among text ([§13](#13-mega65-hardware-mega65h-and-dma-dmah)) | library (`stdlib/seam.c`) |
| `tgi.h`, `tgi/tgi-error.h`, `tgi/tgi-kernel.h`, `tgi/tgi-vectorfont.h` | cc65's TGI graphics: 320x200, 256 colours, one or two pages ([§12a](#12a-graphics-tgih)) | library (`stdlib/tgi*.c`) |
| `input.h` | see [§11a](#11a-keyboard-and-joysticks-inputh) | library |
| `overlay.h` | `__ovl_current` (the code overlay in the region), `__ovl_region`: in a program with code overlays (the runtime `stdlib_asm/ovl.s45`; [ln45: Code Overlays](../bin/ln45.md#code-overlays)) | library / ln45 |
| `conio.h`, `far_ptr.h`, `far_ops.h`, `far_string.h`, `dma_controller.h`, `audio_digi.h`, `audio_midi.h`, `audio_sid.h`, `intwide.h`, `complex.h` | see [§12](#12-other-library-backed-headers) | library / compiler |
| `uart.h` and the graphics, sprite, hardware-helper and game-framework headers | see [§14](#14-headers-without-a-library-implementation) | **no library implementation** |

## 3. Core Headers & Types

- **`<stddef.h>`**: `NULL` (`0`), `size_t` (`unsigned int`, 16-bit), `ptrdiff_t` (`signed int`), `wchar_t` (`unsigned int`, 16-bit — `L"..."`/`L'x'`), `offsetof(type, member)`.
- **`<stdint.h>`**: `int8_t`/`uint8_t` (`char`), `int16_t`/`uint16_t` (`int`), `int32_t`/`uint32_t` (`long`), the `least`/`fast` variants, `intptr_t`/`uintptr_t` (16-bit), `intmax_t`/`uintmax_t` (32-bit), limits and `INTn_C` macros. `int64_t`, `uint64_t` and the 64-bit `least`/`fast` types are `struct __int64` (see `intwide.h`), which cc45 adds to a unit that names a 64-bit type. The signed types are declared `signed`, so they are signed in cc45's default (unsigned) mode.
- **`<stdbool.h>`**: `bool` (`_Bool`, 1 byte, normalizes non-zero to 1), `true` (1), `false` (0), `__bool_true_false_are_defined` (1).
- **`<limits.h>`**: `CHAR_BIT` 8; `SCHAR_MIN`/`SCHAR_MAX` −128/127; `UCHAR_MAX` 255; `CHAR_MIN`/`CHAR_MAX` 0/255; `SHRT_MIN`/`SHRT_MAX`/`USHRT_MAX` and `INT_MIN`/`INT_MAX`/`UINT_MAX` −32768/32767/65535; `LONG_MIN`/`LONG_MAX`/`ULONG_MAX` for 32 bits; `MB_LEN_MAX` 1. The signed limits describe `signed` types; plain `int`/`long` are unsigned in cc45's default mode.
- **`<float.h>`**: CBM 40-bit float (1 exponent byte, 4 mantissa bytes): `FLT_RADIX` 2, `FLT_MANT_DIG` 32, `FLT_DIG` 9, `FLT_MIN_EXP` −127, `FLT_MAX_EXP` 127, `FLT_MIN_10_EXP` −38, `FLT_MAX_10_EXP` 38, `FLT_ROUNDS` 1, `FLT_MAX` 1.7014118342085515e38 (exponent `$FF`, mantissa all ones), `FLT_MIN` 2.9387358770557188e-39 (2^-128), `FLT_EPSILON` 4.6566128730773926e-10 (2^-31), written with enough digits to give exactly those bytes (`test_float_limits_xemu.c`); the predefined `__FLT_MAX__`/`__FLT_MIN__`/`__FLT_EPSILON__` are the same; `DBL_*` and `LDBL_*` are the same (`double` and `long double` are the same format).
- **`<stdarg.h>`**: `va_list` (`unsigned int`), `va_start`, `va_arg`, `va_end` (wrapping `__builtin_va_*`) and `va_copy`. Variadic functions always take their arguments on the stack, also under `-fzpcall`. See [../bin/cc45.md](../bin/cc45.md#variadic-functions).
- **`<sys/types.h>`**: `ssize_t`, `mode_t`, `pid_t`, `uid_t`, `gid_t`, `off_t`, `time_t`.

## 4. General Utilities (`stdlib.h`)

**Conversions**
- **`int atoi(char *s)`**: decimal, optional sign, 16-bit result (assembly). It skips leading spaces only (not tabs/newlines).
- **`long atol(char *s)`**: `strtol(s, NULL, 10)`.
- **`char *itoa(int value, char *str, int base)`** / **`char *utoa(unsigned value, char *str, int base)`** / **`char *ltoa(long value, char *str, int base)`**: number to string in base 2–36, using the MEGA65 hardware divider. `itoa`/`ltoa` print a `-` for negative values in base 10 only. Digits above 9 are PETSCII `$41`… (`a`… in the default character set). Return `str`.
- **`atos(s)`**, **`stoa(value, str, base)`**: macros for `(short)atoi(s)` and `itoa((int)value, str, base)`.
- **`long strtol(char *nptr, char **endptr, int base)`** / **`unsigned long strtoul(...)`**: skip whitespace, optional sign (`strtol`), base 0 (auto: `0x`→16, `0`→8, else 10) or 2–36, `0x`/`0X` prefix accepted for base 16, letters in either case as digits (`$41`–`$5A`, `$61`–`$7A`, `$C1`–`$DA`). The `0x` prefix counts only when a hex digit follows it (`"0x"` is 0, ending after the `0`). `*endptr` is set to `nptr` if no digits were read. A value out of range gives `LONG_MAX`/`LONG_MIN` (`strtol`) or `ULONG_MAX` (`strtoul`) and sets `errno = ERANGE`; `strtoul` negates a value after `-` (as C specifies); a base other than 0 or 2–36 gives 0 and `EINVAL`. Assembly (`stdlib_asm/strtoul.s45`; the multiply by the base on the hardware multiplier).
- **`float strtof(char *nptr, char **endptr)`** / **`float atof(char *nptr)`**: whitespace, sign, digits, fraction, `e`/`E` exponent (only when digits follow it). The digits are collected as an exact integer and scaled by exact powers of ten, so values such as `0.25` convert exactly. `*endptr` is `nptr` when there are no digits. A value beyond the float range gives ±`FLT_MAX`; a non-zero value that underflows gives 0; both set `errno = ERANGE`. `strtod(nptr, endptr)` is a macro for `(double)strtof(...)`.

**Arithmetic**
- **`abs(x)`**, **`labs(x)`**: macros (`(x)<0?-(x):(x)`). Define `__NOMACRO_ABS` / `__NOMACRO_LABS` before including to get the library functions `signed int abs(signed int)` / `signed long labs(signed long)` instead.
- `__builtin_abs`, `__builtin_labs`, `__builtin_ffs`, `__builtin_clz`, `__builtin_ctz`, `__builtin_popcount`, `__builtin_bswap16` are declared here; the library also provides the `l`/`ll` variants of the bit builtins, `__builtin_parity*` and `__builtin_bswap32`.

**Random numbers**
- **`int rand(void)`**: 0–`RAND_MAX` (32767). Until `srand` is called: the MEGA65 hardware RNG (`$D7EF`, waiting on `$D7FE` bit 7 before each of the two byte reads), different on every run. After `srand`: C's reference generator (`next = next * 1103515245 + 12345`, result bits 16–30), so `srand(1)` gives 16838 first.
- **`void srand(unsigned seed)`**: seeds the reproducible generator; the same seed repeats the same sequence (`lib/stdlib_asm/rand.s45`; the multiply by the hardware multiplier).

**Heap** — `void *malloc(size_t)`, `void free(void *)`, `void *calloc(size_t, size_t)`, `void *realloc(void *, size_t)` (assembly, `lib/stdlib_asm/malloc.s45`):
- Sorted singly-linked free list, first fit, coalescing with both neighbours on `free`. Block header: 2-byte size (bit 0 = allocated) and, in free blocks, a 2-byte next pointer. Requests are rounded up to even, minimum 4 bytes including the 2-byte header.
- `malloc(0)` and failure return `NULL` (`errno` is not set). `free(NULL)` is a no-op. `calloc` = `malloc` + `memset`; `realloc` = `malloc` + `memcpy` + `free` (`realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` frees `p`).
- The heap is initialized on the first `malloc` call. It runs from `__heap_start` to `__heap_end`; both default to 0 (library member `__heap_bounds`), which means: start at `__bss_end` (after the program and its private base page), end `__stack_size` bytes below the 16-bit stack top (`$B000` with the default 4096 and `$BFFF` top; cc45 `-fstack-size=n` / `#pragma cc45 stack_size(n)`, ln45 `--stack-size=n`) or at `$C000` with the 8-bit page-1 stack. Define the two symbols in your program to place the heap elsewhere. When the program itself reaches past the heap end (a large program, BSS included), the heap is empty and `malloc` returns `NULL`; ln45 warns about it (`no heap: the program ends at ...`), and a smaller stack share gives such a program a heap.
- `realloc` keeps the old block when the new allocation fails. A size over `$FFFC` (no room for the block's header) and a `calloc` product `nmemb * size` over 64 KB return `NULL` (`test_malloc_xemu.c`).
- `#pragma cc45 heap` makes standalone (non-linked) builds call `_init_heap_crt` at startup; linked programs rely on the lazy initialization.

**Program termination**
- **`void exit(int status)`** (weak) / **`void _exit(int status)`**: jump to the CRT's `__exit`, which switches back to the ROM's base page, stores `status` at `$FD/$FE`, restores the caller's stack pointer and flags and returns to the caller (BASIC). `exit` and a return from `main` first run the `atexit` handlers; `_exit` and `abort` do not.
- **`void abort(void)`** (weak): like `exit`, with status `$FFFE`. If `__exit` is never reached the status reads `$FFFF` (set at startup). A BASIC caller can read it with `PEEK(253)+256*PEEK(254)`.
- **`int atexit(void (*func)())`**: registers up to 32 handlers (returns 1 when full), called in reverse order, each once, by `exit` and after `main` returns (`__atexit_run`; crt0 has a weak empty one for programs that never call `atexit`).
- `#pragma cc45 exit_halt` / `exit_brk` / `exit_rts` only change the startup stub of standalone builds; a program linked with the library always returns to its caller.

**Sorting and searching**
- **`void qsort(void *base, int nmemb, int size, int (*cmp)())`**: quicksort (assembly, `stdlib_asm/sort.s45`) with median-of-three pivot, insertion sort for partitions of ≤ 8 elements, the smaller partition first and the larger on a 16-entry stack (not recursive). The comparator is called through `__sort_cmp` (`stdlib/qsort.c`, in the program's calling convention). The state is in static storage, so `qsort` is not reentrant (a comparator must not call `qsort` or `bsearch`).
- **`void *bsearch(const void *key, const void *base, int nmemb, int size, int (*cmp)())`**: binary search (assembly, `sort.s45`); returns the match or `NULL`.

## 5. String & Memory Operations (`string.h`)

The functions marked *asm* are hand-written assembly in `lib/stdlib_asm/`; the rest are C in `lib/stdlib/`. Lengths and counts are `int` (16-bit). The parameters are declared as in the C standard: `const` where the function only reads, `restrict` where the areas must not overlap (`memcpy`, `strcpy`, `strcat`, …; overlapping copies are undefined, use `memmove`).

| Function | Notes |
|----------|-------|
| `int strlen(const char *s)` | *asm*. Any length up to 65535 (Y walks a page, X counts the pages) |
| `char *strcpy(char *restrict d, const char *restrict s)`, `char *strncpy(char *restrict d, const char *restrict s, int n)` | *asm*. `strncpy` NUL-pads |
| `signed int strcmp(const char *a, const char *b)`, `signed int strncmp(const char *a, const char *b, int n)` | *asm*. Return <0, 0, >0 (declared `signed`: a plain `int` is unsigned in cc45, which made `strcmp(a, b) < 0` always false) |
| `char *strcat(char *restrict d, const char *restrict s)`, `char *strncat(char *restrict d, const char *restrict s, int n)` | *asm*. `strncat` appends at most `n` chars and NUL-terminates |
| `char *strchr(const char *s, int c)`, `char *strrchr(const char *s, int c)` | *asm* |
| `char *strstr(const char *h, const char *n)`, `char *strpbrk(const char *s, const char *accept)` | C |
| `int strspn(const char *s, const char *accept)`, `int strcspn(const char *s, const char *reject)` | C |
| `char *strtok(char *restrict s, const char *restrict delim)`, `char *strtok_r(char *restrict s, const char *restrict delim, char **restrict save)` | Assembly (`strtok.s45`). `strtok` keeps static state; `strtok_r` keeps it in `*save` (NULL after the last token) |
| `int strnlen(const char *s, int maxlen)` | C |
| `int strlcpy(char *restrict dst, const char *restrict src, int size)` | C. Returns `strlen(src)` |
| `int stridx(const char *s, ...)` | C (`stdlib/stridx.c`). Which of the strings after `s` (ended by `NULL`) `s` is: 1 for the first equal one, 0 for none (`switch (stridx(cmd, "load", "save", NULL))`). When every candidate is a string literal cc45 calls `__stridx_table(s, table)` instead: the candidates in one block (a count, then each ended by 0), no variadic call, and the `NULL` may be left out; a literal `s` too gives the index at compile time; at `-O3` a small set (a trie of up to 6 nodes) is compared inline. A call without the `NULL` gets one added |
| `char *strdup(const char *s)` | C. Uses `malloc` |
| `char *strerror(int errnum)` | C. "Success", "Result out of range", "Out of memory", "Invalid argument", "Math domain error", else "Unknown error" |
| `void *memcpy(void *restrict d, const void *restrict s, int n)` | *asm*. A DMA copy job (#325); no overlap handling |
| `void *memmove(void *d, const void *s, int n)` | *asm*. A DMA copy job, downwards from the last bytes when `d > s` |
| `void *memset(void *s, int c, int n)` | *asm*. A DMA fill job |
| `signed int memcmp(const void *a, const void *b, int n)` | *asm*. Returns <0, 0, >0 |
| `void *memchr(const void *s, int c, int n)` | C |

`memcpy`, `memmove` and `memset` fill in one DMA job (`stdlib_asm/dma_mem.s45`, the job and its start in `dma_job.s45`, shared with `dma_copy_far`) and start it with interrupts held off while it is filled in; the CPU waits for the DMA job, which takes much less time than the CPU loops for all but a few bytes. A length of 0 does nothing. They see bank 0 as the CPU does for I/O (the job's I/O flag: `$D000`-`$DFFF`, such as colour RAM at `$D800`, are the registers), and in a program with mapped code overlays (`--overlay-map`, `__far` functions) an address in the window is read where the overlay in it is kept (`__ovl_dma_fix` in `ovl.s45`; crt0's weak version does nothing). Under the ROMs (`$C000`-`$CFFF` and the KERNAL at `$E000` in C65 mode) the DMA sees RAM, not the ROM the CPU reads there. The direction of a downward copy is the F018B command's bits 4 and 5 (the F018A bank bit 6 is not used in that format). They return their first parameter (the CPU loops returned it 256 higher for each full page copied). Sizes: `memcpy` 16 bytes, `memmove` 28, `memset` 16, with the shared 95-byte job builder and 42 bytes of job and start (the CPU loops were 34, 128 and 36). Test: `test_dma_mem_xemu.c`; the `__noescape` overlay test copies from an overlay's constants, mapped.

On string literals cc45 does without some of these calls (`strlen("hello")` is 5, `strcmp(s, "GO") == 0` byte compares, `strcpy(d, "lit")` a `memcpy`; `-fno-builtin` keeps them; [optimizations.md](optimizations.md)).

`string.h` also declares `__builtin_memcpy`, `__builtin_memset`, `__builtin_memmove`, `__builtin_memcmp`, `__builtin_strlen`, `__builtin_strcpy` and `__builtin_strcmp`, which the library provides. The library also exports `memcpy_optimized` (`stdlib/memcpy_fast.c`), which is not declared in any header.

### fixed.h

The fixed-point types `__fixed(I, F)` (`__fixed8_8`, `__fixed16_16`) are cc45's own ([cc45.md](../bin/cc45.md#fixed-point-__fixedi-f)); the header comes with them (included automatically):

| Name | Description |
|------|-------------|
| `signed long __fx32_mul(signed long a, signed long b, unsigned char f)`, `__fx32_div` | *asm* (`stdlib_asm/fixed.s45`). The integers of two 32-bit fixed values of `f` fraction bits: `(a * b) >> f` and `(a << f) / b`, truncated toward 0. The magnitudes go through the hardware multiplier ($D770 × $D774 → $D778, ready at once) and divider (the 32.32 quotient at $D768, after its busy bit at $D70F); 5 bytes of the 64-bit result from byte f / 8 (32 - f for the quotient), shifted the rest. cc45 calls them for `*` and `/` |
| `fixedToFloat(v)`, `fixedToInt(v)` | Macros: any fixed value as a `float` (`printf("%f", fixedToFloat(x))`) or a `signed long`, the fraction dropped toward 0 |
| `toFloat8_8(v)`, `toFloat16_16(v)`, `toInt8_8(v)`, `toInt16_16(v)` | Macros: the same, v converted to `__fixed8_8` / `__fixed16_16` first |

## 6. Character Utilities (`ctype.h`)

All functions are `static inline` in the header and take an `int` PETSCII code (the library also has assembly versions, used only when `ctype.h` is not included). They return 1 or 0, except the conversions.

PETSCII ranges (default upper/lower-case character set): digits `$30`–`$39`; lower-case letters `$41`–`$5A`; upper-case letters `$C1`–`$DA` (what cc45 makes of `'A'`–`'Z'` in C source, as BASIC's `ASC("A")`) and `$61`–`$7A` (the other form of the capitals, shown the same, so the functions accept it too); space `$20`.

| Function | True for |
|----------|----------|
| `isdigit` | `$30`–`$39` |
| `isalpha` | `$41`–`$5A`, `$61`–`$7A`, `$C1`–`$DA` |
| `isalnum` | digit or letter |
| `isspace` | `$20`, `$09`–`$0D` |
| `isblank` | `$20`, `$09` |
| `isupper` / `islower` | `$61`–`$7A`, `$C1`–`$DA` / `$41`–`$5A` |
| `isxdigit` | digit, `$41`–`$46`, `$61`–`$66`, `$C1`–`$C6` |
| `isprint` | `$20`–`$7E`, `$A0`–`$FF` |
| `isgraph` | `$21`–`$7E`, `$A1`–`$FF` |
| `iscntrl` | `$00`–`$1F`, `$80`–`$9F` |
| `ispunct` | everything from `$21` up that is not a digit or letter (this includes `$7F`–`$9F` and `$A0`) |

`toupper(c)` returns `c | $80` for `$41`–`$5A` (so `toupper('a') == 'A'`, `$C1`); `tolower(c)` returns `c - $20` for `$61`–`$7A` and `c & $7F` for `$C1`–`$DA`; other values are returned unchanged.

## 7. Math Library (`math.h`)

`math.h` includes `stdlib.h`.

**Integer**
- `div_t div(signed int, signed int)` and `ldiv_t ldiv(signed long, signed long)` (`quot`, `rem`; the quotient truncated toward zero), declared in `stdlib.h` as C specifies.
- `signed int min(a, b)`, `max(a, b)`, `gcd(a, b)` (Euclid, handles negative inputs), `lcm(a, b)` (`|a / gcd(a,b) * b|`).
- `abs`/`labs` as in `stdlib.h`.

**Floating point** (CBM 40-bit; `double` is the same type)
- ROM-backed (BASIC 65 routines through `float_rom.s45`, which switches to base page 0 for each call): `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf`.
- Written in C: `powf`, `fmodf`, `ceilf`, `floorf`, `roundf` (ties away from zero), `truncf`, `atan2f`, `log10f`, `log2f`, `ldexpf`, `frexpf`, `modff`, `copysignf`, `fmaxf`, `fminf`, `fdimf`. `ldexpf` and `frexpf` change the exponent byte, so they are exact (`ldexpf` beyond the float range gives ±largest float or 0 with `errno = ERANGE`); `log2f` is exact for powers of two.
- The standard names (`sin`, `cos`, `tan`, `atan`, `atan2`, `log`, `log10`, `log2`, `exp`, `pow`, `sqrt`, `fabs`, `fmod`, `ceil`, `floor`, `round`, `trunc`, `ldexp`, `frexp`, `modf`, `copysign`, `fmax`, `fmin`, `fdim`) are macros for the `f` versions; the library also defines them as functions (`math_double.c`) for code that declares them itself, together with `signbit`/`signbitf`.
- Classification: `isnan(x)` and `isinf(x)` are always 0 and `isfinite(x)` always 1 (the format has no NaN or infinity); `__fpclassifyf(x)` returns 0 for zero, else 1.
- Constants: `M_PI`, `M_PI_2`, `M_PI_4`, `M_E`, `M_LOG2E`, `M_LOG10E`, `M_LN2`, `M_LN10`, `M_SQRT2`, `M_SQRT1_2`, `HUGE_VALF` (= `FLT_MAX`), `INFINITY` (= `HUGE_VALF`).

**Domain and range errors** never reach the ROM (whose `?ILLEGAL QUANTITY`/`?OVERFLOW ERROR` would not return):

| Case | Result | `errno` |
|------|--------|---------|
| `sqrtf(x < 0)`, `logf(x < 0)` | 0 | `EDOM` |
| `logf(0)` | −`FLT_MAX` | `ERANGE` |
| `expf(x > 88.0296)` | `FLT_MAX` | `ERANGE` |
| `expf(x < −88.8)` | 0 | `ERANGE` |
| `powf(negative, non-integer)` | 0 | `EDOM` |
| `powf(0, y < 0)` | `FLT_MAX` | `ERANGE` |
| `powf` overflow / underflow | via `expf` | `ERANGE` |
| `fmodf(x, 0)` | 0 | `EDOM` |

Float arithmetic itself (`+ - * /`) saturates to ±`FLT_MAX` on overflow, `x / 0` gives ±`FLT_MAX` (`0 / 0` gives 0) and underflow gives 0; these do not set `errno`.

## 8. I/O (`stdio.h`)

### Console

- **`int putchar(int c)`**: KERNAL `CHROUT` (`$FFD2`).
- **`int getchar(void)`**: waits for a key with KERNAL `GETIN` (`$FFE4`); returns the PETSCII code.
- **`int puts(char *s)`**: prints `s` (any length) and a carriage return (`$0D`) through `CHROUT`; returns 0. `int putstr(const char *s)` (same member, `puts.s45`; also `__builtin_putstr`) prints `s` without the carriage return; returns 0.
- **`printf` of plain text**: cc45 compiles a `printf("text")` statement whose text has no `%` (and whose result is not used) as `puts("text")` when the text ends in `\n` (default PETSCII encoding), else as `putstr("text")`, so the program does not link the formatter (`printf("hello\n")`: 6458 → 620 bytes). Not when the unit defines its own `printf`, `puts` or `putstr`.

These (and every other ROM call in the library) are bracketed with `__bp_rom`/`__bp_prog`, which switch to the ROM's base page 0 and back to the program's relocated base page.

### Formatted output

`printf(char *fmt, ...)`, `vprintf(char *fmt, va_list)`, `sprintf(char *buf, char *fmt, ...)`, `vsprintf`, `snprintf(char *buf, int size, char *fmt, ...)`, `vsnprintf` (writes at most `size-1` characters plus NUL, returns the length that would have been written), `fprintf`/`vfprintf` (to a `FILE`). They return the number of characters written. All share one formatter, `__vformat` (hand-written assembly, `stdlib_asm/__vformat.s45`, 1.8 KB; it was 5.5 KB of compiled C), which writes through a callback (`__fastcall__`: the ZP convention in both libraries):

- Syntax `%[flags][width][.precision][length]conversion`; flags `- + space # 0`; width and precision as digits or `*`; length `hh h l ll L j z t`.
- Conversions: `d i u x X o b p c s n %`, and `f F e E g G`. `int` is 16 bits, `long` 32 bits; `ll` reads 8 bytes and prints the low 32 bits. `%b` is binary, `%p` prints `$hhhh`. An unknown conversion character is printed as is.
- Integer digits are hooks linked only as the program's format strings need them (`lib/stdlib/fmtconv.h`): `__fmt_utoa` (`stdlib_asm/fmt_utoa.s45`, 48 bytes) for `int`/`char` arguments, `__fmt_ultoa` (`fmt_ultoa.s45`, 56 bytes, which also serves `int` arguments when it is linked alone) for `long`. cc45 reads the literal format strings of `printf`, `vprintf`, `fprintf`, `vfprintf`, `sprintf`, `vsprintf`, `snprintf`, `vsnprintf`, `cprintf`, `cprintfxy` and `fcm_cprintf` calls and references the hooks' markers (`__fmt_int_linked`, `__fmt_long_linked`) from the calling function (ca45 `.ref`, so a dropped function needs nothing); a format that is not a literal links `__fmt_ultoa`, as does using one of those functions as a value; one of them passing its own format parameter on (`printf` calling `vprintf`) needs nothing. Without a hook (a weak stub in `__vformat`) the conversion prints no digits but still takes its argument. A program printing only `%s`/`%c` text links no digit code. With the assembly formatter: `sprintf(buf, "%s-%c", ...)` 6970 → 3260 bytes (the C formatter with all its digits), with `%d` 7010 → 3349, with `%ld` 6996 → 3343.
- Floats: `printf`/`sprintf` are linked in an integer-only (weak) version by default; passing a float argument in the `...` of a variadic call (or to an unprototyped function) makes the linker pull `printf_float`/`sprintf_float`, whose strong `printf`/`sprintf` format floats. Without them the float conversions print nothing. The float formatter, `__pf_format` (hand-written assembly, `stdlib_asm/pf_format.s45`, 0.8 KB; it was 3.6 KB of compiled C, and gives the same text in 792 checked cases), takes its digits from the BASIC 65 ROM's own number formatting (FOUT, which `PRINT` and `STR$` use; `__float_fout` in `float_rom.s45`), so every value of the CBM format prints (up to about ±1.7×10³⁸, with the 9–10 significant digits the format holds; further digits are 0). `%f`, `%e`, `%g` and `%F`/`%E`/`%G` follow C: precision (default 6; at most 20), rounding half up (`%.0f` of 2.5 is `3`), `%e` with an exponent of at least two digits (`1.000000e+30`), `%g` choosing the form by exponent and dropping trailing zeros, the `#` flag, and width / `-` / `0` / `+` / space. FOUT works in page 1 (`$0100`), unused with the default 16-bit CPU stack. One ROM quirk is handled: FOUT prints a value just below a power of ten (9.99999999…E19) as `" 0E19"`; the formatter reads that as 1E20. `snprintf`, `vsnprintf`, `vprintf`, `vsprintf` and `fprintf` have no float support.

### Formatted input

`sscanf`/`vsscanf` (a string), `fscanf`/`vfscanf` (a `FILE`) and `scanf`/`vscanf` (`stdin`) share one engine, in assembly (`lib/stdlib_asm/scanf_core.s45`, 1.7 KB; the C version was 4.8 KB):
- Conversions `d i u o x X p c s [set] [^set] n %`, and `f e g` (upper case too) with the float scanf below; field widths; `*` (read but not assigned); length modifiers `hh h l ll L j z t` (`ll` stores 8 bytes, the upper four the sign). `%i` takes `0x` (hex) and `0` (octal) prefixes, `%x` a `0x` or CBM `$` prefix; letters are digits in either case (`'A'` in cc45 source is `$C1`; the other form `$61`–`$7A` too). `%c` reads exactly its width (default 1) without skipping whitespace; `%s` reads a whitespace-delimited word; `%[` a run of the set (`]` first and `a-z` ranges allowed).
- Whitespace in the format matches any amount of input whitespace; other characters must match. Returns the number of items assigned, or `EOF` when the input ends before the first conversion. On a `FILE`, the character after the input used is pushed back with `ungetc`.
- The engine reads through `__scan_getc`/`__scan_ungetc`: weak string-only versions (`scanf_str.c`), replaced by `stdio_format.c`'s FILE-capable ones when `fscanf` and friends are linked, so `sscanf` alone links no FILE code. No call goes through a pointer. These and `__scan_float` are `__fastcall__` (`scanf_core.h`), so the engine calls them in the ZP convention in both libraries; it keeps its state (and the `struct __scan_src` it passes them) in BSS, so it is not reentrant. Tested by `test_scanf_xemu.c`.
- Floats, like printf: `%f %e %g` call `__scan_float`, a weak stub (a matching failure) unless `scanf_float.c` is linked. cc45 references it (`_scanf_float`) when a `scanf`, `fscanf` or `sscanf` call passes a `float *`/`double *`, or when a unit with float code calls `vscanf`, `vfscanf` or `vsscanf`; it then reads `[sign] digits [. digits] [e [sign] digits]` and converts with `strtof`.

### FILE streams

`FILE` (handle, mode, flags, position, buffer, `ungetc` slot), `fpos_t` (`unsigned long`), `stdin`/`stdout`/`stderr`, `EOF` (−1), `SEEK_SET/CUR/END`, `BUFSIZ` 256, `FILENAME_MAX` 16, `FOPEN_MAX` 8, `L_tmpnam` 32, `TMP_MAX` 10000, `_IOFBF`/`_IOLBF`/`_IONBF`, `_IO*` flag bits.

Functions: `fopen`, `freopen`, `fclose`, `fflush`, `fgetc`, `fputc`, `fgets`, `fputs`, `ungetc`, `fread`, `fwrite`, `fprintf`, `vfprintf`, `fscanf`, `vfscanf`, `scanf`, `vscanf`, `fseek`, `ftell`, `rewind`, `fgetpos`, `fsetpos`, `feof`, `ferror`, `clearerr`, `setvbuf`, `setbuf`, `tmpfile`, `tmpnam`, `perror` (prints `s: strerror(errno)` to `stderr`). `fileno(stream)` is a macro (`stream->handle`) and `fdopen(fd, mode)` a cast.

Behaviour on the MEGA65:
- `fopen` opens files on disk device 8. The DOS takes the type and direction from the name, so it opens `NAME,S,R` (`r`), `@0:NAME,S,W` (`w`; the `@` replaces an existing file, as C's `w` truncates) or `NAME,S,A` (`a`); `b` makes it a USR file (`U`). A name that already contains a `,` is used as given. Each file gets its own drive channel (secondary address = its logical file number). `OPEN` succeeds even for a missing file, so `fopen` then reads the drive's status (`cbm_drive_status`) and fails on an error (≥ 20, e.g. 62 FILE NOT FOUND). Tested on xemu with a D81 (`test_fileio_xemu.c`).
- **`r+`, `w+`, `a+`** (CBM drives don't read and write a sequential file at once): the stream is held in memory. `fopen` reads the whole file into the heap (`r+` fails for a missing file; `w+` creates the file empty at once; `a+` creates it when written back); reads, writes and `fseek` work on that copy, and `fflush`/`fclose` write it back with `@0:` replacement. A write past the end after a seek fills the gap with zeros; `a+` always writes at the end. The file must fit in the heap (at most 65280 bytes).
- **`fseek`/`ftell`/`rewind`/`fgetpos`/`fsetpos`** (in `stdio_seek.c`, linked only when used): a memory stream seeks anywhere. A stream opened with `r` seeks forward by reading and backward by reopening the file and reading to the position; `SEEK_END` reads to the end first. A stream opened with `w` or `a` (written sequentially) can only seek to where it is: anything else fails with `errno = ESPIPE`. `ungetc` then `fgetc` keeps `ftell` right. `test_fileseek_xemu.c` (both conventions) covers all of it.
- `fgets`/`fputs` are in `stdio_line.c`, apart from `fprintf`/`fscanf`, so line I/O does not link the printf machinery (also before ln45 dropped unused functions).
- **`setvbuf(stream, buf, mode, size)`** before the stream's first read or write: `_IONBF` passes every character on at once (a 1-byte buffer), `_IOFBF`/`_IOLBF` buffer `size` bytes in `buf` (or a `malloc`'d buffer when `buf` is NULL); a text stream is written out at each newline in either mode. It returns −1 after I/O, for an unknown mode or without memory; a memory stream ignores it (0). `setbuf(stream, buf)` is `setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ)`.
- **`tmpfile()`**: a read/write binary stream held in memory (the heap limits its size), never written to a disk, so nothing is left to remove after `fclose`.
- **`tmpnam(s)`**: `"TMP0000"` to `"TMP9999"`, the next one no file on device 8 has (checked by opening it), in `s` or a static buffer; NULL when all are taken.
- `test_stdio3_xemu.c` covers the scanf family on files, `setvbuf`, `setbuf`, `tmpfile`, `tmpnam` and `freopen`.

### CBM file API

Declared in `stdio.h`, implemented in `stdio_cbm.c`, `stdio_kernal.c`, `stdio_directory.c`, `stdio_rel.c`:

- Files: `fd_t cbm_open(device, filename, type)` (secondary address 0), `cbm_open_ext(device, filename, type, secondary)` (`$FF`: the logical file number), `cbm_close`, `cbm_read`, `cbm_write` (these return `signed int`, −1 on error; `fd_t` is `signed int`), `cbm_drive_status(device)` (the number of the drive's status message: 0 OK, ≥ 20 an error, −1 no channel), `cbm_eof(fd)` (1 once a read reached the file's end), `cbm_status(fd)` (the KERNAL status byte), `cbm_tell(fd)` (bytes read or written) and `cbm_seek(fd, position)` (a file being read: forward by reading, back by reopening it with the same device, secondary address and name (up to 23 characters) and reading; −1 past the end or for a file that was written, unless it stays where it is). Logical file numbers 3–14 are allocated as free ones (15 is the command channel, which stays open while files are open: closing it makes the drive close all its files). `cbm_read` keeps the byte that arrives with the end-of-file status. Constants `CBM_DEVICE_*`, `CBM_T_DEL/SEQ/PRG/USR/REL/CBM/DIR` (`$80`–`$86`: a closed file's directory type byte), `O_*`, `STDIN_FILENO`…
- Directory and disk commands: `cbm_opendir(device)` (opens `$` and skips the load address and the header line), `cbm_readdir(handle, name, &type, &blocks)` (the next entry of the listing the drive sends, a BASIC program: the name between the quotes, the size from the line number, the type byte rebuilt from the type text — the type code, `CBM_FILE_CLOSED` unless marked `*`, `CBM_FILE_LOCKED` for `<`; 1 at `BLOCKS FREE.`), `cbm_closedir`, `cbm_dir_type_name`, `cbm_dir_get_type`, `cbm_dir_is_locked`, `cbm_dir_is_scratched`, `cbm_dir_is_closed`, `cbm_format`, `cbm_delete`, `cbm_rename`; directory type flag bits `CBM_FILE_*`.
- Relative (REL) files: `cbm_rel_open(device, filename, record_size, mode)`, `cbm_rel_read`, `cbm_rel_write`, `cbm_rel_position`, `cbm_rel_tell`, `cbm_rel_size`, `cbm_rel_close`, `cbm_rel_record_size`, `cbm_rel_is_open`, `cbm_rel_append`, `cbm_rel_delete`, `cbm_rel_truncate`, `cbm_rel_update`, `cbm_rel_insert`, `cbm_rel_lasterror`, `cbm_rel_strerror`; `CBM_REL_READ`/`WRITE`, `CBM_REL_ERR_*`. A handle that was never opened is not open (`CBM_REL_ERR_INVALID`), also before the first `cbm_rel_open`.
- KERNAL wrappers: `cbm_k_setnam(filename)`, `cbm_k_setlfs`, `cbm_k_open`, `cbm_k_close`, `cbm_k_chkin`, `cbm_k_chkout`, `cbm_k_clrchn`, `cbm_k_chrin`, `cbm_k_chrout`, `cbm_k_readst`. `test_cbmio_xemu.c` covers the file, directory and KERNAL functions on a D81.

`cbm_open` and `cbm_close` here have different signatures from the `static` `cbm_open(void)`/`cbm_close(char)` in `cbm.h`; in a file that includes both headers the `cbm.h` versions are used without a diagnostic.

## 9. Error Handling (`errno.h`)

- **`extern int errno`**: set by the float functions, `strtof`, `strtol` and `strtoul` (see [§7](#7-math-library-mathh) and [§4](#4-general-utilities-stdlibh)). The library never clears it; `malloc` does not set it.
- **`extern char _errnoc`**: the KERNAL status byte `ST` at `$0090` of the ROM's page 0 (read-only alias).

| Constant | Value | Used for |
|----------|-------|----------|
| `ERANGE` | 1 | result out of float range (`strtof`, `logf(0)`, `expf`, `powf`) |
| `ENOMEM` | 2 | defined; not set by the library |
| `EINVAL` | 3 | defined; not set by the library |
| `EDOM` | 4 | argument without a value (`sqrtf`/`logf` of a negative, `powf` negative^fraction, `fmodf(x, 0)`) |

## 10. Non-Local Jumps, Assertions, `alloca`

- **`setjmp.h`**: `jmp_buf` is `char[8]`: return address (2 bytes), stack pointer inside `setjmp` (2 bytes), the frame pointer `$FD/$FE` (2 bytes), and for code overlays the depth of their return stack and the overlay in the region (1 byte each; `longjmp` out of overlay calls restores both, mapping or copying that overlay back). `int setjmp(jmp_buf)` returns 0, and the value given to `void longjmp(jmp_buf, int val)` when that restores the context (1 if `val` is 0). Assembly (`lib/stdlib_asm/setjmp.s45`).
- **`assert.h`**: `assert(expr)` calls `assert_fail(#expr, __FILE__, __LINE__)`, which prints `Assertion failed: <expr> (file <f>, line <n>)` with `printf` and calls `abort()` (`assert_fail_abort`): exit status `$FFFE`, no `atexit` handlers. `assert_msg(expr, msg)` calls `assert_msg_fail(..., msg)`. With `NDEBUG` both are no-ops.
- **`alloca.h`**: `void *alloca(unsigned)` and `__builtin_alloca`. A constant size is allocated in the frame by the compiler; a variable size uses the runtime routine (`alloca.s45`). A function that calls `alloca` with a variable size uses a stack frame instead of static allocation.

## 11. Timing (`time.h`)

- `clock_t` is `unsigned long`; `time_t` is `signed long`; `struct tm`'s fields are `signed int`.
- **`clock_t clock(void)`**: video frames since power-on. The C65 KERNAL has no C64-style jiffy clock (its `RDTIM`, `$FFDE`, returns the CIA1 time of day in BCD); its raster interrupt decrements a 24-bit counter at `$110C`–`$110E` once per frame from 0 at reset, and `clock` returns 2^24 minus it (read until two reads agree). Wraps after 2^24 frames (about 3.9 days at 50 Hz).
- **`CLOCKS_PER_SEC`**: the frame rate, `(__clocks_per_sec())`: 50 on PAL, 60 on NTSC (`$D06F` bit 7).
- **`time_t time(time_t *tp)`**: the RTC as seconds since 2000-01-01 00:00:00 (`rtc_read` + `mktime`); stored in `*tp` when `tp` is not `NULL`.
- **`signed long difftime(time_t t2, time_t t1)`**: `t2 - t1` (C returns `double`; here seconds as a `long`).
- **`void rtc_read(struct tm *)`**: reads the MEGA65 RTC (BCD registers at `$FFD7110`–`$FFD7116`) into a `struct tm` (copied by DMA: the registers are beyond a 16-bit pointer); years 00–99 map to 2000–2099; only the fields the registers hold: `tm_wday`/`tm_yday` are -1 (call `mktime` afterwards for them: `rtc_read` itself links none of the date arithmetic, about 3.5 KB, #355); `tm_isdst` 0. xemu gives the host's time.
- **`gmtime(timer)`** / **`localtime(timer)`**: `*timer` (seconds since 2000, negative before it: 1931–2068) as a date in a static `struct tm`. The RTC has no time zone, so `localtime` is `gmtime`.
- **`time_t mktime(struct tm *)`**: seconds since 2000-01-01 00:00:00 (not 1970). Fields out of range are carried (`tm_mon` 12 is January of the next year, `tm_mday` 0 the last day of the previous month, negative values count back); `*tm` is rewritten normalized, with `tm_wday` and `tm_yday`.
- **`asctime(tm)`** / **`ctime(timer)`**: `"Wed Jun 30 21:49:08 2026\n"` in a static buffer; `ctime(t)` is `asctime(localtime(t))`.

## 11a. Keyboard and Joysticks (`input.h`)

Where keys arrive on the MEGA65: the keyboard's hardware scanner puts each key press into a queue at `$D610` (ASCII codes; write to `$D610` to take one off). The C65 KERNAL does **not** copy typed keys into its own buffer (`$02B0`, count `$00D0`); its `GETIN` (`getchar`) reads `$D610` when the buffer is empty, and expands programmed function keys into strings. Held keys are read from the matrix (`$D614`/`$D613`, `key_pressed`), the modifiers from `$D611`.

- **`unsigned char input_key(void)`**: the next key, or 0 when none waits (never waits). Letters as cc45 writes them (`'a'`; with SHIFT `'A'`: the queue's ASCII case swapped), RETURN/DEL/HOME/cursor keys as PETSCII (`CH_RETURN`, `CH_DELETE`, `CH_HOME`, `CH_CURSOR_UP`… from `cbm.h`), RUN/STOP `CH_STOP` (3), TAB 9, ESC 27, F1–F14 `CH_F1`… (`$F1`–`$FE`), `£` `$5C`, `↑` `$5E`. Keys a program or BASIC put into the KERNAL's buffer come first (through `GETIN`).
- **`input_wait`** (waits), **`input_waiting`**, **`input_flush`**; **`input_pressed(KEY_x)`** (held now; `<mega65.h>`'s matrix codes, several at once, SHIFT/CTRL/RUN/STOP too); **`input_modifiers`** (`$D611`: bit 0 right SHIFT, 1 left SHIFT, 2 CTRL, 3 MEGA, 4 ALT, 5 NO SCROLL, 6 CAPS LOCK; xemu reports left SHIFT in bit 0).
- **`unsigned char input_joy(port)`** (1 or 2): `JOY_UP`/`DOWN`/`LEFT`/`RIGHT`/`FIRE`, read with no keyboard row selected (`$DC00` = `$FF`, interrupts held off) so held keys don't show. Port 1 is CIA 1 port B (`$DC01`), port 2 port A (`$DC00`). The extra buttons are on the paddle lines (`$D620`/`$D621` port 1, `$D622`/`$D623` port 2) and are read once `input_joy_type(port, type)` says what is connected: `JOYTYPE_1BUTTON` (the default: a mouse or paddles use the same lines), `JOYTYPE_C64GS` (3-button sticks: a button ties its line to +5 V, `JOY_FIRE2` POT X, `JOY_FIRE3` POT Y), `JOYTYPE_SEGA` (Genesis pads: B is `JOY_FIRE`, C `JOY_FIRE2`, pin 9 pulled to ground). A Sega pad's select line is pin 7, +5 V on a Commodore port, so A, START and a 6-button pad's X/Y/Z can't be read, and the pad needs power on pin 5 (an adapter). xemu does not emulate the paddle lines (they read 0); the extra buttons are untested on hardware.
- `conio.h`'s `kbhit`/`getch` use these. Test: `test_input_xemu.c` presses keys through the virtual key registers (`$D615`–`$D617`: a C65 scan code, row × 8 + column, holds the key; `$7F` releases it), the same path as real typing.

## 12. Other Library-Backed Headers

| Header | Contents |
|--------|----------|
| `conio.h` | cc65-style console: colour constants, `clrscr`, `gotoxy`, `wherex`, `wherey`, `textcolor`, `textbackground`, `textattr`, `revers`, `putch`, `cputch`, `cputs`, `cputsxy`, `cprintf`, `cprintfxy`, `getch_xy`, `getattr_xy`, `screenwidth`, `screenheight`, `cursoron`, `cursoroff`, `getch`, `kbhit`. In assembly (`stdlib_asm/conio.s45`) except `cprintf`/`cprintfxy` (`stdlib/conio_printf.c`: `vsprintf` into a 256-byte buffer, then `cputs`). It works on the screen the VIC-IV shows, read on every call (screen RAM `$D060`–`$D063`, row stride `$D058`, columns `$D05E`, rows `$D07B`+1, colour RAM offset `$D064`): 80x25 in MEGA65 mode, also 40 columns, 50 rows or a moved screen, above 64 KB too. A row's screen and colour RAM addresses are 32-bit pointers in `__zp_scratch` (the hardware multiplier gives row × stride), and characters and colours are stored through them with flat addressing (`sta [$08],z`): about 16 times faster than the earlier C version, which did a long multiply and a DMA job per character; clearing and scrolling are DMA fills and copies. Text is PETSCII converted to screen codes; `'\n'` (`$0D`, or `$0A`) starts a new line; output wraps and scrolls. The colour is colour RAM's low nibble (`$FF80000`); text mode has no per-character background, so `textbackground` (and `textattr`'s high nibble) sets the screen's (`$D021`). `revers(1)` sets bit 7 of the screen codes. `getch` waits for a key through `<input.h>` (`input_wait`: PETSCII letters, `CH_STOP`, `CH_F1`…, no echo; with `cursoron` the cursor cell shows reversed while it waits); `kbhit` is `input_waiting`. `getch_xy` returns the screen code, `getattr_xy` the colour RAM byte. Tested by `test_conio_xemu.c` (also a 40-column stride and an 80x50 screen in bank 4); `examples/c/keyboard_viewer` tests the keyboard interactively. With CHR16 (`$D054` bit 0, two bytes a cell: a character number and a colour RAM pair, as the full-colour mode sets up) every function works on the two-byte cells: DMA fills write every other byte (F018B option `$85`, destination step 2); the cursor of `getch` is not shown on a full-colour glyph. **Full-colour mode: a background colour for each character** (`stdlib/conio_fcm.c`, `conio_fcm_printf.c`): the VIC-IV's text mode has one background (`$D021`); its full-colour characters (FCLRHI, `$D054` bit 2: character numbers from 256 are 64 bytes, a byte a pixel, at number × 64; pixel `$00` shows the screen's background, `$FF` the cell's colour from colour RAM, others that palette entry) give each cell its own. `fcm_enable` moves the screen to two bytes a cell in bank 4 or 5 (`far_malloc`, room for 80 columns: 8000 bytes at 80x50) with the cells' colours in colour RAM from `$1000` (above the ROM's screen, which keeps receiving stdio output through the ROM), keeping what is on the screen. A cell on the screen's background shows the font's character (CHARPTR; the VIC-II's `$1000`/`$1800`/`$9000`/`$9800` are the character ROM at `$FF7E000`); one on another background a glyph made from the font's 8 bytes (set pixels `$FF`, clear ones the background; black uses palette entry `$FE`, set to black), kept in a cache after the screen: 256 glyphs (16 KB) by default, `fcm_set_cache_size(n)` before `fcm_enable`. The same character on the same background reuses its glyph; when the cache is full, the glyphs no cell shows are dropped (a scan of the screen), and when all are on the screen a new cell is drawn on the screen's background. `fcm_putch`, `fcm_cputs` (wrapping; `'\n'` starts the next row), `fcm_cprintf`, `fcm_fill_rect`, `fcm_set_line_color` take PETSCII characters and colours 0–15; `getch_xy` gives a glyph cell's character (through `__conio_chtab`), `fcm_getattr_xy` returns `(bg << 4) \| fg`. `fcm_putglyph(x, y, n, fg)` shows a program's own full-colour character `n` (≥ 256, its 64 bytes at `n` × 64; `fcm_disable` leaves a space there; `examples/c/fcm_graphics`). `fcm_disable` puts back the one-byte screen with what is on it (glyph cells as their characters) and frees the memory. About 4.7 KB of code and 1.5 KB of BSS, linked only when used. `conio_set_width(40/80)` (`stdlib/conio_width.c`) writes H640 (`$D031` bit 7), which resets the VIC-IV's layout registers, then puts back the screen's and colour RAM's addresses and sets the characters per row and the stride (twice the width with CHR16) and clears the screen; the ROM's screen editor is not told. `conio_get_width` is `screenwidth`. Test: `test_conio_fcm_xemu.c` (the glyphs' pixels, the cache filling up, the screen kept across enable and disable, both widths). |
| `far_ptr.h` | 24-bit far pointers (`far_ptr_t`, `bank_t`), bank selection (`bank_set`, `bank_get`, `bank_available`, `bank_reset`, `bank_reset_all`) and a far heap (`far_malloc`, `far_calloc`, `far_realloc`, `far_free`, `far_malloc_current`, `far_calloc_current`, `far_size`, `far_alloc_bank`, `far_is_valid`, `far_policy_*`, `far_reserve`; attic RAM: `far_attic_init`, `far_attic_enable`, `far_attic_hook`, `far_attic_available`, `far_attic_malloc`, `far_attic_free`, `far_attic_size`, `far_malloc_long`, `far_size_long`). The chip RAM heap (`far_malloc`, `far_free`, `far_size_long`, `far_reserve`, `bank_reset`, `bank_reset_all`) is assembly, `stdlib_asm/far_heap.s45` (`__fastcall__` in both libraries): a bank's heap ($0100-$FFFF of bank 4 or 5) is up to 32 records in address order (offset, size, a state: allocated, free, kept), arrays of bytes for the two banks reached with X/Y (412 bytes of BSS); first fit, a bigger block split (the rest a free record after it; no record left: refused), `far_free` merging free neighbours; `far_reserve`'s ranges (up to 8) and the code overlays' store carved out at a bank's first use. Attic RAM goes through `far_attic_hook.c`'s routes (linked with `far_attic_hook` / `far_attic_enable`; `far_heap.s45`'s weak ones give nothing), which call `far_attic.c` through the hooks' pointers; the rest of the API is `stdlib/far_malloc.c`. About 1.1 KB of code for `far_malloc` + `far_free` (the C version: 3.9 KB, and 1.9 KB of BSS). Test: `test_far_malloc_xemu.c` (splits, merges, the record limit, `far_reserve`, `bank_reset`, the C part, 1500 random operations against a model of the algorithm). |
| `far_ops.h` | far element access (`far_read_*`/`far_write_*` for 8/16/32-bit, float, double), arrays (`far_array_*`), bulk copies (`far_memcpy*`, `far_memset`, `far_memzero`, `far_strcpy_*`, `far_strlen`, `far_strcmp`) and pointer arithmetic/comparison (`far_add`, `far_sub`, `far_inc`, `far_dec`, `far_diff`, `far_eq`, `far_lt`, `far_gt`, `far_cmp`) |
| `far_string.h` | `memcpy`/`memset`/`strlen`/`strcpy`/`strcmp`/`strcat`/`memcmp` for every mix of local and far pointers (`memcpy__fpvst` …) and typed far array elements; included by `far_ops.h` |
| `dma_controller.h` | F018B DMA request API: `dma_execute`, `dma_transfer_smart`, `dma_fill`, `dma_available`, `dma_is_busy`, `dma_wait`, `dma_enable`, `dma_disable`, `dma_set_interrupt_enable`, `dma_get_status`, `dma_print_status`, `dma_speed_bytes_per_us`. Its `dma_fill` collides with the `dma_fill` macro of `dma.h`. |
| `audio_sid.h` | The four SID chips as 12 voices (voice v: SID v/3, voice v%3): `sid_init`, `sid_note_on`/`sid_note_off`/`sid_play` (MIDI notes), `sid_set_freq` (Hz) / `sid_set_freq_reg` / `sid_set_note`, `sid_set_waveform` (the gate kept apart), `sid_set_pulse`, `sid_set_adsr`, `sid_set_instrument` (`sid_instrument_t`: waveform, AD, SR, pulse width), `sid_gate`, `sid_all_off`, `sid_set_volume`, `sid_set_filter`, `sid_write`/`sid_write_chip`. Every write is kept in a shadow (`sid_get_reg`; the chips are write-only). `sid_set_mirror(1)` repeats voices 0-5 on 6-11 (SIDs 3-4, the right side). Frequencies use the PAL or NTSC SID clock (`$D06F`). The register write itself (the shadow, the mirror) is `stdlib_asm/sid_core.s45`'s `___sid_put`, shared with the tune player; the registers are at `___sid_regs` (`sid_regs.s45`, weak: a test points it at RAM). Used by the `invaders` and `musicgen` examples. |
| `audio_tune.h` | A tune (song) player (`stdlib_asm/tune.s45`, #314): pre-composed music as data (`tune45 convert` makes it), played once a frame by `tune_update` on a range of SID voices (`tune_voices(first)`, up to 12). `tune_play(tune)` / `tune_play_far(addr)` (a 28-bit address: tunes in banks 4-5 are read where they are, flat addressing), `tune_stop`, `tune_playing`, `tune_ticks`, `tune_note(voice)` (the note it sounds, `$FF` silent), `tune_seek(ticks)` (on that many ticks at once and silently, after `tune_play`: back where a tune was; tune45's `TUNE_<NAME>_TICKS` is a time through). A tune: a header (loops, voices, frames per tick as 8.8, the instrument table), 10-byte instruments (waveform, AD, SR, pulse width, vibrato depth, staccato gap, pulse-width sweep bouncing between $200 and $E00, vibrato delay, pitch slide) and a byte stream a voice (notes for d ticks, rests, instrument changes, ties, arpeggios, glides, loop, end). A glide starts the next note s semitones off its pitch and from its second frame moves (distance + 2^k - 1) >> k a frame towards it. A note restarts the envelope; the gap releases it that many ticks before the next event; vibrato is a triangle over 8 frames, `(f >> 8) * depth * tri / 2`. Run-time controls: `tune_tempo` (8.8 scale), `tune_vibrato` (depth added), `tune_transpose` / `tune_detune` (per voice; detune adds amount/64 of the frequency). NTSC frames count 213/256 of a PAL one and the notes use the NTSC clock, so a tune sounds the same. Assembly with no static allocation and no hardware multiplier: callable from an `__interrupt` handler (state changes hold interrupts off). Tests: `test_tune_xemu.c` (its SID writes in a RAM buffer); `test_xemu.sh` compares tune45's model with the player frame by frame (every effect; glides after a `tune_seek`). |
| `audio_digi.h` | Sample playback on the 4 audio DMA channels (`$D720`+; 0-1 left, 2-3 right): `digi_init` (`$D711` = `$80`), `digi_play` (bank 0) / `digi_play_far` (24-bit address): length, rate in Hz (`digi_rate_to_reg`: rate × 2²⁴ / 40.5 MHz), volume, flags `DIGI_LOOP`/`DIGI_SIGNED`/`DIGI_16BIT`/`DIGI_NYBBLE_*`; `digi_stop`, `digi_playing`, `digi_set_volume` (with the other side's), `digi_set_rate`, `digi_position`, `digi_done`. Needs the CPU at 40 MHz. |
| `audio_midi.h` | `midi_parse` turns a MIDI byte stream into `midi_msg_t` (running status; real-time and SysEx skipped); a `midi_synth_t` plays them on a range of SID voices: free voice or the oldest, velocity 0 = note off, sustain pedal (CC 64), pitch bend (±2 semitones), program change (`midi_synth_set_programs`), channel volume (CC 7 → SID volume), all notes off (CC 120/123); `midi_note_to_hz`. |
| `intwide.h` | `struct __int64`, `struct __int128` with operators, and the width-parameterized runtime `__intN_add`, `__intN_sub`, `__intN_mul`, `__intN_cmp_u`, `__intN_cmp_s`, `__intN_neg`, `__intN_not`, `__intN_and`, `__intN_or`, `__intN_xor`, `__intN_shl`, `__intN_shr_u`, `__intN_shr_s` (arithmetic), `__intN_divmod_u` (shift and subtract), `__intN_divmod_s` (truncating) (used by `__int(N)`/`__uint(N)`) |
| `complex.h` | marker only: the `_Complex` types and operators are built into cc45 |

## 12a. Graphics (`tgi.h`)

cc65's Tiny Graphics Interface ([cc65's TGI page](https://cc65.github.io/doc/tgi.html)), so a cc65 TGI program ports with its calls unchanged: every function of the page and cc65's `tgi.h` (`tgi_load_driver`, `tgi_install`, `tgi_init`, `tgi_done`, `tgi_uninstall`, `tgi_unload`, `tgi_geterror`, `tgi_geterrormsg`, `tgi_clear`, the page, colour, palette, resolution and aspect-ratio functions, `tgi_setpixel`, `tgi_getpixel`, `tgi_gotoxy`, `tgi_line`, `tgi_lineto`, `tgi_circle`, `tgi_ellipse`, `tgi_arc`, `tgi_pieslice`, `tgi_bar`, `tgi_settextdir`, `tgi_settextscale`, `tgi_settextstyle`, `tgi_gettextwidth`, `tgi_gettextheight`, `tgi_outtext`, `tgi_outtextxy`, the vector font functions, `tgi_ioctl`, `tgi_imulround`), the `TGI_ERR_*`, `TGI_FONT_*`, `TGI_TEXT_*` and `TGI_COLOR_*` constants (the C64's colour numbers: the default palette's entries 0-15 are the C64's colours) and `<tgi/tgi-kernel.h>`'s variables (`tgi_curx`, `tgi_cury`, `tgi_color`, …; the installed vector font is `__tgi_vfont`: in C its cc65 name would be the type's). `tgi_install` returns the error code (cc65's header: `void`; its page: `unsigned char`).

**Drivers** are linked in, not loaded: `tgi_load_driver` finds one by name (any case), `tgi_install` takes one's address.

| Driver | Name | Pages | Memory |
|--------|------|-------|--------|
| `tgi_static_stddrv` | `tgi_stddrv`, `"mega65-fcm.tgi"` | 1 | 64 KB + 2.7 KB in banks 4/5 |
| `tgi_static_fcm2` | `"mega65-fcm2.tgi"` | 2 | most of banks 4 and 5, 4000 bytes of heap |
| `tgi_static_ncm640` | `"mega65-ncm640.tgi"` | 1 | 64 KB + 2.7 KB in banks 4/5 |

The first two are 320x200 with 256 colours (`tgi_getcolorcount` returns 0, cc65's 256), an 8x8 font, the aspect ratio `$00D4` (cc65's c64-hi: a 4:3 display). **The screen** is the VIC-IV's full-colour characters (`stdlib/tgi.c`; the driver's set-up, `tgi_install`, `tgi_uninstall`, `tgi_init`, `tgi_done`, `tgi_setviewpage` and `tgi_setdrawpage` with the memory they take, is assembly: `stdlib_asm/tgi_setup.s45`): 40x25 cells (CHR16, FCLRHI, H640 and V400 off), each 64 bytes, a byte a pixel; a page's 1000 characters are a column of 25 after another, so pixel (x, y) is at `page + (x >> 3) * 1600 + y * 8 + (x & 7)` (`tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR / _VIEWPAGE_ADDR, &addr)` gives a page's address). Pixel `$00` shows `$D021` and `$FF` the colour RAM's colour; with `$D021` 0, multicolour mode on (8-bit colour RAM colours) and the colour RAM (2000 bytes at offset `$3000`) `$FF`, a pixel's byte is its palette entry. `tgi_install` takes the memory from `far_malloc` (a page 64000 bytes, 64-byte aligned and inside one bank; the palette as it was, 768 bytes; a page's screen of character numbers, 2000 bytes), so `far_malloc`, code overlays and conio's full-colour mode keep the rest; the two-page driver's screens, which do not fit, come from `malloc` (`tgi_fcm2.c`'s heap functions; the one-page driver uses banks 4 and 5 only and links no `malloc`: `tgi_setup.s45`'s weak defaults give nothing), else `TGI_ERR_NO_RES`. The kernel variables are a block in a fixed order (`stdlib_asm/tgi_vars.s45`): `tgi_install` copies a driver's numbers in one loop and clears a new driver's state in another; `tgi_load_driver` is a member of its own (`tgi_load.c`), so a program installing `tgi_static_stddrv` links only that driver. A plot-only program is about 4.8 KB. A new page is cleared; `tgi_init` does not clear (as cc65's). `tgi_init` saves the VIC registers it changes (`$D011`, `$D016`, `$D020`, `$D021`, `$D031`, `$D04C`, `$D04E`, `$D054`, `$D058`/`$D059`, `$D05E`, `$D060`-`$D065`) and `tgi_done` puts them back, `$D031` first (writing it sets the VIC-IV's layout for the mode); `tgi_uninstall` frees the memory.

**The 640x200 driver** (`stdlib_asm/tgi_ncm.s45`, linked only with it) is 16 colours, an 8x8 font, the aspect ratio `$006A` (200 / 640 · 4 / 3): the VIC-IV's nibble-colour characters (NCM: colour RAM byte 0 bit 3) at H640, each 16x8 pixels of 4 bits, so the page is laid out as the 320x200 driver's (40x25 characters of 64 bytes, a column of 25 after another, 64000 bytes): pixel (x, y) is nibble `x & 1` (the left one the low nibble) of `page + (x >> 4) * 1600 + y * 8 + ((x >> 1) & 7)`. Nibble 0 shows `$D021` (0), nibble 15 the colour RAM's byte 1 (`$0F`), the others entry `(byte 1 >> 4) * 16 + nibble`: colour c is palette entry c (`tgi_setpalette` reads 16 entries). The drivers' drawing is in the drivers' members: `tgi_pixel.s45`'s and `tgi.c`'s routines send the line on the screen, the ellipse's 4 point addresses (set, stepped in x, plotted), the 1x glyph and `tgi_bar`'s fill to the 320x200 drivers' (`stdlib_asm/tgi_fcm.s45`, `___tgi_f_*`) or, by the driver's mode byte (`___tgi_mode`, `$80`), the 640x200 one's (`___tgi_n_*`; also `tgi_setpixel`, `tgi_getpixel` and the clipped points). Each set replaces weak defaults that do nothing and is linked with its drivers only (a driver's description refers to its member: `tgi_driver_t`'s `draw`), so a program links one driver's drawing, and of it the routines it calls (a 320x200 program: a few bytes of tests; a 640x200 one with lines, ellipses and text: about 1.4 KB less than with both); lines, ellipses, arcs and text are the 320x200 driver's (the same pixels, compared on random scenes). Addresses are stepped as there, a nibble at a time (a byte every second pixel, +1593 across a character column). A line along x writes a byte's two pixels on one row at once, stored without reading it; a pixel alone in its byte is read, changed and written. The 1x glyph takes a row's bits two at a time, a byte's (after a 0 at an odd x): 4 or 5 bytes, each stored, changed or skipped. `tgi_bar` fills whole character columns (16 pixels) and byte columns (2) by DMA. Speed against the 320x200 driver (`bench_tgi.sh`, frames): pixels 67 / 53, long lines 37 / 30, short ones 33 / 29, circles 15 / 11, text 21 / 11 at 1x and 54 / 79 at 2x, bars 72 / 67.

**Drawing**: coordinates are signed and everything is clipped (`tgi_line`: Cohen-Sutherland with 32-bit intersections in C, then Bresenham in assembly; `tgi_setpixel` outside does nothing, `tgi_getpixel` outside returns 0). `tgi_setpixel`, `tgi_getpixel`, `tgi_line`, `tgi_lineto`, the line on the screen, the ellipse and the 1x glyph are `stdlib_asm/tgi_pixel.s45` (a line wholly on the screen never reaches C: cc45's parameter thunk, `__zp_args`, costs more than a short line); only a line with an end off the screen goes to `tgi.c`'s clipping: a page never crosses a bank, so a pixel's address is the page's bank bytes and a 16-bit sum (column tables for `(x >> 3) * 1600`), stored with `sta [ptr],z`; the line keeps the colour in Y and `x & 7` in X and steps the offset (x ±1 in a character column, ±1593 across one, y ±8), a loop for each major axis and way. `tgi_bar` is DMA fills: a whole character column's rows are one run, other columns a fill every 8th byte (`dma_fill_far_step`). `tgi_clear` is one DMA fill (colour 0). Ellipses are Kennedy's midpoint ellipse (`tgi_pixel.s45`'s `__tgi_ellipse` works out its starting numbers with the hardware multiplier, walks a quarter in two passes with 32-bit Q-register adds and plots the point in the four quarters; `tgi_circle` and `tgi_ellipse` are assembly too: about 1.7 KB in all): the curve's own pixels, not cc65's polygon of 12-degree chords. When the ellipse's box is on the screen the four points are addresses stepped as the walk moves (as the line's); otherwise each point is clipped. An arc is the ellipse's points between its angles (parametric: the point (rx cos t, ry sin t)): for the start and end angles, `c = y·rx·cos b − x·ry·sin b` (≥ 0 when the point is past b) changes by one of two constants per step, so the test is adds too; each quarter is drawn whole, not at all, or where its test(s) hold. `tgi_arc`, `tgi_pieslice` and `tgi_imulround` are `stdlib_asm/tgi_arc.s45` (about 1.1 KB; a program using none of them links none of it, one using only `tgi_imulround`, the vector fonts, only that): an angle's test is four products of the hardware multiplier (its sine and cosine for the angle in the first quarter, 0..90, are >= 0: only the results take a sign); sines are an 8.8 table, needed only for the two angles; `tgi_pieslice` adds cc65's two radii. `tgi_circle` is an ellipse; an ellipse with a 0 radius is the line (x − rx, y − ry) to (x + rx, y + ry), an arc of one cc65's polygon. Y grows downwards, so angles go anticlockwise. The aspect ratio is not applied (as cc65's).

**The palette**: `tgi_getdefpalette` is the identity (256 entries, made when it is called: no table in the program); `tgi_setpalette(p)` shows at entry i the colour the palette had at `tgi_install` at entry `p[i]`; `tgi_getpalette` is that `p`, or the identity while the palette is the default; `tgi_init` and `tgi_done` put the saved palette back when `tgi_setpalette` changed it (so a program that never sets one links neither function nor table).

**Text** (`stdlib/tgi_text.c`): the bitmap font is the lower/upper case half of the VIC's current font (CHARPTR; the character ROM for the VIC-II's `$1000`/`$1800`/`$9000`/`$9800`), so a PETSCII string shows in both cases as written; control characters take their place but draw nothing. It is magnified by the 8.8 scales rounded to whole numbers (driver flag bit 7 clear). At 1x, horizontal and wholly on the screen, a glyph is stored directly (`__tgi_glyph`: a row's 8 pixels in one or two character columns); otherwise each row's runs of set pixels are one bar. The cursor is the text's top left; `TGI_TEXT_VERTICAL` turns it anticlockwise (the cursor at its bottom left, the text going up). `tgi_outtext` moves the cursor by `tgi_gettextwidth`. Vector fonts are cc65's `.vfn` files (`<tgi/tgi-vectorfont.h>`: "TCH", version 0, the data's size, then `struct tgi_vectorfont`, whose character pointers are offsets from `vec_ops`), loaded with `fopen` into `malloc`'d memory (`stdlib/tgi_vfload.c`; `TGI_ERR_CANNOT_LOAD`, `TGI_ERR_INV_FONT`, `TGI_ERR_NO_RES`); `tgi_vectorchar` is cc65's: two bytes an operation, 7-bit signed X and Y (Y up) times the 8.8 scales, bit 7 of Y draws, bit 7 of X ends the character; the string is converted from PETSCII to ASCII. `tgi_gettextheight` is the font's `top` scaled.

Not as cc65: no loadable driver files; `tgi_geterrormsg(TGI_ERR_INSTALLED)` is "A driver is already installed" (cc65's table gives "Unknown error"); `tgi_gettextheight` of the bitmap font is the scaled height's whole pixels (cc65's returns the 8.8 value's low byte); `tgi_install` while a driver is installed returns `TGI_ERR_INSTALLED`; circles, ellipses and arcs are the curve's pixels (midpoint), not cc65's polygons, so they differ from cc65's by a pixel here and there. Tests: `test_tgi_xemu.c` (the driver's numbers, the page layout, clipping, lines, bars, arcs, bitmap and vector text, errors, the palette, text mode back, the memory freed, two pages) and `test_tgi_vfn_xemu.c` (`.vfn` files on a D81), both conventions; `examples/c/tgi_demo`. Speed: `src/test/bench_tgi.sh` (not in `make test`) runs `benchmark_tgi.c` in xemu and prints the frames each workload takes (pixels, long and short lines, circles, text at 1x and 2x, bars) with a checksum of the page it drew, so a faster version must give the same checksums.

## 13. MEGA65 Hardware (`mega65.h`) and DMA (`dma.h`)

`dma.h`: `dma_copy(dst, src, len)` / `dma_fill(dst, len, val)` are compiler builtins with 16-bit bank-0 addresses. `dma_copy_far(dst, src, len)` / `dma_fill_far(dst, len, val)` (`__dma_copy_far`/`__dma_fill_far` in `lib/stdlib_asm/dma_far.s45`, 121 bytes and the shared job: the C was 643, #319; `__fastcall__`, so stack-convention callers need no thunk) take 28-bit linear addresses (`unsigned long`): chip RAM ($0000000–$005FFFF; $0020000–$003FFFF holds the ROM), colour RAM at $FF80000 (all of it, beyond the $D800–$DBFF window), …. They run an enhanced F018B job (megabyte options $80/$81) while the CPU waits; `len` 0 does nothing; the copy runs forwards. Test: `test_dma_far_xemu.c`. `dma_copy_far_step(dst, dst_step, src, src_step, len)` / `dma_fill_far_step(dst, dst_step, len, val)` (`lib/stdlib_asm/dma_step.s45`, its own job with the F018B skip-rate options `$83`/`$85` in front) take a step for each side in whole bytes: 2 writes every other byte (a two-byte cell screen's characters or colours), 1 is the plain copy, a source step of 0 repeats one byte; `len` counts the bytes written. Test: `test_seam_xemu.c`.

`palette.h` (`lib/stdlib/palette.c`): the 256 palette entries ($D100-$D3FF, each component's nibbles swapped in the register) as plain 0-255 values: `palette_set_color(index, r, g, b)`, `palette_set_color_rgb(index, rgb_color_t)`, `palette_get_color(index, &out)` (-1 for an index outside 0-255), `palette_set_range(first, count, rgb)` (r, g, b triples), `palette_save(first, count, buf)` / `palette_restore(first, count, buf)` (3 × count bytes, the registers as they are: the reds, the greens, the blues, so data prepared that way can be restored straight in), `palette_rotate(start, end, direction)` (colour cycling). They work on the palette bank mapped at $D100. The earlier `palette.h` declared fades, interpolation, banks and names that no library implemented (its `lib/src/palette.c` wrote the wrong registers); those are gone. Test: `test_seam_xemu.c`.

`seam.h` (`lib/stdlib/seam.c`): a screen in Super-Extended Attribute Mode (two bytes of screen RAM a cell, a 13-bit character number; two of colour RAM, byte 1 the colour) with FCLRHI, so characters above $FF are full-colour (64 bytes at number × 64, a byte a pixel from the 256-colour palette; pixel $00 shows $D021, $FF the cell's colour) and those up to $FF the character set's text, on one screen. A `seam_screen_t` describes one anywhere in chip RAM with its colour RAM at an offset from $FF80000: `seam_init(&s, screen, colour, cols, rows)` (clears it), `seam_clear`, `seam_show` (saves and sets $D054 bits 0 and 2, $D058/$D059 the stride, $D05E, $D060-$D065), `seam_hide` (puts them back, $D054 first), `seam_put(&s, x, y, ch, colour)`, `seam_text(&s, x, y, codes, n, colour)` (screen codes), `seam_row(&s, x, y, n, codes, colours)` (a row of a one-byte screen's buffers), `seam_cells(&s, x, y, numbers, n)` (character numbers from a linear address: a picture's cells), `seam_cell`/`seam_colour` (the addresses). The writes are stepped DMA jobs. `examples/c/glamlambs` shows its salon this way. Test: `test_seam_xemu.c`. (conio's full-colour mode, §12, keeps its own screen of this kind for text with backgrounds.)

The bank-based APIs are built on them: a bank and a 16-bit address (or a `far_ptr_t`) are the linear address `(bank << 16) | addr`. `dma_controller.h`: `dma_execute`, `dma_transfer_smart`, `dma_fill` copy/fill through DMA; the CPU waits for each job, so `dma_is_busy` is 0 and `dma_wait` returns at once. `far_ops.h`: `far_read_*`/`far_write_*` (float/double are the 5-byte CBM format), `far_memcpy*`, `far_memset`, the string functions and `far_array_*` are DMA jobs; `far_add`/`far_sub`/`far_diff`/`far_cmp` work on the linear address (an offset carries into the next bank). `far_ptr.h`'s allocator (`far_malloc`, `far_calloc`, `far_realloc`, `far_free`) hands out banks 4 and 5 only ($40000–$5FFFF): bank 0 is the program, bank 1 BASIC 65's variables, banks 2–3 the ROM, and 6–7 are not chip RAM; other banks give the null far pointer. **Attic RAM** (Issues #305, #306) is off by default: `far_attic_enable(first, last)` (28-bit addresses in `$8000000`–`$87FFFFF`, clipped to it) writes and reads back both ends and returns the bytes it adds to the far heap (0 when the memory is not there); then `far_malloc(n, BANK_ATTIC)` and `far_malloc_long(n, BANK_ATTIC)` (over 64 KB, up to the whole range; `far_size_long` gives its size, `far_size` at most 65535) allocate from it, and `far_malloc_current`/`far_calloc_current` use it when banks 4 and 5 are full; `far_attic_available()` gives the free bytes, `far_free`/`far_size`/`far_realloc` take attic pointers. It is a buddy allocator: a block is a power of two, at least a granule, aligned to its size (one of 64 KB or less never crosses an attic bank, so a program can address inside it as base + offset), and a freed block merges with its buddy (two free neighbours that are not buddies stay apart). There is no limit on the number of blocks: the table is two bitmaps over the 8 MB, a bit per block of each size. `far_attic_enable` uses 4 KB granules and a 1 KB table of its own and connects attic RAM to `far_malloc`/`far_free` (`far_attic_hook()`); `far_attic_init(first, last, order, meta, size)` sets up only the allocator (`far_attic_malloc` & co.; call `far_attic_hook()` for `far_malloc`) and takes the granule's order (9: 512 bytes, to 20: 1 MB) and the program's memory for the table, `FAR_ATTIC_META_SIZE(order)` bytes (128 for 32 KB granules). A request is rounded up to a power of two (33 KB takes 64 KB): attic RAM suits large blocks a program divides itself, such as an editor's text in 32 KB slabs. The attic code is reached through hooks `far_attic_init` sets, so a program that never enables attic RAM does not link it; a program that uses only attic RAM calls `far_attic_malloc(size)`, `far_attic_free(p)` and `far_attic_size(p)` and does not link the chip RAM heap either (3.2 KB for the allocator, 1.3 KB for `far_attic_init`, separate library members: a program short of bank 0 can compile `lib/stdlib/far_attic.c` and `far_attic_init.c` into overlays of its own with `-foverlay=name`). A `far_ptr_t` bank `$80`–`$FF` is attic RAM, `$8000000 + (bank − $80) × 64 KB + offset`, so `far_add` (a signed 16-bit offset) and the `far_ops.h` DMA functions work across its banks; attic RAM is reachable only by DMA and is slower than chip RAM. Code overlays can be kept there too (ln45 `--overlay-store`, [Overlays in attic RAM](../bin/ln45.md#overlays-in-attic-ram)): `far_attic_init`/`far_attic_enable` leave their span out (`__ovl_attic_bank`/`_first`/`_last_bank`/`_last`), cutting the range given to its larger side of it. In a program with code overlays (or `__far` functions) the part of their bank the overlay file takes (bank 5 from `$50000` by default; ln45 `__ovl_store_bank`/`_first`/`_last`, and `__ovl_store2_*`, for the files in banks 4 and 5) is never handed out, nor the ranges a program keeps with `far_reserve(first, last)` (linear addresses in banks 4–5, up to 8 ranges, called before the first allocation there; `bank_reset` keeps them), such as its own buffers at fixed addresses. `far_malloc_current`/`far_calloc_current` choose their bank by a policy: the bank `bank_set` (or `far_policy_fixed_bank`) selected, bank 4 at first; `far_policy_first_fit()` (bank 4, then 5 when it is full) or `far_policy_round_robin()` (alternating, the other bank when one is full). `dma_disable` makes `dma_execute`/`dma_transfer_smart`/`dma_fill` do nothing until `dma_enable`. `far_string.h` (included by `far_ops.h`, `stdlib_overloads.c`): `memcpy`, `memset`, `strlen`, `strcpy`, `strcmp`, `strcat` and `memcmp` for every mix of local and far pointers, one function per combination since cc45 has no overloading by type — the suffix names the parameters (`pv` `void *`, `pc` `char *`, `fp` `far_ptr_t`, `i` `int`, `st`/`z` `size_t`: `memcpy__fpvst(buf, far, n)` copies far to local, `strcpy__fppc(far, "text")` local to far); far-destination ones return the far pointer; strings have no length limit; plus `far_array_read/write_int16/uint8/uint32(base, index)`. Tests: `test_far_ops_xemu.c`, `test_far2_xemu.c`, `test_far_string_xemu.c`, `test_far_attic_xemu.c`.

`mega65.h` provides `volatile` struct overlays and register macros. All helpers except `key_pressed` are `static` functions in the header.

- **VIC-IV** (`vic4` at `$D000`, `struct vic4_regs` covering `$D000`–`$D07F`): `vic4->border`, `vic4->bg0`, `vic4->sprite_enable`, `vic4->sprite[n].x/.y`, `ctrl1`, `ctrl2`, `ctrl_a`, `ctrl_b`, `ctrl_c`, `key`, … `VIC4_SPRITE(n)` gives a sprite position pointer. Direct registers: `VREG_BORDER`, `VREG_BG0`–`VREG_BG3`, `VREG_CTRL1`, `VREG_CTRL2`, `VREG_RASTER`, `VREG_SPR_ENABLE`, `VREG_SPR_XMSB`, `VREG_MEMPTR`, `VREG_IRQ_STATUS`, `VREG_IRQ_ENABLE`, `VREG_KEY`, `VREG_CTRL_A`, `VREG_CTRL_B`, `VREG_CTRL_C`. Helpers: `vic4_unlock()` (writes `$A5`,`$96` to `$D02F`), `vic4_fast()` (unlock + `VIC4_FAST` in `$D031`), `vic4_sprite_pos(n, x, y)` (handles X > 255). Constants: `COLOR_BLACK` (0) … `COLOR_LGREY` (15); bits `VIC4_RST8`, `VIC4_ECM`, `VIC4_BMM`, `VIC4_DEN`, `VIC4_RSEL`, `VIC4_MCM`, `VIC4_CSEL`, `VIC4_IRQ_*`, `VIC3_*` (`$D030`), `VIC4_H640`, `VIC4_FAST`, `VIC4_FCM`, … (`$D031`), `VIC4_CHR16`, … (`$D054`); `VIC3_KEY1`/`VIC3_KEY2`.
- **SID** (`sid1`–`sid4` at `$D400`/`$D420`/`$D440`/`$D460`; `SID1_VOICE(n)`–`SID4_VOICE(n)` for voice registers `freq_lo`, `freq_hi`, `pw_lo`, `pw_hi`, `ctrl`, `attack_decay`, `sustain_release`; chip registers `filter_lo`, `filter_hi`, `filter_ctrl`, `volume`, `pot_x`, `pot_y`, `osc3`, `env3`). Bits `SID_GATE`, `SID_SYNC`, `SID_RINGMOD`, `SID_TEST`, `SID_TRIANGLE`, `SID_SAWTOOTH`, `SID_PULSE`, `SID_NOISE`, `SID_FILT1`–`SID_FILT3`, `SID_FILT_EXT`, `SID_LP`, `SID_BP`, `SID_HP`, `SID_MUTE3`.
- **CIA** (`cia1` `$DC00`, `cia2` `$DD00`: `pra`, `prb`, `ddra`, `ddrb`, `timer_a_lo/hi`, `timer_b_lo/hi`, `tod_*`, `sdr`, `icr`, `cra`, `crb`). `joy1_read()`/`joy2_read()` return active-high `JOY_UP/DOWN/LEFT/RIGHT/FIRE` bits. `CIA_ICR_*`, `CIA_CRA_*`, `CIA_CRB_*`.
- **Keyboard**: `__regparm unsigned char key_pressed(unsigned char keycode)` (library, assembly) reads the key's row of the MEGA65 keyboard matrix (`$D614` select, `$D613` read; independent of the joysticks and CIA 1) and returns 1/0, allowing several keys at once; 64 `KEY_*` matrix codes. Hardware typing queue: `ASCII_KEY_QUEUE` (`$D610`), `PETSCII_KEY_QUEUE` (`$D619`), `KEY_MODIFIERS_REG` (`$D611`) with `KEY_MOD_*` bits; `getkey_ascii()` (0 if empty) and `getkey_petscii()` (`$FF` if empty) read and advance it.
- **DMA registers** (`dma` at `$D700`: `addr_lsb_trig` — writing it starts the job —, `addr_msb`, `addr_bank`, `mode`, `addr_mb`, `etrig`); `DMA_ADDR_LSB_NOTRIG` (`$D70E`); `DMA_CMD_COPY` 0, `DMA_CMD_FILL` 3, `DMA_CMD_SWAP`/`DMA_CMD_MIX` 4. Write the bank and MSB first and the LSB last.
- **Math accelerator** (`math` at `$D768`: `frac[4]`, `quotient[4]`, `arg1[4]` `$D770`, `arg2[4]` `$D774`, `product[8]` `$D778`); `MATH_BUSY` (`$D70F`, `MATH_BUSY_DIV`/`MATH_BUSY_MUL`); `HW_RANDOM` (`$D7EF`), `HW_RNG_READY` (`$D7FE`). There is no remainder register.
- **Audio mixer / digital audio**: `AUDIO_MIX_SEL` (`$D6F4`), `AUDIO_MIX_DATA` (`$D6F5`), `AUDIO_DIGI_LEFT`/`AUDIO_DIGI_RIGHT` (`$D6F8`/`$D6FA`), `AUDIO_READBACK` (`$D6FC`); `set_mixer_coefficient(index, volume)` writes one of the 128 16-bit coefficients.
- **Floppy controller** (`fdc` at `$D080`), commands `FDC_CMD_READ/WRITE/STEP_IN/STEP_OUT/RESTORE`, status `FDC_BUSY/DRQ/EQ/RNF/WRPROT/TK0`.
- **SD card** (`sd` at `$D680`: `status`, `command`, `addr0`–`addr3`, `buf_addr_lo/hi`, `rdata`), `SD_CMD_RESET/READ/WRITE/FLUSH`, `SD_BUSY/RESET/ERROR/SDHC`.
- **Ethernet** (`eth` at `$D6E0`: `ctrl`, `irq_ctrl`, `txsz_lo/hi`, `cmd`, `mode`, `miim_*`, `mac[6]`), `ETH_RST`, `ETH_TXRST`, `ETH_TXIDLE`, `ETH_CMD_TX`. Packet data is in the `$FFDE800` window.
- **Hypervisor** (`hyper` at `$D640`, 64 trap registers), `HTRAP_SETNAME`, `HTRAP_LOADFILE`.
- **Memory**: `SCREEN_RAM` (`$0800`), `COLOUR_RAM` (`$D800`).

```c
#include <mega65.h>
vic4->border = COLOR_BLACK;
VREG_BG0 = COLOR_BLUE;
SID1_VOICE(0)->ctrl = SID_TRIANGLE | SID_GATE;
if (key_pressed(KEY_SPACE)) { /* ... */ }
```

**`dma.h`**: `dma_copy(dst, src, len)` and `dma_fill(dst, len, val)` are macros for the compiler builtins `__dma_copy`/`__dma_fill`, expanded inline (no library code): the 12-byte F018B job is built on the CPU stack and triggered by writing `$D702`, `$D701`, then `$D700`; registers are preserved. Addresses are 16-bit (bank 0), `len` is 16-bit, `val` 8-bit; not for use inside interrupt handlers.

```c
#include <dma.h>
dma_fill((void *)0x0800, 1000, 0x20);
dma_copy((void *)0x0800, (void *)src_data, 1000);
```

### Packed data (`unpack.h`)

`unsigned long unpack(unsigned long src)` (#345, `lib/stdlib/unpack.c`, `__fastcall__`: the ZP convention in either library; a macro for the library's `__unpack`, #346, so a program's own function named `unpack`, written without `<unpack.h>`, is a separate function and the overlay loader keeps calling the library's) unpacks the stream at the 28-bit address `src` to the address in its header (chip or attic RAM) and returns the number of bytes written; 0 for a stream of another format. The streams are made by [pack45](../bin/pack45.md) (format 1: literals, fills and repeats, each one DMA job) and by `ln45 --pack-overlays`. A stream can be loaded inside the area it unpacks into, as far in as pack45 reports, and unpacked there. Test: `src/test-resources/test_unpack_xemu.c`.

### rom_ram.h

Unused parts of the ROM area ($20000-$3FFFF of chip RAM) as RAM (#339). In C65 mode the C64 BASIC and KERNAL images there are never used: 16 KB for characters, samples, DMA buffers. The hypervisor write-protects the area (CPU and DMA writes are ignored): its memory trap, `$D641`, lifts the protection (`A = $02`) or sets it again (`A = $00`); `$D67D` bit 2 shows it.

| Name | Description |
|------|-------------|
| `ROMRAM_C64` | The C64 region: `$2A000`-`$2BFFF` (C64 BASIC) and `$2E000`-`$2FFFF` (the end of C64 BASIC and the C64 KERNAL). Not `$2C000`-`$2DFFF` between them: parts of the C65 DOS and KERNAL and a character set C65 mode uses. Overwriting it breaks only `GO64`, until the ROM is loaded again (a reset) |
| `unsigned char romram_enable(unsigned char regions)` | *asm* (`stdlib_asm/rom_ram.s45`). Writes on (the trap), then a byte of each range of the regions is changed, read back and restored: 1 when every one took, 0 when not (or a region bit is unknown, or none is given) |
| `void romram_protect(void)` | Writes off again; the contents stay |
| `const unsigned long *romram_ranges(unsigned char region)` | The region's ranges: their count, then first / last pairs (28-bit addresses; reach them with `dma_copy_far` / `dma_fill_far` or far pointers); `NULL` for an unknown region |

A program that leaves the area writable has it protected again when it ends: crt0's `__exit` calls `__romram_exit` (weak `rts` unless `rom_ram.s45` is linked). The allocator of banks 4-5 (`far_malloc`) does not hand these ranges out: a program uses them as it likes. Later regions (the monitor, C65 BASIC with its float routines, the DOS) would say what stops working once they are overwritten; one that `READY.` needs would need the program to reset the machine at its end. Test `test_romram_xemu.c`.

## 14. Headers Without a Library Implementation

These headers declare functions whose sources (`lib/src/*.c`, `lib/stdlib/uart.s`) are not built into any archive, so calls to them fail to link: `uart.h`, `audio.h`, `cache_opt.h`, `data_structures.h`, `event_system.h`, `graphics.h`, `graphics_hal.h`, `graphics_particles.h`, `graphics_rrb.h`, `graphics_sprites.h`, `graphics_sprites_advanced.h`, `graphics_text80x50.h`, `hw_audio_mixer.h`, `hw_io.h`, `hw_peripherals.h`, `hw_raster.h`, `hw_scroll.h`, `joystick.h`, `lightpen.h`, `mega65_version.h`, `mem_pool.h`, `paddle_mouse.h`, `persistence.h`, `procedural_content.h`, `resource_manager.h`, `scripting.h`, `simd.h`, `sprites.h`, `sprite_*.h`, `test_harness.h`, `ui.h`, `world.h`. `sincos.h` and `sincos16.h` contain only `static` code and tables and work without the library.

## 15. Runtime Startup (`crt0_zp.s45` / `crt0.s45`)

The CRT module from the library provides `__init`, which the linker uses as the entry point:

1. Save the caller's stack pointer and flags.
2. Enable the MEGA65 I/O registers (`$47`, `$53` to `$D02F`).
3. Set the exit status at ROM page-0 `$FD/$FE` to `$FFFF`.
4. Relocate the base page (`TAB`) to `__zp_page`, a private page at the end of BSS.
5. Move the CPU stack to the 16-bit stack topped at `__stack_top` (`$BFFF` by default; `-fstack-top=`/`#pragma cc45 stack_top(...)`), unless `-fno-stack16`/`#pragma cc45 no_stack16` keeps the ROM's page-1 stack; then fill BSS with `__bss_fill` (DMA).
6. `_init_features` (`crt0_mega65*.s45`): write `$40` to `$D054`.
7. Call `__mainargs` and pass the `argc`/`argv` it returns to `main` (in the ZP parameter block and on the stack), then fall into `__exit` (see `exit` in [§4](#4-general-utilities-stdlibh)). crt0's weak `__mainargs` returns `argc = 0`, `argv = NULL`; `stdlib_asm/mainargs.s45` replaces it when `main` is defined with parameters (cc45 then references `__mainargs_link`) and parses the BASIC line at `$0200` (`RUN ["NAME"] : [REM] a, b, ...`; see *Startup* in [cc45.md](../bin/cc45.md)).

`__bp_rom`/`__bp_prog` (switch base page for ROM calls) and `__abort` are also defined here.

In a linked program crt0 fills BSS (`__bss_start` to `__bss_end`, including the private base page) with `__bss_fill` (0; cc45 `-fbss-fill=`, ln45 `--bss-fill=`) by a DMA fill job, right after setting up the stack. (`crt_bssinit*.s45`, a byte-at-a-time `_init_bss`, is not used.) Standalone (non-linked) builds generate their own startup stub, which honours `#pragma cc45 no_bssinit`, `heap`, `exit_*` and the other CRT pragmas described in [../bin/cc45.md](../bin/cc45.md).

## 16. Source Layout

```text
lib/
├── include/            # C headers (+ sys/types.h)
├── stdlib/             # C sources, compiled once per calling convention
├── stdlib_asm/         # ZP-convention assembly (*.s45), shared by both archives
├── src/                # float_rom.s45 (in the archives); other files not built
├── crt0.s45, crt0_mega65.s45, crt_heap.s45, crt_bssinit.s45          # stack-convention CRT
├── crt0_zp.s45, crt0_mega65_zp.s45, crt_heap_zp.s45, crt_bssinit_zp.s45  # ZP-convention CRT
├── sac_debug.s45
├── Makefile
└── build/              # c45.lib, c45_zp.lib, c45_stack.lib and the .o45 members
```

## 17. Tests

Runtime tests run on `xemu-xmega65`:

| Script | In `make test` | Covers |
|--------|----------------|--------|
| `src/test/test_xemu.sh` | yes | short/long arithmetic, struct return, array init, compound literals, alloca, memory/string/ctype/stdlib functions; the `src/test-resources/stdlib/` tests (`strdup`, `strlcpy`, `strnlen`, `strtok_r`, `strpbrk`, `strspn`, `strcspn`, `strstr`, `calloc`, `realloc`, `time`, …) in both conventions |
| `src/test/test_stdlib_new.sh` | yes (skips without xemu) | `memchr`, `isgraph`, `strerror`, `qsort`, `bsearch`, array decay |
| `src/test/test_examples.sh` | yes (skips without xemu) | `examples/` programs |

A test program returns its result as the exit status (0 = pass), which the scripts read from `$FD/$FE` in a memory dump.

## See Also

- [stdcbm.md](stdcbm.md) — CBM KERNAL interface (`cbm.h`)
- [calling-conventions.md](calling-conventions.md) — ZP, stack and SAC conventions
- [../bin/cc45.md](../bin/cc45.md) — compiler options, CRT pragmas, inline assembly
- [../bin/ln45.md](../bin/ln45.md) — linking, memory layout
- [../bin/ar45.md](../bin/ar45.md) — `.lib` archives
