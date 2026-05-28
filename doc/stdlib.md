# Minimal C Standard Library (stdlib) for MEGA65/45GS02

This document describes the minimal C standard library (`stdlib`) for the `cc45` compiler targeting the MEGA65 (45GS02). All phases are implemented. The library ships as pre-built `.lib` archives in both stack and ZP calling conventions.

## 1. Core Headers & Types

These files define the fundamental types and constants required for C compliance and consistent memory layout.

- **`<stdint.h>`**: Fixed-width integer types (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`). Maps to `cc45`'s `char` (8-bit), `int` (16-bit), and `long` (32-bit) types.
- **`<stddef.h>`**: Common definitions: `size_t` (typedef to `unsigned int`, 16-bit), `ptrdiff_t`, and `NULL` (`((void *)0)`).
- **`<stdbool.h>`**: Boolean type support. Defines `bool` as `_Bool` (a native 1-byte type that normalizes any non-zero value to `1`), `true` as `1`, `false` as `0`, and `__bool_true_false_are_defined` as `1`.
- **`<limits.h>`**: Implementation-defined limits (`CHAR_BIT=8`, `INT_MAX=65535`, `INT_MIN=0` for unsigned; `SCHAR_MIN=-128`, `SCHAR_MAX=127` for signed char; `LONG_MAX`, `LONG_MIN`, etc. for 32-bit).
- **`<stdarg.h>`**: Variadic function support. Defines `va_list` (typedef to `unsigned int` — a 16-bit stack offset into the caller's arguments), `va_start`, `va_arg`, and `va_end` as macros wrapping `__builtin_va_start`, `__builtin_va_arg`, and `__builtin_va_end`. Variadic functions always use stack-based argument passing, even when compiled with `-fzpcall`. See "Variadic Functions" in [cc45.md](cc45.md#variadic-functions) for comprehensive details.

### Implementation Notes for Core Headers

Since `cc45` supports `typedef`, `#define`, and `signed`/`unsigned` qualifiers, all core headers can be implemented as pure header files using the preprocessor:

```c
// Example: stdint.h
typedef unsigned int uint16_t;
typedef signed int   int16_t;
typedef unsigned char uint8_t;
typedef signed char   int8_t;
typedef uint16_t     size_t;
```

## 2. Memory Management (`stdlib.h`)

Dynamic memory allocation using a sorted free-list allocator. Enabled via `#pragma cc45 heap`. All functions are hand-written 45GS02 assembly in `lib/stdlib/malloc.s`.

- **`malloc(size_t size)`**: First-fit allocation from sorted free list. Block splitting when remainder >= 4 bytes. Returns NULL for `malloc(0)`. Auto-initializes heap on first call. **Implemented.**
- **`free(void *ptr)`**: Returns block to sorted free list. Coalesces adjacent free blocks (both forward and backward). `free(NULL)` is a no-op. **Implemented.**
- **`calloc(size_t nmemb, size_t size)`**: Allocates `nmemb * size` bytes via `malloc`, then zeros with `memset`. **Implemented.**
- **`realloc(void *ptr, size_t size)`**: Allocates new block, copies `min(old_size, new_size)` bytes via `memcpy`, frees old block. `realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` = `free(p)`. **Implemented.**

### Implementation Details

- **Block header**: 4 bytes — 2-byte size (bit 0 = allocated flag), 2-byte next-free pointer.
- **Minimum allocation**: 4 bytes (2-byte header + 2-byte minimum payload), 2-byte aligned.
- **Heap region**: `__heap_start` (default: `__bss_end`) to `__heap_end` (default: `$D000`). Both are weak symbols, overridable at link time.
- **Initialization**: `_heap_init` creates a single free block spanning the entire heap. Called automatically by `crt_heap.o45` during CRT startup, or lazily on first `malloc` call.
- **CRT integration**: `#pragma cc45 heap` links `crt_heap.o45` from `c45.lib`, which calls `_init_heap_crt` → `_heap_init` during the startup sequence (after BSS init, before `_init_features`).

## 3. String & Memory Operations (`string.h`)

All string and memory functions are implemented in hand-written 45GS02 assembly (`lib/stdlib/*.s`) and archived in `c45.lib`. Validated via mmemu emulator tests.

**String functions:**
- **`strlen(char *s)`**: Calculate string length. **Implemented.**
- **`strcpy(char *dest, char *src)`**: Copy string including NUL. **Implemented.**
- **`strncpy(char *dest, char *src, int n)`**: Copy with length limit, NUL-pad remainder. **Implemented.**
- **`strcmp(char *s1, char *s2)`**: Compare strings, returns -1/0/1. **Implemented.**
- **`strncmp(char *s1, char *s2, int n)`**: Compare with length limit. **Implemented.**
- **`strcat(char *dest, char *src)`**: Append src to end of dest. **Implemented.**
- **`strncat(char *dest, char *src, int n)`**: Append at most `n` characters from src to dest, NUL-terminated. **Implemented.**
- **`strchr(char *s, int c)`**: Find first occurrence of char. **Implemented.**
- **`strrchr(char *s, int c)`**: Find last occurrence of char. **Implemented.**
- **`strstr(char *haystack, char *needle)`**: Find first occurrence of substring. Returns pointer to match or NULL. **Implemented.**
- **`strpbrk(char *s, char *accept)`**: Find first character in `s` that matches any character in `accept`. **Implemented.**
- **`strspn(char *s, char *accept)`**: Count leading characters in `s` that are in `accept`. **Implemented.**
- **`strcspn(char *s, char *reject)`**: Count leading characters in `s` that are NOT in `reject`. **Implemented.**
- **`strtok(char *s, char *delim)`**: Tokenize string by delimiters. Uses internal static state for subsequent calls with `s=NULL`. **Implemented.**

**Memory functions:**
- **`memcpy(void *dest, void *src, int n)`**: Copy memory area (no overlap). **Implemented.**
- **`memmove(void *dest, void *src, int n)`**: Copy memory area (overlap-safe, copies backwards when dest > src). **Implemented.**
- **`memset(void *s, int c, int n)`**: Fill memory with a constant byte. **Implemented.**
- **`memcmp(void *s1, void *s2, int n)`**: Compare memory areas, returns -1/0/1. **Implemented.**

## 4. Character Utilities (`ctype.h`)

All character classification and conversion functions are implemented in hand-written 45GS02 assembly (`lib/stdlib/*.s`) and archived in `c45.lib`. They operate on **PETSCII** character codes (the MEGA65's native character set).

**PETSCII character ranges:**
- Digits `'0'`–`'9'` = `$30`–`$39`
- Lowercase `'a'`–`'z'` = `$41`–`$5A` (PETSCII unshifted / default)
- Uppercase `'A'`–`'Z'` = `$C1`–`$DA` (PETSCII shifted)
- Space = `$20`, CR = `$0D`, Tab = `$09`
- Printable: `$20`–`$7E` and `$A0`–`$FF`

**Classification functions:**
- **`isdigit(int c)`**: Returns 1 if `$30`–`$39`. **Implemented.**
- **`isalpha(int c)`**: Returns 1 if `$41`–`$5A` or `$C1`–`$DA`. **Implemented.**
- **`isalnum(int c)`**: Returns 1 if digit or letter. **Implemented.**
- **`isspace(int c)`**: Returns 1 if `$20` (space) or `$09`–`$0D` (tab, LF, VT, FF, CR). **Implemented.**
- **`isprint(int c)`**: Returns 1 if `$20`–`$7E` or `$A0`–`$FF`. **Implemented.**
- **`isupper(int c)`**: Returns 1 if uppercase letter (`$C1`–`$DA`). **Implemented.**
- **`islower(int c)`**: Returns 1 if lowercase letter (`$41`–`$5A`). **Implemented.**
- **`isxdigit(int c)`**: Returns 1 if hex digit (`0`–`9`, `a`–`f`, `A`–`F` in PETSCII). **Implemented.**
- **`ispunct(int c)`**: Returns 1 if printable, non-alphanumeric, non-space. **Implemented.**
- **`isblank(int c)`**: Returns 1 if space (`$20`) or tab (`$09`). **Implemented.**
- **`iscntrl(int c)`**: Returns 1 if control character (`$00`–`$1F` or `$7F`–`$9F`). **Implemented.**

**Conversion functions:**
- **`toupper(int c)`**: If lowercase (`$41`–`$5A`), returns uppercase (`$C1`–`$DA`) via `ORA #$80`. Otherwise returns `c` unchanged. **Implemented.**
- **`tolower(int c)`**: If uppercase (`$C1`–`$DA`), returns lowercase (`$41`–`$5A`) via `AND #$7F`. Otherwise returns `c` unchanged. **Implemented.**

## 5. General Utilities (`stdlib.h`)

- **`abs(int j)`**: Absolute value. Returns `|j|` via two's complement negation if negative. Default is an inline macro; define `__NOMACRO_ABS` to use the function version. **Implemented.**
- **`labs(long j)`**: Long absolute value. Returns `|j|` for 32-bit signed long. Default is an inline macro; define `__NOMACRO_LABS` to use the function version. **Implemented.**
- **`atoi(const char *nptr)`**: Convert string to integer. Handles optional leading sign and decimal digits. **Implemented.**
- **`itoa(int value, char *str, int base)`**: Convert 16-bit integer to string in given base (2-36). Handles negative values for base 10. Uses MEGA65 hardware divider. **Implemented.**
- **`ltoa(long value, char *str, int base)`**: Convert 32-bit long to string in given base (2-36). Handles negative values for base 10. Uses MEGA65 hardware divider's full 32-bit dividend/divisor. **Implemented.**
- **`rand()`**: Returns pseudo-random int (0-32767). Reads two bytes from MEGA65 hardware RNG at `$D7EF`, busy-waiting on `$D7FE` bit 7 for stabilization. Bit 15 cleared to match C standard positive range. **Implemented.**
- **`srand(unsigned int seed)`**: No-op stub for C compatibility (MEGA65 hardware RNG is autonomous). **Implemented.**
- **`strtol(char *nptr, char **endptr, int base)`**: Convert string to signed `long` (32-bit). Handles optional leading sign. Delegates to `strtoul` for digit parsing. **Implemented.**
- **`strtoul(char *nptr, char **endptr, int base)`**: Convert string to unsigned `long` (32-bit). Supports base 0 (auto-detect from prefix), 8, 10, 16. Handles `0x`/`0X` hex prefix and PETSCII letter ranges for bases > 10. **Implemented.**
- **`exit(int status)`**: Terminate program execution. Loads the status code into `.AX` and jumps to the `__exit` label emitted by the CRT startup. The exit behavior depends on the `#pragma cc45 exit_*` mode: `exit_rts` (default) restores the caller's SP and returns, `exit_halt` enters an infinite loop, `exit_brk` triggers a BRK. **Implemented.**

## 5b. Integer Math (`math.h`)

Includes `stdlib.h` and adds integer math operations. All functions are implemented in C (`lib/stdlib/*.c`) and compiled via `cc45`. Each function is in a separate translation unit for selective linking.

- **`abs(int j)`**: Absolute value (re-exported from `stdlib.h`). **Implemented.**
- **`labs(long j)`**: Long absolute value (re-exported from `stdlib.h`). **Implemented.**
- **`div(int numer, int denom)`**: Integer division returning `div_t { int quot; int rem; }`. Uses MEGA65 hardware divider. **Implemented.**
- **`ldiv(long numer, long denom)`**: Long division returning `ldiv_t { long quot; long rem; }`. Uses MEGA65 hardware divider. **Implemented.**
- **`min(int a, int b)`**: Returns the smaller of two integers. **Implemented.**
- **`max(int a, int b)`**: Returns the larger of two integers. **Implemented.**
- **`gcd(int a, int b)`**: Greatest common divisor via Euclidean algorithm. Handles negative inputs. **Implemented.**
- **`lcm(int a, int b)`**: Least common multiple. Computed as `|a / gcd(a,b) * b|` to avoid overflow. **Implemented.**

## 6. I/O (`stdio.h`)

Character I/O, string output, and formatted printing.

- **`putchar(int c)`**: Output a single character via KERNAL `CHROUT` ($FFD2). **Implemented.**
- **`getchar()`**: Input a single character via KERNAL `GETIN` ($FFE4). Busy-waits until a key is pressed. **Implemented.**
- **`puts(const char *s)`**: Output a string followed by a newline. **Implemented.**
- **`sprintf(char *buf, char *fmt, ...)`**: Formatted string output. **Implemented.**
- **`printf(char *fmt, ...)`**: Formatted output to screen via `putchar`. **Implemented.**
- **`sscanf(char *str, char *fmt, ...)`**: Formatted string input parsing. **Implemented.**

### Format Specifiers

| Specifier | Description |
|-----------|-------------|
| `%d` | Signed decimal (16-bit `int`) |
| `%u` | Unsigned decimal (16-bit) |
| `%x` | Hexadecimal (16-bit) |
| `%o` | Octal (16-bit) |
| `%b` | Binary (16-bit, `sprintf` only) |
| `%s` | NUL-terminated string |
| `%c` | Single character |
| `%p` | Pointer (hex with `$` prefix) |
| `%%` | Literal `%` |
| `%ld` | Signed decimal (32-bit `long`) |
| `%lu` | Unsigned decimal (32-bit) |
| `%lx` | Hexadecimal (32-bit) |
| `%lo` | Octal (32-bit) |
| `%lb` | Binary (32-bit, `sprintf` only) |

The `l` length modifier causes 4 bytes to be read from the variadic argument list and converted via `ltoa` instead of `itoa`.

### `sscanf` Format Specifiers

`sscanf` supports the same specifier letters as `sprintf`/`printf` for input parsing:

| Specifier | Description | Destination type |
|-----------|-------------|-----------------|
| `%d` | Signed decimal | `int *` |
| `%u` | Unsigned decimal | `unsigned int *` |
| `%x` | Hexadecimal (accepts `$` or `0x` prefix) | `int *` |
| `%o` | Octal | `int *` |
| `%c` | Single character (no whitespace skip) | `char *` |
| `%s` | Whitespace-delimited string (NUL-terminated) | `char *` |
| `%%` | Literal `%` (matched, not assigned) | — |
| `%ld` | Signed decimal long | `long *` |
| `%lu` | Unsigned decimal long | `unsigned long *` |
| `%lx` | Hexadecimal long | `long *` |
| `%lo` | Octal long | `long *` |

Returns the number of items successfully matched and assigned. Whitespace in the format string matches any amount of whitespace in the input. Compiled as a separate `.o45` object — only linked when referenced.

### Implementation Strategy

These are thin wrappers around KERNAL calls. Since `cc45` supports `__asm__` inline assembly, these can be implemented as C functions with inline assembly for the JSR to KERNAL:

```c
int putchar(int c) {
    __asm__("lda.sp _p_c");
    __asm__("jsr $FFD2");
    return c;
}
```

Alternatively, these can be pure assembly (`.s` files) assembled with `ca45`.

## 7. Error Handling (`errno.h`)

Global error number set by library functions on failure.

- **`extern int errno`**: Global error variable. Set by `strtol`, `strtoul`, `malloc`, etc. on error. Not cleared on success — only meaningful immediately after a function that documents setting it. **Implemented.**
- **`extern char _errnoc`**: CBM KERNAL status byte (`$90`). Reflects the most recent KERNAL I/O operation (serial bus, tape, RS-232). Library routines wrapping KERNAL calls may propagate `_errnoc` into `errno`. **Implemented.**

| Constant | Value | Description |
|----------|-------|-------------|
| `ERANGE` | 1 | Result out of range (`strtol`/`strtoul` overflow) |
| `ENOMEM` | 2 | Not enough memory (`malloc` failure) |
| `EINVAL` | 3 | Invalid argument |
| `EDOM` | 4 | Math argument out of domain (reserved for future math functions) |

## 8. Non-Local Jumps (`setjmp.h`)

Save and restore execution context for non-local control flow (error recovery, co-routines).

- **`jmp_buf`**: Typedef for `char[4]` — stores return address (2 bytes) and stack pointer (2 bytes). **Implemented.**
- **`setjmp(jmp_buf env)`**: Saves the current PC and SP into `env`. Returns 0 on initial call. When `longjmp` restores this context, `setjmp` returns the value passed to `longjmp` (or 1 if that value was 0, per C standard). **Implemented.**
- **`longjmp(jmp_buf env, int val)`**: Restores the execution context saved in `env`, causing the corresponding `setjmp` to return `val`. Does not return. **Implemented.**

Both functions are hand-written 45GS02 assembly (`lib/stdlib/setjmp.s`).

## 9. MEGA65 Hardware (`mega65.h`)

Memory-mapped I/O register struct overlays for the MEGA65's VIC-IV, SID, and CIA chips.

### VIC-IV Register Struct

```c
#include <mega65.h>

vic4->border = COLOR_BLACK;        // struct access ($D020)
vic4->bg0 = COLOR_BLUE;            // struct access ($D021)
vic4->sprite_enable = 0x01;        // enable sprite 0
vic4->sprite[0].x = 100;           // nested sprite position struct
vic4->sprite[0].y = 150;
```

The `vic4` macro expands to `((volatile struct vic4_regs *)0xD000)`, providing type-safe access to all 128 VIC-IV registers ($D000-$D07F).

### Direct Register Defines

For single-register access, `VREG_*` defines provide an alternative syntax:

```c
VREG_BORDER = COLOR_BLACK;         // *(volatile unsigned char *)0xD020
VREG_BG0 = COLOR_BLUE;             // *(volatile unsigned char *)0xD021
VREG_CTRL_B |= VIC4_FAST;          // enable 40 MHz
```

Available defines: `VREG_BORDER`, `VREG_BG0`-`VREG_BG3`, `VREG_CTRL1`, `VREG_CTRL2`, `VREG_RASTER`, `VREG_SPR_ENABLE`, `VREG_SPR_XMSB`, `VREG_MEMPTR`, `VREG_IRQ_STATUS`, `VREG_IRQ_ENABLE`, `VREG_KEY`, `VREG_CTRL_A`, `VREG_CTRL_B`, `VREG_CTRL_C`.

### Convenience Functions

- **`vic4_unlock()`**: Writes VIC-III key sequence ($A5, $96) to enable extended registers.
- **`vic4_fast()`**: Unlocks VIC-III/IV and enables 40 MHz CPU speed.
- **`vic4_sprite_pos(n, x, y)`**: Sets sprite position with X > 255 MSB handling.

### Constants

**Colours:** `COLOR_BLACK` (0) through `COLOR_LGREY` (15).

**Control bits:** `VIC4_DEN`, `VIC4_BMM`, `VIC4_ECM`, `VIC4_MCM`, `VIC4_FAST`, `VIC4_H640`, `VIC4_FCM`, `VIC4_CHR16`, etc.

**VIC-III key:** `VIC3_KEY1` (0xA5), `VIC3_KEY2` (0x96).

### SID Sound Chips

Four SID instances at $D400, $D420, $D440, $D460:

```c
// Play a note on SID 1, voice 1
SID1_VOICE(0)->freq_lo = 0x0C;
SID1_VOICE(0)->freq_hi = 0x11;
SID1_VOICE(0)->attack_decay = 0x09;
SID1_VOICE(0)->sustain_release = 0xA0;
SID1_VOICE(0)->ctrl = SID_TRIANGLE | SID_GATE;
sid1->volume = 0x0F;
```

**Struct pointers:** `sid1`, `sid2`, `sid3`, `sid4` — access filter, volume, pots, and oscillator readback.

**Voice helpers:** `SID1_VOICE(n)` through `SID4_VOICE(n)` — typed access to individual voice registers (frequency, pulse width, waveform control, ADSR envelope).

**Waveform bits:** `SID_TRIANGLE`, `SID_SAWTOOTH`, `SID_PULSE`, `SID_NOISE`, `SID_GATE`, `SID_SYNC`, `SID_RINGMOD`.

**Filter:** `SID_FILT1`-`SID_FILT3`, `SID_FILT_EXT`, `SID_LP`, `SID_BP`, `SID_HP`, `SID_MUTE3`.

### CIA Interface Chips

Two CIA instances at $DC00 (CIA 1) and $DD00 (CIA 2):

```c
// Read joystick 1
unsigned char j = joy1_read();
if (j & JOY_FIRE) { /* fire pressed */ }

// Start CIA 1 timer A (one-shot, 1000 cycles)
cia1->timer_a_lo = 0xE8;
cia1->timer_a_hi = 0x03;
cia1->cra = CIA_CRA_START | CIA_CRA_ONESHOT;
```

**Struct pointers:** `cia1`, `cia2` — ports A/B, data direction, timers A/B, TOD clock, serial data, interrupt control, control registers.

**Convenience:** `joy1_read()`, `joy2_read()` — read joystick with active-low inversion.

**Joystick bits:** `JOY_UP`, `JOY_DOWN`, `JOY_LEFT`, `JOY_RIGHT`, `JOY_FIRE`.

**ICR bits:** `CIA_ICR_TA`, `CIA_ICR_TB`, `CIA_ICR_ALRM`, `CIA_ICR_SP`, `CIA_ICR_FLAG`, `CIA_ICR_SET`.

**Control bits:** `CIA_CRA_START`, `CIA_CRA_ONESHOT`, `CIA_CRA_LOAD`, `CIA_CRB_START`, `CIA_CRB_ALARM`, etc.

### DMA Controller (Direct Register Access)

For advanced DMA operations beyond `dma_copy`/`dma_fill` (chained jobs, fractional stepping):

```c
dma->addr_lo = job_addr & 0xFF;
dma->addr_mid = job_addr >> 8;
dma->addr_hi = 0;
dma->control = 0;  // trigger DMA execution
```

**Struct pointer:** `dma` ($D700) — control, address (lo/mid/hi/bank), enhanced trigger.

**Command constants:** `DMA_CMD_COPY` (0), `DMA_CMD_FILL` (3), `DMA_CMD_SWAP` (4).

### Math Accelerator

Hardware 32-bit multiply and divide:

```c
math_mul->arg1[0] = 10;  // multiplicand (32-bit, little-endian)
math_mul->arg2[0] = 5;   // multiplier
// result available in math_mul->result[] after ~6 cycles

math_div->arg1[0] = 100; // dividend
math_div->arg2[0] = 7;   // divisor
// quotient in math_div->quotient[], remainder shares with mul->arg1
```

**Struct pointers:** `math_div` ($D760), `math_mul` ($D770).

**Status:** `MATH_BUSY` ($D70F) — bit 7 set while computing. `HW_RANDOM` ($D7EF) — hardware RNG byte.

### Audio Mixer

Per-SID left/right volume control:

```c
audio_mixer->sid1_vol = AUDIO_BOTH(15);   // full volume both channels
audio_mixer->sid2_vol = AUDIO_LEFT(10);    // left only, volume 10
```

**Struct pointer:** `audio_mixer` ($D63C) — volume for SID 1-4.

**Macros:** `AUDIO_LEFT(vol)`, `AUDIO_RIGHT(vol)`, `AUDIO_BOTH(vol)` — pack volume nybbles.

### Floppy Disk Controller

Direct access to the internal 3.5" floppy drive:

**Struct pointer:** `fdc` ($D080) — control, command, track, sector, side, data, clock, step rate.

**Commands:** `FDC_CMD_READ`, `FDC_CMD_WRITE`, `FDC_CMD_STEP_IN`, `FDC_CMD_STEP_OUT`, `FDC_CMD_RESTORE`.

**Status bits:** `FDC_BUSY`, `FDC_DRQ`, `FDC_EQ`, `FDC_RNF`, `FDC_WRPROT`, `FDC_TK0`.

### SD Card Controller

Direct sector-level access to the SD card (bypassing KERNAL):

```c
sd->addr0 = sector & 0xFF;       // set sector address (32-bit)
sd->addr1 = (sector >> 8) & 0xFF;
sd->addr2 = (sector >> 16) & 0xFF;
sd->addr3 = (sector >> 24) & 0xFF;
sd->command = SD_CMD_READ;        // read sector into buffer
while (sd->status & SD_BUSY) {}   // wait for completion
```

**Struct pointer:** `sd` ($D680) — status, command, 4-byte sector address, buffer address, read data port.

**Commands:** `SD_CMD_RESET`, `SD_CMD_READ`, `SD_CMD_WRITE`, `SD_CMD_FLUSH`.

**Status bits:** `SD_BUSY`, `SD_RESET`, `SD_ERROR`, `SD_SDHC`.

### Ethernet Controller

Network frame send/receive:

**Struct pointer:** `eth` ($D6E0) — control, TX frame size, MAC address.

**Control bits:** `ETH_RST`, `ETH_TXEN`, `ETH_RXEN`, `ETH_RXIRQ`, `ETH_TXDONE`.

### Hypervisor Traps

MEGA65 hypervisor system calls:

**Struct pointer:** `hyper` ($D640) — 64-byte trap register array.

**Trap constants:** `HTRAP_SETNAME`, `HTRAP_LOADFILE`.

### Memory Pointers

```c
SCREEN_RAM[0] = 'A';       // write to default screen at $0800
COLOUR_RAM[0] = COLOR_WHITE; // write to colour RAM at $D800
```

## 9b. DMA Operations (`dma.h`)

Compiler builtins for the MEGA65's F018B DMA controller (~40MB/s vs ~1MB/s for CPU loops). No library linkage needed — these are expanded inline by the compiler.

- **`dma_copy(dst, src, len)`**: DMA memory copy. Macro wrapping the `__dma_copy` compiler builtin. Builds a DMA job on the hardware stack and triggers it. All registers preserved. **Implemented.**
- **`dma_fill(dst, len, val)`**: DMA memory fill. Macro wrapping `__dma_fill`. Same mechanism as copy. **Implemented.**

### Limitations

- `src`/`dst` must be 16-bit addresses (bank 0 only)
- `len` is 16-bit (max 65535 bytes per call)
- `val` is 8-bit fill value
- Not safe inside interrupts (uses hardware stack for DMA job)

### Example

```c
#include <dma.h>

// Clear 1000 bytes of screen RAM
dma_fill((void *)0x0800, 1000, 0x20);

// Copy 1000 bytes from source to screen
dma_copy((void *)0x0800, (void *)src_data, 1000);
```

## 9c. Timing (`time.h`)

Minimal timing functions based on the MEGA65 jiffy clock (KERNAL RDTIM, 60Hz).

- **`CLOCKS_PER_SEC`**: Defined as `60` (jiffy clock rate). **Implemented.**
- **`clock_t`**: Typedef for `int` (16-bit, wraps every ~18 minutes). **Implemented.**
- **`time_t`**: Typedef for `long` (32-bit, wraps every ~2.3 years). **Implemented.**
- **`clock()`**: Returns low 16 bits of the jiffy clock as `clock_t`. Hand-written assembly calling KERNAL RDTIM. **Implemented.**
- **`time(time_t *tp)`**: Returns jiffy clock as 32-bit `time_t`. If `tp` is non-NULL, stores the result at `*tp`. **Implemented.**
- **`difftime(time_t t2, time_t t1)`**: Returns `t2 - t1` as `long`. **Implemented.**

### Limitations

- No real-time clock — values are elapsed ticks since power-on, not wall-clock time
- `clock()` wraps at 65535 ticks (~18 minutes at 60Hz)
- PAL systems run at ~50Hz but `CLOCKS_PER_SEC` is defined as 60 (NTSC)

## 10. Runtime Startup (`crt0.s`)

The assembly entry point that prepares the environment for `main()`.

- **Stack Initialization**: Set the stack pointer (`SP`) to a known value (e.g., `$01FF`).
- **BSS Clearing**: Zero out uninitialized global variables (iterate BSS segment).
- **Data Initialization**: Copy initialized globals from their ROM/load positions if needed.
- **Hardware Init**: Basic MEGA65 setup (CPU speed to 40MHz via `$D054`, I/O mapping).
- **Heap Init**: Set the heap start pointer to the end of BSS.
- **Call `main`**: `JSR main` — the compiler generates `main` as a standard `proc`.
- **Exit Handler**: After `main` returns, execute the `exit()` sequence.

### Linker Considerations

The `ca45` assembler supports `.org` and segment directives (`.segment code/data/bss`). The `crt0.s` startup should use equates for segment boundaries that are resolved at assemble/link time. The proposed `ln45` linker (see `doc/ln45.md`) will handle final address resolution when multiple object files are combined.

## 11. Source Tree Placement

The stdlib lives within the existing `ccomp` source tree. No separate repository is needed at this stage.

### Directory Layout

```text
ccomp/
├── include/            # Compiler C++ headers
├── src/
│   ├── main/           # Compiler/assembler C++ sources
│   ├── test/           # Shell-based test scripts
│   └── test-resources/ # Test C/asm source files
├── lib/                # Standard library for target
│   ├── include/        # C headers (searched automatically by cc45)
│   │   ├── stdint.h
│   │   ├── stddef.h
│   │   ├── stdbool.h
│   │   ├── stdarg.h
│   │   ├── limits.h
│   │   ├── string.h
│   │   ├── stdlib.h
│   │   ├── stdio.h
│   │   ├── ctype.h
│   │   ├── cbm.h       # CBM KERNAL interface (see doc/stdcbm.md)
│   │   └── mega65.h    # MEGA65 hardware registers (VIC-IV struct overlay)
│   ├── stdlib/          # Library function implementations
│   │   ├── strlen.s     # Hand-written 45GS02 assembly (29 files)
│   │   ├── strcpy.s
│   │   ├── malloc.s
│   │   ├── printf.c     # C implementations (compiled via cc45)
│   │   ├── sprintf.c
│   │   └── sscanf.c
│   ├── stdlib_zp/       # ZP calling convention variants
│   ├── crt0.s           # Startup code (stack convention)
│   ├── crt0_mega65.s    # MEGA65-specific startup
│   ├── crt0_zp.s        # Startup code (ZP convention)
│   ├── crt0_mega65_zp.s # MEGA65-specific startup (ZP)
│   ├── crt_bssinit.s    # BSS initialization
│   ├── crt_heap.s       # Heap initialization
│   └── Makefile         # Builds .lib archives
├── doc/                # Documentation
└── Makefile            # Build system
```

### Build System

The `lib/Makefile` builds four library archives:

| Archive | Convention | Contents |
|---------|-----------|----------|
| `c45.lib` | Stack | CRT + stdlib (combined) |
| `c45_zp.lib` | ZP | CRT + stdlib (combined, ZP variant) |

Each archive contains both CRT startup modules and all standard library functions. Selective linking ensures only referenced modules are pulled into the final binary — unused stdlib functions add zero overhead.

Assembly `.s` files are assembled with `ca45 -c`, C `.c` files are compiled with `cc45 -c`, and all objects are bundled with `ar45 c`. Build via `make lib` from the project root.

### Include Path Convention

`cc45` automatically searches `lib/include/` relative to the compiler binary for standard headers. Additional search paths can be added with `-I<path>`:

```bash
cc45 myprogram.c -o myprogram.s              # finds <stdbool.h> etc. automatically
cc45 -Imy/headers myprogram.c -o myprogram.s  # adds custom search path
```

### Linking

```bash
# Stack convention (default)
ln45 -prg main.o45 -l c45.lib -o program.prg

# ZP convention
ln45 -prg main.o45 -l c45_zp.lib -o program.prg

# With BASIC auto-run stub
ln45 -basic main.o45 -l c45.lib -o program.prg
```

## 12. Test Framework & Suggested Tests

### Existing Test Infrastructure

The project uses a shell-script-based test framework:

| Script | Purpose |
|--------|---------|
| `src/test/test_compiler.sh` | Compile-only validation (compile + assemble, no execution) |
| `src/test/test_assembler.sh` | Assembler feature tests |
| `src/test/test_mmemu.sh` | Full execution validation via `mmemu-cli` emulator |
| `src/test/test_opcodes.sh` | Opcode/addressing mode validation |

The `test_mmemu.sh` framework is the most relevant for stdlib testing — it compiles C code, assembles it, runs it on the `mmemu-cli` emulator, and checks results by inspecting memory or the A register after execution.

### Convention for Validation Tests

Tests return results via:
1. **A register**: `A=$00` means pass, non-zero means fail (used by validation tests in `test_mmemu.sh`)
2. **Memory dumps**: Expected byte patterns at known addresses (used by specific functional tests)

### Proposed Stdlib Test Script

A new `src/test/test_stdlib.sh` following the same pattern as `test_mmemu.sh`:

```bash
#!/bin/bash
CC="./bin/cc45"
AS="./bin/ca45"
MMEMU="mmemu-cli"
LIB_INC="lib/include"

# Compile with stdlib include path, link with crt0 + stdlib modules
compile_and_run() {
    local name=$1
    local steps=${2:-500000}
    $CC -I$LIB_INC "src/test-resources/stdlib/${name}.c" -o "build/test/${name}.s"
    $AS "build/test/${name}.s" -o "build/test/${name}.prg"
    echo -e "load build/test/${name}.prg\nsetpc \$2000\nrun ${steps}\nregs\nq" \
        | $MMEMU -m rawMega65 2>/dev/null
}
```

### Proposed Test Cases

Place stdlib test sources in `src/test-resources/stdlib/`:

| Test File | Validates | Expected Result |
|-----------|-----------|-----------------|
| `test_strlen.c` | `strlen()` on known strings | A=$00 if lengths match |
| `test_memcpy.c` | `memcpy()` copies correctly | Memory comparison at target |
| `test_memset.c` | `memset()` fills correctly | Known byte pattern |
| `test_strcmp.c` | `strcmp()` returns correct ordering | A=$00 for all cases |
| `test_itoa.c` | `itoa()` decimal/hex conversion | String comparison |
| `test_atoi.c` | `atoi()` string-to-int | Value comparison |
| `test_ctype.c` | `isdigit`, `isalpha`, etc. | All classifications correct |
| `test_malloc.c` | `malloc`/`free` basic allocation | Pointer non-null, write/read OK |
| `test_putchar.c` | `putchar()` output | Byte appears at screen memory |
| `test_rand.c` | `rand()`/`srand()` determinism | Same seed produces same sequence |

### Test Pattern Example

```c
// src/test-resources/stdlib/test_strlen.c
#include <string.h>

int main() {
    char *s1 = "hello";
    char *s2 = "";
    char *s3 = "a";

    if (strlen(s1) != 5) return 1;
    if (strlen(s2) != 0) return 2;
    if (strlen(s3) != 1) return 3;
    return 0;  // A=$00 = pass
}
```

### Makefile Integration

```makefile
test-stdlib: all lib
	@echo "Running stdlib tests..."
	@bash src/test/test_stdlib.sh
```

The `test-stdlib` target is included in the main `test` dependency list.

## 13. Implementation Status

All phases are complete:

| Phase | Components | Status |
|-------|-----------|--------|
| 1. Headers | `stdint.h`, `stddef.h`, `stdbool.h`, `limits.h`, `stdarg.h` | Complete |
| 2. Core runtime | `crt0.s`, `putchar`, `puts`, `exit`, CRT pragmas | Complete |
| 3. String/memory | 18 functions in `string.h` (12 asm + 6 C) | Complete |
| 4. Utilities | `ctype.h` (13 functions), `abs`, `atoi`, `itoa`, `ltoa`, `rand`, `srand`, `strtol`, `strtoul` | Complete |
| 5. Heap | `malloc`, `free`, `calloc`, `realloc` with `#pragma cc45 heap` | Complete |
| 6. Formatted I/O | `printf`, `sprintf`, `sscanf` (C implementations) | Complete |
| 7. CBM interface | `cbm.h` KERNAL wrappers (see [stdcbm.md](stdcbm.md)) | Complete |

## 14. Compiler Prerequisites

All `cc45` features required by the standard library are implemented:

- Pointer arithmetic and dereferencing
- `typedef` for type aliases
- `#define` / `#include` via preprocessor (`cp45`)
- `__asm__()` for KERNAL calls
- `signed`/`unsigned` qualifiers
- Arrays with subscript indexing
- String literals (PETSCII, pooled in `.data`)
- `static` (file-scope and local static variables)
- `extern` declarations and `.o45` relocatable linking
- Function pointers (`int (*fp)(int)`, `typedef`, indirect calls)
- Variadic functions (`va_list`, `va_start`, `va_arg`, `va_end`)
- 32-bit `long` type (for `ltoa`, `%ld` format specifiers)

## See Also

- [stdcbm.md](stdcbm.md) — CBM KERNAL interface (`cbm.h`)
- [cc45.md](cc45.md) — C compiler (CRT pragmas, inline assembly, calling conventions)
- [ln45.md](ln45.md) — Linker (library linking, memory layout)
- [ar45.md](ar45.md) — Archiver (`.lib` archive management)
