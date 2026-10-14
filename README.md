# Mega65 C Compiler Suite v1.0.6

A modern development toolchain for the MEGA65 (45GS02) home computer. It consists of:

- **cc45** — C compiler (C source to assembly or `.o45` relocatable objects)
- **ca45** — 45GS02 assembler (assembly to binary or `.o45` relocatable objects)
- **ln45** — Linker (links `.o45` objects and `.lib` archives into flat binaries or PRG executables)
- **ar45** — Archiver (creates `.lib` static library archives from `.o45` objects)
- **nm45** — Symbol lister for `.o45` / `.o65` object files
- **objdump45** — Object file inspector and 45GS02 disassembler
- **cp45** — C preprocessor (standalone)
- **disk45** — CBM disk, tape and archive image utility (D64/D71/D81/D65 and 20 more formats)
- **basic45** — BASIC 65 program generator (label-based source to a tokenized `.prg`)
- **cvt_asm** — Assembly source converter between assembler dialects


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
    - **Constant Propagation**: Substitutes variables with known constant values into expressions at compile time. The optimizer is **branch-aware**, performing constant state merging across `if`/`else` paths to ensure safe invalidation on reassignments.
    - **ZP-Safe Standard Library**: All standard library functions are designed to be disjoint from the compiler's temporary register pool ($02-$0A). Functions utilize the 45GS02 hardware stack and the Z-scratch restoration pattern (`plz/stz`) to preserve both zero page integrity and 32-bit return values (`AXYZ`).

    - **Dead Variable Elimination**: Removes stack allocation and initialization for local variables that are initialized with constants and not subsequently used in the function.
- **Volatile Keyword Support**: The `volatile` keyword is fully supported, preventing unintended compiler optimizations on variables that may be changed by external factors (e.g., hardware, interrupts).
- **MEGA65 First**: Special emphasis is placed on supporting the 45GS02 instruction set enhancements, including a high-level expression engine (`expr`) that handles constant folding and register arithmetic.
- **Compatibility**: Supports KickAssembler-style syntax (comments, `* =`, `.cpu`) for easier porting of existing MEGA65 codebases.
- **Inline Assembly**: Standard C `asm()` and `__asm__()` support for direct hardware control from C source.
- **Compound Literals**: C99 compound literals for anonymous temporary values: `(struct Point){1, 2}`, `(int){42}`, `(int[]){1,2,3}`.
- **Bitfields**: C-standard bitfield support in structs and unions with automatic packing into 8-bit or 16-bit storage units. Optimized codegen uses 45GS02 TRB/TSB instructions for atomic read-modify-write on ZP and absolute targets.
- **32-bit Long Type**: Native `long` and `unsigned long` support using the 45GS02 Q register (AXYZ). All arithmetic, comparison, shift, and bitwise operations leverage native Q instructions.
- **Explicit Casts & Narrowing Warnings**: C-style cast expressions `(type)expr` with compile-time warnings for implicit narrowing conversions.
- **ZP Calling Convention**: Optional zero-page parameter passing (`-fzpcall`) for faster function calls. Parameters are placed in a fixed ZP region instead of the stack, eliminating TSX overhead. Includes caller-save semantics, address-of spill, and per-function metadata (`.zp_uses`, `.zp_clobbers`, `.reg_clobbers`) in `.o45` objects for interprocedural optimization.
- **Extensibility**: The toolchain uses a visitor-based architecture in C++, making it easy to add new optimizations, language features, or hardware targets.

## General Usage

### One C file to a program
```bash
./bin/cc45 hello.c -o hello.prg         # compile, assemble and link in one step
xemu-xmega65 -prgmode 65 -prg hello.prg # run it (or load it on a MEGA65 and RUN)
```
The `.prg` starts with a `10 SYS` BASIC line, so `RUN` starts it. cc45 compiles
one file per run; it links the standard library (`c45.lib`, whose `crt0` is the
startup code) found next to the tools. `-O0` to `-O4` set the optimization
level (`-O2` is the default), `-fno-zpcall` selects the stack calling
convention (and `c45_stack.lib`).

### Several files: compile, then link
```bash
./bin/cc45 -c main.c -o main.o45                            # relocatable objects
./bin/cc45 -c math.c -o math.o45
./bin/ca45 -c module.s45 -o module.o45                      # assembly works the same way
./bin/ln45 -basic -o app.prg main.o45 math.o45 module.o45 lib/build/c45.lib

# Or put modules in a library; ln45 pulls only the members it needs
./bin/ar45 c math.lib math.o45
./bin/ln45 -basic -o app.prg main.o45 math.lib lib/build/c45.lib
```
`ln45 -basic` writes the BASIC `SYS` line and links `crt0` from the library;
`-m` prints a symbol map. `CC45_LIB` lets ln45 find libraries named with `-l`.

### Inspecting
```bash
./bin/cc45 -S main.c -o main.s45        # the generated assembly (for reading)
./bin/nm45 -f main.o45                  # symbols and function attributes
./bin/objdump45 -d main.o45             # disassembly
./bin/objdump45 -bas app.prg            # the BASIC lines of a .prg
```

### Assembly alone
```bash
./bin/ca45 hello.s45 -o hello.prg       # .basicUpstart in the source writes the SYS line
```
A name ending in `.prg` gets a load address (from the first `.org`, or `$2001`
after `.basicUpstart`); any other name is a flat binary.

### BASIC 65 programs
```bash
./bin/basic45 game.bas -o game.prg                     # labels instead of line numbers
./bin/basic45 --link routines.c menu.bas -o menu.prg   # BASIC calling C functions
./bin/basic45 game.bas --disk games.d81 --emulator     # onto a disk image, run in xemu
```

### Disk images
```bash
./bin/disk45 create work.d81 -n WORK
./bin/disk45 add work.d81 app.prg app
./bin/disk45 list work.d81
```

Every tool prints its options with `-?` (`./bin/cc45 -?`; disk45 when run
without arguments); the reference pages are in [doc/bin/](doc/bin/).

## Installation

Install the toolchain binaries, standard library, C headers, and man pages:

```bash
make install                          # Install to /usr/local (may need sudo)
make install_local                    # Install to ~/.local
make install PREFIX=/opt/mega65       # Install to a custom prefix
make uninstall                        # Remove installed files
```

The install targets build everything (including `lib`) automatically. Installed layout:

| Component | Path |
|-----------|------|
| Binaries | `$(PREFIX)/bin/` |
| Libraries (`c45.lib`, `c45_zp.lib`, `c45_stack.lib`, `crt0*.o45`) | `$(PREFIX)/lib/cc45/` |
| C headers | `$(PREFIX)/include/cc45/` |
| Man pages | `$(PREFIX)/share/man/man1/` |

After `make install_local`, ensure `~/.local/bin` is on your `PATH`.

The tools find these relative to their own location, so no environment
variables are needed: cc45/cp45 take the standard headers from
`bin/../lib/include` (the build tree) or `bin/../include/cc45` (installed),
and cc45 and ln45 take the libraries from `bin/../lib/build` or
`bin/../lib/cc45`. `CC45_INCLUDE` and `CC45_LIB` add directories in front of
these. Keep the default `BINDIR`/`LIBDIR`/`INCDIR` (relative to `PREFIX`);
with others, set `CC45_INCLUDE` and `CC45_LIB`.

## Documentation
In addition to the Markdown files in the `doc/` directory (index: [doc/README.md](doc/README.md)), you can generate and view standard Unix man pages for every tool:

```bash
make man
man ./man/cc45.1
man ./man/basic45.1
```

Note: This requires `pandoc` to be installed on your system.

## Testing & Regressions
The suite includes a set of automated tests to verify the compiler and assembler. You can run all tests from the project root:
```bash
make test
```
This runs the compiler, assembler and linker unit tests, the validation units and the examples (no emulator). Outputs are stored in the `build/test/` directory.

The runtime tests run on the [xemu](https://github.com/lgblgblgb/xemu) MEGA65 emulator (`xemu-xmega65` on your `PATH`):

```bash
bash src/test/test_xemu.sh          # runtime tests, both calling conventions
bash src/test/run_gte_all.sh        # GCC torture tests
bash src/test/test_cc_interop.sh    # calling-convention interop
```

For detailed information on each tool, refer to:
- [doc/bin/cc45.md](doc/bin/cc45.md) — Compiler Usage and Features
- [doc/bin/ca45.md](doc/bin/ca45.md) — Assembler Syntax and Reference
- [doc/bin/nm45.md](doc/bin/nm45.md) — Symbol Lister for `.o45` Object Files
- [doc/bin/objdump45.md](doc/bin/objdump45.md) — Object File Inspector and Disassembler
- [doc/bin/cp45.md](doc/bin/cp45.md) — Preprocessor Features and Usage
- [doc/architecture/opcodes.md](doc/architecture/opcodes.md) — 45GS02 Instruction Reference
- [doc/architecture/lib45.md](doc/architecture/lib45.md) — `.o45` Relocatable Object Format Specification
- [doc/bin/ln45.md](doc/bin/ln45.md) — Linker Usage and Reference
- [doc/bin/ar45.md](doc/bin/ar45.md) — Archiver Usage and Reference
- [doc/bin/disk45.md](doc/bin/disk45.md) — Disk/Tape/Archive Image Utility
- [doc/bin/basic45.md](doc/bin/basic45.md) — BASIC 65 Program Generator
- [doc/bin/cvt_asm.md](doc/bin/cvt_asm.md) — Assembly Dialect Converter
- [doc/bin/CONFIGURATION.md](doc/bin/CONFIGURATION.md) — Configuration Files and Environment
