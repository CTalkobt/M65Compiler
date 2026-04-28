# Mega65 C Compiler Suite (cc45, ca45, nm45)
*CONSIDER YOURSELF WARNED:* This project is currently undergoing heavy development


This project provides a modern, object-oriented development toolchain for the MEGA65 (45GS02) home computer. It consists of:

- **cc45** — C compiler (C source to assembly or `.o45` relocatable objects)
- **ca45** — 45GS02 assembler (assembly to binary or `.o45` relocatable objects)
- **nm45** — Symbol lister for `.o45` object files
- **cp45** — C preprocessor (standalone)


## Project Intent
The goal of this suite is to provide a C-based development environment that leverages the advanced features of the MEGA65, such as its 32-bit registers (Q-mode), 28-bit flat memory addressing, and stack-relative addressing. Unlike traditional 6502 compilers, this toolchain is designed from the ground up to support modern calling conventions and procedural abstractions directly at the assembly level.

## Architecture
The compilation process follows a multi-pass pipeline:
1.  **cc45 (Source to Assembly)**: Translates C source code into high-level 45GS02 assembly. It performs lexical analysis, builds an Abstract Syntax Tree (AST), and generates assembly code using an optimized procedure system.
2.  **ca45 (Assembly to Binary)**: Processes the assembly output, resolves symbols, manages stack offsets for procedures, and emits the final machine code binary.

## Design Philosophy
- **Procedural Abstraction**: Function calls are treated as first-class citizens with named parameters and automated stack cleanup.
- **Hierarchical Scoping**: Supports nested local scopes for labels and variables within procedures and `{}` blocks, preventing namespace pollution and allowing safe label reuse.
- **Optimizations**: Includes advanced compiler optimizations such as:
    - **Strength Reduction**: Converts expensive operations (e.g., multiplication by a power of 2) into faster equivalents (e.g., bit shifts).
    - **Constant Propagation**: Substitutes variables with known constant values into expressions at compile time, enabling further folding and simplification.
    - **Dead Variable Elimination**: Removes stack allocation and initialization for local variables that are initialized with constants and not subsequently used in the function.
- **Volatile Keyword Support**: The `volatile` keyword is fully supported, preventing unintended compiler optimizations on variables that may be changed by external factors (e.g., hardware, interrupts).
- **MEGA65 First**: Special emphasis is placed on supporting the 45GS02 instruction set enhancements, including a high-level expression engine (`expr`) that handles constant folding and register arithmetic.
- **Compatibility**: Supports KickAssembler-style syntax (comments, `* =`, `.cpu`) for easier porting of existing MEGA65 codebases.
- **Inline Assembly**: Standard C `asm()` and `__asm__()` support for direct hardware control from C source.
- **Explicit Casts & Narrowing Warnings**: C-style cast expressions `(type)expr` with compile-time warnings for implicit narrowing conversions.
- **Extensibility**: The toolchain uses a visitor-based architecture in C++, making it easy to add new optimizations, language features, or hardware targets.

## General Usage

### Direct to Binary
Compile C to assembly, then assemble to a PRG:
```bash
./bin/cc45 input.c -o output.s          # Compile to assembly
./bin/ca45 output.s -o output.prg       # Assemble to PRG binary
```

### Relocatable Objects (multi-file workflow)
Compile C files to `.o45` relocatable objects for separate compilation and linking:
```bash
./bin/cc45 -c main.c -o main.o45        # Compile to object
./bin/cc45 -c math.c -o math.o45        # Compile to object
./bin/nm45 main.o45                     # Inspect symbols
# ./bin/ln45 main.o45 math.o45 -o app.prg  # Link (future)
```

Assembly files can also produce `.o45` objects directly:
```bash
./bin/ca45 -c module.s -o module.o45    # Assemble to object
```

For a full list of command-line options, use the `-?` flag:
```bash
./bin/cc45 -?
./bin/ca45 -?
./bin/nm45 -?
```

## Documentation
In addition to the Markdown files in the `doc/` directory, you can generate and view standard Unix man pages for both tools:

```bash
make man
man ./man/cc45.1
man ./man/ca45.1
```

Note: This requires `pandoc` to be installed on your system.

## Testing & Regressions
The suite includes a set of automated tests to verify the compiler and assembler. You can run all tests from the project root:
```bash
make test
```
This script compiles C source files from `src/test-resources/` and assembles them to ensure stability and correctness. Outputs are stored in the `build/test/` directory.

For detailed information on each tool, refer to:
- [doc/cc45.md](doc/cc45.md) — Compiler Usage and Features
- [doc/ca45.md](doc/ca45.md) — Assembler Syntax and Reference
- [doc/nm45.md](doc/nm45.md) — Symbol Lister for `.o45` Object Files
- [doc/cp45.md](doc/cp45.md) — Preprocessor Features and Usage
- [doc/opcodes.md](doc/opcodes.md) — 45GS02 Instruction Reference
- [lib.md](lib.md) — `.o45` Relocatable Object Format Specification
- [doc/ln45.md](doc/ln45.md) — Linker Design
