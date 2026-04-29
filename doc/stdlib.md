# Minimal C Standard Library (stdlib) for MEGA65/45GS02

This document outlines the requirements and proposed implementation for a minimal `stdlib` for the `cc45` compiler targeting the MEGA65 (45GS02). It focuses on essential functionality for embedded systems while excluding large features like `printf`, `sprintf`, and floating-point math.

## 1. Core Headers & Types

These files define the fundamental types and constants required for C compliance and consistent memory layout.

- **`<stdint.h>`**: Fixed-width integer types (`uint8_t`, `int8_t`, `uint16_t`, `int16_t`). Maps directly to `cc45`'s `char` (8-bit) and `int` (16-bit) types. Note: `cc45` does not yet support `long`/32-bit types.
- **`<stddef.h>`**: Common definitions: `size_t` (typedef to `unsigned int`, 16-bit), `ptrdiff_t`, and `NULL` (`((void *)0)`).
- **`<stdbool.h>`**: Boolean type support. Defines `bool` as `_Bool` (a native 1-byte type that normalizes any non-zero value to `1`), `true` as `1`, `false` as `0`, and `__bool_true_false_are_defined` as `1`.
- **`<limits.h>`**: Implementation-defined limits (`CHAR_BIT=8`, `INT_MAX=65535`, `INT_MIN=0` for unsigned; `SCHAR_MIN=-128`, `SCHAR_MAX=127` for signed char).

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

Minimal dynamic memory allocation. Given the MEGA65's banked memory architecture, a simple heap manager operating within a single 64KB segment is appropriate for initial implementation.

- **`malloc(size_t size)`**: Allocate a block of memory from the heap.
- **`free(void *ptr)`**: Deallocate a previously allocated block.
- **`calloc(size_t nmemb, size_t size)`**: Allocate and zero-initialize memory.
- **`realloc(void *ptr, size_t size)`**: Resize an existing allocation.

### Implementation Strategy

A simple free-list allocator with 4-byte headers (2-byte size + 2-byte next-free pointer) is recommended. The heap region should be placed after the BSS segment, growing upward. `cc45` supports pointer arithmetic and struct access needed for the allocator logic.

## 3. String & Memory Operations (`string.h`)

All string and memory functions are implemented in hand-written 45GS02 assembly (`lib/stdlib/*.s`) and archived in `stdlib45.lib`. Validated via mmemu emulator tests.

**String functions:**
- **`strlen(char *s)`**: Calculate string length. **Implemented.**
- **`strcpy(char *dest, char *src)`**: Copy string including NUL. **Implemented.**
- **`strncpy(char *dest, char *src, int n)`**: Copy with length limit, NUL-pad remainder. **Implemented.**
- **`strcmp(char *s1, char *s2)`**: Compare strings, returns -1/0/1. **Implemented.**
- **`strncmp(char *s1, char *s2, int n)`**: Compare with length limit. **Implemented.**
- **`strcat(char *dest, char *src)`**: Append src to end of dest. **Implemented.**
- **`strchr(char *s, int c)`**: Find first occurrence of char. **Implemented.**
- **`strrchr(char *s, int c)`**: Find last occurrence of char. **Implemented.**

**Memory functions:**
- **`memcpy(void *dest, void *src, int n)`**: Copy memory area (no overlap). **Implemented.**
- **`memmove(void *dest, void *src, int n)`**: Copy memory area (overlap-safe, copies backwards when dest > src). **Implemented.**
- **`memset(void *s, int c, int n)`**: Fill memory with a constant byte. **Implemented.**
- **`memcmp(void *s1, void *s2, int n)`**: Compare memory areas, returns -1/0/1. **Implemented.**

## 4. Character Utilities (`ctype.h`)

All character classification and conversion functions are implemented in hand-written 45GS02 assembly (`lib/stdlib/*.s`) and archived in `stdlib45.lib`. They operate on **PETSCII** character codes (the MEGA65's native character set).

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

**Conversion functions:**
- **`toupper(int c)`**: If lowercase (`$41`–`$5A`), returns uppercase (`$C1`–`$DA`) via `ORA #$80`. Otherwise returns `c` unchanged. **Implemented.**
- **`tolower(int c)`**: If uppercase (`$C1`–`$DA`), returns lowercase (`$41`–`$5A`) via `AND #$7F`. Otherwise returns `c` unchanged. **Implemented.**

## 5. General Utilities (`stdlib.h`)

- **`abs(int j)`**: Absolute value. Requires `signed int` support (available in `cc45`).
- **`atoi(const char *nptr)`**: Convert string to integer.
- **`itoa(int value, char *str, int base)`**: Convert integer to string (essential for embedded display output).
- **`rand()` / `srand(unsigned int seed)`**: Pseudo-random number generation (16-bit LFSR or xorshift).
- **`exit(int status)`**: Terminate program execution. Loads the status code into `.AX` and jumps to the `__exit` label emitted by the CRT startup. The exit behavior depends on the `#pragma crt exit_*` mode: `exit_rts` (default) restores the caller's SP and returns, `exit_halt` enters an infinite loop, `exit_brk` triggers a BRK. **Implemented.**

## 6. Minimal I/O (`stdio.h`)

Excluding `printf` for now, but providing character-level building blocks.

- **`putchar(int c)`**: Output a single character via KERNAL `CHROUT` ($FFD2).
- **`getchar()`**: Input a single character via KERNAL `GETIN` ($FFE4).
- **`puts(const char *s)`**: Output a string followed by a newline.

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

### Proposed Directory Layout

```text
ccomp/
├── include/            # Compiler C++ headers (existing)
├── src/
│   ├── main/           # Compiler/assembler C++ sources (existing)
│   ├── test/           # Shell-based test scripts (existing)
│   └── test-resources/ # Test C/asm source files (existing)
├── lib/                # NEW: Standard library for target
│   ├── include/        # C headers installed/searched by cc45
│   │   ├── stdint.h
│   │   ├── stddef.h
│   │   ├── stdbool.h
│   │   ├── limits.h
│   │   ├── string.h
│   │   ├── stdlib.h
│   │   ├── stdio.h
│   │   ├── ctype.h
│   │   ├── cbm.h       # CBM-specific (see doc/stdcbm.md)
│   │   ├── peekpoke.h  # PEEK/POKE macros
│   │   └── mega65.h    # MEGA65 register definitions
│   └── src/
│       ├── crt0.s      # Startup code (ca45 assembly)
│       ├── string.s    # Optimized memory/string ops (assembly)
│       ├── string.c    # Fallback pure-C string ops
│       ├── stdlib.c    # abs/atoi/itoa/rand
│       ├── stdio.c     # putchar/getchar/puts
│       ├── ctype.c     # Character classification
│       ├── heap.c      # malloc/free/calloc/realloc
│       └── cbm_io.s    # KERNAL call wrappers (assembly)
├── doc/                # Documentation (existing)
└── Makefile            # Build system (existing, extend)
```

### Integration with Build System

The `Makefile` should be extended with a `lib` target:

```makefile
LIB_SRC = lib/src
LIB_INC = lib/include
LIB_OUT = build/lib

lib: all
	@mkdir -p $(LIB_OUT)
	$(BIN_DIR)/cc45 -I$(LIB_INC) $(LIB_SRC)/stdlib.c -o $(LIB_OUT)/stdlib.s
	$(BIN_DIR)/ca45 $(LIB_SRC)/crt0.s -o $(LIB_OUT)/crt0.o
	$(BIN_DIR)/ca45 $(LIB_SRC)/string.s -o $(LIB_OUT)/string.o
	# ... additional modules
```

### Include Path Convention

`cc45` automatically searches `lib/include` relative to the compiler binary for standard headers. Additional search paths can be added with `-I<path>`:

```bash
cc45 myprogram.c -o myprogram.s              # finds <stdbool.h> etc. automatically
cc45 -Imy/headers myprogram.c -o myprogram.s  # adds custom search path
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

Add `test-stdlib` to the `test` target's dependency list once the stdlib is implemented.

## 10. Implementation Priority

Recommended order based on utility and dependency:

1. **Phase 1 — Headers only** (no linkage needed): `stdint.h`, `stddef.h`, `stdbool.h`, `limits.h` — **Done.**
2. **Phase 2 — Core runtime**: `crt0.s`, `putchar`/`puts`, `exit` (enables visible output and clean termination) — **Done.** CRT pragmas (`exit_rts`/`exit_halt`/`exit_brk`, `no_bssinit`, `no_0100_stack`) implemented.
3. **Phase 3 — String/memory**: `string.h` functions (12 functions, all hand-written 45GS02 assembly, mmemu-validated) — **Done.**
4. **Phase 4 — Utilities**: `ctype.h` (7 functions, all hand-written 45GS02 assembly) — **Done.** Remaining: `atoi`/`itoa`, `abs`, `rand`
5. **Phase 5 — Heap**: `malloc`/`free`/`calloc`/`realloc` (complex, defer until needed)

## 11. Compiler Prerequisites

The following `cc45` features are required and already implemented:

- Pointer arithmetic and dereferencing
- `typedef` for type aliases
- `#define` / `#include` via preprocessor (`cp45`)
- `__asm__()` for KERNAL calls
- `signed`/`unsigned` qualifiers
- Arrays with subscript indexing
- String literals (PETSCII, pooled in `.data`)
- Multiple source file assembly (via `-I` includes)

Features that would benefit stdlib but are **not yet available**:

- **`static`**: Needed for file-scoped globals (e.g., heap state, rand seed). Workaround: use unique prefixed global names.
- **`const`**: Useful for ROM-able lookup tables. Workaround: place in `.data` segment.
- **`extern`**: Required for true multi-module linking. Workaround: single-file inclusion or assembler-level `.include`.
- **Function pointers**: Would enable callback-style APIs. Not critical for initial stdlib.
