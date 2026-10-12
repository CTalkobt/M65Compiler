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

See [Known Problems](#known-problems) for the options that are still placeholders.

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
| `--load <addr>` | Load address, hex (`2001`, `0x2001`; default `2001`, where BASIC 65 programs start; `0801` is the C64's) |
| `--increment <n>` | Line numbering: the first line is `n`, then `2n`, `3n`, ... (default 10). Numbers end at 63999 (an error past that) |
| `--label-table <file>` | Write a tab-separated label table: label, line number, address of the line |
| `--docs <file>` | Write markdown documentation of labeled sections |
| `-I <path>` | Include search path for `#include` (one path; a later `-I` replaces an earlier one) |
| `--minify` | Remove `REM` comments and whitespace, and join lines into the line before them: never across a label (a jump target starts its own line), never onto a line containing `IF` (statements after `THEN` run only when the condition holds), at most 160 characters per line |
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
- Every line with code is one BASIC line, numbered 10, 20, 30, ... (`--increment`).
- A label with no code on its line (`main_loop:` alone) names the next line with code (or, at the end of the source, an extra line holding `:`).
- A label is used by name after `GOTO`, `GOSUB`, `GO TO` (also each item of `ON x GOTO a, b, c`), `THEN`, `ELSE`, `RESTORE`, `TRAP`, `RESUME` and `RUN`, and becomes that line's number. Labels are matched without regard to case (`goto Loop` finds `loop:`). A label may be named like a keyword (`loop:`) when it is used after `GOTO`/`GOSUB`; after `THEN`/`ELSE` a keyword stays a keyword (`then end`).
- Lines starting with `'` or upper-case `REM` are dropped. Lines starting with `#` that are not preprocessor directives are dropped by the preprocessor, so `#` works as a comment.
- Leading/trailing whitespace is removed; blank lines are ignored.

### Validation

Validation messages have the form `file:line:col: error|warning: message`.

| Check | Severity |
|-------|----------|
| A name used as a label (see above) that is not defined, at the line using it | error |
| Label defined more than once (in any case) | error |
| More lines than line numbers up to 63999 allow | error |
| A `{...}` left in the source after symbol substitution that is not a PETSCII escape (`{clear}` is fine) | warning |
| Estimated program size above ~28 KB | warning |

Errors stop compilation (exit status 1); warnings do too with `--strict`. `--validate` only reports.

### Keywords

Keywords are recognised case-insensitively as whole words: `print`, `PRINT` and `Print` all tokenize, `printx` does not. A word may contain `$` and `#`, so `PRINT#1` is one (unknown) word; write `PRINT# 1`. Run `basic45 --list-tokens` for the table (115 single-byte tokens `$80`-`$FD`, 73 two-byte tokens `$FE xx` such as `BANK`, `SPRITE`, `DMA`, `FAST`, `PALETTE`).

The operators `+ - * / ^ > = <` are tokens (`$AA`-`$B3`), as BASIC stores them; `?` is `PRINT`, and `TAB(` / `SPC(` are tokens. After `REM` the rest of the line, and after `DATA` the items up to the next `:` outside quotes, are stored as text (not tokenized). The operators `+ - * / ^ > = <` are tokens (`$AA`-`$B3`), as BASIC stores them; `?` is `PRINT`, and `TAB(` / `SPC(` are tokens. After `REM` the rest of the line, and after `DATA` the items up to the next `:` outside quotes, are stored as text (not tokenized). Variable names and other text outside strings are converted to PETSCII upper case. A space is inserted after a keyword that is followed by a name, number or operator.

### Strings and Escape Sequences

String letters are stored as PETSCII `$41`-`$5A` whatever their case. `{name}` escapes (lower-case names only) insert control codes:

| Escape | Code | Escape | Code |
|--------|------|--------|------|
| `{black}` | `$90` | `{home}` | `$13` |
| `{white}` | `$05` | `{clear}` | `$93` |
| `{red}` | `$1C` | `{reverse}`, `{rev}` | `$12` |
| `{cyan}` | `$9F` | `{off}`, `{rvs-off}`, `{reverse-off}` | `$92` |
| `{magenta}`, `{purple}` | `$9C` | `{under}`, `{underline}` / `{under-off}` | `$02` / `$82` |
| `{green}` | `$1E` | `{insert}`, `{ins}` | `$94` |
| `{blue}` | `$1F` | `{delete}`, `{del}`, `{back}` | `$14` |
| `{yellow}` | `$9E` | `{bell}` | `$07` |
| `{orange}` | `$81` | `{tab}` | `$09` |
| `{brown}` | `$95` | `{esc}` | `$1B` |
| `{light-red}`, `{lred}` | `$96` | `{up}`, `{crsr-up}` | `$91` |
| `{dark-gray}`, `{dark-grey}`, `{dgray}` | `$97` | `{down}`, `{crsr-down}` | `$11` |
| `{gray}`, `{grey}` | `$98` | `{left}`, `{crsr-left}` | `$9D` |
| `{light-green}`, `{lgreen}` | `$99` | `{right}`, `{crsr-right}` | `$1D` |
| `{light-blue}`, `{lblue}` | `$9A` | `{f1}` `{f3}` `{f5}` `{f7}` | `$85` `$86` `$87` `$88` |
| `{light-gray}`, `{light-grey}`, `{lgray}` | `$9B` | `{f2}` `{f4}` `{f6}` `{f8}` (also `{shift-f1}` `{shift-f3}` `{shift-f5}` `{shift-f7}`) | `$89` `$8A` `$8B` `$8C` |

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

1. Load address (2 bytes, little-endian; default `$2001`)
2. One record per line, the first at the load address: link to the next line's address (2 bytes; the last line's link points at the end marker), line number (2 bytes), tokens, `$00`
3. Program end `$00 $00`

## Known Problems

- `--preserve-spaces`, `--watch` and `--source-map` (the map's `"mappings"` is empty) do not do what their names say.
- Keywords are recognised only as whole words, unlike BASIC's own crunching: `PRINT#1` must be written `PRINT# 1`.
- The `--docs` section descriptions are always `*No description*` (the preprocessor removes the `#` comments they would come from).

`test_basic45_xemu.bas` is run by BASIC 65 on xemu (as is and with `--minify`).

## See Also

- **cc45** — C compiler
- **ca45** — Assembler
- **ln45** — Linker (`-m` symbol map)
- **disk45** — put the `.prg` on a disk image
