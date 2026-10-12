# basic45 — MEGA65 BASIC Program Generator

**Version:** the suite version (`basic45 --version` prints e.g. `basic45 v1.0.5 (cc6fbf10)`)

## Overview

`basic45` converts BASIC source text into a tokenized MEGA65 BASIC 65 program file (`.prg`). Source lines carry no line numbers: lines are given labels, and basic45 assigns the numbers. It provides:

- **Keyword tokenization** — BASIC 65 single-byte tokens and `$FE`-prefixed two-byte tokens (`--list-tokens`)
- **PETSCII strings** — `{name}` escapes for colours and control codes
- **Preprocessor** — `#define`, `#undef`, `#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`, `#include`, predefined macros
- **Symbol substitution** — `{symbol}` or `{symbol+expr}` replaced by addresses from a symbol file
- **Validation** — undefined/duplicate labels, unresolved `{...}`, program-size estimate
- **Markdown documentation** of labeled sections (`--docs`)

See [Known Problems](#known-problems) before relying on the generated program.

## Syntax

```bash
basic45 [options] <input.bas> [-o <output.prg>]
```

Without `-o` the output is the input name with its extension replaced by `.prg`. With no input file, the usage text is printed and the exit status is 1. Unrecognised options starting with `-` are ignored silently.

## Options

| Option | Description |
|--------|-------------|
| `-o <file>` | Output file (default: `<input>.prg`) |
| `--symbols <file>` | Load a symbol file for `{...}` substitution |
| `--load <addr>` | Load address, hex (`0801`, `0x2001`; default `0801`) |
| `--increment <n>` | Line number increment. Accepted, but has no effect on the generated numbers (always 10) |
| `--label-table <file>` | Write a label→line table. Currently only the header line is written |
| `--docs <file>` | Write markdown documentation of labeled sections |
| `-I <path>` | Include search path for `#include` (one path; a later `-I` replaces an earlier one) |
| `--minify` | Remove `REM` comments and whitespace and join the lines between labels into one line |
| `--preserve-spaces` | Accepted; no effect |
| `--validate` | Run the preprocessor and validation only; write nothing |
| `--strict` | Treat validation warnings as errors (exit status 1) |
| `--dry-run` | Compile, but only report what would be written |
| `--watch` | Accepted; no effect |
| `--source-map <file>` | Write a JSON source map skeleton (`"mappings"` is currently empty) |
| `--emulator` | After writing the output, run it with `mmemu-cli` or VICE `x64` if found in `/usr/local/bin`, `/usr/bin` or `.` |
| `--list-tokens` | List all keywords with their token values and exit |
| `-v`, `--verbose` | Verbose output |
| `--version` | Show version |
| `-h`, `--help` | Show help (exit status 0) |

## Processing Steps

1. **Preprocessor** — directives are executed and macros expanded.
2. **Symbol substitution** (`--symbols`).
3. **Minify** (`--minify`).
4. **Parse and validate** — labels collected, line numbers assigned, references checked.
5. **Tokenize and emit** the `.prg`, then the optional label table, docs and source map.

## Input Format

### Lines and Labels

Each non-empty source line becomes one BASIC line. Statements on a line are separated by `:` as usual.

```basic
start: print "hello"
loop: print "x"
  goto loop
```

- A line beginning with `identifier:` (letters, digits, `_`; not starting with a digit) defines a label; the rest of the line is the code.
- Labeled lines are numbered 10, 20, 30, ... by their position among all emitted lines (labeled or not).
- A label with no code on its line (`main_loop:` alone) produces no BASIC line.
- Lines starting with `'` or upper-case `REM` are dropped. Lines starting with `#` that are not preprocessor directives are dropped by the preprocessor, so `#` works as a comment.
- Leading/trailing whitespace is removed; blank lines are ignored.

### Validation

Validation messages have the form `file:line:col: error|warning: message`.

| Check | Severity |
|-------|----------|
| `GOTO`/`GOSUB`/`THEN` followed by a name that is not a defined label (upper-case keywords only) | error |
| Label defined more than once | error |
| A `{...}` left in the source after symbol substitution (this includes PETSCII escapes such as `{clear}`) | warning |
| Estimated program size above ~28 KB | warning |

Errors stop compilation (exit status 1); warnings do too with `--strict`. `--validate` only reports.

### Keywords

Keywords are recognised case-insensitively as whole words: `print`, `PRINT` and `Print` all tokenize, `printx` does not. A word may contain `$` and `#`, so `PRINT#1` is one (unknown) word; write `PRINT# 1`. Run `basic45 --list-tokens` for the table (115 single-byte tokens `$80`-`$FD`, 73 two-byte tokens `$FE xx` such as `BANK`, `SPRITE`, `DMA`, `FAST`, `PALETTE`).

Variable names and other text outside strings are converted to PETSCII upper case. A space is inserted after a keyword that is followed by a name, number or operator.

### Strings and Escape Sequences

String letters are stored as PETSCII `$41`-`$5A` whatever their case. `{name}` escapes (lower-case names only) insert control codes:

| Escape | Code | Escape | Code |
|--------|------|--------|------|
| `{black}` | `$90` | `{home}` | `$13` |
| `{white}` | `$05` | `{clear}` | `$93` |
| `{red}` | `$1C` | `{reverse}`, `{rev}` | `$12` |
| `{cyan}` | `$9F` | `{off}` | `$81` |
| `{magenta}`, `{purple}` | `$9C` | `{under}`, `{underline}` | `$82` |
| `{green}` | `$1E` | `{insert}`, `{ins}` | `$94` |
| `{blue}` | `$1F` | `{delete}`, `{del}` | `$14` |
| `{yellow}` | `$9E` | `{back}` | `$08` |
| `{up}`, `{crsr-up}` | `$91` | `{bell}` | `$07` |
| `{down}`, `{crsr-down}` | `$0A` | `{tab}` | `$09` |
| `{left}`, `{crsr-left}` | `$9D` | `{esc}` | `$27` |
| `{right}`, `{crsr-right}` | `$1D` | `{f1}`-`{f8}`, `{shift-f1}`-`{shift-f8}` | `$85`-`$8C` |

An unknown `{name}` is kept as text, with each brace stored as a space.

```basic
start: print "{clear}{white}hello{yellow} world"
```

## Preprocessor

Directives start with `#` (leading whitespace allowed) and are removed from the output.

| Directive | Effect |
|-----------|--------|
| `#define NAME value` | Object-like macro; `NAME` is replaced as a whole word everywhere, including inside strings. No parameters. |
| `#undef NAME` | Remove a macro |
| `#ifdef NAME` / `#ifndef NAME` | Conditional on whether `NAME` is defined |
| `#elif NAME` | Else-if `NAME` is defined |
| `#else` / `#endif` | |
| `#include "file"` | Insert a file (preprocessed). Searched relative to the including file, then in the `-I` path. A file is included only once. |

There is no `#if`; any other `#...` line is dropped. Predefined macros: `__FILE__` (quoted input name), `__LINE__` (current line of the file being preprocessed), `__DATE__` (`"YYYY-MM-DD"`), `__TIME__` (`"HH:MM:SS"`).

```basic
#define MAXH 100
#define DEBUG
#ifdef DEBUG
dbg: print "debug mode"
#else
rel: print "release"
#endif
#include "lib.bas"
show: print MAXH
```

```bash
basic45 program.bas -I ./includes -o program.prg
```

## Symbol Substitution

With `--symbols <file>`, every `{...}` in the source (after preprocessing) is replaced by a decimal number when it is a known symbol or an expression of symbols and numbers (`+ - * / %`, decimal or `0x` hex constants; no parentheses). Anything else is left alone.

Symbol file: one `<hex-address> <name>` per line; lines starting with `;` or `#` are skipped, as are lines that do not parse. This matches the symbol map that `ln45 -m` prints (names carry cc45's leading `_`):

```bash
cc45 -c routines.c -o routines.o45
ln45 -prg -m routines.o45 -L lib/build -l c45.lib -o routines.prg \
  | grep '^  [0-9a-f]\{8\} ' > symbols.txt     # "  00002079 _myroutine"
basic45 program.bas --symbols symbols.txt -o program.prg
```

```basic
go: sys {_myroutine}
go2: sys {_myroutine+3}
```

## Documentation Generation

`--docs <file>` writes markdown listing the labeled sections of the preprocessed source:

```markdown
# program.bas

## Program Structure

### 1. main_loop (Line 10)

*No description*
```

A label is recognised when `name:` is followed by a space or the end of the line. The generator attaches `#` comment lines that precede a label, but the preprocessor has already removed those lines, so every section currently shows `*No description*`. If there are no labels the file says `No labeled sections found.`

## Output Format

1. Load address (2 bytes, little-endian; default `$0801`)
2. One record per line: link to next line (2 bytes), line number (2 bytes), tokens, `$00`
3. Program end `$00 $00`

## Known Problems

The current implementation has defects that make most generated programs unusable on real BASIC:

- **Line links** are 2 bytes too high (computed from load address + 2), and the last line's link is `$0000`, so BASIC drops that line.
- **Unlabeled lines are all numbered 10**; only labeled lines get distinct numbers.
- **Label references are not resolved**: `goto loop` is emitted as the text/keyword `LOOP`, not a line number. `--increment` and `--label-table` have no effect beyond what is listed above.
- **Operators** (`+ - * / = < > ^`) are stored as characters instead of BASIC operator tokens.
- `--minify` merges/cuts statements incorrectly (a labeled line with code can be lost).
- Some escape codes are wrong for PETSCII: `{esc}` gives `$27`, `{down}`/`{crsr-down}` give `$0A` (cursor down is `$11`).

## See Also

- **cc45** — C compiler
- **ca45** — Assembler
- **ln45** — Linker (`-m` symbol map)
- **disk45** — put the `.prg` on a disk image
