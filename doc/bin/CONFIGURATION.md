# MEGA65 Toolchain Configuration

This document describes the configuration files and environment variables the toolchain reads.

## Which Tools Read Configuration

| Tool | Configuration | Format |
|------|---------------|--------|
| `cc45` | `~/.config/m65/cc45.conf` | Command-line arguments, whitespace-separated, quotes allowed |
| `cp45` | `~/.config/m65/cp45.conf` | Same as cc45 (`cp45` is `cc45` under another name; the file is chosen by the program name) |
| `ca45` | `~/.config/m65/ca45.conf`, then `./.ca45rc` | One command-line argument per line |
| `cvt_asm` | `.cvtasmrc` in the current directory or the nearest parent directory | INI sections with `key = value` (see [cvt_asm](cvt_asm.md#configuration-file-cvtasmrc)) |

`ln45`, `ar45`, `nm45`, `objdump45`, `basic45` and `disk45` read no configuration file.

Configuration arguments are placed in front of the command-line arguments and parsed with them, so for options that set a value (`-O2`, `-fzpcall`/`-fno-zpcall`, ...) the command line wins because it comes later.

## cc45 / cp45: `~/.config/m65/cc45.conf`

- Lines whose first character is `#` are comments; blank lines are ignored.
- Every other line is split at spaces/tabs into arguments; `"..."` and `'...'` group text containing spaces.
- `#` later in a line is **not** a comment: `-O0  # debug` adds the arguments `#` and `debug`.
- An unknown option starting with `-` warns (*"cc45: warning: unknown option '-x' ignored"*). Any other word is taken as the input file name; the file named on the command line comes later and replaces it, so such words are silently ignored.

Example (`$HOME` is read from the environment):

```
# ~/.config/m65/cc45.conf
-O2
-fno-zpcall
-I "/opt/m65 headers"
-DDEBUG=1
```

```bash
cc45 -c main.c            # uses -O2 -fno-zpcall ...
cc45 -fzpcall -c main.c   # ZP convention: the later -fzpcall wins
```

For the options cc45 accepts see [cc45](cc45.md) (`cc45 -?`). Besides the `-O` level, single optimisations are switched with `-f<name>` / `-fno-<name>` (see [optimizations.md](../architecture/optimizations.md#optimization-switches)), e.g. a `-fno-inline-small-functions` line.

## ca45: `~/.config/m65/ca45.conf` and `.ca45rc`

- Each non-blank line, with leading/trailing whitespace removed, is **one** argument (no splitting, no quotes). Lines starting with `#` are comments.
- Order of the merged argument list: global file, `./.ca45rc` (project), environment variables (below), command line.
- `-O<n>` resets all per-pass flags for that level, so a `-P` flag only survives if it comes after the last `-O`. A `-O2` on the command line therefore cancels `-P` lines of a config file.
- **Known problem:** while any configuration or environment argument is present, options that take a value (`-o FILE`, `-L FILE`, `-l N`, `--source-map FILE`, `--emulator-path PATH`) read the wrong argument (`CA45_OPTIMIZE=2 ca45 -c x.s45 -o x.o45` fails with `Failed to open input file: x.o45`). Keep such options out of the configuration and avoid `.ca45rc`/`CA45_*` when using them.

```
# ~/.config/m65/ca45.conf
-O2
-PNoTailCall
-Woverflow
```

### ca45 per-pass flags

`-P<Name>` enables and `-PNo<Name>` disables an assembler optimisation (names are case-sensitive). `-O0` turns all of them off; `-O1`, `-O2` (default) and `-O3` turn all of them on.

| Name | Pass |
|------|------|
| `JSRRelocate` | JSR → BSR where possible |
| `TailCall` | JSR + RTS → JMP |
| `BranchInvert` | Inverted branch instead of branch over BRA |
| `JmpBra` | JMP → BRA |
| `NoOpBra` | Remove BRA to the next instruction |
| `CmpElimination` | Remove redundant compares |
| `RedundantLoad` | Remove loads of values already in a register |
| `DeadStore` | Remove dead stores |
| `TailDedup` | Merge identical tails |
| `PreserveXSP` | Keep X holding SP across instructions |
| `SeqExtract` | Sequence extraction (only with `--experimental`) |
| `StoreLoadPair` | Store/load pair optimisation |
| `FCmpOpt` | Float compare optimisation |
| `TSXRedundant` | Remove redundant TSX |

ca45 also accepts `StrengthReduction`, `AlgebraicSimplify`, `TypeNarrowing`, `BranchFold`, `CSE`, `LICM`, `CopyChains` and `AddrElemFusion`, but they have no effect in the assembler. Unknown names are ignored silently.

## cvt_asm: `.cvtasmrc`

See [cvt_asm](cvt_asm.md#configuration-file-cvtasmrc). `cvt_asm --init-config` writes a default file and `--show-config` prints the settings in effect.

## Environment Variables

| Variable | Tools | Description |
|----------|-------|-------------|
| `CC45_INCLUDE` | cc45, cp45, ca45 | Colon-separated include directories, searched before cc45's built-in `<bin>/../lib/include` |
| `CC45_LIB` | ln45 | Colon-separated library directories (see [ln45](ln45.md#environment-variables)) |
| `CA45_OPTIMIZE` | ca45 | Adds `-O<value>` (after the config files, before the command line) |
| `CA45_WARNINGS` | ca45 | Colon-separated warning names, each added as `-W<name>` (e.g. `overflow:underflow`) |
| `CC45_DUMP_IR` | cc45 | Non-empty and not `0`: print the IR module to stderr during code generation |
| `HOME` | cc45, cp45, ca45 | Locates `~/.config/m65/` (ca45 falls back to the password database) |

## Creating, Checking and Disabling a Configuration

```bash
mkdir -p ~/.config/m65
printf -- '-O2\n' > ~/.config/m65/cc45.conf
```

No tool prints the arguments it loaded. To check the effect, compare outputs with and without the file, or run with a different `HOME`:

```bash
HOME=/tmp/empty cc45 -c main.c      # no cc45.conf/ca45.conf
```

To disable a file, rename it (`mv ~/.config/m65/cc45.conf{,.off}`); for ca45 also check for a `.ca45rc` in the current directory.

## See Also

- [cc45.md](cc45.md) — C compiler
- [ca45.md](ca45.md) — Assembler
- [ln45.md](ln45.md) — Linker
- [cvt_asm.md](cvt_asm.md) — Assembler format converter
