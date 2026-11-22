# MEGA65 Compiler Suite Documentation

This directory contains the documentation for the MEGA65 C compiler suite (targeting the 45GS02).

## Directory Structure

Documents are separated by their function and scope:

- **[`bin/`](bin/)**: Program-specific user guides, syntax references, and CLI manuals for the toolchain binaries.
- **[`architecture/`](architecture/)**: Design specifications, calling conventions, file formats, the standard library and compiler development details (coverage, benchmarking, Docker).
  - **[`architecture/bug/`](architecture/bug/)**: Technical bug reports, root cause analyses, and solutions.
- **[`guide/`](guide/README.md)**: Tutorials for getting started (C with cc45, BASIC with basic45, the other tools), troubleshooting, a glossary and the editor support.
- **[`examples/`](examples/README.md)**: The example programs, each described with a link to its source and a screenshot.
- **[`ref/`](ref/README.md)**: Release checklist and planning documents.
- **`archive/`**: Historical session notes and phase reports (not maintained).

## Tools

| Tool | Purpose | Manual |
|------|---------|--------|
| `cc45` | C compiler (C source to assembly, `.o45` object or linked `.prg`) | [bin/cc45.md](bin/cc45.md) |
| `cp45` | C preprocessor (the `cc45` binary run under the name `cp45`) | [bin/cp45.md](bin/cp45.md) |
| `ca45` | 45GS02 assembler | [bin/ca45.md](bin/ca45.md) |
| `al45` | Assembly linter (the `ca45` binary run under the name `al45`) | [bin/al45.md](bin/al45.md) |
| `af45` | Assembly formatter (the `ca45` binary run under the name `af45`) | [bin/af45.md](bin/af45.md) |
| `cl45` | C linter (the `cc45` binary run under the name `cl45`) | [bin/cl45.md](bin/cl45.md) |
| `ln45` | Linker (`.o45` + `.lib` to PRG or flat binary) | [bin/ln45.md](bin/ln45.md) |
| `ar45` | Archiver for `.lib` static libraries | [bin/ar45.md](bin/ar45.md) |
| `nm45` | Symbol, attribute and relocation lister for `.o45` objects and `.lib` archives | [bin/nm45.md](bin/nm45.md) |
| `pack45` | Packer for the library's `unpack()` (`<unpack.h>`) | [bin/pack45.md](bin/pack45.md) |
| `font45` | Fonts: bitmap fonts to 8x8 charsets or `<font.h>` fonts | [bin/font45.md](bin/font45.md) |
| `tune45` | Music for the tune player (`<audio_tune.h>`): text or MIDI to data, playing, composing | [bin/tune45.md](bin/tune45.md) |
| `deploy45` | A program or disk image to a MEGA65 (Ethernet, JTAG serial) or an Ultimate 64 | [bin/deploy45.md](bin/deploy45.md) |
| `dbg45` | Source-level debugger for cc45 programs in xemu (VS Code through `--dap`) | [bin/dbg45.md](bin/dbg45.md) |
| `objdump45` | Object, PRG and binary inspector and disassembler | [bin/objdump45.md](bin/objdump45.md) |
| `disk45` | CBM disk/tape/archive image utility | [bin/disk45.md](bin/disk45.md) |
| `basic45` | BASIC 65 program generator | [bin/basic45.md](bin/basic45.md) |
| `asmCvt45` | Assembly source format converter | [bin/asmCvt45.md](bin/asmCvt45.md) |

Shared configuration files (`~/.config/m65/<tool>.conf`): [bin/CONFIGURATION.md](bin/CONFIGURATION.md).

## Getting Started

- [C with cc45](guide/getting-started-c.md) — a first program to a running one in xemu, on a disk and on a MEGA65; several files; what differs from other C compilers; the hardware; debugging
- [BASIC with basic45](guide/getting-started-basic45.md) — BASIC 65 without line numbers, disks, converting old programs, compiling to machine code, calling C
- [The other tools](guide/getting-started-tools.md) — the assembler, linker and archiver, the inspectors, the linters, disk images, packing, fonts, music, converting assembly, sending to hardware
- [Troubleshooting](guide/troubleshooting.md) — common messages and symptoms, their causes and fixes; setting xemu up with the MEGA65 ROM
- [Glossary](guide/glossary.md) — the terms these pages use, each linked to its reference
- [Editors and IDEs](guide/ide.md) — the language server and the VS Code extension, their settings, debugging in VS Code
- [Contributing](../CONTRIBUTING.md) — branches and pull requests, the tests, adding a test, the docs checks

## Examples

- [Example programs](examples/README.md) — games and demos (Space Invaders, a Mandelbrot viewer, a music generator, DMA scrollers), hardware registers, BASIC calling C; each with its screen and a link to its source

## Reference Documents

- [Calling conventions](architecture/calling-conventions.md) — ZP (default), stack and static-allocation (SAC) conventions; [quick reference](architecture/calling-conventions-quick-reference.md)
- [.o45 object format](architecture/lib45.md)
- [Intermediate representation](architecture/ir.md)
- [Optimizations](architecture/optimizations.md)
- [45GS02 opcodes](architecture/opcodes.md)
- [Standard library](architecture/stdlib.md) and [CBM KERNAL interface](architecture/stdcbm.md)
- [Striped arrays](architecture/striped-arrays.md)
- [Cross-module optimization](architecture/cross-module-optimization.md) and [link-time ZP calling](architecture/link-time-zp-calling.md) (a design proposal, not implemented)
- [Linters](architecture/lint.md) — the design of al45 and cl45 and their shared core
- [asmCvt45 conversion guide](architecture/asmCvt45-guide.md) — what each assembler dialect keeps, translates and loses
- [Benchmarks](architecture/benchmarks.md) — `make bench`: the 6502 C compiler benchmarks in xemu, beside the other compilers' published results
- Development: [Docker](architecture/DOCKER.md), [coverage](architecture/COVERAGE_ANALYSIS.md), [O45 I/O tests](architecture/O45_IO_COVERAGE_TESTS.md), [measuring code size](architecture/PERFORMANCE_VALIDATION.md), [stdlib implementation notes](architecture/stdlib_optimizations.md), [project history](architecture/evolve.md)
- Superseded or historical pages, kept for reference: [calling conventions, v1.0.4 snapshot](architecture/calling-conventions-v1.0.4-actual.md), [v1.0.5 calling-convention corrections](architecture/v1.0.5-calling-conventions-corrections.md), [phase 32 palette management](architecture/phase32-palette-management.md)

## Building, Testing, Installing

Run from the project root:

```bash
make                 # all tools (bin/) and the libraries (lib/build/c45.lib, c45_zp.lib, c45_stack.lib)
make test            # documentation check, unit/validation tests and the examples
make test-xemu       # the runtime tests in xemu-xmega65 (src/test/test_xemu.sh)
make install         # PREFIX=/usr/local by default
make install_local   # same with PREFIX=~/.local
make man             # man pages from doc/bin/*.md (needs pandoc)
```

`make install` puts the tools in `PREFIX/bin` (`cp45` and `cl45` as symlinks to `cc45`, `al45` and `af45` to `ca45`), the three libraries in `PREFIX/lib/cc45` and the headers in `PREFIX/include/cc45`. `make test-xemu` needs `xemu-xmega65` on the `PATH`; the steps of `make test` that run programs (`test_stdlib_new.sh`, `test_examples.sh`) use it when it is there and are skipped without it.

---

## Documentation Specification (Rules & Conventions)

All documentation must follow these directory rules to keep it clean and avoid broken links. `src/test/check_docs.py` enforces them as one of the steps of `make test`.

### 1. File Placement & Naming
- **No files in the root `doc/` directory** other than this `README.md`. The only subdirectories are `bin/`, `architecture/`, `examples/`, `ref/` and `archive/`.
- **`doc/guide/`** contains the tutorials, markdown only (`.md`, no subdirectories). Their commands are run by `src/test/test_guides.py` (`make test`): see [guide/README.md](guide/README.md).
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
  - `font45.md` (Fonts)
  - `deploy45.md` (Deployment to hardware)
  - `tune45.md` (Music for the tune player)
  - `dbg45.md` (Debugger)
  - `al45.md` (Assembly linter)
  - `af45.md` (Assembly formatter)
  - `cl45.md` (C linter)
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
