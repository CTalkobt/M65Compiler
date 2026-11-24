# Mega65 C Compiler Suite v1.1.0

Source, issues and releases: <https://github.com/CTalkobt/M65Compiler>

A modern development toolchain for the MEGA65 (45GS02) home computer. It consists of:

- **cc45** — C compiler (C source to assembly or `.o45` relocatable objects)
- **ca45** — 45GS02 assembler (assembly to binary or `.o45` relocatable objects)
- **libca45** — the assembler as a C++ library (`libca45.a`, `include/ca45/ca45.hpp`: `ca45::assemble`, `ca45::assembleLine`), which ca45 itself links; not built on macOS
- **al45** — Assembly linter: ca45's front end over a source file, `mega65-*` rules, no output (a link to ca45)
- **af45** — Assembly formatter: lays a source file out as al45's style rules describe, whitespace only (a link to ca45)
- **cl45** — C linter: cc45's front end and code generator over a file as it is compiled, no output (a link to cc45)
- **ln45** — Linker (links `.o45` objects and `.lib` archives into flat binaries or PRG executables)
- **ar45** — Archiver (creates `.lib` static library archives from `.o45` objects)
- **pack45** — Packer: data packed for the library's `unpack()` (DMA, to any 28-bit address, in place); `ln45 --pack-overlays` uses it too
- **font45** — Fonts: BDF, PSF, C64 charsets or hand-drawn glyphs to 8x8 text-mode charsets or `<font.h>`'s proportional fonts
- **tune45** — Music for the tune player: text (or MIDI) to data, play on the PC / in xemu / on a MEGA65, compose
- **deploy45** — A program or disk image to real hardware: a MEGA65 over Ethernet or the JTAG adapter's serial port (mega65-tools' etherload, m65, mega65_ftp), an Ultimate 64 over its HTTP API
- **dbg45** — Source-level debugger: a cc45 program in xemu by its C source (breakpoints, stepping, the stack, variables); VS Code through `--dap`
- **nm45** — Symbol, attribute and relocation lister for `.o45` objects and `.lib` archives (and ln45's `.ovd` / `.ov1` overlay files)
- **objdump45** — Object file inspector and 45GS02 disassembler (objects, PRGs, binaries, `.ovd` / `.ov1` overlay files)
- **cp45** — C preprocessor (a link to cc45, which preprocesses only under this name)
- **disk45** — CBM disk, tape and archive image utility (D64/D71/D81/D65 and 21 more formats)
- **basic45** — BASIC 65 program generator (label-based source to a tokenized `.prg`)
- **asmCvt45** — Assembly source converter between assembler dialects


## Project Intent
The goal of this suite is to provide a C-based development environment that leverages the advanced features of the MEGA65, such as its 32-bit registers (Q-mode), 28-bit flat memory addressing, and stack-relative addressing. Unlike traditional 6502 compilers, this toolchain is designed from the ground up to support modern calling conventions and procedural abstractions directly at the assembly level.

## Architecture
The compilation process follows a multi-pass pipeline (`cc45 file.c -o file.prg` runs all of it):
1.  **cc45 (Source to Assembly)**: Translates C source code into high-level 45GS02 assembly. It preprocesses, performs lexical analysis, builds an Abstract Syntax Tree (AST), lowers it to an intermediate representation that the optimizer works on, and generates assembly code using an optimized procedure system.
2.  **ca45 (Assembly to Object)**: Processes the assembly output, resolves symbols, manages stack offsets for procedures, and emits a relocatable `.o45` object (or, for assembly written by hand, a binary directly).
3.  **ln45 (Objects to Program)**: Links the objects with the members it needs of the standard library (`c45.lib`, whose `crt0` is the startup code) into a `.prg`.

## Design
- **The 45GS02's own instructions**: 32-bit `long` arithmetic on the Q register, 28-bit far pointers through
  `[bp],z`, the movable base page, the 16-bit stack, DMA for copies and fills; not 6502 code with a 45GS02
  label on it.
- **Fast calls by default**: arguments in the base page (the ZP convention), and static storage instead of a
  stack frame for every function that is not recursive (SAC), with ln45 overlaying the storage of functions
  never active at once; the stack convention for the rest, and ln45 bridging calls between the two
  ([calling conventions](doc/architecture/calling-conventions.md)).
- **A program that leaves the machine alone**: it runs in a base page of its own, so the ROM's page 0 is
  untouched, and returns to BASIC with its exit status.
- **Larger than bank 0**: code overlays and `__far` functions kept in banks 4-5 or attic RAM, brought in by
  DMA or MAP when called; `__bank(n)` data.
- **C, with extensions where the hardware wants them**: C99/C11 and the GCC extensions most code uses,
  plus `__zp`, `__bit`, `__far` / `__near` pointers, `__interrupt`, fixed point, struct methods and
  operator overloading, `__jsr` for ROM calls ([cc45](doc/bin/cc45.md)). One deliberate difference: plain
  `char`/`int`/`long` are unsigned unless declared `signed` (`-fsigned-default` for standard C).
- **One toolchain**: compiler, assembler, linker and librarian share one object format (`.o45`) with
  function attributes the linker checks; linters, a formatter, inspectors, a debugger, disk images, data
  packing, fonts and music are tools of the same suite. C++17; zlib bundled, SQLite and FUSE optional (for disk45).
- **Checked on the machine**: the runtime tests (over a thousand, each in both calling conventions) and the
  GCC torture tests run in the xemu MEGA65 emulator.

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

### Programs larger than bank 0: code overlays and `__far`
```c
__far int checksum(const char *p, int n) { ... }   /* kept in far memory (bank 5) */
```
```bash
./bin/cc45 -O2 -c main.c -o main.o45
./bin/cc45 -O2 -c tools.c -o tools.o45       # __far functions, or #pragma cc45 overlay(name)
./bin/ln45 -basic -M app.map -o app.prg main.o45 tools.o45 lib/build/c45.lib   # also writes app.ov1 (and app.ovd, which describes it)
./bin/disk45 create app.d81 && ./bin/disk45 add app.d81 app.prg APP && ./bin/disk45 add app.d81 app.ov1 APP.OV1
```
The program loads `APP.OV1` into far memory at start; a call into far code
goes through a resident stub that maps the code into a window
(`--overlay-map`; `__far` code always) or copies it into a region. See
[cc45: Code Overlays](doc/bin/cc45.md#code-overlays) and
[ln45: Code Overlays](doc/bin/ln45.md#code-overlays); examples in
`examples/c/overlays`, `mapped_overlays` and `far_functions`.

### Inspecting
```bash
./bin/cc45 -S main.c -o main.s45        # the generated assembly (for reading)
./bin/nm45 -f main.o45                  # symbols and function attributes
./bin/objdump45 -d main.o45             # disassembly
./bin/objdump45 -bas app.prg            # the BASIC lines of a .prg
```

### Assembly alone
```bash
./bin/ca45 hello.s45 -o hello.prg       # `.basicUpstart start` in the source writes the SYS line
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

Every tool prints its options with `-?` or `--help` (`./bin/cc45 -?`; also `-h`, but in
nm45 and objdump45, where `-h` shows headers) and its version with `-V` or `--version`,
wherever the flag is among the arguments. The reference pages are in [doc/bin/](doc/bin/).

## Download

Each release from the next one on has prebuilt archives on its
[release page](https://github.com/CTalkobt/M65Compiler/releases) (with a `SHA256SUMS` file):

| Archive | For |
|---|---|
| `m65compiler-<version>-linux-x86_64.tar.gz` | any Linux on x86-64: statically linked, no library to install |
| `m65compiler-<version>-linux-arm64.tar.gz` | any Linux on 64-bit ARM (a Raspberry Pi 4 or 5 with a 64-bit system): statically linked |
| `m65compiler-<version>-macos-arm64.tar.gz` | macOS on Apple silicon (not signed: clear the quarantine with `xattr -dr com.apple.quarantine <directory>` after unpacking a browser download) |

Unpack one anywhere and run the tools from its `bin/` (or put that on the `PATH`): they find the headers and
libraries relative to their own location.

```bash
tar -xzf m65compiler-<version>-linux-x86_64.tar.gz
m65compiler-<version>-linux-x86_64/bin/cc45 -O2 hello.c -o hello.prg
```

Or use the Docker image, which needs nothing but Docker (`linux/amd64` and `linux/arm64`; `:runtime` adds the
xemu emulator, see [doc/architecture/DOCKER.md](doc/architecture/DOCKER.md)):

```bash
docker run --rm -v "$PWD":/work --user $(id -u):$(id -g) ghcr.io/ctalkobt/m65compiler -O2 hello.c -o hello.prg
```

The Linux archives' disk45 has the catalog and `mount`, with SQLite and libfuse 3 linked in. Mounting uses the
system's FUSE: `/dev/fuse` and the `fusermount3` helper (the `fuse3` package), which `disk45 mount` asks for
when it is used and names when they are missing; nothing else in the archive needs them. The archives are made by `src/release/package.sh` (the release workflow runs it for each tag), which
also checks each one: unpacked in a directory of its own, its cc45 builds a program. tune45 contains
libresidfp (GPL-2+), disk45 zlib (the zlib licence) and the Linux disk45 libfuse 3 (LGPL-2.1); the licences are in the archive's `share/doc/m65compiler/`.

## Building from Source

The tools are C++17. To build them you need:

- a C++17 compiler: GCC 11 or newer, or clang (`make CXX=clang++`);
- GNU make 3.81 or newer;
- optionally SQLite's headers, for `disk45 catalog`, and FUSE 3 with `pkg-config`, for `disk45 mount`. The
  build detects the two and leaves the command out when one is missing (every tool is still built); its
  first lines say which are in, and the package that adds the other:

```
Configuration (g++; `make check-deps` for all of it):
  zlib (.gz, .zip images): bundled (third_party/zlib, built with gcc)
  disk45 catalog (SQLite): yes
  disk45 mount   (FUSE 3): no - to add it: sudo apt-get install libfuse3-dev pkg-config
```

Everything else is in the repository. `make check-deps` lists what the build and the tests need, what this
machine has, and the command that installs what is missing on this distribution; without the compiler,
`make` stops at once with that command. `make HAVE_SQLITE=` (or `HAVE_FUSE3=`) builds without a library
that is there.

zlib (disk45's `.gz` and `.zip` images) is bundled: [zlib](https://github.com/madler/zlib) itself, as the git
submodule `third_party/zlib` ([third_party/zlib.PROVENANCE.md](third_party/zlib.PROVENANCE.md)), compiled into
the build with the C compiler that goes with the C++ one (`make CC=...` for another). The build's first lines
say which zlib is used:

- `third_party/zlib` has zlib's source (`git clone --recurse-submodules`, or `git submodule update --init`
  in a clone made without): the bundled one.
- It is empty (a plain `git clone`; GitHub's "Source code" archives of a release, which never contain a
  submodule): the system's zlib when its headers are installed, and the build says how to get the bundled
  one. When the system has none either, `make` in a git clone fetches the submodule itself (it says so
  first; this is the only case in which the build uses the network), and anywhere else stops with both
  remedies: the git command and the distribution's zlib package.
- `make SYSTEM_ZLIB=1` links the system's zlib (`-lz`) even when the bundled one is there, for a
  distribution's package (it needs zlib's headers: the packages under Dependencies).

`make test` also uses `bash`, `python3`, `xxd`, `hexdump`, `diff`/`cmp` and `zip`/`unzip`: the second line of
each distribution below. The build itself needs none of them (nor git).

### Dependencies

For the smallest build, the compiler with make is enough: the first package or two of a "to build" line, before
SQLite's (on Debian `build-essential`). No line has zlib: the bundled one is used. For `make SYSTEM_ZLIB=1`,
or a tree without the submodule, the system's is `zlib1g-dev` (Debian / Ubuntu), `zlib-ng-compat-devel`
(Fedora), `zlib-devel` (RHEL, openSUSE), `zlib` (Arch), `zlib-dev` (Alpine); macOS has one in its SDK.

**Debian / Ubuntu** (also Mint, Pop!_OS, Raspberry Pi OS):
```bash
sudo apt-get install build-essential libsqlite3-dev libfuse3-dev pkg-config      # to build
sudo apt-get install git python3 xxd bsdextrautils zip unzip      # to run the tests
```
(Ubuntu 22.04 and Debian 12 have `xxd` in `xxd`; on older releases it is in `vim-common`.)

**Fedora**:
```bash
sudo dnf install gcc-c++ make sqlite-devel fuse3-devel pkgconf-pkg-config      # to build
sudo dnf install git python3 xxd util-linux diffutils zip unzip      # to run the tests
```

**RHEL / Rocky / AlmaLinux 9** (`fuse3-devel` is in CRB):
```bash
sudo dnf install dnf-plugins-core && sudo dnf config-manager --set-enabled crb      # for fuse3-devel
sudo dnf install gcc-c++ make sqlite-devel fuse3-devel pkgconf-pkg-config      # to build
sudo dnf install git python3 vim-common util-linux diffutils zip unzip      # to run the tests
```

**Arch Linux / Manjaro**:
```bash
sudo pacman -S --needed base-devel sqlite fuse3 pkgconf      # to build
sudo pacman -S --needed git python tinyxxd util-linux diffutils zip unzip      # to run the tests
```

**openSUSE Tumbleweed / Leap**:
```bash
sudo zypper install gcc-c++ make sqlite3-devel fuse3-devel pkgconf      # to build
sudo zypper install git python3 xxd util-linux diffutils findutils gawk zip unzip      # to run the tests
```

**Alpine Linux**:
```bash
sudo apk add build-base sqlite-dev fuse3-dev pkgconf      # to build
sudo apk add git python3 bash xxd hexdump zip unzip      # to run the tests
```

**macOS** (Apple clang from the Xcode command line tools; SQLite is in the macOS SDK; there is no
FUSE 3, so disk45 is built without `mount`):
```bash
xcode-select --install      # the compiler, if it is not there yet
brew install make           # GNU make 4 as gmake (what the CI builds with)
gmake -j"$(sysctl -n hw.ncpu)" && gmake lib
```
The Makefile needs nothing newer than GNU make 3.81, the version of macOS's own `make` (checked with a 3.81 on
Linux, not on a Mac: the CI uses Homebrew's `gmake`). `libca45.a`, the assembler as a C++ library, is not built
on macOS (it needs GNU objcopy on ELF objects). The tests are not run on macOS by the CI.

These lists are `src/deps/deps_table.sh`: `make check-deps` prints from it, and `make test-distros` installs
exactly the "to build" packages in a fresh container of each distribution before building.

Optional, at run time:
- `pandoc`, for `make man`.
- [xemu](https://github.com/lgblgblgb/xemu)'s `xemu-xmega65`, for the runtime tests (`make test-xemu`) and
  dbg45. The Docker images build it for you (`make docker-runtime`, see
  [doc/architecture/DOCKER.md](doc/architecture/DOCKER.md)).
- `pw-play`, `paplay` or `aplay`, for `tune45 play` (it can write a `.wav` without them).
- `ffmpeg`, for `pack45` on MP4 / M4A sound (WAV, AU and MP3 are read without it).

### Build

```bash
git clone --recurse-submodules https://github.com/CTalkobt/M65Compiler.git
cd M65Compiler
make check-deps       # optional: what is needed, what is there, what to install
make -j"$(nproc)"     # the tools, in bin/, and the standard libraries, in lib/build/
make test             # optional: unit tests and examples (about 6 minutes)
```

`make CXX=clang++` builds with clang. `make install` (below) builds everything first.

### The bundled zlib: what to run when

zlib is a git submodule (`third_party/zlib`): git keeps it as a link to zlib's own repository at one pinned
commit, and fetches its files only when asked. What to run:

| Situation | Command |
|---|---|
| A new clone | `git clone --recurse-submodules https://github.com/CTalkobt/M65Compiler.git` |
| A clone made without `--recurse-submodules` (`third_party/zlib` is empty) | `git submodule update --init` |
| An existing clone, the first `git pull` after zlib became a submodule | `git submodule update --init` (once) |
| After a `git pull` that moves the pin to a newer zlib (`git status` shows `third_party/zlib` as modified) | `git submodule update` |
| To have every later `git pull` do that by itself | `git config submodule.recurse true` (once per clone) |
| To see which zlib a build will use, and whether the submodule is at the pinned commit | `make check-deps` |
| A "Source code" archive downloaded from a release page (no `.git`, never contains submodules) | install the system's zlib (the package names are under Dependencies), or unpack [zlib's source](https://github.com/madler/zlib/releases) of the pinned version into `third_party/zlib` |
| To build against the system's zlib on purpose (a distribution's package) | `make SYSTEM_ZLIB=1` |

Forgetting is not fatal: with `third_party/zlib` empty the build uses the system's zlib when it is installed
(and names the command above), fetches the submodule itself when it is not (in a git clone; it says so
first), and otherwise stops with both remedies. Moving the pin to a newer zlib (maintainers):
[third_party/zlib.PROVENANCE.md](third_party/zlib.PROVENANCE.md).

These distributions build the tree from a clean checkout with the "to build" packages above (`make
test-distros`, which needs Docker, repeats the check). The CI builds Debian 13, Fedora (GCC and clang), Arch
and Alpine on each push, Alpine again with the compiler and make alone (`alpine-min`: no SQLite, no
FUSE 3, no zlib package), Debian 13 again with `SYSTEM_ZLIB=1` (`debian13-syszlib`), and macOS (Apple clang: the tools, the library, an install, a program compiled with it):

| Distribution | Compiler |
|---|---|
| Ubuntu 22.04, Rocky Linux 9 | GCC 11 |
| Debian 12 | GCC 12 |
| Ubuntu 24.04 | GCC 13 |
| Debian 13 | GCC 14 |
| Alpine (musl) | GCC 15 |
| Fedora, Arch, openSUSE Tumbleweed | GCC 16 |
| Fedora | clang 22 |

If the build fails with a newer compiler (for example `'int64_t' does not name a type`), please open an issue
with the compiler's version (`g++ --version`) and the first error.

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
| `cp45`, `cl45` (links to `cc45`), `al45`, `af45` (links to `ca45`) | `$(PREFIX)/bin/` |
| Libraries (`c45.lib`, `c45_zp.lib`, `c45_stack.lib`, `crt0*.o45`), tune45's player `tuneplay.o45` | `$(PREFIX)/lib/cc45/` |
| objdump45's symbol files (`c64.sym`, `mega65.sym`) | `$(PREFIX)/lib/cc45/symbols/` |
| C headers (with `sys/` and `tgi/`) | `$(PREFIX)/include/cc45/` |
| `libca45.a` (not on macOS) | `$(PREFIX)/lib/` |
| `ca45.hpp` (libca45's header) | `$(PREFIX)/include/ca45/` |
| Man pages (`make man` first) | `$(PREFIX)/share/man/man1/` |

After `make install_local`, ensure `~/.local/bin` is on your `PATH`.

The tools find these relative to their own location, so no environment
variables are needed: cc45/cp45 take the standard headers from
`bin/../lib/include` (the build tree) or `bin/../include/cc45` (installed),
and cc45 and ln45 take the libraries from `bin/../lib/build` or
`bin/../lib/cc45`. `CC45_INCLUDE` and `CC45_LIB` add directories in front of
these. Keep the default `BINDIR`/`LIBDIR`/`INCDIR` (relative to `PREFIX`);
with others, set `CC45_INCLUDE` and `CC45_LIB`.

## Documentation
New to the suite? Start with a tutorial: [C with cc45](doc/guide/getting-started-c.md),
[BASIC with basic45](doc/guide/getting-started-basic45.md) or [the other tools](doc/guide/getting-started-tools.md).
When something goes wrong: [troubleshooting](doc/guide/troubleshooting.md). The terms the pages use:
[glossary](doc/guide/glossary.md). Editors (the language server, the VS Code extension and its debugger):
[doc/guide/ide.md](doc/guide/ide.md). The C library, header by header: [doc/architecture/stdlib.md](doc/architecture/stdlib.md)
(the CBM KERNAL interface, `<cbm.h>`: [doc/architecture/stdcbm.md](doc/architecture/stdcbm.md)). Working on the suite
itself: [CONTRIBUTING.md](CONTRIBUTING.md).

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
This runs the documentation check, the compiler, assembler and linker unit tests, the validation units (the steps of `src/test/test_steps.txt`, in parallel) and then the examples; it takes about 6 minutes. The steps that run programs (`test_stdlib_new.sh`, `test_examples.sh`) use the xemu emulator when `xemu-xmega65` is on the `PATH` and are skipped without it. Outputs are stored in the `build/test/` directory.

The runtime tests run on the [xemu](https://github.com/lgblgblgb/xemu) MEGA65 emulator (`xemu-xmega65` on your `PATH`):

```bash
make test-xemu                      # bash src/test/test_xemu.sh: 1179 runtime tests, both calling conventions
bash src/test/run_gte_all.sh        # GCC torture tests (551 with each calling convention)
bash src/test/test_cc_interop.sh    # calling-convention interop (13 checks)
```

Each runtime test writes its results to memory at `$C0C0` (one byte a check, `$AA` after them) or leaves its exit
status at `$FD/$FE`, which the test reads from xemu's memory dump. A run passes only if xemu got back to BASIC's
`READY.` (a timeout is a failure, even with the results written), and a status of 0 only if the program ran: xemu's
memory is zero at boot, so the tests also want crt0's return address from `main` on the stack (`$BFF0`-`$BFFF`).

For detailed information on each tool, refer to:
- [doc/bin/cc45.md](doc/bin/cc45.md) — Compiler Usage and Features
- [doc/architecture/stdlib.md](doc/architecture/stdlib.md) — Standard C Library: Headers, Functions, Linking
- [doc/bin/ca45.md](doc/bin/ca45.md) — Assembler Syntax and Reference
- [doc/bin/al45.md](doc/bin/al45.md) — Assembly Linter: Rules, Configuration, Output Formats
- [doc/bin/af45.md](doc/bin/af45.md) — Assembly Formatter
- [doc/bin/cl45.md](doc/bin/cl45.md) — C Linter: Rules, Suppressing, Configuration
- [doc/bin/nm45.md](doc/bin/nm45.md) — Symbol Lister for `.o45` Object Files
- [doc/bin/objdump45.md](doc/bin/objdump45.md) — Object File Inspector and Disassembler
- [doc/bin/cp45.md](doc/bin/cp45.md) — Preprocessor Features and Usage
- [doc/architecture/opcodes.md](doc/architecture/opcodes.md) — 45GS02 Instruction Reference
- [doc/architecture/lib45.md](doc/architecture/lib45.md) — `.o45` Relocatable Object Format Specification
- [doc/bin/ln45.md](doc/bin/ln45.md) — Linker Usage and Reference
- [doc/bin/ar45.md](doc/bin/ar45.md) — Archiver Usage and Reference
- [doc/bin/pack45.md](doc/bin/pack45.md) — Packer for `unpack()`: Files, Data in a Program, Assembly
- [doc/bin/font45.md](doc/bin/font45.md) — Fonts: Inputs, Charsets, the Font Format
- [doc/bin/tune45.md](doc/bin/tune45.md) — Tune Player Data, Playing, Composing, MIDI Import
- [doc/bin/dbg45.md](doc/bin/dbg45.md) — Debugging in xemu: Commands, VS Code
- [doc/bin/deploy45.md](doc/bin/deploy45.md) — Programs and Disk Images to a MEGA65 or an Ultimate 64
- [doc/bin/disk45.md](doc/bin/disk45.md) — Disk/Tape/Archive Image Utility
- [doc/bin/basic45.md](doc/bin/basic45.md) — BASIC 65 Program Generator
- [doc/bin/asmCvt45.md](doc/bin/asmCvt45.md) — Assembly Dialect Converter
- [doc/bin/CONFIGURATION.md](doc/bin/CONFIGURATION.md) — Configuration Files and Environment
