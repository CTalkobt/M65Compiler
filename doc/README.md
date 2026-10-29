# MEGA65 Compiler Suite Documentation

This directory contains the documentation for the MEGA65 C compiler suite (targeting the 45GS02).

## Directory Structure

Documents are separated by their function and scope:

- **[`bin/`](bin/)**: Program-specific user guides, syntax references, and CLI manuals for the toolchain binaries.
- **[`architecture/`](architecture/)**: Design specifications, calling conventions, file formats, the standard library and compiler development details (coverage, benchmarking, Docker).
  - **[`architecture/bug/`](architecture/bug/)**: Technical bug reports, root cause analyses, and solutions.
- **[`examples/`](examples/README.md)**: The example programs, each described with a link to its source and a screenshot.
- **[`ref/`](ref/README.md)**: Release checklist and planning documents.
- **`archive/`**: Historical session notes and phase reports (not maintained).

## Tools

| Tool | Purpose | Manual |
|------|---------|--------|
| `cc45` | C compiler (C source to assembly, `.o45` object or linked `.prg`) | [bin/cc45.md](bin/cc45.md) |
| `cp45` | C preprocessor (the `cc45` binary run under the name `cp45`) | [bin/cp45.md](bin/cp45.md) |
| `ca45` | 45GS02 assembler | [bin/ca45.md](bin/ca45.md) |
| `ln45` | Linker (`.o45` + `.lib` to PRG or flat binary) | [bin/ln45.md](bin/ln45.md) |
| `ar45` | Archiver for `.lib` static libraries | [bin/ar45.md](bin/ar45.md) |
| `nm45` | Symbol lister for `.o45`/`.o65` objects | [bin/nm45.md](bin/nm45.md) |
| `pack45` | Packer for the library's `unpack()` (`<unpack.h>`) | [bin/pack45.md](bin/pack45.md) |
| `objdump45` | Object file disassembler | [bin/objdump45.md](bin/objdump45.md) |
| `disk45` | CBM disk/tape image utility | [bin/disk45.md](bin/disk45.md) |
| `basic45` | BASIC program generator | [bin/basic45.md](bin/basic45.md) |
| `asmCvt45` | Assembly source format converter | [bin/asmCvt45.md](bin/asmCvt45.md) |

Shared configuration files (`~/.config/m65/<tool>.conf`): [bin/CONFIGURATION.md](bin/CONFIGURATION.md).

## Examples

- [Example programs](examples/README.md) — games and demos (Space Invaders, a Mandelbrot viewer, a music generator, DMA scrollers), hardware registers, BASIC calling C; each with its screen and a link to its source

## Reference Documents

- [Calling conventions](architecture/calling-conventions.md) — ZP (default), stack and static-allocation (SAC) conventions
- [.o45 object format](architecture/lib45.md)
- [Intermediate representation](architecture/ir.md)
- [Optimizations](architecture/optimizations.md)
- [45GS02 opcodes](architecture/opcodes.md)
- [Standard library](architecture/stdlib.md) and [CBM KERNAL interface](architecture/stdcbm.md)
- [Striped arrays](architecture/striped-arrays.md)
- [Cross-module optimization](architecture/cross-module-optimization.md) and [link-time ZP calling](architecture/link-time-zp-calling.md)
- Development: [Docker](architecture/DOCKER.md), [coverage](architecture/COVERAGE_ANALYSIS.md), [O45 I/O tests](architecture/O45_IO_COVERAGE_TESTS.md), [benchmarking](architecture/PERFORMANCE_VALIDATION.md), [stdlib implementation notes](architecture/stdlib_optimizations.md), [project history](architecture/evolve.md)

## Building, Testing, Installing

Run from the project root:

```bash
make                 # all tools (bin/) and the libraries (lib/build/c45.lib, c45_zp.lib, c45_stack.lib)
make test            # documentation check, unit/validation tests and the xemu-xmega65 runtime tests
make install         # PREFIX=/usr/local by default
make install_local   # same with PREFIX=~/.local
make man             # man pages from doc/bin/*.md (needs pandoc)
```

`make install` puts the tools in `PREFIX/bin` (`cp45` as a symlink to `cc45`), the three libraries in `PREFIX/lib/cc45` and the headers in `PREFIX/include/cc45`. The runtime tests in `make test` (`src/test/test_xemu.sh`, `test_stdlib_new.sh`, `test_examples.sh`) need `xemu-xmega65` on the `PATH`.

---

## Documentation Specification (Rules & Conventions)

All documentation must follow these directory rules to keep it clean and avoid broken links. `src/test/check_docs.py` enforces them as the first step of `make test`.

### 1. File Placement & Naming
- **No files in the root `doc/` directory** other than this `README.md`. The only subdirectories are `bin/`, `architecture/`, `examples/`, `ref/` and `archive/`.
- **`doc/examples/`** contains `README.md` (the examples) and `images/` (their screenshots, `.png`, made by `examples/capture_screens.py`).
- **`doc/bin/`** contains only documentation for toolchain binaries. Allowed files:
  - `ar45.md` (Archiver)
  - `basic45.md` (BASIC Generator)
  - `ca45.md` (Assembler)
  - `cc45.md` (Compiler)
  - `cp45.md` (Preprocessor)
  - `asmCvt45.md` (Format Converter)
  - `disk45.md` (Disk Utility)
  - `ln45.md` (Linker)
  - `nm45.md` (Symbol Lister)
  - `objdump45.md` (Object Disassembler)
  - `pack45.md` (Packer)
  - `CONFIGURATION.md` (Shared configuration file specification)
- **`doc/architecture/`** contains only markdown files (`.md`); the only allowed subdirectory is `bug/`.
- **`doc/architecture/bug/`** contains only bug reports and diagnostic logs ending in `.md`.

### 2. Relative Link Guidelines
Links between files must use correct relative paths:
- **Inside `doc/bin/` linking to `doc/architecture/`**: use the `../architecture/` prefix, for example `[lib45]``(../architecture/lib45.md)`.
- **Inside `doc/architecture/` linking to `doc/bin/`**: use the `../bin/` prefix, for example `[cc45]``(../bin/cc45.md)`.
- **Links within one directory**: use only the filename, for example `[stdcbm]``(stdcbm.md)` inside `stdlib.md`.

Every relative markdown link in `doc/` must resolve to an existing file.

---

## Automated Verification
Run the validation script from the project root:
```bash
python3 src/test/check_docs.py
```
It is run automatically as part of `make test`.
