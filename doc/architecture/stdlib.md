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
- The hand-written assembly in `lib/stdlib_asm/*.s45` uses the ZP convention and goes into both archives; stack-convention callers reach it through ln45's stack→ZP bridge thunks. A `stdlib/X.c` that has a `stdlib_asm/X.s45` is not compiled.
- Only `.s45` files are assembled. The `.s` files in `lib/stdlib/` and `lib/stdlib_asm/` (`strcspn`, `strdup`, `strlcpy`, `strnlen`, `strpbrk`, `strspn`, `strstr`, `uart`) are not built; those functions come from the C versions — except `uart`, which has no built implementation (see [§12](#12-other-library-backed-headers)).
- `lib/src/*.c` (graphics, sprites, audio, conio/hardware helpers, …) is **not** part of any archive. Headers for those modules exist in `lib/include/`, but their functions are not in the library (see [§14](#14-headers-without-a-library-implementation)).

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
| `dma.h` | `dma_copy`/`dma_fill` | compiler builtins |
| `conio.h`, `far_ptr.h`, `far_ops.h`, `dma_controller.h`, `audio_digi.h`, `audio_midi.h`, `audio_sid.h`, `intwide.h`, `complex.h` | see [§12](#12-other-library-backed-headers) | library / compiler |
| `uart.h` and the graphics, sprite, hardware-helper and game-framework headers | see [§14](#14-headers-without-a-library-implementation) | **no library implementation** |

## 3. Core Headers & Types

- **`<stddef.h>`**: `NULL` (`0`), `size_t` (`unsigned int`, 16-bit), `ptrdiff_t` (`signed int`), `wchar_t` (`unsigned int`, 16-bit — `L"..."`/`L'x'`), `offsetof(type, member)`.
- **`<stdint.h>`**: `int8_t`/`uint8_t` (`char`), `int16_t`/`uint16_t` (`int`), `int32_t`/`uint32_t` (`long`), the `least`/`fast` variants, `intptr_t`/`uintptr_t` (16-bit), `intmax_t`/`uintmax_t` (32-bit), limits and `INTn_C` macros. `int64_t`, `uint64_t` and the 64-bit `least`/`fast` types are `struct __int64` (see `intwide.h`), which cc45 adds to a unit that names a 64-bit type. The signed types are declared `signed`, so they are signed in cc45's default (unsigned) mode.
- **`<stdbool.h>`**: `bool` (`_Bool`, 1 byte, normalizes non-zero to 1), `true` (1), `false` (0), `__bool_true_false_are_defined` (1).
- **`<limits.h>`**: `CHAR_BIT` 8; `SCHAR_MIN`/`SCHAR_MAX` −128/127; `UCHAR_MAX` 255; `CHAR_MIN`/`CHAR_MAX` 0/255; `SHRT_MIN`/`SHRT_MAX`/`USHRT_MAX` and `INT_MIN`/`INT_MAX`/`UINT_MAX` −32768/32767/65535; `LONG_MIN`/`LONG_MAX`/`ULONG_MAX` for 32 bits; `MB_LEN_MAX` 1. The signed limits describe `signed` types; plain `int`/`long` are unsigned in cc45's default mode.
- **`<float.h>`**: CBM 40-bit float (1 exponent byte, 4 mantissa bytes): `FLT_RADIX` 2, `FLT_MANT_DIG` 32, `FLT_DIG` 9, `FLT_MIN_EXP` −127, `FLT_MAX_EXP` 127, `FLT_MIN_10_EXP` −38, `FLT_MAX_10_EXP` 38, `FLT_ROUNDS` 1, `FLT_MAX` 1.7014118e38, `FLT_MIN` 2.9387359e-39, `FLT_EPSILON` 4.6566129e-10; `DBL_*` and `LDBL_*` are the same (`double` and `long double` are the same format).
- **`<stdarg.h>`**: `va_list` (`unsigned int`), `va_start`, `va_arg`, `va_end` (wrapping `__builtin_va_*`) and `va_copy`. Variadic functions always take their arguments on the stack, also under `-fzpcall`. See [../bin/cc45.md](../bin/cc45.md#variadic-functions).
- **`<sys/types.h>`**: `ssize_t`, `mode_t`, `pid_t`, `uid_t`, `gid_t`, `off_t`, `time_t`.

## 4. General Utilities (`stdlib.h`)

**Conversions**
- **`int atoi(char *s)`**: decimal, optional sign, 16-bit result (assembly). It skips leading spaces only (not tabs/newlines).
- **`long atol(char *s)`**: `strtol(s, NULL, 10)`.
- **`char *itoa(int value, char *str, int base)`** / **`char *utoa(unsigned value, char *str, int base)`** / **`char *ltoa(long value, char *str, int base)`**: number to string in base 2–36, using the MEGA65 hardware divider. `itoa`/`ltoa` print a `-` for negative values in base 10 only. Digits above 9 are PETSCII `$41`… (`a`… in the default character set). Return `str`.
- **`atos(s)`**, **`stoa(value, str, base)`**: macros for `(short)atoi(s)` and `itoa((int)value, str, base)`.
- **`long strtol(char *nptr, char **endptr, int base)`** / **`unsigned long strtoul(...)`**: skip whitespace, optional sign (`strtol`), base 0 (auto: `0x`→16, `0`→8, else 10) or 2–36, `0x`/`0X` prefix accepted for base 16, letters `$41`–`$5A` and `$C1`–`$DA` as digits. `*endptr` is set to `nptr` if no digits were read. Overflow is **not** detected: the value wraps modulo 2^32 and `errno` is not set.
- **`float strtof(char *nptr, char **endptr)`** / **`float atof(char *nptr)`**: whitespace, sign, digits, fraction, `e`/`E` exponent. A value beyond the float range gives ±`FLT_MAX`; a non-zero value that underflows gives 0; both set `errno = ERANGE`. `strtod(nptr, endptr)` is a macro for `(double)strtof(...)`.

**Arithmetic**
- **`abs(x)`**, **`labs(x)`**: macros (`(x)<0?-(x):(x)`). Define `__NOMACRO_ABS` / `__NOMACRO_LABS` before including to get the library functions `signed int abs(signed int)` / `signed long labs(signed long)` instead.
- `__builtin_abs`, `__builtin_labs`, `__builtin_ffs`, `__builtin_clz`, `__builtin_ctz`, `__builtin_popcount`, `__builtin_bswap16` are declared here; the library also provides the `l`/`ll` variants of the bit builtins, `__builtin_parity*` and `__builtin_bswap32`.

**Random numbers**
- **`int rand(void)`**: 0–32767 from the MEGA65 hardware RNG (`$D7EF`), waiting on `$D7FE` bit 7 before each of the two byte reads; bit 15 cleared.
- **`void srand(unsigned seed)`**: no-op (the hardware RNG cannot be seeded); sequences are not reproducible.

**Heap** — `void *malloc(size_t)`, `void free(void *)`, `void *calloc(size_t, size_t)`, `void *realloc(void *, size_t)` (assembly, `lib/stdlib_asm/malloc.s45`):
- Sorted singly-linked free list, first fit, coalescing with both neighbours on `free`. Block header: 2-byte size (bit 0 = allocated) and, in free blocks, a 2-byte next pointer. Requests are rounded up to even, minimum 4 bytes including the 2-byte header.
- `malloc(0)` and failure return `NULL` (`errno` is not set). `free(NULL)` is a no-op. `calloc` = `malloc` + `memset`; `realloc` = `malloc` + `memcpy` + `free` (`realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` frees `p`).
- The heap is initialized on the first `malloc` call. It runs from `__heap_start` to `__heap_end`; both default to 0 (library member `__heap_bounds`), which means: start at `__bss_end` (after the program and its private base page), end 4 KB below the 16-bit stack top (`$B000` with the default `$BFFF` top) or at `$C000` with the 8-bit page-1 stack. Define the two symbols in your program to place the heap elsewhere. When the program itself reaches past the heap end (a large program, BSS included), the heap is empty and `malloc` returns `NULL`.
- `realloc` keeps the old block when the new allocation fails. Overflow of `nmemb * size` in `calloc` is not detected.
- `#pragma cc45 heap` makes standalone (non-linked) builds call `_init_heap_crt` at startup; linked programs rely on the lazy initialization.

**Program termination**
- **`void exit(int status)`** (weak) / **`void _exit(int status)`**: jump to the CRT's `__exit`, which switches back to the ROM's base page, stores `status` at `$FD/$FE`, restores the caller's stack pointer and flags and returns to the caller (BASIC). `exit` does **not** run `atexit` handlers.
- **`void abort(void)`** (weak): like `exit`, with status `$FFFE`. If `__exit` is never reached the status reads `$FFFF` (set at startup). A BASIC caller can read it with `PEEK(253)+256*PEEK(254)`.
- **`int atexit(void (*func)())`**: registers up to 8 handlers (returns 1 when full). The handlers are stored but nothing calls them (`__atexit_run` is never invoked).
- `#pragma cc45 exit_halt` / `exit_brk` / `exit_rts` only change the startup stub of standalone builds; a program linked with the library always returns to its caller.

**Sorting and searching**
- **`void qsort(void *base, int nmemb, int size, int (*cmp)())`**: quicksort with median-of-three pivot, insertion sort for partitions of ≤ 8 elements, smaller partition first. The comparator and size are kept in globals (`qs_cmp`, `qs_size`), so `qsort` is not reentrant (a comparator must not call `qsort`).
- **`void *bsearch(const void *key, const void *base, int nmemb, int size, int (*cmp)())`**: binary search; returns the match or `NULL`.

## 5. String & Memory Operations (`string.h`)

The functions marked *asm* are hand-written assembly in `lib/stdlib_asm/`; the rest are C in `lib/stdlib/`. Lengths and counts are `int` (16-bit).

| Function | Notes |
|----------|-------|
| `int strlen(char *s)` | *asm*. Any length up to 65535 (Y walks a page, X counts the pages) |
| `char *strcpy(char *d, char *s)`, `char *strncpy(char *d, char *s, int n)` | *asm*. `strncpy` NUL-pads |
| `signed int strcmp(char *a, char *b)`, `signed int strncmp(char *a, char *b, int n)` | *asm*. Return <0, 0, >0 (declared `signed`: a plain `int` is unsigned in cc45, which made `strcmp(a, b) < 0` always false) |
| `char *strcat(char *d, char *s)`, `char *strncat(char *d, char *s, int n)` | *asm*. `strncat` appends at most `n` chars and NUL-terminates |
| `char *strchr(char *s, int c)`, `char *strrchr(char *s, int c)` | *asm* |
| `char *strstr(char *h, char *n)`, `char *strpbrk(char *s, char *accept)` | C |
| `int strspn(char *s, char *accept)`, `int strcspn(char *s, char *reject)` | C |
| `char *strtok(char *s, char *delim)`, `char *strtok_r(char *s, char *delim, char **save)` | C. `strtok` keeps static state |
| `int strnlen(const char *s, int maxlen)` | C |
| `int strlcpy(char *dst, const char *src, int size)` | C. Returns `strlen(src)` |
| `char *strdup(const char *s)` | C. Uses `malloc` |
| `char *strerror(int errnum)` | C. "Success", "Result out of range", "Out of memory", "Invalid argument", "Math domain error", else "Unknown error" |
| `void *memcpy(void *d, void *s, int n)` | *asm*. Byte loop; no overlap handling |
| `void *memmove(void *d, void *s, int n)` | *asm*. Copies backwards when `d > s` |
| `void *memset(void *s, int c, int n)` | *asm* |
| `signed int memcmp(void *a, void *b, int n)` | *asm*. Returns <0, 0, >0 |
| `void *memchr(void *s, int c, int n)` | C |

`string.h` also declares `__builtin_memcpy`, `__builtin_memset`, `__builtin_memmove`, `__builtin_memcmp`, `__builtin_strlen`, `__builtin_strcpy` and `__builtin_strcmp`, which the library provides. The library also exports `memcpy_optimized` (`stdlib/memcpy_fast.c`), which is not declared in any header.

## 6. Character Utilities (`ctype.h`)

All functions are `static inline` in the header and take an `int` PETSCII code (the library also has assembly versions, used only when `ctype.h` is not included). They return 1 or 0, except the conversions.

PETSCII ranges (default upper/lower-case character set): digits `$30`–`$39`; lower-case letters `$41`–`$5A`; upper-case letters `$C1`–`$DA`; space `$20`.

| Function | True for |
|----------|----------|
| `isdigit` | `$30`–`$39` |
| `isalpha` | `$41`–`$5A`, `$C1`–`$DA` |
| `isalnum` | digit or letter |
| `isspace` | `$20`, `$09`–`$0D` |
| `isblank` | `$20`, `$09` |
| `isupper` / `islower` | `$C1`–`$DA` / `$41`–`$5A` |
| `isxdigit` | digit, `$41`–`$46`, `$C1`–`$C6` |
| `isprint` | `$20`–`$7E`, `$A0`–`$FF` |
| `isgraph` | `$21`–`$7E`, `$A1`–`$FF` |
| `iscntrl` | `$00`–`$1F`, `$80`–`$9F` |
| `ispunct` | everything from `$21` up that is not a digit or letter (this includes `$7F`–`$9F` and `$A0`) |

`toupper(c)` returns `c | $80` for `$41`–`$5A`; `tolower(c)` returns `c & $7F` for `$C1`–`$DA`; other values are returned unchanged.

## 7. Math Library (`math.h`)

`math.h` includes `stdlib.h`.

**Integer**
- `div_t div(signed int, signed int)` and `ldiv_t ldiv(signed long, signed long)` (`quot`, `rem`).
- `signed int min(a, b)`, `max(a, b)`, `gcd(a, b)` (Euclid, handles negative inputs), `lcm(a, b)` (`|a / gcd(a,b) * b|`).
- `abs`/`labs` as in `stdlib.h`.

**Floating point** (CBM 40-bit; `double` is the same type)
- ROM-backed (BASIC 65 routines through `float_rom.s45`, which switches to base page 0 for each call): `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf`.
- Written in C: `powf`, `fmodf`, `ceilf`, `floorf`, `roundf` (ties away from zero), `truncf`, `atan2f`, `log10f`, `log2f`, `ldexpf`, `frexpf`, `modff`, `copysignf`, `fmaxf`, `fminf`, `fdimf`.
- The standard names (`sin`, `cos`, `tan`, `atan`, `atan2`, `log`, `log10`, `log2`, `exp`, `pow`, `sqrt`, `fabs`, `fmod`, `ceil`, `floor`, `round`, `trunc`, `ldexp`, `frexp`, `modf`, `copysign`, `fmax`, `fmin`, `fdim`) are macros for the `f` versions; the library also defines them as functions (`math_double.c`) for code that declares them itself, together with `signbit`/`signbitf`.
- Classification: `isnan(x)` and `isinf(x)` are always 0 and `isfinite(x)` always 1 (the format has no NaN or infinity); `__fpclassifyf(x)` returns 0 for zero, else 1.
- Constants: `M_PI`, `M_PI_2`, `M_PI_4`, `M_E`, `M_LOG2E`, `M_LOG10E`, `M_LN2`, `M_LN10`, `M_SQRT2`, `M_SQRT1_2`, `HUGE_VALF` (1.7014118e38 = `FLT_MAX`), `INFINITY` (= `HUGE_VALF`).

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
- **`int puts(char *s)`**: prints `s` (any length) and a carriage return (`$0D`) through `CHROUT`; returns 0.

These (and every other ROM call in the library) are bracketed with `__bp_rom`/`__bp_prog`, which switch to the ROM's base page 0 and back to the program's relocated base page.

### Formatted output

`printf(char *fmt, ...)`, `vprintf(char *fmt, va_list)`, `sprintf(char *buf, char *fmt, ...)`, `vsprintf`, `snprintf(char *buf, int size, char *fmt, ...)`, `vsnprintf` (writes at most `size-1` characters plus NUL, returns the length that would have been written), `fprintf`/`vfprintf` (to a `FILE`). They return the number of characters written. All share one formatter (`__vformat.c`):

- Syntax `%[flags][width][.precision][length]conversion`; flags `- + space # 0`; width and precision as digits or `*`; length `hh h l ll L j z t`.
- Conversions: `d i u x X o b p c s n %`, and `f F e E g G`. `int` is 16 bits, `long` 32 bits; `ll` reads 8 bytes and prints the low 32 bits. `%b` is binary, `%p` prints `$hhhh`. An unknown conversion character is printed as is.
- Floats: `printf`/`sprintf` are linked in an integer-only (weak) version by default; passing a float argument in the `...` of a variadic call (or to an unprototyped function) makes the linker pull `printf_float`/`sprintf_float`, whose strong `printf`/`sprintf` format floats. Without them the float conversions print nothing. The float formatter takes its digits from the BASIC 65 ROM's own number formatting (FOUT, which `PRINT` and `STR$` use; `__float_fout` in `float_rom.s45`), so every value of the CBM format prints (up to about ±1.7×10³⁸, with the 9–10 significant digits the format holds; further digits are 0). `%f`, `%e`, `%g` and `%F`/`%E`/`%G` follow C: precision (default 6; at most 20), rounding half up (`%.0f` of 2.5 is `3`), `%e` with an exponent of at least two digits (`1.000000e+30`), `%g` choosing the form by exponent and dropping trailing zeros, the `#` flag, and width / `-` / `0` / `+` / space. FOUT works in page 1 (`$0100`), unused with the default 16-bit CPU stack. One ROM quirk is handled: FOUT prints a value just below a power of ten (9.99999999…E19) as `" 0E19"`; the formatter reads that as 1E20. `snprintf`, `vsnprintf`, `vprintf`, `vsprintf` and `fprintf` have no float support.

### Formatted input

`sscanf(char *str, char *fmt, ...)` (and `fscanf`/`vfscanf` on a `FILE`): `%d %u %x %o %c %s %f %%` and `%ld %lu %lx %lo`; `%x` accepts a `$` or `0x` prefix, `%c` does not skip whitespace, `%s` reads a whitespace-delimited word, `%f` uses `strtof`. Whitespace in the format matches any amount of input whitespace (space, tab, LF, CR). Returns the number of items assigned.

### FILE streams

`FILE` (handle, mode, flags, position, buffer, `ungetc` slot), `fpos_t` (`unsigned long`), `stdin`/`stdout`/`stderr`, `EOF` (−1), `SEEK_SET/CUR/END`, `BUFSIZ` 256, `FILENAME_MAX` 16, `FOPEN_MAX` 8, `L_tmpnam` 32, `_IO*` flag bits.

Functions: `fopen`, `freopen`, `fclose`, `fflush`, `fgetc`, `fputc`, `fgets`, `fputs`, `ungetc`, `fread`, `fwrite`, `fprintf`, `vfprintf`, `fscanf`, `vfscanf`, `fseek`, `ftell`, `rewind`, `fgetpos`, `fsetpos`, `feof`, `ferror`, `clearerr`, `setvbuf`, `setbuf`, `tmpfile`, `tmpnam`, `perror` (prints `s: strerror(errno)` to `stderr`). `fileno(stream)` is a macro (`stream->handle`) and `fdopen(fd, mode)` a cast.

Behaviour on the MEGA65:
- `fopen` opens files on disk device 8. The DOS takes the type and direction from the name, so it opens `NAME,S,R` (`r`), `@0:NAME,S,W` (`w`; the `@` replaces an existing file, as C's `w` truncates) or `NAME,S,A` (`a`); `b` makes it a USR file (`U`). A name that already contains a `,` is used as given. Each file gets its own drive channel (secondary address = its logical file number). `OPEN` succeeds even for a missing file, so `fopen` then reads the drive's status (`cbm_drive_status`) and fails on an error (≥ 20, e.g. 62 FILE NOT FOUND). Tested on xemu with a D81 (`test_fileio_xemu.c`).
- **`r+`, `w+`, `a+`** (CBM drives don't read and write a sequential file at once): the stream is held in memory. `fopen` reads the whole file into the heap (`r+` fails for a missing file; `w+` creates the file empty at once; `a+` creates it when written back); reads, writes and `fseek` work on that copy, and `fflush`/`fclose` write it back with `@0:` replacement. A write past the end after a seek fills the gap with zeros; `a+` always writes at the end. The file must fit in the heap (at most 65280 bytes).
- **`fseek`/`ftell`/`rewind`/`fgetpos`/`fsetpos`** (in `stdio_seek.c`, linked only when used): a memory stream seeks anywhere. A stream opened with `r` seeks forward by reading and backward by reopening the file and reading to the position; `SEEK_END` reads to the end first. A stream opened with `w` or `a` (written sequentially) can only seek to where it is: anything else fails with `errno = ESPIPE`. `ungetc` then `fgetc` keeps `ftell` right. `test_fileseek_xemu.c` (both conventions) covers all of it.
- `fgets`/`fputs` are in `stdio_line.c`, apart from `fprintf`/`fscanf`, so line I/O does not link the printf machinery (ln45 links whole objects).
- `tmpfile` returns `NULL`; `tmpnam` returns its (empty) buffer.

### CBM file API

Declared in `stdio.h`, implemented in `stdio_cbm.c`, `stdio_kernal.c`, `stdio_directory.c`, `stdio_rel.c`:

- Files: `fd_t cbm_open(device, filename, type)` (secondary address 0), `cbm_open_ext(device, filename, type, secondary)` (`$FF`: the logical file number), `cbm_close`, `cbm_read`, `cbm_write` (these return `signed int`, −1 on error; `fd_t` is `signed int`), `cbm_drive_status(device)` (the number of the drive's status message: 0 OK, ≥ 20 an error, −1 no channel), `cbm_eof`, `cbm_status`, and `cbm_seek`/`cbm_tell` (stubs: −1 / 0). Logical file numbers 3–14 are allocated as free ones (15 is the command channel, which stays open while files are open: closing it makes the drive close all its files). `cbm_read` keeps the byte that arrives with the end-of-file status. Constants `CBM_DEVICE_*`, `CBM_T_PRG/SEQ/USR/REL/CBM/DIR`, `O_*`, `STDIN_FILENO`…
- Directory and disk commands: `cbm_opendir`, `cbm_readdir`, `cbm_closedir`, `cbm_dir_type_name`, `cbm_dir_get_type`, `cbm_dir_is_locked`, `cbm_dir_is_scratched`, `cbm_dir_is_closed`, `cbm_format`, `cbm_delete`, `cbm_rename`; directory type flag bits `CBM_FILE_*`.
- Relative (REL) files: `cbm_rel_open(device, filename, record_size, mode)`, `cbm_rel_read`, `cbm_rel_write`, `cbm_rel_position`, `cbm_rel_tell`, `cbm_rel_size`, `cbm_rel_close`, `cbm_rel_record_size`, `cbm_rel_is_open`, `cbm_rel_append`, `cbm_rel_delete`, `cbm_rel_truncate`, `cbm_rel_update`, `cbm_rel_insert`, `cbm_rel_lasterror`, `cbm_rel_strerror`; `CBM_REL_READ`/`WRITE`, `CBM_REL_ERR_*`.
- KERNAL wrappers: `cbm_k_setnam(filename)`, `cbm_k_setlfs`, `cbm_k_open`, `cbm_k_close`, `cbm_k_chkin`, `cbm_k_chkout`, `cbm_k_clrchn`, `cbm_k_chrin`, `cbm_k_chrout`, `cbm_k_readst` (`cbm_k_chrin`/`cbm_k_chrout` are in the library but not declared in `stdio.h`).

`cbm_open` and `cbm_close` here have different signatures from the `static` `cbm_open(void)`/`cbm_close(char)` in `cbm.h`; in a file that includes both headers the `cbm.h` versions are used without a diagnostic.

## 9. Error Handling (`errno.h`)

- **`extern int errno`**: set only by the float functions and `strtof` listed in [§7](#7-math-library-mathh) and [§4](#4-general-utilities-stdlibh). It is never cleared. `strtol`, `strtoul` and `malloc` do not set it.
- **`extern char _errnoc`**: the KERNAL status byte `ST` at `$0090` of the ROM's page 0 (read-only alias).

| Constant | Value | Used for |
|----------|-------|----------|
| `ERANGE` | 1 | result out of float range (`strtof`, `logf(0)`, `expf`, `powf`) |
| `ENOMEM` | 2 | defined; not set by the library |
| `EINVAL` | 3 | defined; not set by the library |
| `EDOM` | 4 | argument without a value (`sqrtf`/`logf` of a negative, `powf` negative^fraction, `fmodf(x, 0)`) |

## 10. Non-Local Jumps, Assertions, `alloca`

- **`setjmp.h`**: `jmp_buf` is `char[6]`: return address (2 bytes), stack pointer inside `setjmp` (2 bytes) and the frame pointer `$FD/$FE` (2 bytes). `int setjmp(jmp_buf)` returns 0, and the value given to `void longjmp(jmp_buf, int val)` when that restores the context (1 if `val` is 0). Assembly (`lib/stdlib_asm/setjmp.s45`).
- **`assert.h`**: `assert(expr)` calls `assert_fail(#expr, __FILE__, __LINE__)`, which prints `Assertion failed: <expr> (file <f>, line <n>)` with `printf` and stops with `BRK` (`assert_fail_abort`). `assert_msg(expr, msg)` calls `assert_msg_fail(..., msg)`. With `NDEBUG` both are no-ops.
- **`alloca.h`**: `void *alloca(unsigned)` and `__builtin_alloca`. A constant size is allocated in the frame by the compiler; a variable size uses the runtime routine (`alloca.s45`). A function that calls `alloca` with a variable size uses a stack frame instead of static allocation.

## 11. Timing (`time.h`)

- `CLOCKS_PER_SEC` 60; `clock_t` is `int`, `time_t` is `long`.
- **`clock_t clock(void)`**: low 16 bits of the KERNAL jiffy clock (`RDTIM`, `$FFDE`); wraps after 65536 ticks (~18 minutes at 60 Hz).
- **`time_t time(time_t *tp)`**: `clock()` widened to `long` (so it also wraps after 65536 ticks); stored in `*tp` when `tp` is not `NULL`.
- **`long difftime(time_t t2, time_t t1)`**: `t2 - t1`.
- **`struct tm`** (`tm_sec` … `tm_isdst`, all `int`).
- **`void rtc_read(struct tm *)`**: reads the MEGA65 RTC (BCD registers at `$FFD7110`–`$FFD7116`) into a `struct tm`; years 00–99 map to 2000–2099; `tm_isdst` 0. Emulators without an RTC give zeros.
- **`localtime(timer)`** / **`gmtime(timer)`**: ignore `timer`, read the RTC into a static `struct tm` and return it.
- **`time_t mktime(struct tm *)`**: seconds since 2000-01-01 00:00:00 (not 1970); fills in `tm_yday` and `tm_wday`.
- **`asctime(tm)`** / **`ctime(timer)`**: `"Wed Jun 30 21:49:08 2026\n"` in a static buffer; `ctime(t)` is `asctime(localtime(t))`.

The jiffy clock counts 60 Hz ticks from power-on; it is not wall-clock time.

## 12. Other Library-Backed Headers

| Header | Contents |
|--------|----------|
| `conio.h` | cc65-style console: colour constants, `clrscr`, `gotoxy`, `wherex`, `wherey`, `textcolor`, `textbackground`, `textattr`, `putch`, `cputch`, `cputs`, `cputsxy`, `cprintf`, `cprintfxy`, `getch_xy`, `getattr_xy`, `screenwidth`, `screenheight`, `cursoron`, `cursoroff`, `getch`, `kbhit` (`stdlib/conio.c`). `getch` and `kbhit` are stubs that return 0. The `fcm_*` full-colour functions and `conio_set_width`/`conio_get_width` are declared but not in the library. |
| `far_ptr.h` | 24-bit far pointers (`far_ptr_t`, `bank_t`), bank selection (`bank_set`, `bank_get`, `bank_available`, `bank_reset`, `bank_reset_all`) and a far heap (`far_malloc`, `far_calloc`, `far_realloc`, `far_free`, `far_malloc_current`, `far_calloc_current`, `far_size`, `far_alloc_bank`, `far_is_valid`, `far_policy_*`) |
| `far_ops.h` | far element access (`far_read_*`/`far_write_*` for 8/16/32-bit, float, double), arrays (`far_array_*`), bulk copies (`far_memcpy*`, `far_memset`, `far_memzero`, `far_strcpy_*`, `far_strlen`, `far_strcmp`) and pointer arithmetic/comparison (`far_add`, `far_sub`, `far_inc`, `far_dec`, `far_diff`, `far_eq`, `far_lt`, `far_gt`, `far_cmp`) |
| `dma_controller.h` | F018B DMA request API: `dma_execute`, `dma_transfer_smart`, `dma_fill`, `dma_available`, `dma_is_busy`, `dma_wait`, `dma_enable`, `dma_disable`, `dma_set_interrupt_enable`, `dma_get_status`, `dma_print_status`, `dma_speed_bytes_per_us`. Its `dma_fill` collides with the `dma_fill` macro of `dma.h`. |
| `audio_digi.h`, `audio_midi.h`, `audio_sid.h` | `digi_system_create`, `digi_sample_create`, `digi_sample_mix_static`; `midi_receiver_create`, `midi_note_to_frequency`, `frequency_to_midi_note`; `sid_synth_create` |
| `intwide.h` | `struct __int64`, `struct __int128` with operators, and the width-parameterized runtime `__intN_add`, `__intN_sub`, `__intN_mul`, `__intN_cmp_u`, `__intN_cmp_s`, `__intN_neg`, `__intN_not`, `__intN_and`, `__intN_or`, `__intN_xor`, `__intN_shl`, `__intN_shr_u` (used by `__int(N)`/`__uint(N)`) |
| `complex.h` | marker only: the `_Complex` types and operators are built into cc45 |

## 13. MEGA65 Hardware (`mega65.h`) and DMA (`dma.h`)

`mega65.h` provides `volatile` struct overlays and register macros. All helpers except `key_pressed` are `static` functions in the header.

- **VIC-IV** (`vic4` at `$D000`, `struct vic4_regs` covering `$D000`–`$D07F`): `vic4->border`, `vic4->bg0`, `vic4->sprite_enable`, `vic4->sprite[n].x/.y`, `ctrl1`, `ctrl2`, `ctrl_a`, `ctrl_b`, `ctrl_c`, `key`, … `VIC4_SPRITE(n)` gives a sprite position pointer. Direct registers: `VREG_BORDER`, `VREG_BG0`–`VREG_BG3`, `VREG_CTRL1`, `VREG_CTRL2`, `VREG_RASTER`, `VREG_SPR_ENABLE`, `VREG_SPR_XMSB`, `VREG_MEMPTR`, `VREG_IRQ_STATUS`, `VREG_IRQ_ENABLE`, `VREG_KEY`, `VREG_CTRL_A`, `VREG_CTRL_B`, `VREG_CTRL_C`. Helpers: `vic4_unlock()` (writes `$A5`,`$96` to `$D02F`), `vic4_fast()` (unlock + `VIC4_FAST` in `$D031`), `vic4_sprite_pos(n, x, y)` (handles X > 255). Constants: `COLOR_BLACK` (0) … `COLOR_LGREY` (15); bits `VIC4_RST8`, `VIC4_ECM`, `VIC4_BMM`, `VIC4_DEN`, `VIC4_RSEL`, `VIC4_MCM`, `VIC4_CSEL`, `VIC4_IRQ_*`, `VIC3_*` (`$D030`), `VIC4_H640`, `VIC4_FAST`, `VIC4_FCM`, … (`$D031`), `VIC4_CHR16`, … (`$D054`); `VIC3_KEY1`/`VIC3_KEY2`.
- **SID** (`sid1`–`sid4` at `$D400`/`$D420`/`$D440`/`$D460`; `SID1_VOICE(n)`–`SID4_VOICE(n)` for voice registers `freq_lo`, `freq_hi`, `pw_lo`, `pw_hi`, `ctrl`, `attack_decay`, `sustain_release`; chip registers `filter_lo`, `filter_hi`, `filter_ctrl`, `volume`, `pot_x`, `pot_y`, `osc3`, `env3`). Bits `SID_GATE`, `SID_SYNC`, `SID_RINGMOD`, `SID_TEST`, `SID_TRIANGLE`, `SID_SAWTOOTH`, `SID_PULSE`, `SID_NOISE`, `SID_FILT1`–`SID_FILT3`, `SID_FILT_EXT`, `SID_LP`, `SID_BP`, `SID_HP`, `SID_MUTE3`.
- **CIA** (`cia1` `$DC00`, `cia2` `$DD00`: `pra`, `prb`, `ddra`, `ddrb`, `timer_a_lo/hi`, `timer_b_lo/hi`, `tod_*`, `sdr`, `icr`, `cra`, `crb`). `joy1_read()`/`joy2_read()` return active-high `JOY_UP/DOWN/LEFT/RIGHT/FIRE` bits. `CIA_ICR_*`, `CIA_CRA_*`, `CIA_CRB_*`.
- **Keyboard**: `__regparm unsigned char key_pressed(unsigned char keycode)` (library, assembly) scans the CIA1 matrix and returns 1/0, allowing several keys at once; 64 `KEY_*` matrix codes. Hardware typing queue: `ASCII_KEY_QUEUE` (`$D610`), `PETSCII_KEY_QUEUE` (`$D619`), `KEY_MODIFIERS_REG` (`$D611`) with `KEY_MOD_*` bits; `getkey_ascii()` (0 if empty) and `getkey_petscii()` (`$FF` if empty) read and advance it.
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

## 14. Headers Without a Library Implementation

These headers declare functions whose sources (`lib/src/*.c`, `lib/stdlib/uart.s`) are not built into any archive, so calls to them fail to link: `uart.h`, `audio.h`, `cache_opt.h`, `data_structures.h`, `event_system.h`, `graphics.h`, `graphics_hal.h`, `graphics_particles.h`, `graphics_rrb.h`, `graphics_sprites.h`, `graphics_sprites_advanced.h`, `graphics_text80x50.h`, `hw_audio_mixer.h`, `hw_io.h`, `hw_peripherals.h`, `hw_raster.h`, `hw_scroll.h`, `joystick.h`, `lightpen.h`, `mega65_version.h`, `mem_pool.h`, `paddle_mouse.h`, `palette.h`, `persistence.h`, `procedural_content.h`, `resource_manager.h`, `scripting.h`, `simd.h`, `sprites.h`, `sprite_*.h`, `test_harness.h`, `ui.h`, `world.h`. `sincos.h` and `sincos16.h` contain only `static` code and tables and work without the library.

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
| `src/test/test_xemu.sh` | yes | short/long arithmetic, struct return, array init, compound literals, alloca, memory/string/ctype/stdlib functions |
| `src/test/test_stdlib_new.sh` | yes (skips without xemu) | `src/test-resources/stdlib/` tests: `strdup`, `strlcpy`, `strnlen`, `strtok_r`, `strpbrk`, `strspn`, `strcspn`, `strstr`, `calloc`, `realloc`, `time`, … |
| `src/test/test_examples.sh` | yes (skips without xemu) | `examples/` programs |

A test program returns its result as the exit status (0 = pass), which the scripts read from `$FD/$FE` in a memory dump. `make test-stdlib` (`src/test/test_stdlib.sh`) is an older runner for the mmemu emulator and is not part of `make test`.

## See Also

- [stdcbm.md](stdcbm.md) — CBM KERNAL interface (`cbm.h`)
- [calling-conventions.md](calling-conventions.md) — ZP, stack and SAC conventions
- [../bin/cc45.md](../bin/cc45.md) — compiler options, CRT pragmas, inline assembly
- [../bin/ln45.md](../bin/ln45.md) — linking, memory layout
- [../bin/ar45.md](../bin/ar45.md) — `.lib` archives
