# Minimal C Standard Library (stdlib) for MEGA65/45GS02

This document describes the minimal C standard library (`stdlib`) for the `cc45` compiler targeting the MEGA65 (45GS02). All phases are implemented. The library ships as pre-built `.lib` archives in both stack and ZP calling conventions.

## 1. Core Headers & Types

These files define the fundamental types and constants required for C compliance and consistent memory layout.

- **`<stdint.h>`**: Fixed-width integer types (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`, `uint32_t`, `int32_t`). Maps to `cc45`'s `char` (8-bit), `int` (16-bit), and `long` (32-bit) types.
- **`<stddef.h>`**: Common definitions: `size_t` (typedef to `unsigned int`, 16-bit), `ptrdiff_t`, and `NULL` (`((void *)0)`).
- **`<stdbool.h>`**: Boolean type support. Defines `bool` as `_Bool` (a native 1-byte type that normalizes any non-zero value to `1`), `true` as `1`, `false` as `0`, and `__bool_true_false_are_defined` as `1`.
- **`<limits.h>`**: Implementation-defined limits (`CHAR_BIT=8`, `INT_MAX=65535`, `INT_MIN=0` for unsigned; `SCHAR_MIN=-128`, `SCHAR_MAX=127` for signed char; `LONG_MAX`, `LONG_MIN`, etc. for 32-bit).
- **`<stdarg.h>`**: Variadic function support. Defines `va_list` (typedef to `unsigned int`), `va_start`, `va_arg`, and `va_end` as macros wrapping `__builtin_va_start`, `__builtin_va_arg`, and `__builtin_va_end`.

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

Dynamic memory allocation using a sorted free-list allocator. Enabled via `#pragma crt heap`. All functions are hand-written 45GS02 assembly in `lib/stdlib/malloc.s`.

- **`malloc(size_t size)`**: First-fit allocation from sorted free list. Block splitting when remainder >= 4 bytes. Returns NULL for `malloc(0)`. Auto-initializes heap on first call. **Implemented.**
- **`free(void *ptr)`**: Returns block to sorted free list. Coalesces adjacent free blocks (both forward and backward). `free(NULL)` is a no-op. **Implemented.**
- **`calloc(size_t nmemb, size_t size)`**: Allocates `nmemb * size` bytes via `malloc`, then zeros with `memset`. **Implemented.**
- **`realloc(void *ptr, size_t size)`**: Allocates new block, copies `min(old_size, new_size)` bytes via `memcpy`, frees old block. `realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` = `free(p)`. **Implemented.**

### Implementation Details

- **Block header**: 4 bytes — 2-byte size (bit 0 = allocated flag), 2-byte next-free pointer.
- **Minimum allocation**: 4 bytes (2-byte header + 2-byte minimum payload), 2-byte aligned.
- **Heap region**: `__heap_start` (default: `__bss_end`) to `__heap_end` (default: `$D000`). Both are weak symbols, overridable at link time.
- **Initialization**: `_heap_init` creates a single free block spanning the entire heap. Called automatically by `crt_heap.o45` during CRT startup, or lazily on first `malloc` call.
- **CRT integration**: `#pragma crt heap` links `crt_heap.o45` from `c45.lib`, which calls `_init_heap_crt` → `_heap_init` during the startup sequence (after BSS init, before `_init_features`).

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

- **`abs(int j)`**: Absolute value. Returns `|j|` via two's complement negation if negative. **Implemented.**
- **`atoi(const char *nptr)`**: Convert string to integer. Handles optional leading sign and decimal digits. **Implemented.**
- **`itoa(int value, char *str, int base)`**: Convert 16-bit integer to string in given base (2-36). Handles negative values for base 10. Uses MEGA65 hardware divider. **Implemented.**
- **`ltoa(long value, char *str, int base)`**: Convert 32-bit long to string in given base (2-36). Handles negative values for base 10. Uses MEGA65 hardware divider's full 32-bit dividend/divisor. **Implemented.**
- **`rand()`**: Returns pseudo-random int (0-32767). Reads two bytes from MEGA65 hardware RNG at `$D7EF`, busy-waiting on `$D7FE` bit 7 for stabilization. Bit 15 cleared to match C standard positive range. **Implemented.**
- **`srand(unsigned int seed)`**: No-op stub for C compatibility (MEGA65 hardware RNG is autonomous). **Implemented.**
- **`strtol(char *nptr, char **endptr, int base)`**: Convert string to signed `long` (32-bit). Handles optional leading sign. Delegates to `strtoul` for digit parsing. **Implemented.**
- **`strtoul(char *nptr, char **endptr, int base)`**: Convert string to unsigned `long` (32-bit). Supports base 0 (auto-detect from prefix), 8, 10, 16. Handles `0x`/`0X` hex prefix and PETSCII letter ranges for bases > 10. **Implemented.**
- **`exit(int status)`**: Terminate program execution. Loads the status code into `.AX` and jumps to the `__exit` label emitted by the CRT startup. The exit behavior depends on the `#pragma crt exit_*` mode: `exit_rts` (default) restores the caller's SP and returns, `exit_halt` enters an infinite loop, `exit_brk` triggers a BRK. **Implemented.**

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

## 7. Runtime Startup (`crt0.s`)

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

## 8. Source Tree Placement

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
│   │   └── cbm.h       # CBM KERNAL interface (see doc/stdcbm.md)
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

## 9. Test Framework & Suggested Tests

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

## 10. Implementation Status

All phases are complete:

| Phase | Components | Status |
|-------|-----------|--------|
| 1. Headers | `stdint.h`, `stddef.h`, `stdbool.h`, `limits.h`, `stdarg.h` | Complete |
| 2. Core runtime | `crt0.s`, `putchar`, `puts`, `exit`, CRT pragmas | Complete |
| 3. String/memory | 18 functions in `string.h` (12 asm + 6 C) | Complete |
| 4. Utilities | `ctype.h` (13 functions), `abs`, `atoi`, `itoa`, `ltoa`, `rand`, `srand`, `strtol`, `strtoul` | Complete |
| 5. Heap | `malloc`, `free`, `calloc`, `realloc` with `#pragma crt heap` | Complete |
| 6. Formatted I/O | `printf`, `sprintf`, `sscanf` (C implementations) | Complete |
| 7. CBM interface | `cbm.h` KERNAL wrappers (see [stdcbm.md](stdcbm.md)) | Complete |

## 11. Compiler Prerequisites

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
