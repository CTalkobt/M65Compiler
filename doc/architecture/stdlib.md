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

- The library C sources are compiled with `-O2 -fimplicit-function-declaration -fsigned-default -funsigned-char` (and `-fzp-locals-max=8`: a function's base-page block is at most 8 bytes): they are written as standard C (signed `int`/`long`), unlike cc45's unsigned-by-default user code.
- They are also compiled with `-ffunction-sections`: every function, variable, string literal and static-allocation block is a section, and ln45 drops what the program does not reach (one `conio` call no longer links all of `conio`, nor `cprintf`'s `vsprintf`, nor the unused functions' storage). The hand-written assembly (`stdlib_asm/*.s45`, `src/float_rom.s45`) is assembled with `ca45 -ffunction-sections`: a section per `proc`, and `float_rom.s45` marks each entry with `.split` (routines that branch into one another stay together). Over the runtime tests, the memory a program uses up to the end of BSS is 10.5% smaller than without dropping; crt0 stays whole.
- The hand-written assembly in `lib/stdlib_asm/*.s45` uses the ZP convention and goes into both archives; stack-convention callers reach it through ln45's stack→ZP bridge thunks. A `stdlib/X.c` that has a `stdlib_asm/X.s45` is not compiled.
- Compiler runtime in `stdlib_asm/divmod.s45`: `__divs16`, `__divu16`, `__mods16`, `__modu16` and the `32` forms, which cc45 calls for `/` and `%` (dividend in A/X or A/X/Y/Z, divisor in `__zp_scratch2`, result in the same registers), and `__mul16` / `__mul32` for `*` outside loops (the same way; `__mul16y` takes a byte factor in Y; `__mul16y32` too, for a 24-bit product in A/X/Y, in loops as well, #585). Plain labels, so ZP- and stack-convention code call them directly. It declares `cc45.icallinfo` (no calls through pointers) for ln45's call graph.
- Only `.s45` files are assembled. The `.s` files in `lib/stdlib/` and `lib/stdlib_asm/` (`strcspn`, `strdup`, `strlcpy`, `strnlen`, `strpbrk`, `strspn`, `strstr`, `uart`) are not built; those functions come from the C versions — except `uart`, which has no built implementation (see [§12](#12-other-library-backed-headers)).
- `lib/src/*.c` (graphics, sprites, audio, hardware helpers, …) is **not** part of any archive. Headers for those modules exist in `lib/include/`, but their functions are not in the library (see [§14](#14-headers-without-a-library-implementation)).

`lib/build/` also holds `tuneplay.o45` (`lib/tools/tuneplay.c`), not a library member: the player program `tune45 play --emulator` / `--send` links with the tunes ([tune45.md](../bin/tune45.md)).

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
| `time.h` | jiffy clock, MEGA65 RTC, `strftime`, `clock_gettime` | library |
| `inttypes.h`, `signal.h`, `locale.h` | C99's format macros and `intmax_t` functions, signals, the "C" locale ([§12](#12-other-library-backed-headers)) | library |
| `zlib.h` | `crc32`, `adler32` ([§12](#12-other-library-backed-headers)) | library (`stdlib_asm/crc32.s45`, `adler32.s45`) |
| `joystick.h` | cc65's joystick API over `input_joy` ([§12](#12-other-library-backed-headers)) | library (`stdlib_asm/joystick.s45`) |
| `unistd.h` | `sleep`, `getopt` ([§12](#12-other-library-backed-headers)) | library (`stdlib_asm/sleep.s45`, `stdlib/getopt.c`) |
| `serial.h` | cc65's serial API on the buffered UART ([§12](#12-other-library-backed-headers)) | library (`stdlib_asm/serial.s45`) |
| `ringbuf.h`, `pqueue.h`, `vector.h`, `bitset.h`, `list.h`, `set.h`, `map.h`, `hashmap.h` | containers: a ring buffer (deque, queue, stack; a byte queue), a priority queue, a growable array and views, bit arrays, intrusive lists, ordered sets and maps (sorted array, red-black tree), hash sets and maps ([§12b](#12b-containers)) | library (`stdlib_asm/ringbuf.s45`, `pqueue.s45`, `vector.s45`, `bitset.s45`, `list.s45`, `set.s45`, `tset.s45`, `hashmap.s45`, `container.s45`) |
| `cbm.h` | KERNAL wrappers ([stdcbm.md](stdcbm.md)) | `static __forceinline` functions in the header (always inlined) |
| `mega65.h` | hardware register overlays | header (+ `key_pressed` in the library) |
| `dma.h` | `dma_copy`/`dma_fill` (16-bit, bank 0); `dma_copy_far`/`dma_fill_far` (28-bit); `dma_copy_far_step`/`dma_fill_far_step` (with a step) | compiler builtins; `lib/stdlib_asm/dma_far.s45`, `dma_step.s45` |
| `palette.h` | the VIC-IV's 256-entry palette: set, get, save, restore, rotate ([§13](#13-mega65-hardware-mega65h-and-dma-dmah)) | library (`stdlib_asm/palette.s45`) |
| `seam.h` | screens of two-byte cells: full-colour characters among text ([§13](#13-mega65-hardware-mega65h-and-dma-dmah)) | library (`stdlib_asm/seam.s45`) |
| `tgi.h`, `tgi/tgi-error.h`, `tgi/tgi-kernel.h`, `tgi/tgi-vectorfont.h` | cc65's TGI graphics: 320x200, 256 colours, one or two pages; 640x200; the VIC-II bitmaps; text screens with block characters ([§12a](#12a-graphics-tgih)) | library (`stdlib/tgi*.c`, `stdlib_asm/tgi_*.s45`) |
| `input.h` | see [§11a](#11a-keyboard-and-joysticks-inputh) | library |
| `sprites.h` | the VIC-IV's 8 hardware sprites ([§13](#hardware-sprites-spritesh)) | library (`stdlib_asm/sprites.s45`) |
| `font.h` | font45's fonts: measured, drawn into bitmaps and full-colour characters, 8x8 charsets ([§13](#fonts-fonth)) | library (`stdlib_asm/font.s45`, `font_glyph.s45`; `stdlib/font_load.c`) |
| `load.h` | a file loaded to any 28-bit address ([§13](#loading-files-anywhere-loadh)) | library (`stdlib/load.c`) |
| `unpack.h` | pack45's streams unpacked by DMA ([§13](#packed-data-unpackh)) | library (`stdlib_asm/unpack.s45`: the dispatch; the decoders `unpack1.s45`, `unpack2.s45`) |
| `rom_ram.h` | the unused C64 ROM space as RAM ([§13](#rom_ramh)) | library (`stdlib_asm/rom_ram.s45`) |
| `rrb.h` | the raster rewrite buffer: 16-bit character rows, GOTOX tokens ([§13](#raster-rewrite-buffer-rrbh)) | library (`stdlib/rrb.c`) |
| `hw_raster.h` | raster interrupts: the program's `__interrupt` functions or plain callbacks at lines, the KERNAL's handler kept; the line, waits, frames ([§13](#raster-interrupts-hw_rasterh)) | library (`stdlib_asm/hw_raster.s45`; `stdlib/hw_raster_cb.c`, `hw_raster_exit.c`) |
| `hw_scroll.h` | hardware scrolling of the text screen: fine and coarse, smooth moves ([§13](#hardware-scrolling-hw_scrollh)) | library (`stdlib_asm/hw_scroll.s45`) |
| `mega65_version.h` | the model ID, the FPGA date and firmware ID ([§13](#model-and-fpga-version-mega65_versionh)) | library (`stdlib/mega65_version.c`) |
| `hw_audio_mixer.h` | the SIDs' volumes, pan, mute, fades on the audio mixer ([§13](#audio-mixer-hw_audio_mixerh)) | library (`stdlib/hw_audio_mixer.c`) |
| `hw_io.h` | the SD card, the F011 floppy, Ethernet, the RTC, the hypervisor's DOS calls ([§13](#sd-card-floppy-ethernet-hypervisor-hw_ioh)) | library (`stdlib/hw_io.c`, `stdlib_asm/hw_hyper.s45`) |
| `hw_peripherals.h` | CPU speed, identification, debug output, system information ([§13](#cpu-speed-debug-output-system-information-hw_peripheralsh)) | library (`stdlib/hw_peripherals.c`) |
| `event_system.h` | publish / subscribe events, deferred queue ([§12](#12-other-library-backed-headers)) | library (`stdlib/event_system.c`) |
| `mem_pool.h` | fixed-size object pools, linear arenas, a pool per type ([§12](#12-other-library-backed-headers)) | library (`stdlib/mem_pool.c`) |
| `fixed.h` | the run time and macros of `__fixed(I, F)` ([§5](#fixedh)) | library (`stdlib_asm/fixed.s45`); included automatically |
| `audio_tune.h` | the tune (song) player ([§12](#12-other-library-backed-headers)) | library (`stdlib_asm/tune.s45`, `tune_code.s45`) |
| `sincos.h`, `sincos16.h` | integer sine tables ([§14](#14-headers-without-a-library-implementation)) | header only (`static` tables and code) |
| `overlay.h` | `__ovl_current` (the code overlay in the region), `__ovl_region`: in a program with code overlays (the runtime `stdlib_asm/ovl.s45`, and `ovl_load.s45`, `ovl_map.s45`, `ovl_jmp.s45` when needed, #689; [ln45: Code Overlays](../bin/ln45.md#code-overlays)) | library / ln45 |
| `conio.h`, `far_ptr.h`, `far_ops.h`, `far_string.h`, `dma_controller.h`, `audio_digi.h`, `audio_midi.h`, `audio_sid.h`, `intwide.h`, `complex.h` | see [§12](#12-other-library-backed-headers) | library / compiler |
| `uart.h` and the `sprite_*`, input and game-framework headers | see [§14](#14-headers-without-a-library-implementation) | **no library implementation** |

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
- **`signed int atoi(char *s)`**: decimal, optional sign, 16-bit result (assembly). It skips leading spaces only (not tabs/newlines).
- **`signed long atol(char *s)`**: `strtol(s, NULL, 10)`.
- **`void ftoa(float f, char *s)`** (oscar64's; `stdlib/ftoa.c`): `f` as `printf`'s `%g` writes it (`__pf_format`): 6 significant digits, an exponent outside 1e-4 .. 1e6.
- **`char *itoa(signed int value, char *str, int base)`** / **`char *utoa(unsigned value, char *str, int base)`** / **`char *ltoa(signed long value, char *str, int base)`** / **`char *ultoa(unsigned long value, char *str, int base)`**: number to string in base 2–36, using the MEGA65 hardware divider. `itoa`/`ltoa` print a `-` for negative values in base 10 only. Digits above 9 are PETSCII `$41`… (`a`… in the default character set). Return `str`.
- **`atos(s)`**, **`stoa(value, str, base)`**: macros for `(short)atoi(s)` and `itoa((int)value, str, base)`.
- **`signed long strtol(char *nptr, char **endptr, int base)`** / **`unsigned long strtoul(...)`**: skip whitespace, optional sign (`strtol`), base 0 (auto: `0x`→16, `0`→8, else 10) or 2–36, `0x`/`0X` prefix accepted for base 16, letters in either case as digits (`$41`–`$5A`, `$61`–`$7A`, `$C1`–`$DA`). The `0x` prefix counts only when a hex digit follows it (`"0x"` is 0, ending after the `0`). `*endptr` is set to `nptr` if no digits were read. A value out of range gives `LONG_MAX`/`LONG_MIN` (`strtol`) or `ULONG_MAX` (`strtoul`) and sets `errno = ERANGE`; `strtoul` negates a value after `-` (as C specifies); a base other than 0 or 2–36 gives 0 and `EINVAL`. Assembly (`stdlib_asm/strtoul.s45`; the multiply by the base on the hardware multiplier).
- **`float strtof(char *nptr, char **endptr)`** / **`float atof(char *nptr)`**: whitespace, sign, digits, fraction, `e`/`E` exponent (only when digits follow it). The digits are collected as an exact integer and scaled by exact powers of ten, so values such as `0.25` convert exactly. `*endptr` is `nptr` when there are no digits. A value beyond the float range gives ±`FLT_MAX`; a non-zero value that underflows gives 0; both set `errno = ERANGE`. `strtod(nptr, endptr)` is a macro for `(double)strtof(...)`.

**Arithmetic**
- **`abs(x)`**, **`labs(x)`**: macros (`(x)<0?-(x):(x)`). Define `__NOMACRO_ABS` / `__NOMACRO_LABS` before including to get the library functions `signed int abs(signed int)` / `signed long labs(signed long)` instead.
- `__builtin_abs`, `__builtin_labs`, `__builtin_ffs`, `__builtin_clz`, `__builtin_ctz`, `__builtin_popcount`, `__builtin_bswap16` are declared here (cc45 computes `__builtin_abs` / `__builtin_labs` inline, without a call); the library also provides the `l`/`ll` variants of the bit builtins, `__builtin_parity*` and `__builtin_bswap32`. `__builtin_trap()` (declared here; `stdlib_asm/__builtin_trap.s45`) jumps to `__abort`; `__builtin_unreachable()` is the compiler's (no code, no library symbol).

**Random numbers**
- **`int rand(void)`**: 0–`RAND_MAX` (32767). Until `srand` is called: the MEGA65 hardware RNG (`$D7EF`, waiting on `$D7FE` bit 7 before each of the two byte reads), different on every run. After `srand`: C's reference generator (`next = next * 1103515245 + 12345`, result bits 16–30), so `srand(1)` gives 16838 first.
- **`void srand(unsigned seed)`**: seeds the reproducible generator; the same seed repeats the same sequence (`lib/stdlib_asm/rand.s45`; the multiply by the hardware multiplier).
- **`void _randomize(void)`** (cc65's): `srand` with 32 bits of seed from the hardware generator.

**Heap** — `void *malloc(size_t)`, `void free(void *)`, `void *calloc(size_t, size_t)`, `void *realloc(void *, size_t)` (assembly, `lib/stdlib_asm/malloc.s45`):
- Sorted singly-linked free list, first fit, coalescing with both neighbours on `free`. Block header: 2-byte size (bit 0 = allocated) and, in free blocks, a 2-byte next pointer. Requests are rounded up to even, minimum 4 bytes including the 2-byte header.
- `malloc(0)` and failure return `NULL` (`errno` is not set). `free(NULL)` is a no-op. `calloc` = `malloc` + `memset`; `realloc` = `malloc` + `memcpy` + `free` (`realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` frees `p`).
- The heap is initialized on the first `malloc` call. It runs from `__heap_start` to `__heap_end`; both default to 0 (library member `__heap_bounds`), which means: start at `__bss_end` (after the program and its private base page), end `__stack_size` bytes below the 16-bit stack top (`$B000` with the default 4096 and `$BFFF` top; cc45 `-fstack-size=n` / `#pragma cc45 stack_size(n)`, ln45 `--stack-size=n`) or at `$C000` with the 8-bit page-1 stack. Define the two symbols in your program to place the heap elsewhere. When the program itself reaches past the heap end (a large program, BSS included), the heap is empty and `malloc` returns `NULL`; ln45 warns about it (`no heap: the program ends at ...`), and a smaller stack share gives such a program a heap.
- `realloc` keeps the old block when the new allocation fails. A size over `$FFFC` (no room for the block's header) and a `calloc` product `nmemb * size` over 64 KB return `NULL` (`test_malloc_xemu.c`).
- **`int posix_memalign(void **memptr, size_t alignment, size_t size)`** (`stdlib/posix_memalign.c`): `alignment` a power of two (it returns `EINVAL` otherwise, `ENOMEM` when there is no room, 0 when it worked; `errno` is not set); above 2 the block is allocated with room to spare and its first part, at least 4 bytes up to the aligned address, freed again, so `free` returns it all.
- cc65's heap functions (assembly, `stdlib_asm/heapinfo.s45`): **`_heapmemavail()`** (oscar64's **`heapfree()`**) the free bytes (the free blocks' sizes less their headers), **`_heapmaxavail()`** the largest `malloc` that can succeed now, **`_heapblocksize(p)`** a block's size (the request rounded up to even), **`_heapadd(mem, size)`** memory given to the heap (evened out, ignored under 4 bytes; it joins the free list, and neighbours, as a freed block). They set the heap up as the first `malloc` would.
- `#pragma cc45 heap` makes standalone (non-linked) builds call `_init_heap_crt` at startup; linked programs rely on the lazy initialization.

**Program termination**
- **`void exit(int status)`** (weak) / **`void _exit(int status)`**: jump to the CRT's `__exit`, which switches back to the ROM's base page, stores `status` at `$FD/$FE`, restores the caller's stack pointer and flags and returns to the caller (BASIC). `exit` and a return from `main` first run the `atexit` handlers; `_exit` and `abort` do not.
- **`EXIT_SUCCESS`** (0), **`EXIT_FAILURE`** (1).
- **`void abort(void)`** (weak): like `exit`, with status `$FFFE`; in a program that uses `signal`/`raise`, `signal.c`'s strong `abort` raises `SIGABRT` first. If `__exit` is never reached the status reads `$FFFF` (set at startup). A BASIC caller can read it with `PEEK(253)+256*PEEK(254)`.
- **`int atexit(void (*func)())`**: registers up to 32 handlers (returns 1 when full), called in reverse order, each once, by `exit` and after `main` returns (`__atexit_run`; crt0 has a weak empty one for programs that never call `atexit`).
- `#pragma cc45 exit_halt` / `exit_brk` / `exit_rts` only change the startup stub of standalone builds; a program linked with the library always returns to its caller.

**Environment and commands** (`stdlib/environ.c`)
- **`char *getenv(const char *name)`** / **`int putenv(char *string)`**: the environment starts empty; `putenv` adds a `"NAME=value"` string (the string itself, which the program keeps) to a `malloc`'d list, replacing one of the same name; a string without `=` removes the name (`EINVAL` for an empty one or one starting with `=`, `ENOMEM`).
- **`int system(const char *command)`**: no command processor: 0 for `NULL`, -1 for a command.
- **`void _swap(void *p, void *q, size_t size)`** (cc65's; assembly, `stdlib_asm/swap.s45`): the bytes exchanged.

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
| `char *strndup(const char *s, size_t n)` | C. At most `n` characters, NUL-terminated |
| `char *stpcpy(char *restrict d, const char *restrict s)` | *asm* (`strmisc.s45`). `strcpy` returning the end of `d` (its NUL) |
| `signed int strcasecmp(a, b)`, `signed int strncasecmp(a, b, n)`; `stricmp`, `strnicmp` the same | *asm* (`strcase.s45`). Letters fold as `tolower` does (`$C1`–`$DA` and `$61`–`$7A` to `$41`–`$5A`); the sign of the folded characters' difference |
| `char *strcasestr(const char *h, const char *n)` | *asm*. `strstr` ignoring case (needles up to 255 characters) |
| `char *strlwr(char *s)`, `char *strupr(char *s)`; `strlower`, `strupper` the same | *asm*. In place, as `tolower` / `toupper`; return `s` |
| `char *strqtok(char *s, const char *delim)` | C (cc65's). `strtok` where a token may be in double quotes: the text between them, delimiters included, without the quotes; a quote also ends an unquoted token; a quoted token without its closing quote runs to the end |
| `signed int strcoll(a, b)`, `size_t strxfrm(char *d, const char *s, size_t n)` | *asm* (`strmisc.s45`). The "C" locale: `strcoll` is `strcmp`; `strxfrm` copies `s` when it fits in `n` bytes with its NUL and returns its length |
| `void bzero(void *p, size_t n)` | *asm*. `memset(p, 0, n)` |
| `char *strerror(int errnum)` | C. "Success", "Result out of range", "Out of memory", "Invalid argument", "Math domain error", "Illegal seek" (`ESPIPE`), "No such file or directory" (`ENOENT`), "File exists" (`EEXIST`), "Permission denied" (`EACCES`), "Input/output error" (`EIO`), else "Unknown error" |
| `void *memcpy(void *restrict d, const void *restrict s, int n)` | *asm*. A DMA copy job (#325); no overlap handling |
| `void *memmove(void *d, const void *s, int n)` | *asm*. A DMA copy job, downwards from the last bytes when `d > s` |
| `void *memset(void *s, int c, int n)` | *asm*. A DMA fill job |
| `signed int memcmp(const void *a, const void *b, int n)` | *asm*. Returns <0, 0, >0 |
| `void *memchr(const void *s, int c, int n)` | C |

`memcpy`, `memmove` and `memset` fill in one DMA job (`stdlib_asm/dma_mem.s45`, the job and its start in `dma_job.s45`, shared with `dma_copy_far`) and start it with interrupts held off while it is filled in; the CPU waits for the DMA job, which takes much less time than the CPU loops for all but a few bytes. A length of 0 does nothing. They see bank 0 as the CPU does for I/O (the job's I/O flag: `$D000`-`$DFFF`, such as colour RAM at `$D800`, are the registers), and in a program with mapped code overlays (`--overlay-map`, `__far` functions) an address in the window is read where the overlay in it is kept (`__ovl_dma_fix` in `ovl_map.s45`; crt0's weak version does nothing). Under the ROMs (`$C000`-`$CFFF` and the KERNAL at `$E000` in C65 mode) the DMA sees RAM, not the ROM the CPU reads there. The direction of a downward copy is the F018B command's bits 4 and 5 (the F018A bank bit 6 is not used in that format). They return their first parameter (the CPU loops returned it 256 higher for each full page copied). Sizes: `memcpy` 16 bytes, `memmove` 28, `memset` 16, with the shared 95-byte job builder and 42 bytes of job and start (the CPU loops were 34, 128 and 36). Test: `test_dma_mem_xemu.c`; the `__noescape` overlay test copies from an overlay's constants, mapped.

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

`isascii(c)` is true for 0–127 and `toascii(c)` is `c & $7F` (also in the library: `stdlib_asm/isascii.s45`).

`toupper(c)` returns `c | $80` for `$41`–`$5A` (so `toupper('a') == 'A'`, `$C1`); `tolower(c)` returns `c - $20` for `$61`–`$7A` and `c & $7F` for `$C1`–`$DA`; other values are returned unchanged.

## 7. Math Library (`math.h`)

`math.h` includes `stdlib.h`.

**Integer**
- `div_t div(signed int, signed int)` and `ldiv_t ldiv(signed long, signed long)` (`quot`, `rem`; the quotient truncated toward zero), declared in `stdlib.h` as C specifies.
- `signed int min(a, b)`, `max(a, b)`, `gcd(a, b)` (Euclid, handles negative inputs), `lcm(a, b)` (`|a / gcd(a,b) * b|`).
- `abs`/`labs` as in `stdlib.h`.

**Floating point** (CBM 40-bit; `double` is the same type)
- `+ - * /` and comparisons: `float_rom.s45`'s `__float_add` … `__float_cmp` (the ROM's routines), or with cc45 `-ffast-float=1` / `-Ospeed` (basic45 `--compile --speed`) `stdlib_asm/float_fast.s45`'s `__ffast_add` … `__ffast_cmp`; a comparison's value 0/1 (A; X = 0): `__float_eq`, `_ne`, `_lt`, `_le`, `_gt`, `_ge` (`__ffast_eq` …), each `cmp` and its test: in RAM, about twice as fast, the results of the ROM with its rounding fixed (correctly rounded products and quotients; the hardware multiplier and divider), see cc45's [`-ffast-float=N`](../bin/cc45.md) (`-ffast-float` alone is the earlier spelling of `=1`; the functions — `sinf`, `logf`, … — use the ROM at either level).
- ROM-backed (BASIC 65 routines through `float_rom.s45`, which switches to base page 0 for each call): `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf`.
- How the ROM is reached (#614): `__float_romcall` (`stdlib_asm/float_map.s45`, 65 bytes — 57 of code, 8 for the saved map and registers — in the `init` segment, so below `$4000`) maps the BASIC ROM at `$4000`-`$7FFF` itself, calls the routine and puts the map it found back (a mapped overlay's or a `__far` function's window too), keeping the KERNAL's current map at `$011C` in step. About 110 cycles a call against 260 through the KERNAL's `JSRFAR`: ROM `+ - * /` about 15% faster, the functions about 5%. There is no option for the old path. While a ROM float routine runs the program's memory at `$4000`-`$7FFF` is not visible (`JSRFAR` hid `$2000`-`$7FFF`): a CPU stack moved there (`-fstack-top`) or an `__interrupt` handler placed there would fail, as before. A ROM error (overflow, division by zero, illegal quantity) would leave through `$2FEC`, the program's memory now and not BASIC's handler; the library never lets the ROM reach it (`float_rom.s45` scales and saturates, `math.c` and compiled BASIC's run time check the arguments first). Checked against ROM 920413: all the float routines and their tables lie in `$4000`-`$7FFF`. `__jsrfar_basic` (`JSRFAR`) remains for compiled BASIC's own ROM calls (`basic_rt.s45`, `basic_usr.s45`).
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
- **stdin** (`fgetc(stdin)`, `getc(stdin)`, `fgets`, `scanf`, `gets`) reads a typed line (`cbm_read` of handle 0, `stdio_cbm.c`): keys by `getchar`, echoed by `putchar`, DEL takes one back, RETURN ends it (`'\n'`, `$0D`, included); other control keys are dropped. (Before, handle 0 could not be read: stdin gave EOF at once.)
- **`char *gets(char *s)`** / **`char *gets_s(char *s, size_t n)`** (`stdlib/gets.c`): a line from stdin without its `'\n'`; `gets` has no bound (C11 dropped it; cc65 and oscar64 keep it); `gets_s` is C11's: at most `n - 1` characters, a longer line read to its end and `""` and `NULL` returned, as for `s` NULL or `n` 0. NULL at EOF with nothing read.
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

Functions: `fopen`, `freopen`, `fclose`, `fflush`, `fgetc`, `fputc`, `getc`, `putc` (macros for `fgetc` / `fputc`, and functions too, `stdlib/putc_getc.c`: linked for their address or after `#undef`; #750), `fgets`, `fputs`, `ungetc`, `fread`, `fwrite`, `fprintf`, `vfprintf`, `fscanf`, `vfscanf`, `scanf`, `vscanf`, `fseek`, `ftell`, `rewind`, `fgetpos`, `fsetpos`, `feof`, `ferror`, `clearerr`, `setvbuf`, `setbuf`, `tmpfile`, `tmpnam`, `perror` (prints `s: strerror(errno)` to `stderr`). `fileno(stream)` is a macro (`stream->handle`) and `fdopen(fd, mode)` a cast.

Behaviour on the MEGA65:
- `fopen` opens files on disk device 8. The DOS takes the type and direction from the name, so it opens `NAME,S,R` (`r`), `@0:NAME,S,W` (`w`; the `@` replaces an existing file, as C's `w` truncates) or `NAME,S,A` (`a`); `b` makes it a USR file (`U`). A name that already contains a `,` is used as given. Each file gets its own drive channel (secondary address = its logical file number). `OPEN` succeeds even for a missing file, so `fopen` then reads the drive's status (`cbm_drive_status`) and fails on an error (≥ 20, e.g. 62 FILE NOT FOUND). Tested on xemu with a D81 (`test_fileio_xemu.c`).
- **`r+`, `w+`, `a+`** (CBM drives don't read and write a sequential file at once): the stream is held in memory. `fopen` reads the whole file into the heap (`r+` fails for a missing file; `w+` creates the file empty at once; `a+` creates it when written back); reads, writes and `fseek` work on that copy, and `fflush`/`fclose` write it back with `@0:` replacement. A write past the end after a seek fills the gap with zeros; `a+` always writes at the end. The file must fit in the heap (at most 65280 bytes).
- **`fseek`/`ftell`/`rewind`/`fgetpos`/`fsetpos`** (in `stdio_seek.c`, linked only when used): a memory stream seeks anywhere. A stream opened with `r` seeks forward by reading and backward by reopening the file and reading to the position; `SEEK_END` reads to the end first. A stream opened with `w` or `a` (written sequentially) can only seek to where it is: anything else fails with `errno = ESPIPE`. `ungetc` then `fgetc` keeps `ftell` right. `test_fileseek_xemu.c` (both conventions) covers all of it.
- `fgets`/`fputs` are in `stdio_line.c`, apart from `fprintf`/`fscanf`, so line I/O does not link the printf machinery (also before ln45 dropped unused functions).
- **`setvbuf(stream, buf, mode, size)`** before the stream's first read or write: `_IONBF` passes every character on at once (a 1-byte buffer), `_IOFBF`/`_IOLBF` buffer `size` bytes in `buf` (or a `malloc`'d buffer when `buf` is NULL); a text stream is written out at each newline in either mode. It returns −1 after I/O, for an unknown mode or without memory; a memory stream ignores it (0). `setbuf(stream, buf)` is `setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ)`.
- **`remove(name)`** / **`rename(old, new)`** (`stdio_remove.c`, #763): `cbm_delete` / `cbm_rename` on device 8, the
  drive `fopen` uses; 0, else -1 and `errno`: `ENOENT` (no such file; for `remove`, nothing scratched), `EEXIST`
  (`rename` to a name that exists), `EACCES` (26 WRITE PROTECT ON), `EINVAL` (no name, or one too long for the
  command), `EIO` (another drive error, no drive).
- **`tmpfile()`**: a read/write binary stream held in memory (the heap limits its size), never written to a disk, so nothing is left to remove after `fclose`.
- **`tmpnam(s)`**: `"TMP0000"` to `"TMP9999"`, the next one no file on device 8 has (checked by opening it), in `s` or a static buffer; NULL when all are taken.
- `test_stdio3_xemu.c` covers the scanf family on files, `setvbuf`, `setbuf`, `tmpfile`, `tmpnam` and `freopen`.

```c
#include <stdio.h>
#include <string.h>

int main(void) {
    char line[40];
    FILE *f = fopen("scores", "w");              /* @0:SCORES,S,W on drive 8 */
    if (!f) return 1;
    fprintf(f, "ada %d\n", 1200);
    fputs("bob 900\n", f);
    fclose(f);

    f = fopen("scores", "r");
    if (!f) return 2;                            /* e.g. no disk: the drive's error */
    if (!fgets(line, sizeof line, f) || strcmp(line, "ada 1200\n") != 0) return 3;
    fclose(f);

    f = fopen("scores", "a");                    /* appended to */
    fputs("cy 50\n", f);
    fclose(f);
    return 0;
}
```

### CBM file API

Declared in `stdio.h`, implemented in `stdio_cbm.c`, `stdio_kernal.c`, `stdio_directory.c`, `stdio_rel.c`:

- Files: `fd_t cbm_open(device, filename, type)` (secondary address 0), `cbm_open_ext(device, filename, type, secondary)` (`$FF`: the logical file number), `cbm_close`, `cbm_read`, `cbm_write` (these return `signed int`, −1 on error; `fd_t` is `signed int`), `cbm_drive_status(device)` (the number of the drive's status message: 0 OK, ≥ 20 an error, −1 no channel), `cbm_eof(fd)` (1 once a read reached the file's end), `cbm_status(fd)` (the KERNAL status byte), `cbm_tell(fd)` (bytes read or written) and `cbm_seek(fd, position)` (a file being read: forward by reading, back by reopening it with the same device, secondary address and name (up to 23 characters) and reading; −1 past the end or for a file that was written, unless it stays where it is). Logical file numbers 3–14 are allocated as free ones (15 is the command channel, which stays open while files are open: closing it makes the drive close all its files). `cbm_read` keeps the byte that arrives with the end-of-file status. Constants `CBM_DEVICE_*`, `CBM_T_DEL/SEQ/PRG/USR/REL/CBM/DIR` (`$80`–`$86`: a closed file's directory type byte), `O_*`, `STDIN_FILENO`…
- Directory and disk commands: `cbm_opendir(device)` (opens `$` and skips the load address and the header line), `cbm_readdir(handle, name, &type, &blocks)` (the next entry of the listing the drive sends, a BASIC program: the name between the quotes, the size from the line number, the type byte rebuilt from the type text — the type code, `CBM_FILE_CLOSED` unless marked `*`, `CBM_FILE_LOCKED` for `<`; 1 at `BLOCKS FREE.`), `cbm_closedir`, `cbm_dir_type_name`, `cbm_dir_get_type`, `cbm_dir_is_locked`, `cbm_dir_is_scratched`, `cbm_dir_is_closed`, `cbm_format(device, name, id)` (`N0:name,id`; an empty id: the quick `N0:name`), `cbm_delete(device, name)` (`S0:name`, wildcards as the DOS has them; 0 only when a file was scratched: the DOS answers `01, FILES SCRATCHED,00,00` for none), `cbm_rename(device, old, new)` (`R0:new=0:old`; a name that exists is refused here, with the DOS's 63: the MEGA65's DOS would make a second file of the name) — DOS commands on the command channel that `cbm_drive_status` keeps open (`cbm_fileops.c`, `cbm_dos_cmd.c`: the command letters as PETSCII codes, as BASIC 65 sends them; the drive's last answer in `__cbm_dos_msg`; #762); directory type flag bits `CBM_FILE_*`.
- Relative (REL) files (`stdio_rel.c`, #783), through the drive as CBM DOS has them: `cbm_rel_open(device, filename, record_size, mode)` opens `0:name,L,<size>` on a channel of its own (created when there is none; an existing file's record size must be given; up to 8 at once), and finds the record count; before each record `P` + the channel + the record number goes on the command channel. `cbm_rel_read` (the record up to its last non-zero byte, the rest of the buffer cleared; 0 for an empty record and past the end), `cbm_rel_write` (any record: past the end the file grows to it, the records between empty), `cbm_rel_position`, `cbm_rel_tell`, `cbm_rel_size` (up to the last record not empty: the empty ones the DOS creates a block at a time after it do not count), `cbm_rel_close`, `cbm_rel_record_size`, `cbm_rel_is_open`, `cbm_rel_append`, `cbm_rel_delete` (empties the record: the DOS's `$FF` alone, `CBM_REL_DELETED`; a REL file cannot lose records in the middle), `cbm_rel_update` (an existing record), `cbm_rel_truncate(handle, n)` (#764: the first n records copied into a new REL file, `RELTRUNC.TMP`, the old file scratched and the copy renamed to its name and reopened on the handle — it needs room on the disk for the copy; more than the file has: it grows), `cbm_rel_lasterror`, `cbm_rel_strerror`; `CBM_REL_READ`/`WRITE` (the drive opens a REL file for both), `CBM_REL_ERR_*` (from the drive's status: 62 not found, 26 write protected, 52/72 full, 50/51/64 corrupt). A record holding the single byte `$FF` reads as empty: that is how the DOS marks one. A handle that was never opened is not open (`CBM_REL_ERR_INVALID`), also before the first `cbm_rel_open`. Tests: `test_relfile_xemu.c`, `test_dosfile_xemu.c` (scratch D81s).
- KERNAL wrappers: `cbm_k_setnam(filename)`, `cbm_k_setlfs`, `cbm_k_open`, `cbm_k_close`, `cbm_k_chkin`, `cbm_k_chkout`, `cbm_k_clrchn`, `cbm_k_chrin`, `cbm_k_chrout`, `cbm_k_readst`. `test_cbmio_xemu.c` covers the file, directory and KERNAL functions on a D81.

`cbm_open` and `cbm_close` here have different signatures from the `static` `cbm_open(void)`/`cbm_close(char)` in `cbm.h`; in a file that includes both headers the `cbm.h` versions are used without a diagnostic.

## 9. Error Handling (`errno.h`)

- **`extern int errno`**: set by the float functions, `strtof`, `strtol` and `strtoul` (see [§7](#7-math-library-mathh) and [§4](#4-general-utilities-stdlibh)) and by the functions of the table below. The library never clears it; `malloc` does not set it.
- **`extern unsigned char _oserror`** (cc65's): the KERNAL's error of the last `cbm_load` / `cbm_save` / `cbm_k_load` (0: none; [stdcbm.md](stdcbm.md)).
- **`extern char _errnoc`**: the KERNAL status byte `ST` at `$0090` of the ROM's page 0 (read-only alias).

| Constant | Value | Used for |
|----------|-------|----------|
| `ERANGE` | 1 | result out of range (`strtol`, `strtoul`, `strtof`, `logf(0)`, `expf`, `powf`, `ldexpf`) |
| `ENOMEM` | 2 | no memory: `putenv`, the FILE streams (`stdio_file.c`: a stream's or a memory stream's storage) |
| `EINVAL` | 3 | an argument that is not allowed: a base of `strtol`/`strtoul`, `putenv`'s string, `signal`'s signal, a clock or value of `clock_gettime`/`clock_getres`/`clock_settime`, the FILE streams (`stdio_file.c`, `stdio_seek.c`) |
| `EDOM` | 4 | argument without a value (`sqrtf`/`logf` of a negative, `powf` negative^fraction, `fmodf(x, 0)`) |
| `ESPIPE` | 5 | a seek a stream written sequentially cannot do (`fseek`, [§8](#file-streams)) |
| `ENOENT` | 6 | no such file (`remove`, `rename`: the drive's 62, or nothing scratched) |
| `EEXIST` | 7 | `rename` to a name that exists (63) |
| `EACCES` | 8 | the disk is write protected (26) |
| `EIO` | 9 | another drive error, or no drive (`remove`, `rename`) |

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
- **`size_t strftime(char *s, size_t max, const char *format, const struct tm *tm)`** (`stdlib/strftime.c`): C99's conversions `%a %A %b %B %c %C %d %D %e %F %g %G %h %H %I %j %m %M %n %p %r %R %S %t %T %u %U %V %w %W %x %X %y %Y %z %Z %%` in the "C" locale (`%c` is `"%a %b %e %H:%M:%S %Y"`, `%x` `"%m/%d/%y"`); the `E` and `O` modifiers are skipped; `%z` is `+0000` and `%Z` empty (no time zones); `%G %g %V` are the ISO 8601 week. Returns the characters written without the NUL, or 0 (and an empty string) when they do not fit in `max`.
- **`int clock_gettime(clockid_t, struct timespec *)`**, **`clock_getres`**, **`clock_settime`** (`stdlib/clock_time.c`): `CLOCK_REALTIME` is the RTC (`time()`, a resolution of a second, `tv_nsec` 0), `CLOCK_MONOTONIC` the frame counter (`clock()`; a frame: 20 ms PAL, 16 666 666 ns NTSC). `clock_settime(CLOCK_REALTIME, tp)` writes the RTC's registers (BCD, by DMA; years 2000–2099; the I2C controller passes them to the clock chip; xemu keeps the host's time); another clock or an out-of-range value: -1, `errno` `EINVAL`. **`tzset()`** does nothing.

## 11a. Keyboard and Joysticks (`input.h`)

Where keys arrive on the MEGA65: the keyboard's hardware scanner puts each key press into a queue at `$D610` (ASCII codes; write to `$D610` to take one off). The C65 KERNAL does **not** copy typed keys into its own buffer (`$02B0`, count `$00D0`); its `GETIN` (`getchar`) reads `$D610` when the buffer is empty, and expands programmed function keys into strings. Held keys are read from the matrix (`$D614`/`$D613`, `key_pressed`), the modifiers from `$D611`.

- **`unsigned char input_key(void)`**: the next key, or 0 when none waits (never waits). Letters as cc45 writes them (`'a'`; with SHIFT `'A'`: the queue's ASCII case swapped), RETURN/DEL/HOME/cursor keys as PETSCII (`CH_RETURN`, `CH_DELETE`, `CH_HOME`, `CH_CURSOR_UP`… from `cbm.h`), RUN/STOP `CH_STOP` (3), TAB 9, ESC 27, F1–F14 `CH_F1`… (`$F1`–`$FE`), `£` `$5C`, `↑` `$5E`. Keys a program or BASIC put into the KERNAL's buffer come first (through `GETIN`).
- **`input_wait`** (waits), **`input_waiting`**, **`input_flush`**; **`input_pressed(KEY_x)`** (held now; `<mega65.h>`'s matrix codes, several at once, SHIFT/CTRL/RUN/STOP too); **`input_modifiers`** (`$D611`: bit 0 right SHIFT, 1 left SHIFT, 2 CTRL, 3 MEGA, 4 ALT, 5 NO SCROLL, 6 CAPS LOCK; xemu reports left SHIFT in bit 0).
- **`unsigned char input_joy(port)`** (1 or 2): `JOY_UP`/`DOWN`/`LEFT`/`RIGHT`/`FIRE`, read with no keyboard row selected (`$DC00` = `$FF`, interrupts held off) so held keys don't show. Port 1 is CIA 1 port B (`$DC01`), port 2 port A (`$DC00`). The extra buttons are on the paddle lines (`$D620`/`$D621` port 1, `$D622`/`$D623` port 2) and are read once `input_joy_type(port, type)` says what is connected: `JOYTYPE_1BUTTON` (the default: a mouse or paddles use the same lines), `JOYTYPE_C64GS` (3-button sticks: a button ties its line to +5 V, `JOY_FIRE2` POT X, `JOY_FIRE3` POT Y), `JOYTYPE_SEGA` (Genesis pads: B is `JOY_FIRE`, C `JOY_FIRE2`, pin 9 pulled to ground). A Sega pad's select line is pin 7, +5 V on a Commodore port, so A, START and a 6-button pad's X/Y/Z can't be read, and the pad needs power on pin 5 (an adapter). xemu does not emulate the paddle lines (they read 0); the extra buttons are untested on hardware.
- **`joyport_device_t input_joyport_detect(port)`** (#645, `stdlib/input_joyport.c`): what is on port 1 or 2, from its lines now: `JOYPORT_EMPTY` (nothing pressed and no paddle line in use: a joystick at rest reads the same), `JOYPORT_JOYSTICK` (a direction or fire pressed), `JOYPORT_MOUSE_1351` (both paddle lines below `$80`: a 1351 gives its position modulo 64, times 2), `JOYPORT_MOUSE_AMIGA` (the same with the Amiga mouse support on for the port), `JOYPORT_PADDLE` (the paddle lines in use otherwise), `JOYPORT_UNKNOWN` (another port). **`input_set_mouse_mode(port, mode)`**: the MEGA65's Amiga mouse support (`$D61B`, write only, the value kept by the library): `INPUT_MOUSE_1351` (off; a 1351, the hardware's de-jitter on), `INPUT_MOUSE_AMIGA` (an Amiga mouse, read as a 1351), `INPUT_MOUSE_AUTO` (on, the mouse detected); 1, 0 for another port or mode. **`input_joyport_name(device)`**: "empty", "joystick", "1351 mouse", "Amiga mouse", "paddles", "unknown". xemu reads the paddle lines as `$FF` unless its mouse is grabbed. Test: `test_hw_peripherals_xemu.c`
- `conio.h`'s `kbhit`/`cgetc` use these (`input_wait`/`input_waiting` are `__fastcall__`: conio's assembly calls them in either library). Test: `test_input_xemu.c` presses keys through the virtual key registers (`$D615`–`$D617`: a C65 scan code, row × 8 + column, holds the key; `$7F` releases it), the same path as real typing.

## 12. Other Library-Backed Headers

| Header | Contents |
|--------|----------|
| `conio.h` | cc65's conio, all of it: `clrscr`, `kbhit`, `gotox`, `gotoy`, `gotoxy`, `wherex`, `wherey`, `cputc`, `cputcxy`, `cputs`, `cputsxy`, `cprintf`, `vcprintf`, `cgetc`, `cgets`, `cscanf`, `vcscanf`, `cpeekc`, `cpeekcolor`, `cpeekrevers`, `cpeeks`, `cursor`, `revers`, `textcolor`, `bgcolor`, `bordercolor` (the three return the old colour), `chline`, `chlinexy`, `cvline`, `cvlinexy`, `cclear`, `cclearxy`, `screensize`, `cputhex8`, `cputhex16`, and the line characters `CH_HLINE`, `CH_VLINE`, `CH_ULCORNER`… (PETSCII, as cc65's `<cbm.h>`); besides them colour constants, `textattr`, `cputch` (at (x, y), the cursor staying), `cprintfxy`, `getch_xy`, `getattr_xy`, `screenwidth`, `screenheight`, and the earlier names, entries of the cc65 functions: `putch` (`cputc`, returning its character), `getch` (`cgetc`), `textbackground` (`bgcolor`), `cursoron`/`cursoroff` (`cursor(1)`/`cursor(0)`). In assembly (`stdlib_asm/conio.s45`, a section per function: a program links what it calls) except the variadic `cprintf`, `cprintfxy` and `cscanf` (`stdlib/conio_printf.c`), which pass their arguments to `vcprintf` / `vcscanf`. `vcprintf` runs `__vformat` with a put routine writing at the cursor (no length limit); `%f`/`%e`/`%g` through `__conio_ffmt`, a weak null pointer that `printf_float.c` (linked when a float is passed to a variadic function) replaces with `__pf_format`. `cgets` is cc65's: echoed with the cursor shown, up to size − 1 characters or RETURN (echoed as a new row, not stored), DEL or `'\b'` takes the last character back (up a row at column 0); it returns NULL when nothing was typed. `cscanf`/`vcscanf` read a line as `cgets` does (80 characters at most) and convert it with the scanf engine (`__vscanf_core`, a string): what the format leaves is dropped, and a format needing more than the line gets fewer items (cc65 reads on). cc45 links the `%f` scanf for `cscanf` as for `scanf`, and checks `vcprintf`'s format as `vprintf`'s. `cpeekc`/`cpeeks` turn screen codes back into PETSCII as `cputc` writes it (`'A'` `$C1`); `cpeeks` goes on into the next row and stops at the bottom; `cvline` stops at the bottom row. It works on the screen the VIC-IV shows, read on every call (screen RAM `$D060`–`$D063`, row stride `$D058`, columns `$D05E`, rows `$D07B`+1, colour RAM offset `$D064`): 80x25 in MEGA65 mode, also 40 columns, 50 rows or a moved screen, above 64 KB too. A row's screen and colour RAM addresses are 32-bit pointers in `__zp_scratch` (the hardware multiplier gives row × stride), and characters and colours are stored through them with flat addressing (`sta [$08],z`): about 16 times faster than the earlier C version, which did a long multiply and a DMA job per character; clearing and scrolling are DMA fills and copies. Text is PETSCII converted to screen codes; `'\n'` (`$0D`) starts a new row, `'\r'` (`$0A`) goes to column 0 (cc65's `"\r\n"` is one new row; cc65's `'\n'` keeps the column, cc45's does not), other control characters are not shown; output wraps and scrolls. The colour is colour RAM's low nibble (`$FF80000`); text mode has no per-character background, so `bgcolor` (and `textattr`'s high nibble) sets the screen's (`$D021`). `revers(1)` sets bit 7 of the screen codes. `cgetc` waits for a key through `<input.h>` (`input_wait`: PETSCII letters, `CH_STOP`, `CH_F1`…, no echo; with `cursor(1)` the cursor cell shows reversed while it waits); `kbhit` is `input_waiting` (both `__fastcall__`, so stack-mode programs need no thunks). `getch_xy` returns the screen code, `getattr_xy` the colour RAM byte. Tested by `test_conio_xemu.c` (also a 40-column stride and an 80x50 screen in bank 4) and `test_conio_cc65_xemu.c` (the cc65 functions, `cgets`/`cscanf` from keys in the KERNAL's buffer); `examples/c/keyboard_viewer` tests the keyboard interactively. With CHR16 (`$D054` bit 0, two bytes a cell: a character number and a colour RAM pair, as the full-colour mode sets up) every function works on the two-byte cells: DMA fills write every other byte (F018B option `$85`, destination step 2); the cursor of `getch` is not shown on a full-colour glyph. **Full-colour mode: a background colour for each character** (assembly, `stdlib_asm/conio_fcm.s45`; the variadic `fcm_cprintf` in `stdlib/conio_fcm_printf.c`): the VIC-IV's text mode has one background (`$D021`); its full-colour characters (FCLRHI, `$D054` bit 2: character numbers from 256 are 64 bytes, a byte a pixel, at number × 64; pixel `$00` shows the screen's background, `$FF` the cell's colour from colour RAM, others that palette entry) give each cell its own. `fcm_enable` moves the screen to two bytes a cell in bank 4 or 5 (`far_malloc`, room for 80 columns: 8000 bytes at 80x50) with the cells' colours in colour RAM from `$1000` (above the ROM's screen, which keeps receiving stdio output through the ROM), keeping what is on the screen. A cell on the screen's background shows the font's character (CHARPTR; the VIC-II's `$1000`/`$1800`/`$9000`/`$9800` are the character ROM at `$FF7E000`); one on another background a glyph made from the font's 8 bytes (set pixels `$FF`, clear ones the background; black uses palette entry `$FE`, set to black), kept in a cache after the screen: 256 glyphs (16 KB) by default, `fcm_set_cache_size(n)` before `fcm_enable`. The same character on the same background reuses its glyph; when the cache is full, the glyphs no cell shows are dropped (a scan of the screen), and when all are on the screen a new cell is drawn on the screen's background. `fcm_putch`, `fcm_cputs` (wrapping; `'\n'` starts the next row), `fcm_cprintf`, `fcm_fill_rect`, `fcm_set_line_color` take PETSCII characters and colours 0–15; `getch_xy` gives a glyph cell's character (through `__conio_chtab`), `fcm_getattr_xy` returns `(bg << 4) \| fg`; `fcm_is_enabled()` is 1 while the mode is on. `fcm_putglyph(x, y, n, fg)` shows a program's own full-colour character `n` (≥ 256, its 64 bytes at `n` × 64; `fcm_disable` leaves a space there; `examples/c/fcm_graphics`). `fcm_disable` puts back the one-byte screen with what is on it (glyph cells as their characters) and frees the memory. About 2.7 KB of code and 1 KB of BSS, linked only when used (the C version: 4.7 KB and 1.5 KB). The cells are written with `conio.s45`'s `__conio_put16`, the rows and glyphs moved by DMA jobs of 28-bit addresses. `conio_set_width(40/80)` (in `conio.s45`) writes H640 (`$D031` bit 7), which resets the VIC-IV's layout registers, then puts back the screen's and colour RAM's addresses and sets the characters per row and the stride (twice the width with CHR16) and clears the screen; the ROM's screen editor is not told. `conio_get_width` is `screenwidth`. Test: `test_conio_fcm_xemu.c` (the glyphs' pixels, the cache filling up, the screen kept across enable and disable, both widths). |
| `inttypes.h` | C99's `PRI`/`SCN` format macros for the `<stdint.h>` types (the 8-bit ones print as `int` and scan with `hh`; `intmax_t` is `long`: `l`; the 64-bit ones `ll`, of which `printf` prints the low 32 bits), `imaxdiv_t`, `imaxabs`, `imaxdiv`, `strtoimax`, `strtoumax` (`stdlib/inttypes.c`: `strtol`/`strtoul`) |
| `signal.h` | `signal`, `raise` (`stdlib/signal.c`): `SIGABRT`, `SIGFPE`, `SIGILL`, `SIGINT`, `SIGSEGV`, `SIGTERM`, `SIGUSR1`, `SIGUSR2`; nothing raises them but `raise` and `abort`. `raise` puts `SIG_DFL` back and calls the handler; `SIG_IGN` does nothing; `SIG_DFL` ends the program (`SIGABRT` as `abort`, status `$FFFE`; the others `_exit(EXIT_FAILURE)`). `signal` returns the old handler, or `SIG_ERR` with `errno` `EINVAL` for an unknown signal. `signal.c` has a strong `abort` that raises `SIGABRT` first |
| `locale.h` | The "C" locale (`stdlib/locale.c`): `setlocale` returns `"C"` for `NULL`, `""`, `"C"` and `"POSIX"`, else `NULL`; `localeconv` gives `"."` and `CHAR_MAX` (255) for the rest |
| `zlib.h` | **`unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len)`** (as cc65's and zlib's; assembly, `stdlib_asm/crc32.s45`): CRC-32 (polynomial `$EDB88320`, reflected), continued from `crc` (0 to start; `crc32(0, NULL, 0)` is 0). A 16-entry table (64 bytes) two steps a byte. **`unsigned long adler32(unsigned long adler, const unsigned char *buf, unsigned len)`** (assembly, `stdlib_asm/adler32.s45`): Adler-32, continued from `adler` (1 to start; `adler32(0, NULL, 0)` is 1); a sum over 65520 less 65521 is the sum plus 15 in 16 bits. cc65's `inflatemem`/`uncompress` are not there (pack45 and `unpack.h` are the library's compression) |
| `joystick.h` | cc65's joystick API (assembly, `stdlib_asm/joystick.s45`): the driver built in (`joy_install(joy_static_stddrv)`, `joy_load_driver(any name)`: `JOY_ERR_OK`; `joy_uninstall()` / `joy_unload()` too: `joy_read` reads the port either way), `joy_count()` 2, `joy_read(JOY_1 / JOY_2)` is `input_joy(1 / 2)` (`__fastcall__`): `JOY_UP_MASK` … `JOY_BTN_1_MASK` (fire, `$10`), `JOY_BTN_2_MASK`/`JOY_BTN_3_MASK` the paddle-line buttons; the `JOY_UP(v)` … `JOY_FIRE(v)` macros test them, as cc65's (the header redefines `input.h`'s `JOY_UP` … masks as those macros: use the `_MASK` names with both). The earlier `joystick.h` declared an unimplemented `sprite_joystick_*` API |
| `unistd.h` | **`unsigned int sleep(unsigned int seconds)`** (assembly, `stdlib_asm/sleep.s45`): `seconds * CLOCKS_PER_SEC` frames of `clock()` (the product by the hardware multiplier); 0. **`int getopt(int argc, char *const argv[], const char *optstring)`** (`stdlib/getopt.c`), POSIX's: letters grouped in a word, an argument attached or the next word (`x:`), `-` alone, a non-option or `--` ends them (`--` taken); `'?'` with `optopt` for an unknown letter or a missing argument (`':'` for the latter with a leading `':'`), a message on `stderr` unless `opterr` is 0 or `optstring` starts with `':'`; `optarg`, `optind` (set it to 1 to start again) |
| `serial.h` | cc65's serial API (assembly, `stdlib_asm/serial.s45`) on the MEGA65's buffered UART (`$D0E0`–`$D0E6`: `$D0E1` status, bit 3 transmit buffer full, bit 6 receive buffer empty; `$D0E2` the received byte, a write takes it off; `$D0E3` the byte to send; `$D0E4`–`$D0E6` the divisor, 80 MHz / (divisor + 1); xemu connects it to a TCP socket with `-serialtcp HOST:PORT`). The driver is built in: `ser_install(ser_static_stddrv)` or `ser_load_driver(any name)` installs it (`SER_ERR_INSTALLED` twice), `ser_uninstall`/`ser_unload` remove it. `ser_open` takes every `SER_BAUD_` rate and only 8N1 without handshake (else `SER_ERR_INIT_FAILED`; `SER_ERR_BAUD_UNAVAIL` for an unknown rate) and empties the receive buffer; `ser_get` gives `SER_ERR_NO_DATA` when nothing came, `ser_put` `SER_ERR_OVERFLOW` while the transmit buffer is full (both `SER_ERR_NOT_OPEN` before `ser_open`); `ser_close` empties the receive buffer and leaves the driver installed (`SER_ERR_NOT_OPEN` when the port is not open); `ser_status` 0; `ser_ioctl(SER_IOCTL_DIVISOR, &divisor)` sets any other rate (an `unsigned long`). Test: `test_serial_xemu.c` (an echo over `-serialtcp`, `src/test/serial_echo.py`) |
| `far_ptr.h` | 24-bit far pointers (`far_ptr_t`: a bank byte and a 16-bit offset; `bank_t`; `static inline` in the header: `far_make(bank, offset)`, `far_bank`, `far_offset`, `far_is_null`, `far_to_linear(p)` / `far_of_linear(a)` to and from a 28-bit address, a bank from `$80` attic RAM), bank selection (`bank_set`, `bank_get`, `bank_available`, `bank_reset`, `bank_reset_all`) and a far heap (`far_malloc`, `far_calloc`, `far_realloc`, `far_free`, `far_malloc_current`, `far_calloc_current`, `far_size`, `far_alloc_bank`, `far_is_valid`, `far_policy_*`, `far_reserve`; attic RAM: `far_attic_init`, `far_attic_enable`, `far_attic_hook`, `far_attic_available`, `far_attic_malloc`, `far_attic_free`, `far_attic_size`, `far_malloc_long`, `far_size_long`). The chip RAM heap (`far_malloc`, `far_free`, `far_size_long`, `far_reserve`, `bank_reset`, `bank_reset_all`) is assembly, `stdlib_asm/far_heap.s45` (`__fastcall__` in both libraries): a bank's heap ($0100-$FFFF of bank 4 or 5) is up to 32 records in address order (offset, size, a state: allocated, free, kept), arrays of bytes for the two banks reached with X/Y (412 bytes of BSS); first fit, a bigger block split (the rest a free record after it; no record left: refused), `far_free` merging free neighbours; `far_reserve`'s ranges (up to 8) and the code overlays' store carved out at a bank's first use. Attic RAM goes through `far_attic_hook.c`'s routes (linked with `far_attic_hook` / `far_attic_enable`; `far_heap.s45`'s weak ones give nothing), which call `far_attic.c` through the hooks' pointers; the rest of the API is `stdlib/far_malloc.c`. About 1.1 KB of code for `far_malloc` + `far_free` (the C version: 3.9 KB, and 1.9 KB of BSS). Test: `test_far_malloc_xemu.c` (splits, merges, the record limit, `far_reserve`, `bank_reset`, the C part, 1500 random operations against a model of the algorithm). |
| `far_ops.h` | far element access (`far_read_*`/`far_write_*` for 8/16/32-bit, float, double), arrays (`far_array_*`), bulk copies (`far_memcpy*`, `far_memset`, `far_memzero`, `far_strcpy_*`, `far_strlen`, `far_strcmp`) and pointer arithmetic/comparison (`far_add`, `far_sub`, `far_inc`, `far_dec`, `far_diff`, `far_eq`, `far_lt`, `far_gt`, `far_cmp`): each function in [§12c](#12c-far-memory-far_opsh-far_stringh) |
| `far_string.h` | `memcpy`/`memset`/`strlen`/`strcpy`/`strcmp`/`strcat`/`memcmp` for every mix of local and far pointers (`memcpy__fpvst` …) and typed far array elements; included by `far_ops.h` ([§12c](#12c-far-memory-far_opsh-far_stringh)) |
| `dma_controller.h` | F018B DMA request API: `dma_execute`, `dma_transfer_smart`, `dma_fill`, `dma_available`, `dma_is_busy`, `dma_wait`, `dma_enable`, `dma_disable`, `dma_set_interrupt_enable`, `dma_get_status`, `dma_print_status`, `dma_speed_bytes_per_us`. Its `dma_fill` collides with the `dma_fill` macro of `dma.h`. |
| `audio_sid.h` | The four SID chips as 12 voices (voice v: SID v/3, voice v%3): `sid_init`, `sid_note_on`/`sid_note_off`/`sid_play` (MIDI notes), `sid_set_freq` (Hz) / `sid_set_freq_reg` / `sid_set_note`, `sid_set_waveform` (the gate kept apart), `sid_set_pulse`, `sid_set_adsr`, `sid_set_instrument` (`sid_instrument_t`: waveform, AD, SR, pulse width), `sid_gate`, `sid_all_off`, `sid_set_volume`, `sid_set_filter`, `sid_write`/`sid_write_chip`. Every write is kept in a shadow (`sid_get_reg`; the chips are write-only). `sid_set_mirror(1)` repeats voices 0-5 on 6-11 (SIDs 3-4, the right side). Frequencies use the PAL or NTSC SID clock (`$D06F`). The register write itself (the shadow, the mirror) is `stdlib_asm/sid_core.s45`'s `___sid_put`, shared with the tune player; `sid_hz_to_reg(hz)` (up to about 3800 Hz) and `sid_note_to_reg(note)` (a MIDI note, 0-106; 69 is A4, 440 Hz) give the frequency register value `sid_set_freq_reg` takes; the registers are at `___sid_regs` (`sid_regs.s45`, weak: a test points it at RAM). Used by the `invaders` and `musicgen` examples. |
| `audio_tune.h` | A tune (song) player (`stdlib_asm/tune.s45`, #314): pre-composed music as data (`tune45 convert` makes it), played once a frame by `tune_update` on a range of SID voices (`tune_voices(first)`, up to 12). `tune_play(tune)` / `tune_play_far(addr)` (a 28-bit address: tunes in banks 4-5 are read where they are, flat addressing), `tune_stop`, `tune_playing`, `tune_ticks`, `tune_note(voice)` (the note it sounds, `$FF` silent), `tune_seek(ticks)` (on that many ticks at once and silently, after `tune_play`: back where a tune was; tune45's `TUNE_<NAME>_TICKS` is a time through). Coded data (`tune45 convert --coded`: each field Huffman-coded, repeats as patterns nested two deep) is read as it plays, nothing unpacked, by a reader of its own (`stdlib_asm/tune_code.s45`, 1438 bytes and 306 of RAM) that `tune_codec()` links and sets up: tune45's `music.h` has `tune_play`, `tune_play_far` and `tune_queue` call it first for a coded tune (one started without it plays nothing); a program with plain tunes alone has none of it. A tune: a header (loops, voices, frames per tick as 8.8, the instrument table), 17-byte instruments (waveform, AD, SR, pulse width, vibrato depth, staccato gap, pulse-width sweep bouncing between $200 and $E00, vibrato delay, pitch slide, hard restart, filter, cutoff and its sweep, the wave table; 10 bytes in older data, still played) and the wave tables they use, and a byte stream a voice (notes for d ticks, rests, instrument changes, ties, arpeggios, glides, tempo changes, pattern calls and returns, loop, end). A glide starts the next note s semitones off its pitch and from its second frame moves (distance + 2^k - 1) >> k a frame towards it. A note restarts the envelope; the gap releases it that many ticks before the next event; hard restart turns the gate off and AD/SR to 0 a few frames before the next note; a wave table sets the waveform and pitch a frame; a filtered instrument's note sets its chip's cutoff (with a sweep a frame), resonance and mode and routes the voice (the volume kept, from the shadow); vibrato is a triangle over 8 frames, `(f >> 8) * depth * tri / 2`. Run-time controls: `tune_tempo` (8.8 scale), `tune_vibrato` (depth added), `tune_transpose` / `tune_detune` (per voice; detune adds amount/64 of the frequency). For a game: `tune_volume(0-15)` (the four SIDs' $D418 volume, the rest of it kept, and the digi voices', 17 a step), `tune_fade(frames, volume)` / `tune_fading` (a step a frame from `tune_update`), `tune_borrow(voice, frames)` / `tune_return` (a voice lent to the program's sound effects: the music goes on but writes none of its registers or its filter routing; back, it sounds from its next note), `tune_queue(tune, bar)` / `tune_queue_far` / `tune_queued` (the next tune started at the playing one's bar line: its ticks a multiple of `bar`, tune45's `TUNE_<NAME>_BAR`), `tune_filter(chip, cutoff, resonance, mode)` (a chip's filter at once, its routing and volume kept: BASIC's `FILTER`), `tune_voice_play(voice, offset)` / `tune_voice_stop(voice)` / `tune_voice_busy(voice)` (one voice of the tune playing started on a stream of its data, ended, asked: the others going on; BASIC's `PLAY` and `RPLAY`, #478). BASIC 65's `PLAY` events (#474-#477): a chip's volume, a voice's filter routing alone, BASIC's modulation and portamento. Digi voices (up to 4) play samples on the audio DMA, a channel each from `tune_digi_channels(first)` (the rest stay the program's): a hit sets the channel up as `digi_play` does (8-bit unsigned, once or looping from its loop's start, at its pitch's rate, the volume `tune_digi_volume` times the voice's `vol:` on both sides) from the samples at `tune_samples(addr)` (chip RAM, one bank of at most 64 KB: tune45's `tune_sample_bank` or `--samples` file); the next hit cuts it short, a stop ends it; the registers go to `___digi_regs` (`$D710`, weak: a test points it at RAM). NTSC frames count 213/256 of a PAL one and the notes use the NTSC clock, so a tune sounds the same. Assembly with no static allocation and no hardware multiplier: callable from an `__interrupt` handler (state changes hold interrupts off). Tests: `test_tune_xemu.c` (its SID writes in a RAM buffer); `test_xemu.sh` compares tune45's model with the player frame by frame (every effect; glides after a `tune_seek`; the filter, wave tables, hard restart and patterns; BASIC's `PLAY` events; digi voices, pitched, looping, their volume and stops, the audio DMA's registers too); `api_xemu.c` the controls for a game. |
| `audio_digi.h` | Sample playback on the 4 audio DMA channels (`$D720`+; 0-1 left, 2-3 right): `digi_init` (`$D711` = `$80`), `digi_play` (bank 0) / `digi_play_far` (24-bit address): length, rate in Hz (`digi_rate_to_reg`: rate × 2²⁴ / 40.5 MHz), volume, flags `DIGI_LOOP`/`DIGI_SIGNED`/`DIGI_16BIT`/`DIGI_NYBBLE_*`; `digi_stop`, `digi_playing`, `digi_set_volume` (with the other side's), `digi_set_rate`, `digi_position`, `digi_done`. Needs the CPU at 40 MHz. |
| `audio_midi.h` | `midi_parse(&parser, byte, &msg)` turns a MIDI byte stream into `midi_msg_t` (a `midi_parser_t` set up by `midi_parser_init`; running status; real-time and SysEx skipped); a `midi_synth_t` (`midi_synth_init(&s, first_voice, num_voices, inst)`: every channel playing `inst`; `midi_synth_set_instrument(&s, channel, inst)` for one channel) plays them (`midi_synth_message`) on a range of SID voices (`midi_synth_active`: those sounding; `midi_synth_all_off` releases every note): free voice or the oldest, velocity 0 = note off, sustain pedal (CC 64), pitch bend (±2 semitones), program change (`midi_synth_set_programs`), channel volume (CC 7 → SID volume), all notes off (CC 120/123); `midi_note_to_hz`. |
| `intwide.h` | `struct __int64`, `struct __int128` with operators, and the width-parameterized runtime `__intN_add`, `__intN_sub`, `__intN_mul`, `__intN_cmp_u`, `__intN_cmp_s`, `__intN_neg`, `__intN_not`, `__intN_and`, `__intN_or`, `__intN_xor`, `__intN_shl`, `__intN_shr_u`, `__intN_shr_s` (arithmetic), `__intN_divmod_u` (shift and subtract), `__intN_divmod_s` (truncating) (used by `__int(N)`/`__uint(N)`) |
| `complex.h` | marker only: the `_Complex` types and operators are built into cc45 |
| `event_system.h` | Publish / subscribe (`stdlib/event_system.c`, #645): `sprite_event_system_create(max_subscribers)` (`malloc`: the subscriptions and a queue of deferred events, up to 255; `NULL` for 0 or out of memory), `_destroy`; `sprite_event_subscribe(sys, id, callback, user)` (the id's count of subscribers; 0 when full or the callback `NULL`), `sprite_event_unsubscribe` (1 if it was there); `sprite_event_publish(sys, id, data, size)` calls each subscriber of the id at once, in the order they subscribed (the number called); `sprite_event_publish_deferred` queues it (the data pointer, not a copy; `timestamp` is `clock()`); `sprite_event_system_process` delivers the queued events in order (one a callback queues meanwhile waits for the next call), `_process_immediate(sys, id)` only that id's; `_get_subscriber_count`, `_get_queue_size`, `_clear_queue` (the number dropped). A callback may subscribe, unsubscribe (itself too) and publish. Test: `test_event_system_xemu.c` |
| `mem_pool.h` | Pools and arenas (`stdlib/mem_pool.c`, #645). **Object pool**: `mem_pool_create(object_size, max_objects)` (1-256 objects in one `malloc` block; `NULL` otherwise), `mem_pool_alloc` / `mem_pool_free` O(1) (a stack of free slots; `free` refuses a pointer that is not one of the pool's objects or is free already: 0), `mem_pool_owns`, `mem_pool_available`, `mem_pool_reset` (all free), `mem_pool_get_stats` (totals, peak, the counts of allocations and frees), `mem_pool_destroy`. **Linear arena**: `mem_arena_create(size)`, `mem_arena_alloc(arena, size, alignment)` (1, 2, 4, 8 or 16, of the address; `NULL` when it does not fit), `mem_arena_offset`, `mem_arena_save` / `mem_arena_restore` (back to an offset), `mem_arena_reset`, `mem_arena_get_stats`, `mem_arena_destroy`. **Manager**: a pool per type id, `mem_pool_manager_create`, `_register(mgr, id, size, count)` (0 when the id has one), `_alloc` / `_free` / `_stats` by id, `_destroy` (its pools too). Test: `test_mem_pool_xemu.c` |
| (no header) | The run time of `basic45 --compile` (#369, [basic45.md](../bin/basic45.md#compiling-compile)): `stdlib_asm/basic_rt.s45` (PRINT of strings, numbers by the ROM's FOUT, `,` `TAB(` `SPC(` as BASIC 65's cursor rights; BASIC's error messages and the exit; division with `?DIVISION BY ZERO`, `INT` / `^` by the ROM, `SQR` / `LOG` / `EXP` / `^` checked; AYINT, GETADR, GETBYT's ranges) and `stdlib_asm/basic_str.s45` (strings: a length byte and the characters, a variable's on the heap; `+`, comparison, `LEFT$` `RIGHT$` `MID$` `CHR$` `STR$`; BASIC's stack of `GOSUB` return addresses and `FOR` loops, 128 entries; arrays: `stdlib_asm/basic_aref.s45`'s `__bas_aref` makes them (`DIM`, or 11 elements a dimension), checks the subscripts and gives an element's address, `stdlib_asm/basic_ael.s45` reaches one-dimensional ones; `stdlib_asm/basic_fns.s45`: `DEC`, `HEX$`, `STRBIN$`, `INSTR`, AND / OR / NOT / XOR's integers; `stdlib/basic_fn.c`: `RPT$`; `stdlib_asm/basic_fin.s45`: `VAL` and the numbers of `INPUT` and `READ` (read as `strtof` reads them, the same floats, without linking it); `stdlib/basic_in.c`: `INPUT`'s line / `GET` (KERNAL BASIN / GETIN), `INPUT#` / `GET#`, and `stdlib_asm/basic_items.s45`: `INPUT`'s items, `READ` from the program's `DATA` text, `RESTORE`; `stdlib_asm/basic_ti.s45`: `TI`, `CLR TI`, `TI$`, `SLEEP`; `stdlib_asm/basic_mixed.s45`: an interpreted program's variable by its `POINTER` (bank 0 from `$F700`, else 1), for basic45's compiled procs (#577); `stdlib_asm/basic_bank.s45` / `basic_bankx.s45`: `--bank=N`'s arrays and string variables in bank 1, 4 or 5 (#580); `stdlib_asm/basic_call.s45`: `__basic_call`, one C function run from a BASIC `SYS` (`--link`); `stdlib/basic_io.c`: `OPEN`, `CLOSE`, `ST`, `DOPEN` / `APPEND` / `DCLOSE`, `DS` / `DS$`, `DLOAD` / `LOAD` chaining (with `__bas_chain`) and the `REM PRAGMA EXPORT` / `IMPORT` block (a DOS statement's parameters parsed by `stdlib_asm/basic_dos.s45`; `DIR U12`, the SD card's directory, by `stdlib_asm/basic_dirsd.s45`, reached through `basic_dirhook.s45`); basic_rt.s45's `__bas_chkout` / `__bas_chkin` / `__bas_clrch` for `PRINT#`, `CMD`, `INPUT#`, `GET#`; `stdlib/basic_gfx.c` (with `stdlib_asm/basic_gfxa.s45`: the dot, lines, BOX, triangles, ellipses, PAINT; `LOADIFF` / `SAVEIFF` through `basic_io.c`'s DOS commands 17-19): the graphics statements, BASIC 65's graphics kernel written again over its own variables at `$1F00`; `stdlib/basic_scr.c`: `FOREGROUND`, `BACKGROUND`, `BORDER`, `HIGHLIGHT`, `RCOLOR`, `CURSOR`, `RCURSOR`, `WINDOW`, `RWINDOW`, `SPEED`, `RSPEED`, `PALETTE COLOR` / `RESTORE`, `VSYNC`; `stdlib/basic_using.c`: `PRINT USING`; `stdlib/basic_key.c`: `KEY`, `FONT`, `CHAR DEF`, `DECBIN`; `stdlib/basic_snd.c`: `SOUND`, `PLAY` (its strings read when it runs into the tune player's streams by `stdlib_asm/basic_play.s45`, `<audio_tune.h>`), `RPLAY`, `TEMPO`, `VOL`, `ENVELOPE`, `FILTER`; `stdlib/basic_dev.c`: `JOY`, `POT`, `LPEN`, `MOUSE`, `RMOUSE`; `stdlib/basic_sys.c` and `stdlib_asm/basic_usr.s45`: `SYS`, `RREG`, `USR`, `DMA`, `EDMA`, `FRE`, CLR's helpers; `stdlib/basic_sprite.c`: `SPRITE`, `SPRCOLOR`, `MOVSPR`, `SPRSAV`, `COLLISION`, `RSPRITE`, `RSPCOLOR`, `RSPPOS`, `BUMP` on the VIC-IV's registers, and `stdlib_asm/basic_spr.s45`: the interrupt hook that moves the sprites and keeps their collisions (in the KERNAL's IRQ vector through a stub at `$0500`; crt0's `__exit` calls `__irqhook_exit`, weak, to put the vector back)). The arguments are variables (`__bas_n`, `__bas_sa`, … and the float runtime's `__float_a` / `__float_b`), not parameters |

### Examples

conio's full-colour mode (`conio.h`):

```c
#include <conio.h>

int main(void) {
    clrscr();
    if (!fcm_enable()) return 1;                     /* no room in banks 4-5 */
    fcm_fill_rect(0, 0, 79, 0, ' ', WHITE, BLUE);   /* a title bar */
    fcm_cputs(2, 0, "file  edit  view", WHITE, BLUE);
    fcm_putch(10, 5, 'x', YELLOW, RED);
    fcm_cprintf(0, 2, BLACK, CYAN, "score %d", 1200);
    if (fcm_getattr_xy(10, 5) != ((RED << 4) | YELLOW)) return 2;
    fcm_disable();                                   /* one byte a cell again, text kept */
    return 0;
}
```

A tune (`audio_tune.h`) played to its end, `tune_update` once a frame from the main loop (a game calls it from a raster `__interrupt` handler: `examples/c/tunes`); `jingle.txt`:

```
instrument lead pulse 09 a6 0600 1 1
tune jingle 5 once
v1: @lead c4:2 e4:2 g4:4
v2: @lead c3:8
```

```bash
tune45 convert jingle.txt -o music.c                 # music.c, music.h: tune_jingle
cc45 -O2 -c tune.c -o tune.o45
cc45 -O2 -c music.c -o music.o45
ln45 -basic -o tune.prg tune.o45 music.o45 -l c45.lib
```

```c
#include <audio_tune.h>
#include "music.h"                               /* tune45 convert jingle.txt -o music.c */

#define RASTER (*(volatile unsigned char *)0xD012)

int main(void) {
    tune_play(tune_jingle);
    while (tune_playing()) {
        while (RASTER != 0xF8) ;                 /* once a frame */
        while (RASTER == 0xF8) ;
        tune_update();
    }
    return 0;
}
```

The serial port (`serial.h`; in xemu `-serialtcp HOST:PORT` connects it to a TCP peer):

```c
#include <serial.h>

static const struct ser_params p = {
    SER_BAUD_9600, SER_BITS_8, SER_STOP_1, SER_PAR_NONE, SER_HS_NONE
};

int main(void) {
    const char *s = "hi";
    char c;
    unsigned int wait = 0;
    if (ser_install(ser_static_stddrv) != SER_ERR_OK) return 1;
    if (ser_open(&p) != SER_ERR_OK) return 2;    /* 8N1 only */
    while (*s)
        if (ser_put(*s) == SER_ERR_OK) s++;      /* SER_ERR_OVERFLOW: buffer full, again */
    while (ser_get(&c) == SER_ERR_NO_DATA)
        if (++wait == 50000) return 3;           /* nothing came back */
    ser_close();
    ser_uninstall();
    return 0;
}
```

## 12a. Graphics (`tgi.h`)

cc65's Tiny Graphics Interface ([cc65's TGI page](https://cc65.github.io/doc/tgi.html)), so a cc65 TGI program ports with its calls unchanged: every function of the page and cc65's `tgi.h` (`tgi_load_driver`, `tgi_install`, `tgi_init`, `tgi_done`, `tgi_uninstall`, `tgi_unload`, `tgi_geterror`, `tgi_geterrormsg`, `tgi_clear`, the page, colour, palette, resolution and aspect-ratio functions, `tgi_setpixel`, `tgi_getpixel`, `tgi_gotoxy`, `tgi_line`, `tgi_lineto`, `tgi_circle`, `tgi_ellipse`, `tgi_arc`, `tgi_pieslice`, `tgi_bar`, `tgi_settextdir`, `tgi_settextscale`, `tgi_settextstyle`, `tgi_gettextwidth`, `tgi_gettextheight`, `tgi_outtext`, `tgi_outtextxy`, the vector font functions, `tgi_ioctl`, `tgi_imulround`), the `TGI_ERR_*`, `TGI_FONT_*`, `TGI_TEXT_*` and `TGI_COLOR_*` constants (the C64's colour numbers: the default palette's entries 0-15 are the C64's colours) and `<tgi/tgi-kernel.h>`'s variables (`tgi_curx`, `tgi_cury`, `tgi_color`, …; the installed vector font is `__tgi_vfont`: in C its cc65 name would be the type's). `tgi_install` returns the error code (cc65's header: `void`; its page: `unsigned char`).

**Drivers** are linked in, not loaded: `tgi_load_driver` finds one by name (any case), `tgi_install` takes one's address.

| Driver | Name | Pages | Memory |
|--------|------|-------|--------|
| `tgi_static_stddrv` | `tgi_stddrv`, `"mega65-fcm.tgi"` | 1 | 64 KB + 2.7 KB in banks 4/5 |
| `tgi_static_fcm2` | `"mega65-fcm2.tgi"` | 2 | most of banks 4 and 5, 4000 bytes of heap |
| `tgi_static_ncm640` | `"mega65-ncm640.tgi"` | 1 | 64 KB + 2.7 KB in banks 4/5 |
| `tgi_static_ncm2` | `"mega65-ncm2.tgi"` | 2 | most of banks 4 and 5, 4000 bytes of heap |
| `tgi_static_hires` | `"mega65-hires.tgi"` | 1 | 16 KB in bank 4 or 5 (320x200, 2 colours a cell) |
| `tgi_static_mcm` | `"mega65-mcm.tgi"` | 1 | 16 KB in bank 4 or 5 (160x200, 4 colours a cell) |
| `tgi_static_text40` | `"mega65-text-40x25.tgi"` | 1 | none: the KERNAL's 40x25 text screen (80x50 pixels) |
| `tgi_static_text80` | `"mega65-text-80x50.tgi"` | 1 | none: the KERNAL's 80x50 text screen (160x100 pixels) |

The first two are 320x200 with 256 colours (`tgi_getcolorcount` returns 0, cc65's 256), an 8x8 font, the aspect ratio `$00D4` (cc65's c64-hi: a 4:3 display). **The screen** is the VIC-IV's full-colour characters (`stdlib/tgi.c`; the driver's set-up, `tgi_install`, `tgi_uninstall`, `tgi_init`, `tgi_done`, `tgi_setviewpage` and `tgi_setdrawpage` with the memory they take, is assembly: `stdlib_asm/tgi_setup.s45`): 40x25 cells (CHR16, FCLRHI, H640 and V400 off), each 64 bytes, a byte a pixel; a page's 1000 characters are a column of 25 after another, so pixel (x, y) is at `page + (x >> 3) * 1600 + y * 8 + (x & 7)` (`tgi_ioctl(TGI_IOCTL_DRAWPAGE_ADDR / _VIEWPAGE_ADDR, &addr)` gives a page's address). For drawing on a page directly (DMA, far pointers), `tgi_ioctl(TGI_IOCTL_LAYOUT, &layout)` fills a `tgi_layout_t` with the current draw page and the layout every driver shares: character columns of `col_bytes` 1600 bytes, a pixel row `row_bytes` 8 after the one above, `col_px` 8 pixels a column of `bpp` 8 bits (16 of 4 with the 640x200 drivers, the left pixel the low nibble); `TGI_PIXEL_ADDR(&layout, x, y)` is a pixel's byte and `TGI_PIXEL_NIBBLE(&layout, x)` its nibble, and `tgi_col_lo[c]` / `tgi_col_hi[c]` is column c's offset c * 1600 (the drivers' own table, `tgi_pixel.s45`), #653; test `test_tgi_layout_xemu.c`. Pixel `$00` shows `$D021` and `$FF` the colour RAM's colour; with `$D021` 0, multicolour mode on (8-bit colour RAM colours) and the colour RAM (2000 bytes at offset `$3000`) `$FF`, a pixel's byte is its palette entry. `tgi_install` takes the memory from `far_malloc` (a page 64000 bytes, 64-byte aligned and inside one bank; the palette as it was, 768 bytes; a page's screen of character numbers, 2000 bytes), so `far_malloc`, code overlays and conio's full-colour mode keep the rest; the two-page drivers' screens, which do not fit, come from `malloc` (`tgi_heap.c`'s heap functions, which a two-page driver's description refers to: `tgi_driver_t`'s `heap`; a one-page driver uses banks 4 and 5 only and links no `malloc`: `tgi_setup.s45`'s weak defaults give nothing), else `TGI_ERR_NO_RES`. The kernel variables are a block in a fixed order (`stdlib_asm/tgi_vars.s45`): `tgi_install` copies a driver's numbers in one loop and clears a new driver's state in another; `tgi_load_driver` is a member of its own (`tgi_load.c`), so a program installing `tgi_static_stddrv` links only that driver. A plot-only program is about 4.8 KB. A new page is cleared; `tgi_init` does not clear (as cc65's). `tgi_init` saves the VIC registers it changes (`$D011`, `$D016`, `$D020`, `$D021`, `$D031`, `$D04C`, `$D04E`, `$D054`, `$D058`/`$D059`, `$D05E`, `$D060`-`$D065`) and `tgi_done` puts them back, `$D031` first (writing it sets the VIC-IV's layout for the mode); `tgi_uninstall` frees the memory.

**The 640x200 drivers** (`stdlib_asm/tgi_ncm.s45`, linked only with them; one page or two) are 16 colours, an 8x8 font, the aspect ratio `$006A` (200 / 640 · 4 / 3): the VIC-IV's nibble-colour characters (NCM: colour RAM byte 0 bit 3) at H640, each 16x8 pixels of 4 bits, so the page is laid out as the 320x200 driver's (40x25 characters of 64 bytes, a column of 25 after another, 64000 bytes): pixel (x, y) is nibble `x & 1` (the left one the low nibble) of `page + (x >> 4) * 1600 + y * 8 + ((x >> 1) & 7)`. Nibble 0 shows `$D021` (`tgi_init` sets it to 0), nibble 15 the colour RAM's byte 1 (`$0F`), the others entry `(byte 1 >> 4) * 16 + nibble`: colour c is palette entry c for 1-14, and 0 and 15 only while `$D021` and the colour RAM keep those values (a program that changes `$D021` changes colour 0; `tgi_setpalette` reads 16 entries). The drivers' drawing is in the drivers' members: `tgi_pixel.s45`'s and `tgi.c`'s routines send the line on the screen, the ellipse's 4 point addresses (set, stepped in x, plotted), the 1x glyph and `tgi_bar`'s fill to the 320x200 drivers' (`stdlib_asm/tgi_fcm.s45`, `___tgi_f_*`) or, by the driver's mode byte (`___tgi_mode`, `$80`), the 640x200 one's (`___tgi_n_*`; also `tgi_setpixel`, `tgi_getpixel` and the clipped points). Each set replaces weak defaults that do nothing and is linked with its drivers only (a driver's description refers to its member: `tgi_driver_t`'s `draw`), so a program links one driver's drawing, and of it the routines it calls (a 320x200 program: a few bytes of tests; a 640x200 one with lines, ellipses and text: about 1.4 KB less than with both); lines, ellipses, arcs and text are the 320x200 driver's (the same pixels, compared on random scenes). Addresses are stepped as there, a nibble at a time (a byte every second pixel, +1593 across a character column). A line along x writes a byte's two pixels on one row at once, stored without reading it; a pixel alone in its byte is read, changed and written. The 1x glyph goes a byte column at a time down its 8 rows, two bits a row (a lone right pixel first at an odd x), until no bits are left: each byte stored, changed or skipped. A whole ellipse's 4 points are plotted without the arc's quarter tests. `tgi_bar` fills whole character columns (16 pixels) and byte columns (2) by DMA. Speed against the 320x200 driver (`bench_tgi.sh`, frames): pixels 67 / 53, long lines 37 / 30, short ones 33 / 29, circles 13 / 11, text 13 / 11 at 1x and 46 / 60 at 2x, bars 72 / 57.

**Drawing**: coordinates are signed and everything is clipped (`tgi_line`: Cohen-Sutherland with 32-bit intersections (the hardware multiplier's product, the library's signed division: as C's `long`), then Bresenham, all assembly; `tgi_setpixel` outside does nothing, `tgi_getpixel` outside returns 0). `tgi_setpixel`, `tgi_getpixel`, `tgi_line`, `tgi_lineto`, the line on the screen, the ellipse and the 1x glyph are `stdlib_asm/tgi_pixel.s45` (a line never reaches C: cc45's parameter thunk, `__zp_args`, costs more than a short line); a line with an end off the screen is clipped first (`__tgi_clipline_c`): a page never crosses a bank, so a pixel's address is the page's bank bytes and a 16-bit sum (column tables for `(x >> 3) * 1600`), stored with `sta [ptr],z`; the line keeps the colour in Y and `x & 7` in X and steps the offset (x ±1 in a character column, ±1593 across one, y ±8), a loop for each major axis and way. `tgi_bar` is DMA fills: a whole character column's rows are one run, other columns a fill every 8th byte (`dma_fill_far_step`). `tgi_clear` is one DMA fill (colour 0). Ellipses are Kennedy's midpoint ellipse (`tgi_pixel.s45`'s `__tgi_ellipse` works out its starting numbers with the hardware multiplier, walks a quarter in two passes with 32-bit Q-register adds and plots the point in the four quarters; `tgi_circle` and `tgi_ellipse` are assembly too: about 1.7 KB in all): the curve's own pixels, not cc65's polygon of 12-degree chords. When the ellipse's box is on the screen the four points are addresses stepped as the walk moves (as the line's); otherwise each point is clipped. An arc is the ellipse's points between its angles (parametric: the point (rx cos t, ry sin t)): for the start and end angles, `c = y·rx·cos b − x·ry·sin b` (≥ 0 when the point is past b) changes by one of two constants per step, so the test is adds too; each quarter is drawn whole, not at all, or where its test(s) hold. `tgi_arc`, `tgi_pieslice` and `tgi_imulround` are `stdlib_asm/tgi_arc.s45` (about 1.1 KB; a program using none of them links none of it, one using only `tgi_imulround`, the vector fonts, only that): an angle's test is four products of the hardware multiplier (its sine and cosine for the angle in the first quarter, 0..90, are >= 0: only the results take a sign); sines are an 8.8 table, needed only for the two angles; `tgi_pieslice` adds cc65's two radii. `tgi_circle` is an ellipse; an ellipse with a 0 radius is the line (x − rx, y − ry) to (x + rx, y + ry), an arc of one cc65's polygon. Y grows downwards, so angles go anticlockwise. The aspect ratio is not applied (as cc65's).

**The palette**: `tgi_getdefpalette` is the identity (256 entries, made when it is called: no table in the program); `tgi_setpalette(p)` shows at entry i the colour the palette had at `tgi_install` at entry `p[i]`; `tgi_getpalette` is that `p`, or the identity while the palette is the default; `tgi_init` and `tgi_done` put the saved palette back when `tgi_setpalette` changed it (so a program that never sets one links neither function nor table).

**The generic drivers** (#645; mode byte bit 6, `$40`): the bitmap and text screens. A generic driver's description has, as its `draw`, a table of its own routines (install, uninstall, init, done, clear, set pixel, get pixel; `stdlib_asm/tgi_gen.s45` copies it at `tgi_install` and dispatches through it), and everything else is drawn through its pixels: `tgi_pixel.s45`'s and `tgi.c`'s routines send the line, the 1x glyph, `tgi_bar`'s fill and `tgi_clear` to `tgi_gendraw.c` (a pixel at a time), the ellipse a clipped point at a time, the pattern fill a pixel at a time in the pattern's colours (`tgi_pattern.s45`). `tgi_install` takes no pages for them, `tgi_init` / `tgi_done` call the driver's own, and `tgi_ioctl`'s page and layout codes give `TGI_ERR_INV_FUNC`. The weak defaults keep a program with the other drivers from linking any of it.

- **`mega65-hires.tgi` / `mega65-mcm.tgi`** (`stdlib_asm/tgi_bmp.s45`, `stdlib/tgi_bmpsetup.c`): the VIC-II's bitmap modes on the VIC-IV. `tgi_install` takes a 16 KB `far_malloc` block in bank 4 or 5: the 8000-byte bitmap at its 8 KB boundary (the VIC-IV takes the bitmap from the character pointer, `$D068`-`$D06A`, without its low 13 bits) and the 1000 colour bytes (the screen pointer) in the room before or after it. `tgi_init` sets the screen editor's 40x25 (ESC 4), turns the VIC-IV's hot registers off and sets bitmap mode (`$D011` bit 5; multicolour `$D016` bit 4), the pointers, no 16-bit or full-colour characters; `tgi_done` puts all of it back and the screen's size as it was. hires: 320x200, an 8x8 cell has two colours: a pixel set in colour c is on and makes c the cell's foreground (the colour byte's high nibble; the low one the background, 0); in 0, off. mcm: 160x200, a 4x8 cell has the background (`$D021`, colour 0) and three colours (the colour byte's two nibbles, the colour RAM byte): a pixel set in c takes the pair that has it, else one no pixel of the cell uses, else the first (the cell's other pixels in it change), the hardware's own limit. 16 colours; an 8x8 font (the font from the character pointer as `tgi_init` found it: the bitmap has taken the register, `___tgi_fontptr`).
- **`mega65-text-40x25.tgi` / `mega65-text-80x50.tgi`** (`stdlib_asm/tgi_txt.s45`, mode `$60`): the KERNAL's own text screen, its size set by the screen editor as BASIC sets it (ESC 4, ESC 5; *Programming the MEGA65*, ch. 2), so text and pixels share it. A pixel is a quarter of a character cell (80x50 and 160x100 pixels), the cell's four quarters one of the 16 PETSCII block characters (`$20`, `$7E` `$7C` `$7B` `$6C` the quarters, `$61` `$E1` `$62` `$E2` the halves, `$7F` `$FF` the diagonals, `$EC` `$FB` `$FC` `$FE` three, `$A0` all; in both character sets of the ROM, `$29000` and `$29800`); a cell with another character counts as empty. A pixel's colour is its cell's colour RAM byte: set in c, the quarter on and the cell c; in 0, off. With the bitmap font, `tgi_outtext` writes the characters themselves into the cells (`tgi_gendraw.c`: a character a cell, 2x2 pixels, at the cell of the cursor, in the current colour; horizontal right, vertical up), so text and block graphics mix; a vector font is drawn in pixels. `tgi_done` puts the screen's size (ESC 4 / ESC 8 / ESC 5) and its character set back.
- **`unsigned char tgi_toggle_case(void)`**: the other character set, CHR$(142) / CHR$(14) to the screen editor (the upper-case / graphics one and the lower / upper-case one; `$D069` bit 3 tells which): 1 when lower case now. The block characters are in both.
- **`void tgi_copyrect(int x1, int y1, int x2, int y2, int dx, int dy)`**: the rectangle to (dx, dy), any driver, a pixel at a time (`tgi_getpixel` / `tgi_setpixel`), in the order that keeps an overlapping copy right; off the screen 0 is read and nothing written.

Test: `test_tgi_modes_xemu.c` (both conventions). These are the VIC-II-compatible bitmaps; bitplane screens for C (BASIC 65's `SCREEN` layout) are #656.

**Text** (`stdlib_asm/tgi_text.s45`): the bitmap font is the lower/upper case half of the VIC's current font (CHARPTR; the character ROM for the VIC-II's `$1000`/`$1800`/`$9000`/`$9800`), so a PETSCII string shows in both cases as written; control characters take their place but draw nothing. It is magnified by the 8.8 scales rounded to whole numbers (driver flag bit 7 clear). At 1x, horizontal and wholly on the screen, a glyph is stored directly (`__tgi_glyph`: a row's 8 pixels in one or two character columns); otherwise each row's runs of set pixels are one bar. The cursor is the text's top left; `TGI_TEXT_VERTICAL` turns it anticlockwise (the cursor at its bottom left, the text going up). `tgi_outtext` moves the cursor by `tgi_gettextwidth`. Vector fonts are cc65's `.vfn` files (`<tgi/tgi-vectorfont.h>`: "TCH", version 0, the data's size, then `struct tgi_vectorfont`, whose character pointers are offsets from `vec_ops`), loaded with `fopen` into `malloc`'d memory (`stdlib/tgi_vfload.c`; `TGI_ERR_CANNOT_LOAD`, `TGI_ERR_INV_FONT`, `TGI_ERR_NO_RES`); `tgi_vectorchar` is cc65's: two bytes an operation, 7-bit signed X and Y (Y up) times the 8.8 scales, bit 7 of Y draws, bit 7 of X ends the character; the string is converted from PETSCII to ASCII. `tgi_gettextheight` is the font's `top` scaled.

**Fill patterns** (a cc45 extension; `stdlib_asm/tgi_pattern.s45`, linked only by a program that sets one): `tgi_setpattern(pattern, transparent)` (64 colours, 8x8 row by row; the 640x200 drivers use their low nibbles; `transparent` a colour left as it is, or `TGI_TRANSPARENT`), `tgi_setpattern_mono(bits, fg, bg)` (8 bytes, bit 7 the left pixel; `bg` `TGI_TRANSPARENT`: clear bits left as they are) and `tgi_setsolid()` (`tgi_setpattern(NULL, …)` too) choose what `tgi_bar` fills with. The pattern is anchored to the screen (pixel (x, y) takes the pattern's (x & 7, y & 7)), so bars side by side meet. The drivers keep a character column's pixel rows as 8 bytes each (a byte a pixel at 320x200, two at 640x200: pixel x at `__tgi_addr(x >> 1, y)`), so the pattern repeated down 200 rows (a 1600-byte strip, built when the pattern or the mode changes) has the screen's layout: a whole character column of the bar is one DMA copy from the strip's row y1, a column of single pixels (or 640x200's pixel pairs) a copy of every 8th byte (the DMA's source and destination steps, options `$83`/`$85`), and a transparent colour the DMA's transparency (`$07`, `$86`: a byte equal to it not written). At 640x200 a byte holds two pixels, so a transparent pattern there, and a bar's single-pixel columns, are plotted a pixel at a time (`tgi_setpixel` in the pattern's colours). `tgi_bar` asks `__tgi_pbar` first (`tgi.c`'s weak one does nothing); 1.7 KB of BSS and 0.9 KB of code. Test: `test_tgi_pattern_xemu.c` (ragged edges and whole columns, transparency, mono patterns, both drivers).

Not as cc65: no loadable driver files; `tgi_geterrormsg(TGI_ERR_INSTALLED)` is "A driver is already installed" (cc65's table gives "Unknown error"); `tgi_gettextheight` of the bitmap font is the scaled height's whole pixels (cc65's returns the 8.8 value's low byte); `tgi_install` while a driver is installed returns `TGI_ERR_INSTALLED`; circles, ellipses and arcs are the curve's pixels (midpoint), not cc65's polygons, so they differ from cc65's by a pixel here and there. Tests: `test_tgi_xemu.c` (the driver's numbers, the page layout, clipping, lines, bars, arcs, bitmap and vector text, errors, the palette, text mode back, the memory freed, two pages) and `test_tgi_vfn_xemu.c` (`.vfn` files on a D81), both conventions; `examples/c/tgi_demo`. Speed: `src/test/bench_tgi.sh` (not in `make test`) runs `benchmark_tgi.c` in xemu and prints the frames each workload takes (pixels, long and short lines, circles, text at 1x and 2x, bars) with a checksum of the page it drew, so a faster version must give the same checksums.

**The functions** (an error is kept in `tgi_error` for `tgi_geterror`; a function that needs a driver does nothing without one):

| Function | Description |
|----------|-------------|
| `unsigned char tgi_install(const void *driver)` | Checks the driver (`"tgi"`, API version 5, 1 or 2 pages: else `TGI_ERR_INV_DRIVER`), takes its memory (`TGI_ERR_NO_RES`), clears the pages. Returns the error; `TGI_ERR_INSTALLED` while one is installed |
| `void tgi_load_driver(const char *name)` | `tgi_install` of the linked driver named `name` (any case; `tgi_stddrv` too): `TGI_ERR_CANNOT_LOAD` for another name. Links all eight drivers |
| `void tgi_uninstall(void)`, `void tgi_unload(void)` | `tgi_done`, the memory freed, no driver; `tgi_unload` without a driver: `TGI_ERR_NO_DRIVER` |
| `void tgi_init(void)` | Graphics on (the VIC registers saved), draw page 0, the palette saved at install back if it was changed, colour 1 (white), the bitmap font at 1x horizontal; the page is not cleared. `TGI_ERR_NO_DRIVER` without a driver |
| `void tgi_done(void)` | Text mode back (the saved registers, `$D031` first; the palette); nothing when graphics are off |
| `unsigned char tgi_geterror(void)` | The last error, and `tgi_error` back to `TGI_ERR_OK` |
| `const char *tgi_geterrormsg(unsigned char code)` | The code's message (above `TGI_ERR_INSTALLED`: "Unknown error") |
| `unsigned tgi_ioctl(unsigned char code, void *data)` | `TGI_IOCTL_DRAWPAGE_ADDR` / `TGI_IOCTL_VIEWPAGE_ADDR` (an `unsigned long` stored), `TGI_IOCTL_LAYOUT` (a `tgi_layout_t`): `TGI_ERR_OK`; another code, or no driver: `TGI_ERR_INV_FUNC` (also in `tgi_error`) |
| `void tgi_clear(void)` | The draw page filled with colour 0 (one DMA job) |
| `unsigned tgi_getpagecount(void)` | 1 or 2 |
| `void tgi_setviewpage(unsigned char page)`, `void tgi_setdrawpage(unsigned char page)` | The page shown / drawn on; a page ≥ the count: `TGI_ERR_INV_ARG`, nothing changed |
| `unsigned char tgi_getcolorcount(void)`, `unsigned char tgi_getmaxcolor(void)` | 0 (cc65's 256) and 255; 16 and 15 with the 640x200 drivers |
| `void tgi_setcolor(unsigned char c)`, `unsigned char tgi_getcolor(void)` | The drawing colour; a colour ≥ a 16-colour driver's count: `TGI_ERR_INV_ARG`, kept |
| `void tgi_setpalette(const unsigned char *p)`, `const unsigned char *tgi_getpalette(void)`, `const unsigned char *tgi_getdefpalette(void)` | See **The palette** above (`tgi_setpalette` reads 256 entries, 16 at 640x200) |
| `unsigned tgi_getxres(void)`, `tgi_getyres`, `tgi_getmaxx`, `tgi_getmaxy` | 320 (640) and 200; 319 (639) and 199 |
| `unsigned tgi_getaspectratio(void)`, `void tgi_setaspectratio(unsigned r)` | The 8.8 ratio (`$00D4`, `$006A`); stored only (nothing applies it) |
| `void tgi_gotoxy(int x, int y)` | The cursor (`tgi_curx`, `tgi_cury`): any value, off the screen too |
| `void tgi_setpixel(int x, int y)`, `unsigned char tgi_getpixel(int x, int y)` | A pixel in the colour; its colour, 0 off the screen |
| `void tgi_line(int x1, int y1, int x2, int y2)`, `void tgi_lineto(int x2, int y2)` | A clipped line; the cursor moves to (x2, y2); `tgi_lineto` from the cursor |
| `void tgi_circle(int x, int y, unsigned char r)`, `void tgi_ellipse(int x, int y, unsigned char rx, unsigned char ry)` | The outline (radii 0-255; a 0 radius: a line) |
| `void tgi_arc(int x, int y, unsigned char rx, unsigned char ry, unsigned sa, unsigned ea)`, `void tgi_pieslice(...)` | The ellipse from angle `sa` to `ea` (degrees, anticlockwise from 3 o'clock); `tgi_pieslice` adds the two radii |
| `void tgi_bar(int x1, int y1, int x2, int y2)` | A filled rectangle, corners in any order, clipped; in the colour or the fill pattern |
| `void tgi_setpattern(const unsigned char *pattern, int transparent)`, `void tgi_setpattern_mono(const unsigned char *bits, unsigned char fg, int bg)`, `void tgi_setsolid(void)` | `tgi_bar`'s fill (see **Fill patterns**) |
| `void tgi_settextdir(unsigned char dir)`, `void tgi_settextscale(unsigned w, unsigned h)`, `void tgi_settextstyle(unsigned w, unsigned h, unsigned char dir, unsigned char font)` | `TGI_TEXT_HORIZONTAL` / `_VERTICAL`; the 8.8 scales (`0x100` 1x); both and `TGI_FONT_BITMAP` / `TGI_FONT_VECTOR` |
| `unsigned tgi_gettextwidth(const char *s)`, `unsigned tgi_gettextheight(const char *s)` | The string's size at the scales in pixels |
| `void tgi_outtext(const char *s)`, `void tgi_outtextxy(int x, int y, const char *s)` | The string at the cursor, which moves past it; at (x, y) (the cursor set there first) |
| `const tgi_vectorfont *tgi_load_vectorfont(const char *name)`, `void tgi_install_vectorfont(const tgi_vectorfont *f)`, `void tgi_free_vectorfont(const tgi_vectorfont *f)` | A `.vfn` file read into `malloc`'d memory (NULL and `TGI_ERR_CANNOT_LOAD` / `_INV_FONT` / `_NO_RES`); the font `TGI_FONT_VECTOR` draws with (NULL: none); freed (uninstalled first if installed) |
| `int tgi_imulround(int a, int b)` | `a · b / 256`, signed, rounded half away from 0: a value times an 8.8 factor |

```c
#include <tgi.h>

int main(void) {
    unsigned char ok;
    if (tgi_install(tgi_static_stddrv) != TGI_ERR_OK) return 1;   /* 320x200, 256 colours */
    tgi_init();                                  /* graphics on, colour white */
    tgi_clear();
    tgi_setcolor(TGI_COLOR_RED);
    tgi_line(0, 0, tgi_getmaxx(), tgi_getmaxy());
    tgi_setcolor(TGI_COLOR_YELLOW);
    tgi_circle(160, 100, 50);
    tgi_bar(10, 150, 60, 190);
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_outtextxy(100, 20, "hello, tgi");
    ok = tgi_getpixel(30, 170) == TGI_COLOR_YELLOW;
    tgi_done();                                  /* text mode back */
    tgi_uninstall();                             /* the memory freed */
    return ok ? 0 : 2;
}
```

## 12b. Containers

The container library (#409) is assembly, type-erased as `qsort` is: a container keeps its element size, and
copies elements in and out by it. The fixed-capacity ones (ring buffer, priority queue, bit arrays, lists) never
allocate: the storage is the caller's; the vector, the sets, the maps and the hash tables grow with `malloc` /
`realloc` and report running out
(NULL / -1), never `abort`. All functions are `__fastcall__` (both libraries), each its own section: a program links
what it calls. They replace `data_structures.h`'s `sprite_vector_*`, `sprite_queue_*` and `sprite_hashmap_*`, which
no library implemented (the header is gone).

**`<ringbuf.h>`** (#405; `stdlib_asm/ringbuf.s45`): `ringbuf_t` over `cap` elements of `esize` bytes —
`rb_init(r, buf, esize, cap)`, `rb_push_back`, `rb_push_front`, `rb_pop_front(r, out)`, `rb_pop_back` (0, or -1
when full / empty; `out` may be NULL), `rb_front`, `rb_back`, `rb_at(r, i)` (pointers into the buffer, NULL when
there is none), `rb_count`, `rb_full`, `rb_empty`, `rb_clear`; a queue is `rb_enqueue`/`rb_dequeue`, a stack
`rb_push`/`rb_pop`/`rb_top` (macros). All `cap` slots are used (any `cap` up to 32767): head and tail run over
0 … 2·cap−1, so full and empty differ without a count both ends would write. **One producer and one consumer
need no lock** — e.g. `rb_push_back` in an `__interrupt` handler and `rb_pop_front` in the program — when `cap` ≤
128 (an index is one byte) and `esize` is a power of two (the element's address by shifts; the hardware
multiplier would be shared): each side writes only its own index, after copying the element, and the routines
keep nothing outside the zero page (the handler's own base page). The byte queue `rbb_t` (`rbb_init`, `rbb_put`
0 / -1 full, `rbb_get` the byte / -1 empty, `rbb_peek`, `rbb_count`, `rbb_empty`, `rbb_full`): 256 bytes of storage,
255 queued, byte indices (the wrap free); one producer and one consumer, any use. Sizes: `rb_push_back` with what it
uses 307 bytes, the whole deque 699, the byte queue 130.

**`<pqueue.h>`** (#405; `stdlib_asm/pqueue.s45`): a binary heap — `pq_init(q, buf, esize, cap, cmp)`, `pq_push`
(0, or -1 full), `pq_pop(q, out)` (0, or -1 empty; `out` may be NULL), `pq_top` (NULL when empty), `pq_count`. The
top is the element `cmp` puts first (`cmp(a, b) < 0`: `a` before `b`; reverse it for the largest first); `cmp`
NULL compares the bytes, unsigned, the first first (a big-endian key). `cmp` is called through `stdlib/ct_shim.c`'s
`__ct_cmp`, compiled in the library's convention (as `qsort`'s `__sort_cmp`): it must not use a priority queue
itself, and the queue keeps its state in BSS (not from an `__interrupt` handler while the program uses one).
869 bytes with what it uses.

**`<vector.h>`** (#404; `stdlib_asm/vector.s45`): `vec_t` (`data`, `size`, `cap`, `esize`) — `vec_init(v, esize)`,
`vec_push(v, e)` (the new element's address, NULL without memory; `e` NULL makes the place without filling it),
`vec_pop(v, out)`, `vec_at`, `vec_back` (NULL past the end), `vec_insert(v, i, e)` (`i` ≤ size), `vec_erase(v, i)`
(the rest moved down by `memmove`), `vec_erase_unordered(v, i)` (the last element into the hole), `vec_reserve(v,
n)` (0 / -1), `vec_shrink`, `vec_clear` (the memory kept), `vec_free`; `vec_sort` / `vec_bsearch` are `qsort` /
`bsearch` on the elements (macros), `vec_size`, `vec_empty`, `vec_data`. It grows by doubling (4 at first) with
`realloc`: **element addresses change when it grows**. `cap · esize` stays below 64 KB. It calls `malloc`'s
functions and `memmove` directly (all `__fastcall__` assembly: no thunks in either library) and keeps what it needs
across them in BSS (not reentrant). Sizes: `vec_push` with what it uses 394 bytes (not counting `realloc`),
`vec_at` 166, the whole vector 1062. The views are `static inline` in the header, no library code: `span_t`
(`span_of`, `span_of_vec`, `span_sub`, `span_at`) and `strview_t` (`sv_make`, `sv_from`, `sv_eq`,
`sv_starts_with`, `sv_find` (by `memchr`), `sv_sub`; a sub-view is cut to the view).

**`<bitset.h>`** (#408; `stdlib_asm/bitset.s45`): arrays of bits indexed at run time (cc45's `__bit`/`__bits` are
for named flags). `BITSET(name, nbits)` declares the bytes (bit `i` in byte `i >> 3`, bit `i & 7`); `bs_set`,
`bs_clear`, `bs_toggle`, `bs_test`, `bs_assign` are macros: a constant index is an expression on its byte (by
`__builtin_constant_p`), another calls the library (`__bs_set` …: the byte by shifts, the bit by an 8-byte table).
Over all of it, given the number of bits: `bs_set_all`, `bs_clear_all`, `bs_not`, `bs_set_range(b, from, n)`,
`bs_clear_range` (bit by bit to a byte boundary, then whole bytes), `bs_count` (a nibble table), `bs_any`,
`bs_none`, `bs_find_first_set(b, nbits, from)` / `bs_find_first_clear` (the index or -1; whole bytes of the other
kind skipped), `bs_and` / `bs_or` / `bs_xor(dst, src, nbits)`. The bits past `nbits` in the last byte stay 0
(start with `bs_clear_all` or a zeroed array); up to 32767 bits. Zero page only, no multiplier. Sizes: the bit
operations 105 bytes with `__bs_at`, all of it 772.

| Name | Description |
|------|-------------|
| `BITSET(name, nbits)`, `BS_BYTES(nbits)` | Declares `unsigned char name[(nbits + 7) >> 3]`; that byte count |
| `bs_set(b, i)`, `bs_clear(b, i)`, `bs_toggle(b, i)` | Bit `i` set, cleared, inverted (macros: `__bs_set(unsigned char *b, unsigned int i)` … for an index not constant). No bounds check |
| `bs_test(b, i)` | 1 when bit `i` is set, else 0 (`__bs_test` returns an `int` 0 / 1) |
| `bs_assign(b, i, v)` | `bs_set` when `v` is non-zero, else `bs_clear` |
| `void bs_set_all(unsigned char *b, unsigned int nbits)`, `void bs_clear_all(b, nbits)` | Every bit below `nbits` set / cleared; the last byte's bits past `nbits` cleared |
| `void bs_not(unsigned char *b, unsigned int nbits)` | Every bit below `nbits` inverted; those past it stay 0 |
| `void bs_set_range(unsigned char *b, unsigned int from, unsigned int n)`, `void bs_clear_range(b, from, n)` | Bits `from` … `from + n − 1` (nothing for `n` 0); not checked against the array's size |
| `unsigned int bs_count(const unsigned char *b, unsigned int nbits)` | The bits set (over the bytes `nbits` covers: bits past it must be 0) |
| `int bs_any(const unsigned char *b, unsigned int nbits)`, `bs_none(b, nbits)` | 1 when a bit is set / none is (the macro `!bs_any`) |
| `signed int bs_find_first_set(const unsigned char *b, unsigned int nbits, unsigned int from)`, `bs_find_first_clear(b, nbits, from)` | The first set / clear bit at `from` or above and below `nbits`, or −1 (also for `from` ≥ `nbits`) |
| `void bs_and(unsigned char *dst, const unsigned char *src, unsigned int nbits)`, `bs_or`, `bs_xor` | `dst` op= `src`, byte by byte over the bytes `nbits` covers (the last whole: `src`'s bits past `nbits` must be 0 for `bs_or` / `bs_xor`) |

**`<list.h>`** (#406; `stdlib_asm/list.s45`): intrusive linked lists — the links are members of the program's
structs (`container_of(ptr, type, member)` gets the struct back), nothing is allocated. `dlist`: circular with a
sentinel head (`dl_init`, `dl_push_front`, `dl_push_back`, `dl_insert_after(pos, n)`, `dl_remove(n)` without the
list, `dl_pop_front` / `dl_pop_back` (NULL when empty), `dl_splice(dst, src)`, `dl_empty`, `dl_first`, `dl_last`,
`dl_foreach(it, l)`, `dl_foreach_safe(it, tmp, l)` to remove while iterating); `slist`: NULL-terminated (`sl_init`,
`sl_push`, `sl_pop`, `sl_remove` (a walk: 1 / 0), `sl_reverse`, `sl_empty`, `sl_foreach`) — a stack, or a free
list over a pool. Zero page only. Sizes: the dlist 281 bytes (`dl_insert_after` 50, `dl_remove` 44), the slist
186. Calls beat inline functions here: cc45's C for `dl_remove` is 73 bytes and for the insert 65, and a call site
about 17. `dl_sort` (#406's optional merge sort) is not there.

**`<set.h>`** (#403): ordered sets of elements copied in by their size, ordered by a comparator (`cmp(a, b) < 0`:
`a` first; NULL compares the bytes, unsigned, the first first — a big-endian key or a string), as struct methods:
`struct set s; s.init(esize, cmp); s.insert(&e)` (1 inserted, 0 there already, -1 no memory), `s.erase(&key)` (1 /
0), `s.find(&key)` (the element or NULL), `s.lower_bound(&key)` (the first element not before key, or NULL),
`s.first()` / `s.next(e)` (in order, NULL at the end), `s.size()`, `s.clear()`, `s.free()` — the functions are
`set__insert` & co. (`__fastcall__`, defined in assembly). Two kinds with the same methods: **`struct set`**
(`stdlib_asm/set.s45`, 849 bytes with the map's methods, and `<vector.h>`'s routines: its first fields are a `vec_t`'s) is a sorted array —
a binary search, insert and erase moving the elements after the place; small, for tens to a few hundred elements;
**element addresses change** on insert and erase. **`struct tset`** (`stdlib_asm/tset.s45`, 1977 bytes with the map's) is a
red-black tree of `malloc`'d nodes (`struct tset_node`, opaque: two links, the parent, a colour byte, then the element:
7 bytes over the element) — O(log n) insert and erase, and an element stays where it is until it is erased; `clear`
frees every node. The tree's working pointers are in the zero page (`$08`–`$2B`, the parameter block: `(ptr),y` to a
node's fields), kept in BSS around `cmp`, `malloc` and `free`. `cmp` is called through `__ct_cmp`, as `<pqueue.h>`'s:
it must not use a set itself (not from an `__interrupt` handler while the program uses a set).

**`<map.h>`** (#407): ordered maps, the sets with an element of a key and then a value — `struct map` (on `struct set`)
and `struct tmap` (on `struct tset`): `m.init(ksize, vsize, cmp)` (`cmp` compares keys; NULL: the key's bytes),
`m.put(&key, &val)` (1 inserted, 0 the value replaced, -1 no memory), `m.get(&key)` (the value or NULL), `m.erase`,
`m.find` (the element: the key, the value `ksize` bytes on), `m.lower_bound`, `m.first()` / `m.next(e)` (in key
order), `size`, `clear`, `free`. The code is the sets': a set's `ksize` (offset 10 / 8) is its `esize`, and
`set::insert` copies the key's bytes and then a value from its second argument (none in a set), replacing it on an
element already there; the map's other methods jump to the set's (3 bytes each).

**`<hashmap.h>`** (#407; `stdlib_asm/hashmap.s45`, 1593 bytes): hash tables, open addressing with linear probing.
`struct hashmap`: `m.init(ksize, vsize, cap, hash, cmp)`, `m.put(&key, &val)` (1 / 0 replaced / -1), `m.get(&key)`
(the value or NULL), `m.find` (the element), `m.erase` (1 / 0; the slot becomes a tombstone), `m.first()` /
`m.next(e)` (every element, in no order), `m.size()`, `m.rehash(n)` (0 / -1), `clear`, `free`; `struct hashset` is
the same without a value (`init(ksize, cap, hash, cmp)`, `insert`). A slot is a state byte (0 empty, 1 erased, `$80`
\| the hash's high 7 bits: used, compared before the key) and the element; `cap` slots, a power of two of at least 8,
allocated by the first put; a put that would fill more than 3/4 of them (with tombstones) rebuilds the table for
twice the elements, without the tombstones — **element addresses change on put**. Keys: `ksize` bytes copied in, or
`ksize` 0 for C strings (the table keeps the pointer: `*(char **)e`; equal text in another buffer finds it). `hash`
NULL is `hash_bytes(p, n)` / `hash_str(s)` (djb2, `h · 33 ^ byte`, from 5381; both exported); `cmp` NULL compares
the bytes (strings: equal as `strcmp`); a program's `cmp` returns 0 when the keys are equal (`memcmp`, `strcmp`), and
is called through `__ct_cmp`, its `hash` through `__ct_hash` (`stdlib/ct_hash.c`): neither may use a hash table. The
state is in the zero page (`$10`–`$2F`), kept in BSS around the program's functions, `calloc` and `free` (not
reentrant).

Shared (`stdlib_asm/container.s45`): an element's address (`base + i·esize`: shifts for a power of two, else the
hardware multiplier; 96 bytes), copying (33), swapping (45) and comparing bytes (40). Tests: `test_ringbuf_xemu.c`
(random operations against an array model for 1- and 3-byte elements, a 300-byte element, the byte queue, and a
raster interrupt producing bytes and words while the program takes them: 3000 of each in order, the queues full now
and then), `test_pqueue_xemu.c` (against sorting and a sorted-array model; with and without `cmp`, a 5-byte struct),
`test_vec_xemu.c` (1500 random push / pop / insert / erase / erase_unordered against an array model, reserve and
shrink, sorting and searching, running out of memory, the views), `test_bitset_xemu.c` (random bit operations,
constant indices, ranges, find from every place, and/or/xor, all against a byte-per-bit model),
`test_list_xemu.c` (2000 random dlist operations against an array of ids, walked both ways; iteration with
removal, splicing; slist operations and a free list), `test_set_xemu.c` (1500 random inserts and erases on each kind against a presence
table, the tree's red-black rules checked after each; erasing every kind of tree node, addresses kept; struct elements,
`cmp` NULL; running out of memory), `test_map_xemu.c` (1500 random puts and erases with values against a table
for each map; replacing in place; struct values, `cmp` NULL; running out of memory), `test_hashmap_xemu.c` (2000
random puts / erases / gets against a table, with the built-in hash and with a program's 4-valued one; string keys;
a set of 5-byte keys, `rehash`, `clear`, reuse after `free`; running out of memory).

```c
#include <ringbuf.h>
#include <vector.h>
#include <bitset.h>
#include <hashmap.h>

struct enemy { int x, y; unsigned char hp; };

int main(void) {
    static int keys[16];                 /* a queue of 16 ints in the program's array */
    ringbuf_t q;
    int k = 42, out;
    rb_init(&q, keys, sizeof(int), 16);
    rb_push_back(&q, &k);
    if (rb_pop_front(&q, &out) != 0 || out != 42) return 1;

    vec_t v;                             /* a growable array of structs */
    struct enemy e = { 10, 20, 3 };
    vec_init(&v, sizeof(struct enemy));
    vec_push(&v, &e);
    e.x = 30;
    vec_push(&v, &e);
    if (v.size != 2 || ((struct enemy *)vec_at(&v, 1))->x != 30) return 2;
    vec_erase_unordered(&v, 0);          /* the last element into the hole */
    vec_free(&v);

    BITSET(used, 200);                   /* 200 slots, a bit each */
    bs_clear_all(used, 200);
    bs_set_range(used, 0, 10);
    bs_set(used, 12);
    if (bs_find_first_clear(used, 200, 0) != 10 || bs_count(used, 200) != 11) return 3;

    struct hashmap ages;                 /* C string keys (ksize 0) to ints */
    int age = 7, *p;
    char probe[4] = "ada";
    ages.init(0, sizeof(int), 16, NULL, NULL);
    ages.put("ada", &age);               /* the table keeps the pointer */
    p = ages.get(probe);                 /* equal text in another buffer */
    if (!p || *p != 7) return 4;
    ages.free();
    return 0;
}
```

## 12c. Far Memory (`far_ops.h`, `far_string.h`)

`stdlib/far_ops.c` and `stdlib/stdlib_overloads.c` (C, a section per function). A `far_ptr_t` (`<far_ptr.h>`) is 3 bytes, a bank and a 16-bit offset; every access is a `dma_copy_far` / `dma_fill_far` job on its linear address (`far_to_linear`: banks `$00`-`$7F` chip RAM, `(bank << 16) | offset`; `$80`-`$FF` attic RAM), so it is done when the call returns, and an offset carries into the next bank (`far_add`, `far_sub` and `far_diff` work on the linear address, across banks). A local pointer is a bank 0 address. The `int` offsets and indexes are signed in the library (−32768 … 32767; a larger value from cc45's unsigned `int` goes backwards) except the typed `far_array_*` of `far_string.h` (0 … 65535). The results of `far_cmp`, `far_diff`, `far_strcmp` and the far `strcmp`/`memcmp` forms are declared `signed int` (#710), so `far_cmp(a, b) < 0` holds in cc45's default mode, where a plain `int` is unsigned.

**`far_ops.h`** (includes `far_string.h`):

| Function | Description |
|----------|-------------|
| `uint8_t far_read_uint8(far_ptr_t p, int offset)`, `int8_t far_read_int8(...)`, `uint16_t far_read_uint16`, `int16_t far_read_int16`, `uint32_t far_read_uint32`, `int32_t far_read_int32`, `float far_read_float`, `double far_read_double` | The value at `p + offset`: 1, 2, 4 or 5 bytes (low byte first; `float` and `double` are the same 5-byte CBM format) copied by DMA |
| `void far_write_uint8(far_ptr_t p, int offset, uint8_t v)`, `far_write_int8`, `far_write_uint16`, `far_write_int16`, `far_write_uint32`, `far_write_int32`, `far_write_float`, `far_write_double` | Stores `v` at `p + offset` |
| `void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len)` | One DMA copy, forwards (an overlap with `dst` above `src` is not handled); `len` 0 does nothing |
| `void far_memcpy_smart(...)`, `void far_memcpy_stream(...)` | The same as `far_memcpy` |
| `void far_memcpy_to_local(void *dst, far_ptr_t src, size_t len)`, `void far_memcpy_from_local(far_ptr_t dst, const void *src, size_t len)` | Far to bank 0, bank 0 to far |
| `void far_memset(far_ptr_t p, int val, size_t len)`, `void far_memzero(far_ptr_t p, size_t len)` | A DMA fill with `val`'s low byte (0) |
| `far_ptr_t far_add(far_ptr_t p, int offset)`, `far_sub`, `far_inc(p)`, `far_dec(p)` | `p` ± `offset` (± 1) on the linear address, across banks |
| `signed int far_diff(far_ptr_t a, far_ptr_t b)` | The linear `a − b`, cut to 16 bits (any banks; meaningful within ±32767) |
| `signed int far_cmp(far_ptr_t a, far_ptr_t b)`; `int far_eq`, `far_lt`, `far_gt` | −1, 0, 1 by linear address; 1 / 0 |
| `far_ptr_t far_array_element(far_ptr_t base, int index, size_t esize)` | `base + index · esize` (a 32-bit product: past 64 KB too) |
| `void far_array_read(far_ptr_t base, int index, size_t esize, void *dst, size_t dst_size)`, `void far_array_write(far_ptr_t base, int index, size_t esize, const void *src, size_t src_size)` | Element `index`'s `esize` bytes to `dst` / from `src`; nothing when `dst_size` / `src_size` < `esize` |
| `size_t far_strlen(far_ptr_t s)` | The length, reading 32-byte chunks (up to 31 bytes past the NUL are read) |
| `void far_strcpy_to_local(char *dst, far_ptr_t src, size_t max)`, `void far_strcpy_from_local(far_ptr_t dst, const char *src, size_t max)` | At most `max − 1` characters and a NUL (longer strings cut); nothing for `max` 0 |
| `signed int far_strcmp(far_ptr_t a, const char *b)` | The difference of the first differing bytes (unsigned, −255 … 255), 0 when equal |

**`far_string.h`**: the `<string.h>` functions for each mix of local and far arguments, a function per combination (cc45 has no overloading by type). The suffix names the parameters in order: `pv` `void *`, `pc` `char *`, `fp` `far_ptr_t`, `i` `int`, `st` / `z` `size_t` (in `memcmp`'s, `f` is a far one). The all-local forms (`memcpy__pvpvst`, `memset__pvist`, `strlen__pc`, `strcpy__pcpc`, `strcmp__pcpc`, `strcat__pcpc`, `memcmp__pvpvz`) call the `<string.h>` function. A function whose destination is far returns its far pointer, one whose destination is local that pointer. Strings have no length limit.

| Function | Description |
|----------|-------------|
| `void *memcpy__fpvst(void *dst, far_ptr_t src, size_t len)`, `far_ptr_t memcpy__pvfpst(far_ptr_t dst, const void *src, size_t len)`, `far_ptr_t memcpy__fpfpst(far_ptr_t dst, far_ptr_t src, size_t len)` | `far_memcpy_to_local`, `far_memcpy_from_local`, `far_memcpy` |
| `far_ptr_t memset__fpist(far_ptr_t p, int val, size_t len)` | `far_memset` |
| `size_t strlen__fp(far_ptr_t s)` | `far_strlen` |
| `char *strcpy__pcfp(char *dst, far_ptr_t src)`, `far_ptr_t strcpy__fppc(far_ptr_t dst, const char *src)`, `far_ptr_t strcpy__fpfp(far_ptr_t dst, far_ptr_t src)` | The string and its NUL, measured first, then one copy (no bound on the destination) |
| `char *strcat__pcfp(char *dst, far_ptr_t src)`, `far_ptr_t strcat__fppc(far_ptr_t dst, const char *src)`, `far_ptr_t strcat__fpfp(far_ptr_t dst, far_ptr_t src)` | Appended at `dst`'s NUL |
| `signed int strcmp__fppc(far_ptr_t a, const char *b)`, `signed int strcmp__pcfp(const char *a, far_ptr_t b)`, `signed int strcmp__fpfp(far_ptr_t a, far_ptr_t b)` | <0, 0, >0 by the first differing unsigned byte (the difference); far strings read 32 bytes at a time |
| `signed int memcmp__fpvz(far_ptr_t a, const void *b, size_t len)`, `signed int memcmp__pvfz(const void *a, far_ptr_t b, size_t len)`, `signed int memcmp__fpfz(far_ptr_t a, far_ptr_t b, size_t len)` | The same over `len` bytes (0 for `len` 0) |
| `int16_t far_array_read_int16(far_ptr_t base, int index)`, `uint8_t far_array_read_uint8`, `uint32_t far_array_read_uint32`; `void far_array_write_int16(far_ptr_t base, int index, int16_t v)`, `far_array_write_uint8`, `far_array_write_uint32` | Element `index` (unsigned, 0 … 65535; the address in 32 bits) of an array of 2-, 1- or 4-byte elements |

Allocation, banks and attic RAM are `<far_ptr.h>`'s ([§12](#12-other-library-backed-headers), [§13](#13-mega65-hardware-mega65h-and-dma-dmah)). Tests: `test_far_ops_xemu.c`, `test_far2_xemu.c`, `test_far_string_xemu.c`, `test_far_attic_xemu.c`.

```c
#include <far_ptr.h>
#include <far_ops.h>

int main(void) {
    char buf[16];
    unsigned int i;
    far_ptr_t a = far_malloc(2000, BANK4);       /* 1000 ints in bank 4 */
    if (far_is_null(a)) return 1;
    for (i = 0; i < 1000; i++) far_array_write_int16(a, i, (int16_t)(i * 3));
    if (far_array_read_int16(a, 999) != 2997) return 2;
    far_write_uint8(a, 0, 0x55);                 /* a byte at an offset */
    if (far_read_uint8(a, 0) != 0x55) return 3;

    far_ptr_t s = far_add(a, 1000);              /* strings in far memory */
    strcpy__fppc(s, "hello");                    /* local -> far */
    strcat__fppc(s, " world");
    if (strlen__fp(s) != 11 || strcmp__fppc(s, "hello world") != 0) return 4;
    far_strcpy_to_local(buf, s, sizeof buf);     /* far -> local, bounded */
    far_free(a);

    /* attic RAM (the 8 MB HyperRAM), when the machine has it */
    if (far_attic_enable(0x8000000UL, 0x80FFFFFUL)) {
        far_ptr_t big = far_malloc_long(100000UL, BANK_ATTIC);   /* over 64 KB */
        if (far_is_null(big)) return 5;
        far_memset(big, 0xAA, 1000);
        far_write_uint32(big, 512, 0x12345678UL);
        if (far_read_uint32(big, 512) != 0x12345678UL) return 6;
        far_free(big);
    }
    return 0;
}
```

## 13. MEGA65 Hardware (`mega65.h`) and DMA (`dma.h`)

`dma.h`: `dma_copy(dst, src, len)` / `dma_fill(dst, len, val)` are compiler builtins with 16-bit bank-0 addresses. `dma_copy_far(dst, src, len)` / `dma_fill_far(dst, len, val)` (`__dma_copy_far`/`__dma_fill_far` in `lib/stdlib_asm/dma_far.s45`, 121 bytes and the shared job: the C was 643, #319; `__fastcall__`, so stack-convention callers need no thunk) take 28-bit linear addresses (`unsigned long`): chip RAM ($0000000–$005FFFF; $0020000–$003FFFF holds the ROM), colour RAM at $FF80000 (all of it, beyond the $D800–$DBFF window), …. They run an enhanced F018B job (megabyte options $80/$81) while the CPU waits; `len` 0 does nothing; the copy runs forwards. Test: `test_dma_far_xemu.c`. `dma_copy_far_step(dst, dst_step, src, src_step, len)` / `dma_fill_far_step(dst, dst_step, len, val)` (`lib/stdlib_asm/dma_step.s45`, its own job with the F018B skip-rate options `$83`/`$85` in front) take a step for each side in whole bytes: 2 writes every other byte (a two-byte cell screen's characters or colours), 1 is the plain copy, a source step of 0 repeats one byte; `len` counts the bytes written. Test: `test_seam_xemu.c`.

`palette.h` (`lib/stdlib_asm/palette.s45`, assembly): the 256 palette entries ($D100-$D3FF, each component's nibbles swapped in the register) as plain 0-255 values: `palette_set_color(index, r, g, b)`, `palette_set_color_rgb(index, rgb_color_t)`, `palette_get_color(index, &out)` (-1 for an index outside 0-255), `palette_set_range(first, count, rgb)` (r, g, b triples), `palette_save(first, count, buf)` / `palette_restore(first, count, buf)` (3 × count bytes, the registers as they are: the reds, the greens, the blues, so data prepared that way can be restored straight in), `palette_rotate(start, end, direction)` (colour cycling). They work on the palette bank mapped at $D100. The earlier `palette.h` declared fades, interpolation, banks and names that no library implemented (its `lib/src/palette.c` wrote the wrong registers); those are gone. Tests: `test_palette_xemu.c` (the edges: entries 0 and 255, out-of-range indexes and counts, the whole palette saved, rotation over 0-255), `test_seam_xemu.c`.

`seam.h` (`lib/stdlib_asm/seam.s45`, `__fastcall__`): a screen in Super-Extended Attribute Mode (two bytes of screen RAM a cell, a 13-bit character number; two of colour RAM, byte 1 the colour) with FCLRHI, so characters above $FF are full-colour (64 bytes at number × 64, a byte a pixel from the 256-colour palette; pixel $00 shows $D021, $FF the cell's colour) and those up to $FF the character set's text, on one screen. A `seam_screen_t` describes one anywhere in chip RAM with its colour RAM at an offset from $FF80000: `seam_init(&s, screen, colour, cols, rows)` (clears it), `seam_clear`, `seam_show` (saves and sets $D054 bits 0 and 2, $D058/$D059 the stride, $D05E, $D060-$D065), `seam_hide` (puts them back, $D054 first), `seam_put(&s, x, y, ch, colour)`, `seam_text(&s, x, y, codes, n, colour)` (screen codes), `seam_row(&s, x, y, n, codes, colours)` (a row of a one-byte screen's buffers), `seam_cells(&s, x, y, numbers, n)` (character numbers from a linear address: a picture's cells), `seam_cell`/`seam_colour` (the addresses). `seam_put` writes its cell's four bytes through a 32-bit pointer; the runs (`seam_clear`, `seam_text`, `seam_row`) are stepped DMA jobs, `seam_cells` one copy; a cell's offset comes from the hardware multiplier. `examples/c/glamlambs` shows its salon this way. Tests: `test_seam_xemu.c`, `test_seam2_xemu.c` (offsets over 16 bits, cells across a 64 KB boundary, 200 columns). (conio's full-colour mode, §12, keeps its own screen of this kind for text with backgrounds.)

```c
#include <seam.h>
#include <dma.h>

static seam_screen_t s;

int main(void) {
    unsigned char i;
    for (i = 0; i < 64; i++)                     /* character $1000 ($40000): a palette ramp */
        dma_fill_far(0x40000UL + i, 1, 16 + i);
    seam_init(&s, 0x58000UL, 0x1000, 80, 25);    /* bank 5; colour RAM from $1000 */
    seam_text(&s, 0, 0, s"score", 5, 1);         /* screen codes, white */
    for (i = 0; i < 8; i++)
        seam_put(&s, 10 + i, 5, 0x1000, 0);      /* full-colour cells among the text */
    seam_show(&s);
    /* ... */
    seam_hide(&s);                               /* the screen shown before */
    return 0;
}
```

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

### Hardware sprites (`sprites.h`)

The VIC-IV's 8 sprites (`lib/stdlib_asm/sprites.s45`: every function assembly, `__fastcall__`, the ZP convention in either library, each in its own section, so a program links the ones it calls; about 1 KB for all). The header was earlier an unimplemented software-sprite API; it is now the hardware's.

- `sprite_init()` saves the sprite registers it changes, turns 16-bit sprite pointers on (SPRPTR16, `$D06E` bit 7) with the pointer table in the library (16 bytes, 16-byte aligned: in that mode the VIC ignores the table address's bits 0-3), and sets every sprite off at (0, 0), hi-res, normal size, in front, image 0. `sprite_done()` turns them off and puts the saved registers back (`$D054` only its bit 4).
- `sprite_image(n, addr)`: the sprite's data at the 28-bit address `addr` (a multiple of 64, below 4 MB): images can be in banks 4-5 and take no room in bank 0.
- `sprite_pos(n, x, y)`, `sprite_move(n, dx, dy)`, `sprite_x(n)`, `sprite_y(n)`: sprite coordinates (the screen's top left `SPRITE_X0`, `SPRITE_Y0` = 24, 50); bit 8 goes to `$D010` / `$D077`, bit 9 to `$D05F` / `$D078` (the H640 and V400 MSBs). The library keeps the positions.
- `sprite_show(n)`, `sprite_hide(n)` (`$D015`); `sprite_color(n, c)` (`$D027`+n; a 16-colour sprite's transparent colour), `sprite_multicolors(c1, c2)` (`$D025`/`$D026`).
- `sprite_mode(n, SPRITE_HIRES / SPRITE_MULTICOLOR / SPRITE_16COLOR)` (`$D01C`, `$D06B`; 16-colour: colour c of sprite n is palette entry n·16 + c, the transparent colour set to 0); `sprite_size(n, flags)`: `SPRITE_EXPAND_X` (`$D01D`), `SPRITE_EXPAND_Y` (`$D017`), `SPRITE_WIDE` (8 bytes a row, 64 pixels: `$D057`), `SPRITE_TALL` (`sprite_height(rows)` rows: `$D055`/`$D056`); `sprite_behind(n, on)` (`$D01B`); `sprite_h640(on)` (SPR640, `$D054` bit 4: positions and pixels at the 640-wide resolution).
- `sprite_palette_bank(bank)`: the palette bank (0-3) sprites take their colours from (`$D070` bits 2-3, SPRPALSEL), so the text and graphics keep theirs; `sprite_init` keeps the current one (the KERNAL's `$D070` is `$FF`: bank 3 shown, mapped and for sprites). Fill a bank with `<palette.h>` after mapping it at `$D100` (`$D070` bits 6-7), then map the shown one back. `sprite_altcolors(n, on)`: a 16-colour sprite's second set, entries n·16 + c + 128 (SPRBPMEN: `$D049` / `$D04B` bits 4-7). `sprite_v400(n, on)`: rows one physical pixel high (`$D076`; y then counts physical lines). `sprite_tile(n, on)`: the sprite repeated to the end of its raster line (SPRTILEN: `$D04D` / `$D04F` bits 4-7). `sprite_alpha(n, on)`, `sprite_alpha_level(level)`: blended with what is behind (`$D074`, `$D075`; the hardware's: xemu does not show it). `sprite_init` / `sprite_done` save and restore these too, of the shared registers only the sprites' bits. `SPRITE_TALL` sprites share one height (`$D056`).
- `sprite_collisions()`, `sprite_bg_collisions()`: the hardware's collision bits since the last read (`$D01E`, `$D01F`; reading clears them). `sprite_overlap(a, b)`: 1 when the two sprites' boxes overlap (positions and sizes: 24, 64 wide or 16 for 16-colour, 21 or the tall height, doubled when expanded; shown or not).

Test: `test_sprites_xemu.c` (the registers, the pointer table, positions with their MSBs, the boxes, a real sprite-sprite collision, the registers back). Examples: `examples/c/sprite_demo` (a short movie using every function), `examples/c/invaders` (16-colour, wide sprites in bank 4), `examples/c/text_scroller` (a sprite in front of and behind text).

```c
#include <sprites.h>
#include <dma.h>

int main(void) {
    unsigned char i;
    sprite_init();
    dma_fill_far(0x40000UL, 63, 0xFF);           /* a solid 24x21 image in bank 4 */
    for (i = 0; i < 2; i++) {
        sprite_image(i, 0x40000UL);
        sprite_color(i, 1 + i);
        sprite_pos(i, SPRITE_X0 + 100 + i * 10, SPRITE_Y0 + 80);
        sprite_show(i);
    }
    sprite_size(1, SPRITE_EXPAND_X | SPRITE_EXPAND_Y);
    sprite_move(0, 4, 0);                        /* 4 pixels right */
    if (sprite_x(0) != SPRITE_X0 + 104 || !sprite_overlap(0, 1)) return 1;
    sprite_done();                               /* the registers as they were */
    return 0;
}
```

### Packed data (`unpack.h`)

`unsigned long unpack(unsigned long src)` (#345, `lib/stdlib_asm/unpack.s45`, `__fastcall__`: the ZP convention in either library; a macro for the library's `__unpack`, #346, so a program's own function named `unpack`, written without `<unpack.h>`, is a separate function and the overlay loader keeps calling the library's) unpacks the stream at the 28-bit address `src` to the address in its header (chip or attic RAM) and returns the number of bytes written, or `UNPACK_ERROR` (`0xFFFFFFFFUL`, `<limits.h>`'s `ULONG_MAX`; nothing written) for a stream no linked decoder takes: an unknown format byte, a format whose decoder is not linked, a format 2 header the decoder cannot take (0 is a stream of no bytes). The streams are made by [pack45](../bin/pack45.md) (format 1: literals, fills and repeats, each one DMA job) and by `ln45 --pack-overlays`. A stream can be loaded inside the area it unpacks into, as far in as pack45 reports, and unpacked there. `unsigned long unpack_to(unsigned long src, unsigned long dst)` (`__unpack_to`) unpacks to `dst` instead of the header's destination (a stream used in more than one place). Both are assembly (about 30% faster than the earlier C on the test stream, and 214 bytes smaller): literals up to 16 bytes copied by the CPU, longer ones, fills and repeats one DMA job each; from assembly, the addresses in `$10`-`$13` / `$14`-`$17` and `jsr ___unpack` / `___unpack_to`, the count in Q (pack45.md: examples with `.incbin` and from assembly). Test: `src/test-resources/test_unpack_xemu.c`.

**Format 2** (#666, `pack45 --image`): an image in up to 16 colours, `w` (even, at most 256) x `h` pixels, unpacked to `h` rows of `w / 2` bytes, 4 bits a pixel (the left one the low nibble), each the pixel's palette entry. Each pixel's rank (its colour's place in the stream's order of the colours, dark to light) minus a prediction (the pixel above, or the one to its left: pack45 keeps the smaller) is coded in a canonical Huffman code of at most 8 bits; texture_demo's coding, its order table in the stream. The decoder (`lib/stdlib_asm/unpack2.s45`, 0.9 KB of BSS: two 256-entry lookup tables, a row of ranks and of bytes) finds a code by one lookup on the next 8 bits and writes a row by a DMA copy when it is decoded, so a format 2 stream unpacks in place too. **The decoders are chosen** (#666): `lib/stdlib_asm/unpack.s45` is the dispatch on the format byte, and each format's decoder a member of its own that the program links by naming it at file scope, once: `UNPACK_FORMAT1;` (`unpack1.s45`, 315 bytes and 5 of BSS) and `UNPACK_FORMAT2;` (`unpack2.s45`, 581 bytes and 952 of BSS; `UNPACK_IMAGES;` is the same). The macros name `___unpack_format1` / `___unpack_format2`, which link the member, whose `___unpack1` / `___unpack2` then take the place of `unpack.s45`'s weak defaults (nothing written, `UNPACK_ERROR`; `load_unpack`: `UNPACK_ERROR` and `LOAD_NOT_PACKED`). A program that names none gets format 1, as before formats were chosen: `<unpack.h>` (and `<load.h>`, which includes it) names it unless `UNPACK_EXPLICIT_FORMATS` is defined first, in every file that includes them (or `-DUNPACK_EXPLICIT_FORMATS`). A program of format 2 streams alone so links no format 1 decoder (315 bytes of code fewer). The library's own users: `load.c` defines `UNPACK_EXPLICIT_FORMATS` (`load_unpack` takes what the program linked), `font_load.c` does not (fonts are packed in format 1), and `ovl_unpack.s45`, which `ln45 --pack-overlays` links, names format 1 (the overlay files are format 1). Assembly names the decoders it needs itself (`.extern ___unpack_format1`). Tests: `src/test-resources/test_unpack2_xemu.c` (chip RAM, in place, attic RAM, `unpack_to`, both formats in one program; streams from `src/test/gen_unpack2_stream.sh`), `test_unpack2only_xemu.c` (format 2 alone: a format 1 stream gives `UNPACK_ERROR`, and `test_xemu.sh` checks its map has no format 1 decoder).

### Fonts (`font.h`)

Fonts made by [font45](../bin/font45.md) (#466, `lib/stdlib_asm/font.s45`; `font_load` in `lib/stdlib/font_load.c`): a font at 1, 2 or 4 bits a pixel, fixed or proportional, measured and drawn into an 8-bit-a-pixel bitmap; and 8x8 charsets for the VIC-IV's text mode. The font data can be anywhere in the 28-bit space and is read through far pointers. A font's codes are those of the program's strings: PETSCII unless font45 was given `--encoding ascii`.

| Name | Description |
|------|-------------|
| `font_t` | `height`, `baseline`, `first`, `last`, `bpp`, `flags` (`FONT_PROPORTIONAL`, `FONT_FCM`, `FONT_NCM`), `cellw`, `rowbytes`, `cols`, `rows` (a glyph set's characters a glyph), `widths` (the width table's address, 0 when fixed), `glyphs` (the first glyph's address) |
| `const font_t *font_from(font_t *f, unsigned long data)` | Fills `f` from font45's data at `data` (a C array: `(unsigned long)(unsigned int)name`; or a file font45 wrote with `--bin`, loaded anywhere). Returns `f`, or `NULL` when the data is not a format 1 font |
| `unsigned char font_glyph_width(const font_t *f, unsigned char c)` | A code's advance in pixels: 0 for a code outside the font |
| `unsigned int font_width(const font_t *f, const char *s)` | A string's width |
| `void font_ramp(unsigned char first, unsigned char levels)` | The palette entries the coverage levels are drawn in: level n (1 up to 1, 3 or 15) is entry `first + n - 1`. The default `first` is 1 |
| `void font_clip(signed int x0, signed int y0, signed int x1, signed int y1)` | The rectangle drawing keeps to, inclusive. The default is x, y ≥ 0 |
| `signed int font_draw_bitmap(const font_t *f, unsigned long lin, unsigned int pitch, signed int x, signed int y, const char *s)` | Draws `s` into the bitmap at 28-bit address `lin`, `pitch` bytes a row, with the cell's top-left at x, y (not the baseline). Blank pixels (coverage 0) are left as they are. Returns the x after the last glyph |
| `signed int font_draw_fcm(const font_t *f, unsigned long chars, signed int strip, signed int x, signed int y, const char *s)` | The same, into full-colour characters (8x8, a byte a pixel) at `chars`: numbered down the columns, `strip` characters a column (`strip` > 0), or along the rows, −`strip` a row (`strip` < 0). x, y are pixels |
| `signed int font_draw_ncm(...)` | The same into nibble-colour characters (16x8, 4 bits a pixel, the left pixel the low nibble; the ramp's entries 0–15) |
| `unsigned char font_put_cells(const font_t *f, unsigned long screen, unsigned int row_bytes, unsigned char col, unsigned char row, const char *s)` | A glyph set's characters (font45 `--fcm` / `--ncm`) on a 16-bit character screen (2 bytes a character) at `screen`, `row_bytes` a row: each glyph its cols × rows characters from column `col`, row `row` (a proportional one as many columns as its width covers). Returns the column after. The set must be where the VIC-IV sees it; colour RAM is the program's (NCM: byte 0 bit 3 of each character) |
| `const font_t *font_load(const char *name, unsigned char len, unsigned long lin)` | A font file font45 wrote (`--bin` with `--load-at`; `--pack ADDR` too) loaded from the program's drive to `lin` with `load_far` and, when packed, unpacked to ADDR: a `font_t` for it (one, which the next `font_load` overwrites), or NULL (`load_error` says why; 0: not a font). In `font_load.c`, so only a program that loads fonts links `<load.h>` |
| `void font_install(unsigned long charset, unsigned int bytes, unsigned long lin)` | Copies a charset to chip RAM at `lin` (DMA) |
| `void font_select(unsigned long lin)` | Makes the VIC-IV's character set the one at `lin` (`$D068`-`$D06A`) |

Everything but `font_load` is assembly (`stdlib_asm/font.s45`, `__fastcall__`: 2.6 KB of code, 94 bytes of BSS; the C version was 5.1 KB in the stack library, 4.8 KB in the ZP one). `font_draw_bitmap` clips each glyph's cell (as longs) and draws its visible rows with `stdlib_asm/font_glyph.s45` (`[ptr],z` far reads and stores): about 4 times the C loop's speed (an 11-pixel 2 bpp font, 1040 glyphs: 18 frames against 72). `font_draw_fcm` / `font_draw_ncm` draw a glyph's visible rows a span at a time (#670): the clip once a glyph (as the bitmap's), the first cell's character number once a glyph (the hardware multiplier) and stepped from there in 16 bits (the next cell: + strip, or + 1 along the rows; a row of characters down: + 1, or + -strip), the address once a cell, its pixels walked with `z` (FCM a byte each, NCM a nibble each, read, changed and written), the pixel values shifted out of the glyph row's bytes and a byte's blank rest skipped at once: about 3.2 times the pixel-at-a-time loop's speed (a 16-row 2 bpp font, 1000 glyphs: FCM 29 frames against 93, NCM 31 against 99; `font_draw_bitmap` 27); `font_width` 3 times. Example: `examples/c/fonts`. Tests: `test_font_cells_xemu.c` (glyph sets, `font_draw_fcm` / `_ncm`, `font_load`), `test_font_xemu.c`, `test_font_ref_xemu.c` (each function against the C it replaced: every edge of the clip, ramps, both strips, made-up width tables), with fonts font45 makes from `src/test-resources/font45/tiny.txt`.

### Loading files anywhere (`load.h`)

`unsigned int load_far(const char *name, unsigned char len, unsigned char device, unsigned long lin)` (#444, `lib/stdlib/load.c`) loads a file (a PRG: its two-byte load address skipped) to a 28-bit address: chip RAM (below `$8000000`) by the KERNAL's `LOAD` into that bank, attic RAM through a stage in chip RAM and a DMA copy. It returns the bytes loaded, which `cbm_load_far` does not (`LOAD`'s end address less the start), and 0 on an error. `unsigned long load_unpack(name, len, device, at)` loads a pack45 stream to `at` (as `load_far`) and unpacks it to the destination in its header (`unpack`). It returns the bytes unpacked (`load_error` 0); 0 when the load failed (`load_error` the KERNAL's error or `LOAD_TOO_BIG`; a stream of no bytes also gives 0, with `load_error` 0); `UNPACK_ERROR` when no linked decoder takes the stream (an unknown format, a decoder not linked: `load_error` `LOAD_NOT_PACKED`, #666). `void load_stage(unsigned long lin, unsigned int size)` sets the stage: `$40000`, all of bank 4, by default; a file for attic RAM larger than `size` is `LOAD_TOO_BIG`. `device` 0 is the drive the program came from (`$BA`), or 8. `unsigned char load_error`: 0, the KERNAL's error (4 file not found, 5 device not present...), `LOAD_TOO_BIG` or `LOAD_NOT_PACKED`; a variable of its own rather than `_oserror`, since two of its values are not the KERNAL's. The KERNAL runs the load, so a program that has taken over the machine hands it back around the call. File names are PETSCII: with cc45's default encoding, `"level1"` for the directory's `LEVEL1`. Test: `src/test-resources/test_load_far_xemu.c` (from a D81: chip RAM, attic RAM, a packed file, the three errors).

A font compiled into the program, and a packed file loaded and unpacked (`<load.h>` includes `<unpack.h>`):

```bash
font45 convert tiny.txt --bpp 1 --proportional -o tiny.c   # tiny.c, tiny.h
pack45 -d 0x50000 tiles.bin tiles.pak
disk45 create game.d81 && disk45 add game.d81 tiles.pak TILES
cc45 -O2 -c fontload.c -o fontload.o45
cc45 -O2 -c tiny.c -o tiny.o45
ln45 -basic -o fontload.prg fontload.o45 tiny.o45 -l c45.lib
```

```c
#include <font.h>
#include <load.h>
#include <dma.h>
#include "tiny.h"                    /* font45 convert tiny.txt --bpp 1 --proportional -o tiny.c */

#define BITMAP 0x40000UL             /* 320 x 8 pixels, a byte each, in bank 4 */

int main(void) {
    font_t f;
    unsigned long n;
    unsigned char px;

    /* a font compiled into the program, drawn into a bitmap */
    if (!font_from(&f, (unsigned long)(unsigned int)tiny)) return 1;
    dma_fill_far(BITMAP, 320 * 8, 0);
    font_draw_bitmap(&f, BITMAP, 320, 0, 0, "AB");   /* set pixels: palette entry 1 */
    dma_copy_far((unsigned long)(unsigned int)&px, BITMAP + 1, 1);
    if (px != 1 || font_width(&f, "AB") == 0) return 2;

    /* a packed file from the drive the program came from:
       pack45 -d 0x50000 tiles.bin tiles.pak, on the disk as TILES */
    n = load_unpack("tiles", 5, 0, 0x48000UL);   /* loaded at $48000, unpacked to $50000 */
    if (n == 0 || n == UNPACK_ERROR) return 3;   /* load_error says why */
    return 0;
}
```

### Raster interrupts (`hw_raster.h`)

The program's code at raster lines, beside the KERNAL's own interrupt (#645; assembly, `stdlib_asm/hw_raster.s45`). `hw_raster_init` puts the library's handler in the KERNAL's IRQ vector (`$0314`; put back by `hw_raster_done`, which `hw_raster_init` gives to `atexit`). It keeps a table of lines, sorted, and sets the VIC's raster compare to each in turn:

- **The KERNAL's line** (255 at first; `hw_raster_set_line` moves it): the KERNAL's handler runs there once a frame, so the keyboard, the cursor and `clock()` go on; `hw_raster_get_frame` counts it.
- **Up to `HW_RASTER_MAX` (8) lines of the program's**, each with a function:
  - `hw_raster_set_isr(line, f)`: an `__interrupt` function, entered as the CPU would enter it (a return address and the flags pushed); its `RTI` returns to the library, which returns from the interrupt. ln45 checks what it reaches as for any `__interrupt` handler.
  - `hw_raster_set_callback(line, f, user)`: a plain function, `void f(int line, void *user)`, called by the library's own `__interrupt` handler (`stdlib/hw_raster_cb.c`). The call goes through a pointer from an interrupt, so ln45 wants every function of that type whose address is taken to have a stack frame (`#pragma cc45 recurse`), not static allocation.
  - A line that is the KERNAL's too: the program's function first, then the KERNAL's handler.

`hw_raster_clear_callback(line)` takes either kind off; `hw_raster_current()` is the line being handled (in a line's function); `hw_raster_disable` / `hw_raster_enable` turn the program's lines off and on (the KERNAL's keeps running: masking the raster interrupt would stop the keyboard and `clock()`). Without the handler: `hw_raster_get_line` (the VIC-II's lines: 0-311 PAL, 0-262 NTSC; the standard screen's 25 text rows are 51-250), `hw_raster_wait_line(line)` (the next time it begins), `hw_raster_in_vblank` / `hw_raster_wait_vblank` (outside the text rows: lines 251 to the end of the frame and 0-50), `hw_raster_get_x` (XPOS, `$D050`/`$D051`; xemu does not emulate it: it moves on at each read). The handler and its table are in the `init` segment (below `$4000`): the float routines map the ROM over `$4000`-`$7FFF` while they run, and an interrupt then must find them (a program's own `__interrupt` function there must not run then either). The future header also had `hw_raster_in_hblank` and `hw_raster_get_cycle`, which have no hardware behind them on the MEGA65; they are not there. Test: `test_hw_raster_xemu.c`.

### Hardware scrolling (`hw_scroll.h`)

The text screen scrolled by the VIC-IV (#645; assembly, `stdlib_asm/hw_scroll.s45`). `hw_scroll_init` takes the screen as it is as the origin: the screen and colour pointers (`$D060`-`$D063`, `$D064`/`$D065`) and the text position (`$D04C`/`$D04D` X, `$D04E`/`$D04F` Y). A position in pixels (`hw_scroll_set_x`, `_set_y`; `_set_fine_x`, `_set_fine_y`: the fine part only, 0-7) then shows the screen memory from there: x / 8 characters and y / 8 rows move both pointers on by row × LINESTEP (`$D058`/`$D059`) + column (two bytes a character with 16-bit characters, `$D054` bit 0; the hardware multiplier), and x % 8, y % 8 move the text up and left: the text position less 2 a pixel (1 at 640 wide, `$D031` bit 7, and at 400 lines, bit 3), the registers' high nibbles kept. The memory past the last column and row shows as the view moves (a program scrolling a wider map keeps LINESTEP its width). `hw_scroll_to(x, y, ms)` moves smoothly: each `hw_scroll_update(delta_ms)` steps it (start + distance × elapsed / duration, by the hardware multiplier and divider), 1 while it is still moving; `hw_scroll_is_active`; `hw_scroll_get_state` (x, y, fine x, fine y); `hw_scroll_reset` puts the origin back. A `$D011` / `$D016` write with the hot registers on (`$D05D` bit 7) sets the text position again from those registers: set the position after one. The future header's parallax (`hw_scroll_set_parallax`) had nothing to drive (the VIC-IV has no background layers in text mode) and is not there. Test: `test_hw_scroll_xemu.c`.

### Model and FPGA version (`mega65_version.h`)

Which MEGA65 the program runs on (#645; `stdlib/mega65_version.c`), from the hardware's registers: `mega65_get_version_raw()` the model ID (`$D629`); `mega65_get_revision()` that as a `mega65_revision_t` (`MEGA65_R1`-`MEGA65_R6`, `MEGA65_MEGAPHONE_R1`/`_R4`, `MEGA65_NEXYS4`, `MEGA65_NEXYS4DDR`, `_WIDGET`, `MEGA65_QMTECH_A100T`/`A200T`/`A325T`, `MEGA65_WUKONG`, `MEGA65_SIMULATION`, `MEGA65_EMULATOR`; `MEGA65_UNKNOWN` for another); `mega65_get_fpga_date()` the FPGA design's date as YYYYMMDD (`$D630`/`$D631`: days since 2020-01-01); `mega65_get_fpga_id()` the firmware ID (`$D632`-`$D635`, `$D632` the low byte); `mega65_get_version_info()` all of it; `mega65_revision_name(rev)` ("MEGA65 R3", …, "unknown"); `mega65_version_print()` ("MEGA65 R3, FPGA 2026-10-29, ID 756d6558" on `stdout`). xemu reports model 3 and ID "Xemu" (`$58 $65 $6D $75`). The future header read `$D07C` (a VIC-IV register) and had feature flags; every MEGA65 core has the features it named (H640, V400, full-colour text, DMA, 40 MHz, the SD card, the floppy controller), so they are not there. Test: `test_mega65_version_xemu.c`.

### Audio mixer (`hw_audio_mixer.h`)

The SIDs' volumes on the MEGA65's audio mixer (#645; `stdlib/hw_audio_mixer.c`). The mixer (`$D6F4` a coefficient's number, `$D6F5` its value) is a cross-bar of 16-bit gains, an input on an output; the SIDs are two of its inputs, SIDs 1-2 (`$D400`, `$D420`) and SIDs 3-4 (`$D440`, `$D460`), so `SID_1` and `SID_2` share their volumes, as `SID_3` and `SID_4` do. A volume (0-15) is the input's gain on the left outputs (the speakers' and the headphones') and on the right ones, both bytes of each coefficient volume × 17, as the hypervisor writes them (and waiting after `$D6F4` as it does, for the mixer to fetch the coefficient). `hw_mixer_init` (the volumes as the mixer has them), `hw_mixer_set_volume` / `_get_volume`, `hw_mixer_set_master_volume` (all four SIDs), `hw_mixer_mute` (the gains 0, the volumes kept), `hw_mixer_pan(chip, 0-15)` (0 left only, 8 both at the louder volume, 15 right only), `hw_mixer_fade_to(chip, l, r, ms)` stepped by `hw_mixer_update_fades(delta_ms)` (the fades still going), `hw_mixer_is_fading`, `hw_mixer_crossfade(from, to, ms)`, `hw_mixer_reset` (the hypervisor's stereo setting: each side's SIDs `$BE` on their own side, `$40` on the other). The other inputs (the digi channels, the OPL, the microphones) and the master volumes are left as they are; the volumes stay after the program ends (the hypervisor sets them at reset). Test: `test_hw_audio_mixer_xemu.c`.

### SD card, floppy, Ethernet, hypervisor (`hw_io.h`)

The MEGA65's I/O by its registers (#645; `stdlib/hw_io.c`, `stdlib_asm/hw_hyper.s45`; mega65-core's `sdcardio.vhdl`, `ethernet.vhdl`, hyppo's `dos.asm`):

- **SD card** (`$D680`-`$D684`): `hw_sd_read_sector(sector, buf)` / `hw_sd_write_sector` (512 bytes; `unsigned long` block numbers on SDHC, a byte address otherwise; the write gate opened, `$57`; sector 0, the partition table, refused), `hw_sd_flush`, `hw_sd_is_busy` / `_has_error` / `_is_sdhc`, `hw_sd_init` (1 when the card is ready: the hypervisor has set it up), `hw_sd_reset` (the controller, the card's addressing kept). The sector buffer (`$FFD6E00`) is in a program's view of `$FFD6xxx` while `$D689` bit 7 is set (the F011's otherwise): the library sets it around its copies. A program writing sectors must stay clear of the card's file system and the hypervisor's mounted images.
- **Floppy** (the F011, `$D080`-`$D08F`; buffer `$FFD6C00`): drive 0 (the internal drive or the D81 the hypervisor attached) by CBM track 1-80 and sector 0-39, 256 bytes, as a D81's: physical track - 1, side sector / 20, sector (sector % 20) / 2 + 1, the half (sector & 1) of the 512 bytes (`hw_fdc_write_sector` reads the physical sector first and keeps the other half). `hw_fdc_init` (drive 0, the motor on), `hw_fdc_seek_track`, `hw_fdc_is_busy`, `hw_fdc_get_status` (`$D082`).
- **Ethernet** (`$D6E0`-`$D6EE`; the frame buffers at `$FFDE800`): `hw_eth_init` (out of reset, broadcast frames accepted), `hw_eth_send_packet(data, 8-2048 bytes)`, `hw_eth_recv_packet(buf, max)` (the next frame, its length first in the buffer; 0 none), `hw_eth_tx_ready`, `hw_eth_rx_ready`, `hw_eth_get_mac`, `hw_eth_reset`. xemu exchanges frames only with a TAP device (`-ethertap`).
- **RTC**: `hw_rtc_read_time` / `hw_rtc_set_time` (`rtc_time_t`: year 2000-2099 … second) through `rtc_read` and `clock_settime`.
- **Hypervisor DOS** (`$D640`, A the function, then `CLV`; the carry: done): `hw_hyper_set_filename` (`$2E`: up to 63 characters, PETSCII to ASCII, copied to a page-aligned buffer below `$7F00` as the hypervisor wants), `hw_hyper_load_file(address)` (`$36`: the file named, whole, to a 28-bit address, from the SD card's FAT32 partition), `hw_hyper_trap(function, param)` (any: X / Y the parameter).

Test: `test_hw_io_xemu.c`, on a scratch SD card image test_xemu.sh makes (never xemu's own card).

### CPU speed, debug output, system information (`hw_peripherals.h`)

(#645; `stdlib/hw_peripherals.c`.) `hw_get_revision` (`mega65_get_revision`), `hw_get_cpu_speed` / `hw_set_cpu_speed` (1, 2 (C128 FAST, `$D030` bit 0), 3 (3.5 MHz, `$D031` bit 6) or 40 (also VFAST, `$D054` bit 6; the hot registers off while they change)), `hw_has_sdram` (an R6), `hw_get_vic_version` (4), `hw_debug_print` (to the serial monitor's UART by the hypervisor's trap `$03`, `$D643`, a character at a time: PETSCII to ASCII, `'\n'` as CR LF; `hw_debug_enable(0)` turns it off), `hw_get_cycles` (the CPU's cycle counter, `$D7E4`-`$D7E7`; xemu does not emulate it: `$FFFFFFFF`), `hw_sleep(ms)` (waits by `clock()`; the MEGA65 has no low-power mode, so `hw_wake` does nothing), `hw_get_sysinfo` (all of them, and `ram_size` in KB: 384 of chip RAM, 8192 more when attic RAM answers at `$8000000`). Without hardware behind them: `hw_set_breakpoint` / `hw_clear_breakpoint` give 0 (the MEGA65's breakpoints are the serial monitor's, set from the computer it is connected to: dbg45, `m65`), `hw_battery_percent` -1. The joystick ports' functions are `<input.h>`'s ([§11a](#11a-keyboard-and-joysticks-inputh)). Test: `test_hw_peripherals_xemu.c`.

### Raster rewrite buffer (`rrb.h`)

16-bit character rows with GOTOX tokens (#645; `stdlib/rrb.c`; mega65-core `viciv.vhdl`). With 16-bit characters (`$D054` bit 0) a cell is two bytes of screen RAM and two of colour RAM, and a cell whose colour byte 0 has bit 4 set is a GOTOX token: the characters after it on the row are drawn from x (its screen bytes' bits 0-9, pixels, doubled at 320 wide) over what is drawn already, so a row holds layers. `rrb_setup(chrcount)` (CHR16 on, CHRCOUNT `$D05E` and `$D063` bits 4-5, LINESTEP 2 × chrcount) / `rrb_done(chrcount)` (8-bit characters, LINESTEP chrcount), `rrb_double(on)` (DBLRR, `$D051` bit 6: the buffer redrawn every second raster line, twice the time for a row's tokens and characters, V200 only), `rrb_screen_cell(row, col)` / `rrb_colour_cell(row, col)` (a cell's 28-bit addresses), `rrb_gotox(row, col, x, flags, mask)` (`RRB_TRANSPARENT`: colour byte 0 bit 7, the characters after it leave their background; `RRB_ROWMASK`: bit 3, colour byte 1 a row mask), `rrb_putchar(row, col, ch, colour)` (a 13-bit character, its colour in byte 1). The screen's rows stay the KERNAL's, whose editor knows nothing of 16-bit characters: the cells are the program's to write. The future header's layer system and its raster-time estimates are not there (no documented cost per glyph to estimate from). Test: `test_rrb_xemu.c`.

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

29 of the headers in `lib/include` are kept for future work: they declare functions that no library defines, their sources (`lib/src/*.c`, `lib/stdlib/uart.s`) not being built into any archive. Each begins with a comment saying so, and its whole contents are inside `#ifdef CC45_FUTURE`: included by accident, such a header declares nothing, and a call of one of its functions is a compile error ("implicit declaration of function") instead of a failed link. `-DCC45_FUTURE` brings the declarations back, for work on an implementation.

They are, by family:

| Family | Headers |
|---|---|
| Software sprites | the 16 `sprite_*.h` headers (not `sprites.h`, the library's hardware sprites) |
| Hardware access | `uart.h` (use `serial.h`, the library's serial port; `mega65.h`, `hw_io.h`, `hw_peripherals.h`) |
| Input | `lightpen.h`, `paddle_mouse.h` (use `input.h`, `joystick.h`) |
| Audio and vectors | `audio.h` (use `audio_tune.h`, `audio_sid.h`, `audio_digi.h`), `simd.h` |
| Application framework | `cache_opt.h`, `persistence.h`, `procedural_content.h`, `resource_manager.h`, `scripting.h`, `test_harness.h`, `ui.h`, `world.h` |

The future graphics framework (`graphics.h`, `graphics_hal.h`, `graphics_text80x50.h`, `graphics_rrb.h`, `graphics_sprites.h`, `graphics_sprites_advanced.h`, `graphics_particles.h`) is gone (#645): its bitmap modes, block graphics and rectangle copy are `tgi.h`'s drivers and `tgi_copyrect` ([§12a](#12a-graphics-tgih)), its raster rewrite buffer `rrb.h` ([§13](#raster-rewrite-buffer-rrbh)); a software-sprite manager with animation and particle emitters are #756 and #757.

In headers that are otherwise implemented, a few declarations have no definition either: `stdio.h`'s `fileno` is declared as a function, but only the macro exists.

`sincos.h` and `sincos16.h` contain only `static` code and tables and work without the library: `isin(angle)` / `icos(angle)` (macros: a 256-entry table, the angle 0-255 a full turn, the value a `signed char` in 1.7 fixed point, -127 … 127), `fixmul(a, b)` (a 1.7 value times an integer), `isin_q` / `icos_q`; `sincos16.h` the same in 1.15 (`isin16`, `icos16`, `fixmul16`).

## 15. Runtime Startup (`crt0_zp.s45` / `crt0.s45`)

The CRT module from the library provides `__init`, which the linker uses as the entry point:

1. Save the caller's stack pointer and flags.
2. Enable the MEGA65 I/O registers (`$47`, `$53` to `$D02F`), with interrupts masked between the two writes (`php`/`sei` … `plp`, #667: an IRQ between them could turn the KERNAL's Ctrl-S pause on).
3. Set the exit status at ROM page-0 `$FD/$FE` to `$FFFF`.
4. Relocate the base page (`TAB`) to `__zp_page`, a private page at the end of BSS.
5. Move the CPU stack to the 16-bit stack topped at `__stack_top` (`$BFFF` by default; `-fstack-top=`/`#pragma cc45 stack_top(...)`), unless `-fno-stack16`/`#pragma cc45 no_stack16` keeps the ROM's page-1 stack; then fill BSS with `__bss_fill` (DMA).
6. `jsr _init_features`: crt0's weak one, an `rts`; a program's own `_init_features` (C: `void init_features(void)`) replaces it. `crt0_mega65.o45` / `crt0_mega65_zp.o45` (`lib/build/`, installed in `lib/cc45/`) have a strong one that sets the CPU to 40 MHz as BASIC 65's `SPEED` does (FAST `$D031` and VFAST `$D054` bit 6, the other bits kept, the VIC-IV hot registers off around it). To use it, name the object of the program's calling convention on the command line, before or after the library: `ln45 -basic -o prog.prg crt0_mega65_zp.o45 main.o45 c45.lib`, or `cc45 -O2 main.c -o prog.prg -Wl,<lib dir>/crt0_mega65_zp.o45`. It is in the archives too, but a library member is linked only for a symbol nothing defines, and crt0's weak definition is there when the archive is searched, so it is never pulled in from them (#741).
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
├── symbols/            # c64.sym, mega65.sym (ROM routines and I/O registers by name, for objdump45 -S)
├── tools/              # tuneplay.c (tune45's player program, build/tuneplay.o45)
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
