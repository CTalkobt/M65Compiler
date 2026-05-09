# MEGA65 C Compiler Suite — Codebase Documentation

**Status:** v0.99.dev (v1.0 release ready, target: 2026-05-31)  
**Last Updated:** 2026-05-08  
**Maintainer:** Craig Taylor (CTalkobt)

---

## Project Overview

The MEGA65 C Compiler Suite is a modern toolchain for developing 6502-compatible code on the MEGA65 (45GS02-based) home computer. It provides:

- **cc45** — Optimizing C compiler with support for 32-bit long, volatile, const qualifiers, and inline assembly
- **ca45** — 45GS02 assembler with support for relocatable objects, stack-relative addressing, and sophisticated expression evaluation
- **ln45** — Linker that produces PRG executables or flat binaries from `.o45` relocatable objects
- **ar45** — Archiver for creating `.lib` static library archives
- **nm45** — Symbol table inspector for `.o45` and `.o65` object files
- **objdump45** — Object file disassembler with symbolic annotation
- **cp45** — C preprocessor

## Architecture

### Compilation Pipeline

```
C Source
   ↓
cc45 (Lexer → Parser → AST → Optimizer → CodeGenerator)
   ↓
Assembly (or .o45 relocatable object with -c)
   ↓
ca45 (Assembly → Binary / .o45 object with -c)
   ↓
ln45 (Links .o45 objects and .lib archives)
   ↓
PRG Executable or Flat Binary
```

### Key Design Decisions

1. **Calling Conventions**: Two modes supported:
   - **Stack convention** (default): Parameters on stack, return value in AXYZ (for long)
   - **ZP calling convention** (-fzpcall): Parameters in fixed ZP region, faster, with per-function clobber tracking

2. **Optimization Framework**:
   - Constant propagation with branch-aware state merging
   - Dead variable elimination
   - Strength reduction (e.g., multiply by power of 2 → bit shift)
   - Loop-result elimination

3. **Symbol Scoping**: Hierarchical scoping with nested procedures and blocks, allowing label/variable reuse without namespace pollution

4. **Object Format**: Custom `.o45` relocatable object format with:
   - Symbol table with relocation info
   - Function attributes (calling convention, clobber masks)
   - Stack frame metadata

### Key Files and Directories

```
src/
  compiler/          # cc45 implementation (C++17)
  assembler/         # ca45 implementation (C++17)
  linker/            # ln45 implementation (C++17)
  archiver/          # ar45 implementation (C++17)
  preprocessor/      # cp45 implementation (C++17)
  common/            # Shared code (Object format, IR structures)
  test-resources/    # Test suites and examples
  
lib/
  src/               # Assembly source for stdlib45.lib (hand-written 45GS02 asm)
  include/           # C headers (limits.h, stdio.h, string.h, stdlib.h, etc.)
  
doc/
  *.md               # Tool-specific documentation
  opcodes.md         # 45GS02 instruction reference
  lib45.md           # .o45 relocatable object format specification
  
Makefile            # Build configuration, test targets, install targets
.plan/              # Development plans and tracking
  completed.md      # Finished tasks (v1.0 blockers and should-fix items)
  todo.md           # Future work and optimization roadmap
```

## Build System

### Targets

- `make` or `make all` — Build all toolchain binaries and libraries
- `make test` — Run all compiler and assembler unit tests
- `make test-mmemu` — Run emulator-based execution tests (requires mmemu-cli)
- `make clean` — Remove build artifacts
- `make install` — Install to `/usr/local` (may require sudo)
- `make install_local` — Install to `~/.local`
- `make man` — Generate man pages from markdown (requires pandoc)

### Dependencies

- C++17 compiler (g++, clang++)
- GNU Make
- (Optional) mmemu-cli for emulator-based tests
- (Optional) pandoc for man page generation

## Testing

### Test Infrastructure

Tests are organized by compiler/assembler unit and validation level:

```
src/test-resources/
  test_compiler.sh       # C compiler unit test driver
  test_assembler.sh      # Assembler unit test driver
  validation/            # Error condition validation tests (Units 1-8)
  examples/              # Practical examples with makefiles
  *.c / *.s              # Individual test cases
```

### Test Categories

1. **Compiler Unit Tests**: C source → assembly validation
2. **Assembler Unit Tests**: Assembly → binary validation
3. **Validation Tests**: Error conditions and semantic constraints
4. **Execution Tests**: Emulator-based output validation

### Running Tests

```bash
make test                # All unit tests (no emulator required)
make test-mmemu          # Full suite including execution tests
make clean && make test  # Clean rebuild and test
```

## Language Features

### Implemented (v0.99.dev)

- **Types**: `char`, `short`, `int`, `long`, `unsigned` variants, pointers, arrays, structs, unions, function pointers
- **Qualifiers**: `const`, `volatile`, `static` (all orderings), `register`, `extern`
- **Declarations**: Function declarations, variable declarations, typedefs, type qualifiers in any order
- **Initializers**: Scalar, aggregate, designated initializers (`{.x=1}`, `{[2]=3}`), flexible array members
- **Operators**: All C arithmetic, logical, bitwise, comparison, ternary, cast, sizeof, alignof, etc.
- **Control Flow**: if/else, while, do-while, for, switch/case, break, continue, return, goto
- **Inline Assembly**: `asm("...")` and `__asm__("...")`
- **Pragmas**: `#pragma once`, `#pragma clobbers` (metadata)
- **Compound Literals**: `(int){42}`, `(struct Point){1,2}`
- **Bitfields**: `struct S { int x:4; unsigned y:4; }`
- **Alignment**: `_Alignas(N)` for locals and struct members
- **Preprocessor**: Full C99 preprocessor support (cc45 -E or cp45 standalone)

### Not Implemented (v1.0 Won't-Fix)

- `float`, `double` (no FPU; software float is post-1.0)
- `inline` function expansion (accepted as keyword, calls normally)
- `_Bool` and `_Complex` types

## Standard Library

### Provided Headers

- **`stdio.h`**: `printf`, `sprintf`, `puts`, `getchar`, `putchar`
- **`stdlib.h`**: `malloc`, `free`, `exit`, `strtol`, `strtoul`
- **`string.h`**: `strlen`, `strcpy`, `strcmp`, `strcat`, `strchr`, `strstr`, `strtok`, `strncat`, `strpbrk`, `strspn`, `strcspn`, `memcpy`, `memset`, `memmove`, `memcmp`
- **`ctype.h`**: `toupper`, `tolower`, `isalpha`, `isdigit`, `isxdigit`, `islower`, `isupper`, `ispunct`, `isblank`, `iscntrl`, `isalnum`, `isspace`, `isgraph`, `isprint`
- **`limits.h`**: `CHAR_BIT`, `INT_MAX`, `LONG_MAX`, `UINT_MAX`, `ULONG_MAX`, etc.
- **`stddef.h`**: `size_t`, `ptrdiff_t`, `NULL`, `offsetof`
- **`assert.h`**: `assert(expr)` macro

### Calling Convention Support

All stdlib functions support both stack and ZP calling conventions:
- Hand-written 45GS02 assembly for performance-critical functions
- ZP temps ($20-$2A) are safe from clobbering in ZP-convention code
- Proper clobber tracking in `.o45` objects

## Assembler Capabilities

### Addressing Modes

- Immediate: `#$20`
- Absolute: `$D000`
- Absolute, X-indexed: `$D000, X`
- Zero Page (8-bit): `$20`
- Zero Page, X-indexed: `$20, X`
- Indirect: `($20)`
- Indirect, X-indexed: `($20, X)`
- Indirect, Y-indexed: `($20), Y`
- Stack-relative: `$20, S` (TSX-based offset computation)
- Frame-relative: `.fp var_name` (frame-pointer-relative with symbol lookup)
- Indexed with register pairs: `$1000, XY` (45GS02 specific)

### Directives

- **Layout**: `* = address`, `.align`, `.byte`, `.word`, `.long`, `.ascii`, `.asciiz`, `.fill`, `.space`
- **Symbols**: `.label`, `.global`, `.extern`, `.const`, `.data`
- **Flow Control**: `.if`, `.else`, `.endif`, `.ifndef`, `.ifdef`, `.defined(symbol)`
- **Metadata**: `.cpu`, `.o45`, `.func_flags`, `.reg_clobbers`, `.flag_clobbers`, `.zp_uses`, `.zp_clobbers`
- **Equates**: `SYMBOL = expr`

### Simulated Operations

- `MOVE src, dest` (2-operand) and `MOVE src, dest, len` (3-operand)
- `FILL dest, len` (2-operand) and `FILL dest, len, value` (3-operand)
- Automatically expanded to appropriate 45GS02 instruction sequences

### Expression Evaluation

- Arithmetic: `+`, `-`, `*`, `/`, `%`, `**` (exponentiation)
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Ternary: `? :`
- Special: `defined(symbol)`, `$` (current address), address arithmetic

## Known Limitations & Future Work

### Phase 2: Fine-Grained Register Invalidation

Partially implemented. Phase 1 (accurate clobber tracking) is complete and emits `.reg_clobbers` / `.flag_clobbers` directives. Future phases:
- Phase 2: Selective invalidation at call sites (track which regs can be reused)
- Phase 3: Emit `.func_flags leaf` for leaf functions
- Phase 4: Header annotations for inter-TU optimization
- Phase 5: Assembler optimizer uses clobber info at JSR

### Calling Convention Safety

Planned (not critical for v1.0):
- Per-function calling convention attribute in `.o45`
- Linker-generated convention thunks on mismatch (stack ↔ ZP)
- `--no-thunks` flag and `-Wthunk` warning

### Debugging & Introspection

Not implemented:
- DWARF debug info
- Source-level debugging

## Development Guidelines

### Code Style

- C++17, no external dependencies (self-contained toolchain)
- Visitor pattern for AST traversal and code generation
- Context objects for semantic analysis (type tracking, scope management)
- Clear separation: Parser (syntax) → Validator (semantics) → CodeGenerator (emission)

### Adding Features

1. **Parser changes**: Extend grammar in `Parser.cpp`, add AST nodes in `ASTNode.h`
2. **Semantic validation**: Add checks in `Validator.cpp` or specialized visitors
3. **Code generation**: Add emission logic in `CodeGenerator.cpp` or visitors
4. **Testing**: Add unit tests in `src/test-resources/`, update `test_compiler.sh` or `test_assembler.sh`
5. **Documentation**: Update relevant `.md` in `doc/` and this file

### Common Tasks

#### Adding a Standard Library Function

1. Implement in `lib/src/func_name.s` (hand-written 45GS02 asm or `func_name.c`)
2. Declare in `lib/include/<header>.h`
3. Add export to `lib/Makefile` or `.o45` archive
4. Add test in `src/test-resources/test_*.c`
5. Document in `doc/stdlib.md`

#### Adding a Compiler Feature

1. Update lexer/parser if needed
2. Add AST node if introducing new syntax
3. Add validation rules in `Validator.cpp`
4. Implement code generation in `CodeGenerator.cpp`
5. Add unit test with expected output
6. Update CHANGELOG.md

#### Running a Single Test

```bash
cd src/test-resources
../scripts/test_compiler.sh test_name.c
```

## Debugging Tips

### Compiler Output

```bash
./bin/cc45 -S input.c -o input.s    # Generate assembly (useful for inspection)
./bin/cc45 -C input.c               # Print AST to stderr during parsing
./bin/cc45 input.c 2>&1 | head -50  # First 50 lines of output/errors
```

### Assembler Output

```bash
./bin/ca45 input.s -o output.prg          # Assemble to binary
./bin/ca45 -c input.s -o output.o45       # Generate relocatable object
./bin/nm45 output.o45                     # Inspect symbol table
./bin/objdump45 -d output.o45             # Disassemble
./bin/objdump45 -r output.o45             # Show relocations
```

### Object Format Inspection

The `.o45` relocatable object format is documented in `doc/lib45.md`. Key sections:
- Header with file signature and section offsets
- Symbol table (name, address, scope, type)
- Relocation table (address, type, symbol reference, addend)
- Text/data/zero-page sections with code/data bytes

## Release Checklist (v1.0)

- [x] All v1.0 blockers resolved
- [x] All v1.0 should-fix items implemented
- [x] Comprehensive test coverage (Units 1-8 validation tests)
- [x] Documentation updated (README, tool docs, stdlib, opcodes)
- [ ] Final QA on target hardware (MEGA65 emulator)
- [ ] Release notes prepared
- [ ] Version bumped to v1.0 in code and documentation

## References

- **MEGA65 Hardware**: https://github.com/MEGA65/mega65-core
- **45GS02 CPU**: Extended 6502 with Q register (AXYZ) and 32-bit operations
- **Test Coverage**: 176 assembler validation tests (Units 1-7), semantic/parser error tests
- **Standards**: C99 preprocessor, C89/C99 subset for language features

---

**For the latest status, see:**
- ROADMAP.md — Current work and release timeline
- CHANGELOG.md — Recent changes and commits
- .plan/todo.md — Future optimizations and research items
