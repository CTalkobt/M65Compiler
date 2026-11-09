# basic45 — MEGA65 BASIC Program Generator

**Version:** the suite version (`basic45 --version` prints e.g. `basic45 v1.1.0 (55f03ab1)`)

## Overview

`basic45` converts BASIC source text into a tokenized MEGA65 BASIC 65 program file (`.prg`). Source lines carry no line numbers: lines are given labels, and basic45 assigns the numbers. It provides:

- **Keyword tokenization** — BASIC 65 single-byte tokens and `$FE`-prefixed two-byte tokens (`--list-tokens`)
- **Strings** — PETSCII, mixed case (`p"…"`), screen codes (`s"…"`) or ASCII (`@"…"`); `{name}` escapes for colours, control codes, MEGA65 ESC sequences and PETSCII graphics
- **Preprocessor** — `#define`, `#undef`, `#ifdef`/`#ifndef`/`#elif`/`#else`/`#endif`, `#include`, predefined macros
- **Symbol substitution** — `{symbol}` or `{symbol+expr}` replaced by addresses from a symbol file
- **Validation** — undefined/duplicate labels, unresolved `{...}`, typo warnings (variables read but never assigned), unreachable code, unused procs, over-long lines; `--report` for memory use
- **Long variable names** — renamed to unique two-character BASIC names ([Variable Names](#variable-names))
- **Block IF, select and procedures** — `if ... then` / `else` / `endif`, `select` / `case`, `proc name(params)` / `call`, local `.labels` ([Block IF and Procedures](#block-if-and-procedures))
- **Assembly and binary data** — `asm` … `endasm`, `incbin`, `datainc` ([Assembly and Binary Data](#assembly-and-binary-data))
- **Calling C** — `--link file.c`: C functions called with `SYS {name}` ([Calling C Functions](#calling-c-functions-link))
- **Compiling** — `--compile`: the program as machine code, through cc45's IR and code generator; a subset of BASIC 65 so far ([Compiling](#compiling-compile))
- **Listing, source map, decompiler** — `--list`, `--source-map`, `--decompile` ([Listing, Source Map and Decompiling](#listing-source-map-and-decompiling))
- **Converting other dialects** — `--convert=basic7`, `basic35`, `simons`: a BASIC 7.0 (C128), BASIC 3.5 (C16 / Plus/4) or Simons' BASIC (C64) program as BASIC 65 source ([Converting BASIC 7.0 Programs](#converting-basic-70-programs-convertbasic7))
- **Disk image, emulator, hardware** — `--disk`, `--emulator` (xemu), `--send` (a MEGA65), `--watch`
- **Formatting** — `--format` ([Formatting](#formatting))
- **Markdown documentation** of labeled sections (`--docs`)

See [Known Problems](#known-problems) for the remaining limits.

## Syntax

```bash
basic45 [options] <input.bas> [-o <output.prg>]
basic45 --decompile <program.prg> [-o <output.bas>]
basic45 --compile [-O<n>] [--asm <file.s45>] <input.bas|program.prg> [-o <output.prg>]
basic45 --convert=basic7|basic35|simons [--ntsc] <program.prg|listing.txt> [-o <output.bas>]
```

Without `-o` the output is the input name with its extension replaced by `.prg` (`.bas` when decompiling; `-o -` writes the source to standard output; `-compiled.prg` for a `.prg` given to `--compile`). A `.prg` input is decompiled (with `--compile`: compiled). With no input file, the usage text is printed and the exit status is 1. Unrecognised options starting with `-` are ignored silently.

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
| `--preserve-spaces` | Keep the blanks the source has between tokens (outside strings) instead of basic45's spacing (one space after a keyword before a name, number or operator), so the program lists as it was typed. Not the indentation: BASIC lists one space after the line number |
| `--validate` | Run the preprocessor and validation only; write nothing |
| `--strict` | Treat validation warnings as errors (exit status 1) |
| `--dry-run` | Compile, but only report what would be written |
| `--watch` | Build, then rebuild whenever the source, an `#include`d file, a `--link` file, the `--symbols` file or `incbin`/`datainc` data changes (polled every half second; Ctrl-C stops). With `--emulator` xemu is restarted after each good build, with `--send` the program is sent again |
| `--source-map <file>` | Write a JSON map from each BASIC line number to its source file, line and text |
| `--list` | Print each BASIC line's number and address next to its source file, line and text, and the variable renames |
| `--keep-names` | Do not rename variables longer than two characters ([Variable Names](#variable-names)) |
| `--disk <image>` | Also add the program to a disk image (created when missing; the format follows the extension, e.g. `.d81`); an existing file of the same name is replaced |
| `--disk-name <name>` | The program's name on the disk (default: the output file name without its extension; at most 16 characters) |
| `--emulator` | After writing the output, run it in xemu (`xemu-xmega65 -prgmode 65 -prg`), with the `--disk` image as drive 8 |
| `--emulator-path <file>` | The xemu binary (default: `xemu-xmega65` found on `PATH`) |
| `--send` | Send the program to a MEGA65 and run it: `etherload -r <prg>` (mega65-tools, over Ethernet) when `etherload` is on `PATH`, else `m65 -F -r <prg>` (serial/JTAG). These defaults are not tested here (no MEGA65 attached); `--send-cmd` sets the command |
| `--send-cmd <command>` | The command for `--send`, `{prg}` standing for the program file (e.g. `--send-cmd "etherload -i 192.168.1.65 -r {prg}"`) |
| `--encoding <e>` | Encoding of strings without a prefix: `upper` (default), `petscii`, `screencode`, `ascii` ([Strings and Escape Sequences](#strings-and-escape-sequences)) |
| `--report` | Print the memory use: the BASIC program's range and size, the linked code, the free program space up to `$F6FF`, the longest line, the number of scalar variables (of 256) and arrays |
| `--format` | Print the source re-laid out (to standard output, or to the `-o` file) and build nothing ([Formatting](#formatting)) |
| `--link <file>` | Link a C (`.c`, compiled with `cc45 -c -O2`), assembly (`.s45`) or object (`.o45`) file after the program; repeatable ([Calling C Functions](#calling-c-functions-link)) |
| `--lib <file>` | The library for `--link` (default: `c45.lib` next to the tools, else `ln45`'s `CC45_LIB` search) |
| `--decompile` | Turn a tokenized BASIC `.prg` into basic45 source ([Listing, Source Map and Decompiling](#listing-source-map-and-decompiling)) |
| `--compile` | Compile the program (the source, or a tokenized BASIC 65 `.prg`) to machine code: a `.prg` started with `RUN` ([Compiling](#compiling-compile)) |
| `-O0` … `-O4` | `--compile`: cc45's optimization level (default `-O2`) |
| `--asm <file>` | `--compile`: also write the generated ca45 assembly |
| `--import-map FILE` | `--compile`: the map file of `REM PRAGMA IMPORT` without one (the exporting program's `<output>.map`; see Chaining) |
| `--speed`, `-Ospeed` | `--compile`: `-O3` and float `+ - * /` and comparisons by the library's RAM routines (`float_fast.s45`) instead of the ROM's: faster, with the ROM's rounding fixes (see Compiling) |
| `--convert=<dialect>` | Another BASIC's `.prg` or text listing as BASIC 65 basic45 source, with warnings for what needs a hand: `basic7` (C128 BASIC 7.0), `basic35` (C16 / Plus/4 BASIC 3.5), `simons` (C64 Simons' BASIC and TSB), `tsbneo` (TSBneo: `AT(row, column)`) ([Converting BASIC 7.0 Programs](#converting-basic-70-programs-convertbasic7)) |
| `--ntsc`, `--pal` | With `--convert`: the source machine's video timing, for `TEMPO` and BASIC 3.5's `SOUND` (default `--pal`) |
| `--data-loader` | The input is machine code (a `.prg`, or a binary with `--load-at`): write a BASIC program that POKEs it from `DATA` lines and SYSes it, a type-in listing ([DATA Loaders](#data-loaders-data-loader)); with `--hex`, `--checksum`, `--per-line <n>`, `--start <line>`, `--increment <n>`, `--no-sys`, `--load-at <addr>` |
| `--list-tokens` | List all keywords with their token values and exit |
| `-v`, `--verbose` (… `-vvvv`, `-v0`) | Log on stderr as `[basic45:LEVEL] …` (the toolchain's format, `include/Logger.hpp`); cumulative, at most `-vvvv`; `-v0` off: the files, load address, included files, the ca45/ln45 commands of `--link`, the variable renames (`--list` prints those on stdout instead); notes for unused labels |
| `--version` | Show version |
| `-h`, `--help` | Show help (exit status 0) |

## Processing Steps

1. **Preprocessor** — directives are executed and macros expanded.
2. **Block IF and procedures** are lowered to plain BASIC.
3. **Symbol substitution** (`--symbols`, and the `--link` addresses).
4. **Minify** (`--minify`).
5. **Parse and validate** — labels collected, line numbers assigned, references checked. Messages name the original file and line, through `#include` and `--minify`.
6. **Tokenize and emit** the `.prg` (variables renamed), then the optional label table, listing, docs, source map, disk image and emulator run. With `--link` the program is built twice: once to place the linked code after it, once with the real addresses.

## Input Format

### Lines and Labels

Each non-empty source line becomes one BASIC line. Statements on a line are separated by `:` as usual.

```basic
start: print "hello"
again: print "x"
  goto again
```

- A line beginning with `identifier:` (letters, digits, `_`; not starting with a digit) defines a label; the rest of the line is the code. A reserved word is never a label: a BASIC 65 keyword (`end`, `loop`, `rcursor`, …) or one of basic45's own (`endif`, `proc`, `endproc`, `call`, `select`, `case`, `endselect`, `is`, `elif`, `asm`, `endasm`, `incbin`, `datainc`, `as`), in any case. `10 end: print` is `END` and `PRINT`.
- Every line with code is one BASIC line, numbered 10, 20, 30, ... (`--increment`).
- A label with no code on its line (`main_loop:` alone) names the next line with code (or, at the end of the source, an extra line holding `:`).
- A label is used by name after `GOTO`, `GOSUB`, `GO TO` (also each item of `ON x GOTO a, b, c`), `THEN`, `ELSE`, `RESTORE`, `TRAP`, `RESUME` and `RUN`, and becomes that line's number. Labels are matched without regard to case (`goto Again` finds `again:`). A reserved word alone after `GOTO`, `GOSUB` or `GO TO` (`goto end`, an item of `ON x GOTO`) is an error; after `THEN` / `ELSE` it is the statement (`then end`).
- Lines starting with `'` or upper-case `REM` are dropped. Lines starting with `#` that are not preprocessor directives are dropped by the preprocessor, so `#` works as a comment.
- Leading/trailing whitespace is removed; blank lines are ignored.

### Numbered Programs

A classic listing, each line starting with its number, is a **numbered program**: its lines keep their numbers (`--increment` does not apply), and `GOTO 100`, `GOSUB`, `THEN`, `ELSE`, `RESTORE`, `TRAP`, `RESUME`, `RUN` and `ON ... GOTO` lists with numbers jump to them.

```basic
10 print "hello"
20 goto 10
```

A program is numbered when any line starts with a number (no BASIC statement starts with a digit), and then every line with code needs one (`20print` works too). The numbers must ascend and stay at or below 63999, and every line a number jumps to must exist (BASIC's `?UNDEF'D STATEMENT`); a number alone on a line is an error (BASIC deletes such a line). Labels still work in a numbered program: a label names its line's number, a label alone the next line's.

### Validation

Validation messages have the form `file:line:col: error|warning: message`.

| Check | Severity |
|-------|----------|
| A name used as a label (see above) that is not defined, at the line using it | error |
| Label defined more than once (in any case) | error |
| More lines than line numbers up to 63999 allow | error |
| A numbered program: line numbers not ascending, above 63999 or alone on a line, a line without one, a jump to a line that does not exist | error |
| A statement that starts with a name but is not an assignment: a word BASIC 65 does not know (a typo, `slow`, a BASIC 7.0 keyword such as `draw`), a `?SYNTAX ERROR` when run; the name of BASIC 65's keyword where it has one under another name (`SPEED` for `SLOW`, `WPOKE` for `POKEW`) | warning |
| A function under the MEGA65 Book's name or of BASIC 7.0 / 3.5 used in an expression (`peekw(a)`, `rdot(x)`), which is read as an array | warning |
| A `{...}` left in the source after symbol substitution that is not a PETSCII escape (`{clear}` is fine) | warning |
| Estimated program size above ~28 KB | warning |
| A variable read but never assigned (a typo?) | warning |
| A variable assigned with `=` but never read (not `FOR`, `INPUT`, `READ`, `GET`, `RREG` targets) | warning |
| A line that can never run: no label, after an unconditional `GOTO`/`END`/`RETURN`/`STOP`/`RUN` | warning |
| A `proc` that is never called | warning |
| A line that lists longer than 160 characters (BASIC's input line: it cannot be edited on the MEGA65) | warning |
| More than 256 scalar variables (BASIC 65 keeps them at `$F700`-`$FEFF`) | warning |
| A string with the byte `$00` or `$22` (`s"@"` is screen code 0) — it would end the line or the string | error |
| A label that is not used | note, with `-v` |

Errors stop compilation (exit status 1); warnings do too with `--strict`. `--validate` only reports.

### Keywords

Keywords are recognised case-insensitively as whole words: `print`, `PRINT` and `Print` all tokenize, `printx` does not. `PRINT#1` and `INPUT#1` are `PRINT#` / `INPUT#` followed by `1`; `GETKEY` is the two tokens `GET KEY`, as BASIC stores it. Numbers keep their exponent (`1E5`, `2.5E-3`). Run `basic45 --list-tokens` for the table: 117 single-byte tokens `$80`-`$FD`, 78 two-byte keywords `$FE xx` such as `BANK`, `SPRITE`, `DMA`, `FAST`, `PALETTE`, `VSYNC`, and 22 two-byte functions `$CE xx`: `POT`, `BUMP`, `LPEN`, `RSPPOS`, `RSPRITE`, `RSPCOLOR`, `LOG10`, `RWINDOW`, `POINTER`, `MOD`, `PIXEL`, `RPALETTE`, `RSPEED`, `RPLAY`, `WPEEK`, `DECBIN`, `STRBIN$`, `HASBIT`, `RPT$`, `RDISK`, `LOG2`, `RSPRSYS`. The tables are the ROM's own (BASIC 65 V920422; `HASBIT` to `RSPRSYS` are not in V920413). The MEGA65 Book's `SLOW`, `PEEKW` and `POKEW` are `SPEED`, `WPEEK` and `WPOKE` in these ROMs (basic45 warns about the Book's names, which tokenize as variables). `GO64` is `GO` followed by `64`, as BASIC stores it.

The operators `+ - * / ^ > = <` are tokens (`$AA`-`$B3`) and the shifts `<<` and `>>` are `$FE $52` / `$FE $53`, as BASIC stores them; `?` is `PRINT`, and `TAB(` / `SPC(` are tokens. After `REM` the rest of the line, and after `DATA` the items up to the next `:` outside quotes, are stored as text (not tokenized). Variable names and other text outside strings are converted to PETSCII upper case. A space is inserted after a keyword that is followed by a name, number or operator.

### Strings and Escape Sequences

A string's encoding is chosen by a prefix touching its opening quote, as in ca45 (`p"…"`, `s"…"`) and cc45 (`@"…"`):

| String | Encoding | `"Hello"` is stored as |
|--------|----------|------------------------|
| `"…"` | the default (`upper`, or `--encoding` / `#pragma encoding`) | |
| `p"…"` | PETSCII as ca45 and cc45 map it: `a`-`z` → `$41`-`$5A`, `A`-`Z` → `$C1`-`$DA` (as BASIC's `ASC("A")`) — mixed case, for the lower-case character set | `$C8 $45 $4C $4C $4F` |
| `s"…"` | screen codes (ca45's `s"…"`): letters `$01`-`$1A`, `@` `$00` | `$08 $05 $0C $0C $0F` |
| `a"…"`, `@"…"` | ASCII, as typed | `$48 $65 $6C $6C $6F` |
| `upper` (default) | PETSCII with letters folded: either case → `$41`-`$5A` (upper case on the default screen) | `$48 $45 $4C $4C $4F` |

`#pragma encoding upper|petscii|screencode|ascii` on a line of its own sets the encoding of the strings that follow (until the next one); `--encoding` sets it for the start of the program. A one-letter variable in front of a string needs a space or a `;` (`print a "x"`, `print a;"x"`): `a"x"` is an ASCII string. A string cannot hold `$00` (it ends the BASIC line; screen code 0 is `@`) or `$22` (it ends the string): basic45 reports them as errors.

`{name}` escapes (lower-case names only) give the same bytes in every encoding:

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

MEGA65 control codes (The MEGA65 Book, appendix C):

| Escape | Code | Escape | Code |
|--------|------|--------|------|
| `{flash}`, `{blink}` / `{flash-off}`, `{blink-off}` | `$0F` / `$8F` | `{lower}`, `{lowercase}` / `{upper}`, `{uppercase}` | `$0E` / `$8E` |
| `{case-lock}`, `{lock}` / `{case-unlock}`, `{unlock}` | `$0B` / `$0C` (SHIFT+C= case switching off / on) | `{return}` | `$0D` |
| `{linefeed}`, `{lf}` | `$0A` | `{tab-left}` | `$1A` |
| `{word-left}` / `{word-right}` | `$15` / `$17` | `{tab-set}` | `$18` (set/clear a tab stop) |
| `{f9}` `{f10}` `{f11}` `{f12}` `{f13}` `{f14}` | `$10` `$15` `$16` `$17` `$19` `$1A` | `{help}` | `$84` |
| `{default}` | `$06` (the default colour, CTRL+F) | | |

The codes are checked against the MEGA65 ROM's screen editor (`control_vectors`, `coltab` and the function key table `funtab` in mega65-rom's `system.src`; `code_*` in `b65.src`). F9-F14 share their codes with CTRL keys (F10 is `{word-left}`, F12 `{word-right}`, F14 `{tab-left}`).

MEGA65 escape sequences are ESC (`$1B`) followed by a key; `{esc-k}` writes ESC and the key `k` (a letter, digit or `@`), and these names write the sequences of The Book:

| Escape | ESC + | Escape | ESC + |
|--------|-------|--------|-------|
| `{clear-eos}` | `@` clear to the end of the screen | `{line-start}` / `{line-end}` | `J` / `K` |
| `{insert-on}` / `{insert-off}` | `A` / `C` auto-insert mode | `{scroll-on}` / `{scroll-off}` | `L` / `M` |
| `{window-top}` / `{window-bottom}` | `T` / `B` window corner at the cursor | `{normal}` | `O` cancel quote, reverse, underline, flash |
| `{delete-line}` / `{insert-line}` | `D` / `I` | `{erase-sol}` / `{erase-eol}` | `P` / `Q` erase to the start / end of the line |
| `{cursor-solid}` / `{cursor-flash}` | `E` / `F` | `{colors-high}` / `{colors-low}` | `S` / `U` colour range 16-31 / 0-15 (also `{colours-...}`) |
| `{bell-on}` / `{bell-off}` | `G` / `H` | `{scroll-up}` / `{scroll-down}` | `V` / `W` |
| `{40-80}`, `{40col}`, `{80col}` | `X` toggle 40/80 columns (clears) | `{tabs-default}` / `{tabs-clear}` | `Y` / `Z` |

PETSCII graphics: `{shift-a}`-`{shift-z}` are the shifted letters `$C1`-`$DA`, `{cbm-a}`-`{cbm-z}` the C= key graphics (as printed on the keyboard: `{cbm-a}` `$B0`, `{cbm-z}` `$AD`), and `{pound}` `$5C`, `{arrow-up}` `$5E`, `{arrow-left}` `$5F`, `{pi}` `$DE`, `{space}` `$20`, `{shift-space}` `$A0`.

`{$xx}` writes the byte `xx` (hex), for any code without a name. `{name*n}` repeats an escape `n` times (1-255): `{down*5}`, `{$a0*40}`. An unknown `{name}` is kept as text, with each brace stored as a space.

```basic
start: print "{clear}{white}hello{yellow} world"
```

## Variable Names

BASIC 65 tells variables apart by their first two characters (with the `$` / `%` type and array-ness): `score` and `scale` would be the same variable `SC`, and `store` would be the reserved status variable `ST`. basic45 renames every variable name longer than two characters to a two-character name that

- no other variable of the program uses (names of one or two characters are kept as written, and kept clear of),
- is not a reserved variable (`ST`, `TI`, `DS`, `ER`, `EL`),
- is not a keyword of one or two characters (`TO`, `IF`, `ON`, `OR`, `GO`, `FN`, `DO`, ...), which would be read as the keyword when the line is listed and entered again,

preferring the name's first two letters. The `$`, `%` and `&` (byte) suffixes, arrays and `FN` function names follow along (`def fn double(x)` becomes `DEF FN DU(X)`). A name after `$` or `%` (`$FF`, `%1010`) is a number, not a variable. `--list` and `-v` print the renames (`SCORE->SR STORE->SO`); `--keep-names` turns renaming off.

## Block IF and Procedures

```basic
  if lives > 0 then            ' THEN ends the line: a block
    print "again"
  else
    print "game over"
  endif
  call draw_board              ' gosub draw_board
proc draw_board                ' a subroutine; code before it jumps over it
  print "{clear}"
endproc                        ' return
```

| Source | BASIC |
|--------|-------|
| `if <cond> then` (nothing after `THEN`) | `IF (<cond>)=0 THEN <else>` — `=0` is "false" for any value (BASIC's `NOT` is bitwise) |
| `else` | `GOTO <endif>`, and the else part's label |
| `endif` (or `end if`) | the label after the block |
| `proc name` or `proc name(a, b$, ...)` | `GOTO <after>` and the label `name` |
| `endproc` | `RETURN` and the label after it |
| `call name` | `GOSUB name` |
| `call name(x, y$, ...)` | `A = x : B$ = y$ : GOSUB name` — the arguments are assigned to the parameters left to right (they are ordinary, global variables) |
| `select <expr>` | `ZSELn = <expr>` (`ZSELn$` when the expression is a string; renamed like any long name) |
| `case 1, 2` / `case 3 to 5` / `case is > 10` | `IF (ZSELn=1 OR ZSELn=2)=0 THEN <next case>`; a range is `>=` and `<=`, `is` takes `= <> < > <= >=`; several items are or-ed |
| `case else` | the code when no case matched |
| `endselect` (or `end select`) | the label after the block |
| `.name` | a label local to its `proc` (or to the code outside procs): `.loop:` defines it, `goto .loop` uses it; two procs can each have a `.loop` |

```basic
  select key$
  case "a" to "z"
    print "a letter"
  case " ", "{down}"
    call move(1, 0)
  case else
    print "?"
  endselect
proc move(dx, dy)
.step: px = px + dx : py = py + dy
  if blocked then .step
endproc
```

Blocks nest; the generated labels are `__else<n>`, `__endif<n>`, `__endproc<n>`, `__case<n>_<k>`, `__endsel<n>`, `__l<scope>_<name>`. A `call` with the wrong number of arguments is an error. A single-line `IF ... THEN ... ELSE ...` is BASIC's own and stays as it is (BASIC 65's `IF ... THEN BEGIN ... BEND` and `DO ... LOOP` work too). An `else`, `endif`, `case`, `endselect` or `endproc` without its block, and a block left open, are errors at their source line.

## Assembly and Binary Data

```basic
  sys {flash_border}                   ' an asm label: called directly
  bank 0 : first = peek({sprite}) : bank 128
  restore spritedata : read a, b, c
asm
flash_border:
    inc $d020
    rts
endasm
incbin "sprite.bin" as sprite          ' the bytes after the program
datainc "sprite.bin", 8 as spritedata  ' the bytes as DATA lines, 8 per line
```

- `asm` … `endasm`: assembly (ca45 syntax) assembled with `ca45 -c` and linked after the program, like `--link` code. A label at the start of a line is made global; `SYS {label}` calls it directly (no C thunk: the routine runs with BASIC's zero page and stack, registers from the SYS as usual).
- `incbin "file" as name`: the file's bytes after the program. `{name}` is their address, `{name_size}` their size.
- `datainc "file"[, n] [as label]`: `DATA` lines with the file's bytes in decimal, `n` per line (default 16); `as label` (or a label on the line) names the first `DATA` line, for `RESTORE label`.
- File names are relative to the file containing the line (so `#include`d files work), else to the current directory.

## Calling C Functions (`--link`)

```bash
basic45 --link routines.c menu.bas -o menu.prg
```

```basic
  sys {sum_to}, 100 : rreg lo, hi      ' lo + 256 * hi is sum_to(100)
```

- `SYS {name}` calls the C function `name` (`_name` in the object; `{_name}` works too). The SYS's A, X and Y are the function's first three parameters (`unsigned char` or `unsigned int`, ZP calling convention), and its result is in A/X afterwards: `RREG A, X`.
- Each such call goes through a thunk (`__bas_name`) into the library's `__basic_call` (`stdlib_asm/basic_call.s45`, linked only by programs with such functions), which switches to the program's own zero page and 16-bit stack (as for `main`), calls the function, and returns to BASIC with BASIC's zero page and stack. The first call fills BSS and runs `_init_features`; later calls keep the globals.
- Other `{name}` references are the addresses of C symbols, e.g. `peek({counter})` (a C global). BASIC's `PEEK` and `POKE` use the current `BANK`; the default bank shows ROM over parts of bank 0, so read the C data with `BANK 0 : V = PEEK({counter}) : BANK 128`, and call `SYS` in the default `BANK 128` (the C code calls the ROM).
- The code is linked (`ln45 -prg -t`) at the first page boundary after the BASIC program and written into the same `.prg`; the program and the code load together. A `main` is not needed (a weak one is supplied).
- The C files are compiled with `cc45 -c -O2` (the ZP convention, `c45.lib`).

`examples/basic/c_routines` is a complete example; `test_basic45_link_xemu.bas` with `test_basic45_link_xemu.c` is run by BASIC 65 on xemu.

## Compiling (`--compile`)

`--compile` (#369) turns the program into machine code instead of tokens for the interpreter. The program is
built as usual (labels, block IF, procs, the preprocessor), then the tokenized lines are compiled to cc45's
IR (`src/main/BasicCompiler.cpp`), which cc45's IR passes (`-O<n>`) and code generator make into ca45
assembly; ca45 and `ln45 -basic` with `c45.lib` link it. The result is a `.prg` at `$2001` with a `SYS` line,
started with `RUN`. A tokenized BASIC 65 `.prg` can be compiled as it is (`basic45 --compile game.prg`).

```bash
basic45 --compile prog.bas -o prog.prg          # machine code
basic45 --compile --asm prog.s45 prog.bas       # and the generated assembly
```

The same program gives the same output interpreted and compiled. The floats are BASIC's 40-bit floats,
computed by the BASIC 65 ROM's routines (the library's float runtime), PRINT formats numbers with the ROM's
FOUT, and run-time errors print BASIC's message (`?DIVISION BY ZERO ERROR IN 30`) and end the program (the exit
status at `$FD`/`$FE` is the error number), after which BASIC's `READY.` follows.

**What is compiled** (a first step; anything else is an error naming the statement, at its line):

| | |
|---|---|
| Statements | `LET` and assignments, `PRINT` (`;`, `,`, `TAB(`, `SPC(`), `IF` … `THEN` / `GOTO` (a line number or statements), `ELSE`, `BEGIN` / `BEND`, `GOTO`, `GO TO`, `GOSUB`, `RETURN`, `ON` … `GOTO` / `GOSUB`, `FOR` … `TO` … `STEP` / `NEXT` (a float variable; `NEXT a, b`), `POKE`, `DIM`, `INPUT` (a prompt with `;` or `,`), `GET`, `GETKEY`, `DO` … `LOOP` (`WHILE` / `UNTIL` on either), `EXIT`, `READ`, `DATA`, `RESTORE` (a line number or none), `DEF FN`, `WPOKE`, `WAIT`, `SLEEP`, `SCNCLR` (the text screen), `BANK`, `TRAP` (a line number or none), `RESUME` (none, `NEXT` or a line), `OPEN`, `CLOSE`, `PRINT#`, `INPUT#`, `GET#`, `CMD`, `DOPEN`, `APPEND`, `DCLOSE`, `DLOAD`, `LOAD` (chaining), `HEADER`, `SCRATCH` / `ERASE` / `DELETE "name"`, `RENAME`, `COPY` (on one unit), `CONCAT`, `COLLECT`, `DCLEAR`, `BACKUP`, `RECORD#`, `BLOAD`, `BSAVE`, `BVERIFY`, `DIR` / `CATALOG`, `FREAD#`, `FWRITE#`, `LINE INPUT`, `LINE INPUT#`, `SPRITE` (and `SPRITE CLR`), `SPRCOLOR`, `MOVSPR`, `SPRSAV`, `COLLISION`, `COLOR` / `FOREGROUND`, `BACKGROUND`, `BORDER`, `HIGHLIGHT`, `CURSOR`, `RCURSOR`, `WINDOW`, `SPEED` / `FAST`, `PALETTE COLOR` / `PALETTE RESTORE`, `VSYNC`, `SCREEN`, `PEN`, `DMODE`, `DPAT`, `DOT`, `LINE`, `BOX`, `CIRCLE`, `ELLIPSE`, `POLYGON`, `PALETTE` of a screen, `SCNCLR colour`, `GRAPHIC CLR`, `LOADIFF`, `SAVEIFF`, `SYS`, `RREG`, `DMA`, `EDMA`, `MOUSE`, `RMOUSE`, `PLAY` (string constants), `SOUND`, `TEMPO`, `VOL`, `ENVELOPE`, `FILTER`, `PRINT USING`, `RUN` (a line or a program), `FGOTO`, `FGOSUB`, `KEY`, `FONT`, `CHAR DEF`, `TYPE`, `DISK`, `LOCK`, `UNLOCK`, `MKDIR`, `CHDIR`, `SET DEF` / `DISK` / `VERIFY` / `BIT`, `CLR BIT`, `BOOT "name"`, `CLR` (also `CLR` a variable, `ERR$`, `TI`, `ST`, `DS`), `END`, `STOP`, `REM`; `POKE` / `WPOKE` with a list of values |
| Values | floats, `%` integers (-32768..32767), strings (up to 255 characters); arrays of each (`DIM a(n, m)`, up to 8 subscripts; sizes at run time) |
| Operators | `+ - * / ^`, unary `-`, `= <> < > <= >=` (numbers and strings), `AND`, `OR`, `XOR`, `NOT` (-32768..65535, as BASIC 65's), `<<`, `>>` |
| Functions | `ABS`, `INT`, `SGN`, `SQR`, `USR`, `FRE`, `JOY`, `POT`, `LPEN`, `RPLAY`, `DECBIN`, `SIN`, `COS`, `TAN`, `ATN`, `LOG`, `EXP`, `PEEK`, `LEN`, `ASC`, `CHR$`, `STR$`, `LEFT$`, `RIGHT$`, `MID$`, `RND`, `FN`, `POS`, `VAL`, `DEC`, `HEX$`, `STRBIN$`, `INSTR` (a pound sign's pattern too), `MOD`, `WPEEK`, `RPT$`, `ERR$`, `RSPRITE`, `RSPCOLOR`, `RSPPOS`, `BUMP`, `RCOLOR`, `RWINDOW`, `RSPEED`, `RGRAPHIC`, `PIXEL`, `RPEN`, `RPALETTE`, π; the variables `TI`, `TI$`, `ER`, `EL`, `ST`, `DS` and `DS$` |

**How**: the program is one function; each line and statement starts a block. `GOTO` is a branch; `GOSUB`
pushes the address of the code after it on BASIC's stack and `RETURN` jumps back to it. `FOR` pushes its
loop's number and `NEXT` looks it up as BASIC 65 does, so a `GOTO` out of a loop, a `NEXT` in an `IF`, a loop
restarted with `GOTO` and a `RETURN` from inside a loop work as in the interpreter. `IF` jumps, when false, to
the statement after the line's next `ELSE` (or to the next line), past the matching `BEND` first for
`THEN BEGIN`, as the ROM's `IF` does. Integers known to be exact (`%` variables, whole numbers written
without a point, and their `+ - *` while they fit 31 bits) are computed as integers; everything else is
float arithmetic. **Integer narrowing**: a float variable that only ever holds whole numbers below 2^31 is
an integer (16 bits, or 32 when its values need them), and so are its `FOR` loop's limit and step. The
compiler proves it from the program: every value assigned to the variable (a `FOR`'s start and limit too)
must be a whole number within a bound (an integer expression of other such variables, `%` variables, whole
literals, `PEEK`, `LEN`, `ASC`, comparisons, `AND` / `OR` / `NOT`), and its `NEXT` adds the step to values
in that bound (a step past them at most, once for each loop of the variable on BASIC's stack). The bounds are
found by compiling until they hold; a variable whose bound keeps growing (`n = n + 1`, `s = s + i`) or that
is given a fraction (`/`, `SQR`, a number with a point) stays a float. BASIC's floats are exact for such
values, so the results are the interpreter's. Variables are told apart by their first two characters (and `$` / `%`), as in BASIC 65.
**Input**: `INPUT` reads a line with the KERNAL's screen editor (`"? "` first, `"?? "` when more items are
needed, nothing after a prompt and a comma), its items as BASIC 65's: leading spaces skipped, a string in quotes
or up to the next `,` / `:`, a number followed only by spaces and `,`; else `?REDO FROM START` and the `INPUT`
again; an empty line leaves the variables as they were, input left over is ignored. `GET` / `GETKEY` take a key
(KERNAL GETIN; `GETKEY` waits): a string variable gets the character (`""` for none), a numeric one its code (0
for none; a signed byte, -128..127, as BASIC 65's `float`), as BASIC 65's. `DO` / `LOOP` / `EXIT` use BASIC's stack as the interpreter does (a `DO`'s entry, `LOOP`
going back to the `DO` on top); `EXIT` and a false `DO WHILE` go on after the matching `LOOP` (found in the text,
nested ones passed; none: `?LOOP NOT FOUND`). `READ` takes the items of the `DATA` statements in program order
(the same rules as `INPUT`'s: `?OUT OF DATA`, `?TYPE MISMATCH`), `RESTORE n` from the first `DATA` at or after
line `n`. `RND` is the ROM's (the interpreter's seed and sequence), `TI` CIA 2's timers as BASIC 65 reads them,
`TI$` the clock (`"hh:mm:ss"`). The run time of these is `lib/stdlib/basic_in.c`.

**Files**: `OPEN lf [, device [, secondary [, name]]]` (no device: the default drive, secondary 0; a device
without one: 0 below device 4, else none, as the ROM's `paoc`), `CLOSE lf`, and the channels through the KERNAL's
CHKOUT / CHKIN / CLRCH (on the page-1 stack the disk code needs, `__rom_call`): `PRINT# lf, ...` prints to the
file and goes back to the screen, `CMD lf, ...` stays on the file (until a `PRINT#` or an error); a number's
cursor right is a space there, a carriage return to a file 128-255 a line feed after it. `INPUT# lf, ...` reads
as `INPUT` without prompts or echo (a line as BASIC's InputBuffer: up to 160 characters; bad data
`?FILE DATA`; a bad status, `ST AND 135`, ends it), `GET# lf, ...` never waits (a 0 byte is `CHR$(0)`). `ST` is the
KERNAL's I/O status, signed (`-62` after reading a missing file). A KERNAL error is BASIC's of that number
(`?FILE NOT OPEN`, `?DEVICE NOT PRESENT`, …); an error releases the channel. Routines: `basic_rt.s45`'s
`__bas_chkout` / `__bas_chkin` / `__bas_clrch` (reached from `__bas_out` and `__bas_error` through vectors, so a
program without files links none of it), `lib/stdlib/basic_io.c` (`OPEN`, `CLOSE`, `ST`).

**DOS statements**: `DOPEN# lf, name [, D drive] [, U unit] [, W] [, L length] [ON U unit]`, `APPEND# lf, name [, D]
[, U]`, `DCLOSE [# lf] [, U unit]`, `HEADER name [, I id] [, D] [, U]`, `SCRATCH name [, D] [, U] [, R]` (also `ERASE`,
`DELETE "name"`), `RENAME old TO new`, `COPY [name] [, D] [, U] TO [name] [, D] [, U]`, `CONCAT a TO b`, `COLLECT [D]
[, U]`, `DCLEAR [D] [, U]`, `BACKUP D a TO D b` / `U a TO U b` and `RECORD# lf, record [, byte]` take their parameters
as the ROM's `dosprs` (`lib/stdlib/basic_io.c`'s `dosprs` checks them, a statement's own syntax errors are
compile errors: any order, each once; `U8`, `D0`, `L80`, `U(x)`, `I42`, `I(x$)`: the letter, then a value; a
name in quotes or `(expression)`, `"@name"` to replace; `TO` and the second file's name, `D`, `U`; `ON U unit` is
checked but, as the ROM's, not used) and send its command strings (`sendp`'s `tabld`: `"@0:name,W,S"`, `",L,"` and
the length, `"0:name,A"`, `"R0:new=0:old"`, `"S0:name"`, `"F-R0:name"`, `"N0:name,id"`, `"C0:b=0:a"`, `"V0"`,
`"I0"`, `"D1=0"`, `"P"` and the record; an empty name takes the character before it away), a file on a free
secondary address `$62`-`$6E` (`?TOO MANY FILES`), the others on the command channel; its checks too
(`?ILLEGAL QUANTITY` for `#0`, a drive over 9, `L0`, an id not 2 characters; `?ILLEGAL DEVICE NUMBER`,
`?MISSING FILE NAME`, `?SYNTAX` for a `"@"` where it is not allowed). `DCLOSE` without `#` closes all the unit's
files, `HEADER` and `COLLECT` all files first, `DCLEAR` and `BACKUP` the unit's. `COPY` between the internal
drives' units (CBDOS, bank 1 `$0113`) is a copy between drives 0 and 1; between other units (IEC drives, which
the ROM copies file by file) it is `?ILLEGAL DEVICE NUMBER` here. `DS` / `DS$` read the drive's error
channel when no message is kept (the KERNAL's device if a drive, else the last DOS statement's unit) and keep it
until a `DOPEN` or an `OPEN` of a drive, as BASIC 65's `Read_DS` / `Clear_DS`. **Binary files, listing**: `BSAVE name, P start TO P end [, B bank] [, D] [, U]` (the end not saved; a
16-bit address in bank `B`, default `BANK`'s, else a 28-bit one) writes the file as the KERNAL's SAVE does
(the start's low 16 bits, then the bytes; on secondary address 1); `BLOAD name [, P address] [, B bank] [, D]
[, U] [, R]` and `BVERIFY` load or compare through the KERNAL's LOAD with the ROM's bank setting
(`setbank_from_dos_args`; no `P`: the file's address); `?LOAD ERROR` for a bad status, `BVERIFY`'s
`VERIFY ERROR AT $0FCA` (no carriage return, as the ROM's). `DIR` / `CATALOG [name] [, D] [, U] [, R] [, W]
[, P]` list the directory as the ROM's `directory` / `ldir` (the blocks right-aligned in 4; `W`: 20 characters
in columns of 21 rows; `P`: a page, `[MORE]` and a key; `R`: recoverable files; STOP ends it); through `CMD`
each line goes to the channel (the installed ROM sends only the first there when the channel is a file on the
same drive). `FREAD# lf, address, size` / `FWRITE#` move `size` bytes (0: 65536) between the file and the
28-bit address (`BANK` not used). `LINE INPUT# lf, a$ [, ...]` reads a line into each (up to 160 characters,
as `InputBuffer`), `LINE INPUT ["prompt";] a$ [, ...]` the rest of the typed line as it is (commas, colons,
leading spaces). Not compiled: `DIRECTORY` (basic45 does not tokenize it), `DIR` of the SD card (unit 12).
`DSAVE`, `SAVE`, `VERIFY` and `DVERIFY` are errors: they save or compare the BASIC program's text, which a compiled
program does not have.

**Chaining**: `DLOAD name [, D drive] [, U unit] [ON U unit]` and `LOAD [name [, device [, secondary]]]` in a running
program load the next program and run it from its first line, as the interpreter does (`load_it`; the program
loaded can be compiled or BASIC text). The load overwrites the program, so a stub in BASIC's input buffer (`$0200`)
does it on BASIC's stack and base page, sets BASIC's text and DATA pointers to the new program's start and returns
through the `SYS`; a load error (`?FILE NOT FOUND`, …) is the old program's, at its line (`TRAP` works). The
interpreter keeps the variables across the load; a compiled program's are its own memory, so they are passed
explicitly:

```
10 rem pragma export                        part1.bas: every scalar variable (or: rem pragma export a$, n)
20 a$ = "test" : n = 42
30 dload "part2"

10 rem pragma import "part1.map" a$, n      part2.bas: those it names (none: all of the map's)
20 print a$; n
```

`REM PRAGMA EXPORT [name, ...]` makes basic45 `--compile` write `<output>.map`: the variables in order, their
types and an id. At the `DLOAD` / `LOAD` the program writes them to the chain block (attic RAM, `$87F0000`: a
string's characters with it). `REM PRAGMA IMPORT ["file.map"] [name, ...]` reads the map at compile time (a
name in the pragma beside the source; `--import-map FILE`, as given, when the pragma names none; a name not in it, or of another type, is an error) and at the
start takes the variables from the block when it holds that exporter's (then forgets it: a later `RUN` starts
empty, as BASIC's). Names are BASIC's (two characters and `$` / `%`); arrays are not passed. A program started
otherwise keeps its variables empty. In the interpreter the pragmas are `REM`s, and basic45 does not warn about
the variables they name. Run time: `basic_rt.s45`'s `__bas_chain`, `basic_io.c`'s `_bas_chainprep` and
`_bas_x*`. Test: `test_basic_compile6_xemu.bas` / `test_basic_compile6b_xemu.bas`.

**Sprites**: `SPRITE n [, on] [, colour] [, behind] [, x expanded] [, y expanded] [, multicolour]` (each value set as it
is read, an empty one skipped; a flag other than 0 / 1 is `?ILLEGAL QUANTITY` after those before it), `SPRITE CLR`
(all off, unexpanded, in front, hires, stopped, the data pointers at `$0600`), `SPRCOLOR [mc1] [, mc2]`, `MOVSPR n, x, y`
(`+` / `-` before a coordinate: relative; a negative number's 16 bits), `MOVSPR n, x, y TO x, y, speed` (`+` / `-`
after `TO`: from the last target, as the ROM's `xdest` / `ydest`) and `MOVSPR n, angle # speed`, `SPRSAV` (a sprite
0-7 or a string of 64 characters to a sprite or a string variable), `RSPRITE(n, 0-5)`, `RSPCOLOR(1 / 2)`,
`RSPPOS(n, 0-2)` and `BUMP(1 / 2)` write and read the VIC-IV's registers directly, as BASIC 65's (`sprite`,
`movspr`, `metric`, `angle_mode`, … of V920413; `lib/stdlib/basic_sprite.c`), with the ROM's arithmetic and
checks (the moves' speeds from its `metric` and `flpint`; `?DIVISION BY ZERO` for a speed 0 `TO` a target).
Moving sprites and collisions belong to BASIC's interrupt, which is off while a `SYS` runs, so a program with
`MOVSPR`, `BUMP` or `COLLISION` puts a hook in the KERNAL's IRQ vector (`lib/stdlib_asm/basic_spr.s45`): each frame
it moves the sprites a frame (`movspr_irq`'s steps and stop at the target) and keeps the VIC's collisions for `BUMP`
and `COLLISION`. The vector leads to a stub at `$0500` (BASIC's run-time stack, which a compiled program does not
use): while the program calls a ROM routine BASIC's ROM is where the program is, and the stub only counts the
frame, moved at the next one. The program's end (and a `DLOAD`) puts the vector back. `COLLISION type, line` (a line
number) is a `GOSUB` to the line at the next statement after a collision of that type (none while one runs; light
pen, sprite/background, sprite/sprite, as BASIC's `ngone`); `COLLISION type` turns it off. Not compiled:
`SPRITE SAVE` / `SPRITE LOAD`. Test: `test_basic_compile9_xemu.bas`.

**Screen and colours**: `FOREGROUND` / `COLOR c` (the text colour; 16 and up: the colour - 16 with attribute bit 6),
`BACKGROUND c`, `BORDER c`, `HIGHLIGHT c [, mode]` (the error messages' colour; mode 1 `REM`s, 2 keywords, 3 both:
`LIST`'s), `RCOLOR(0-3)`, `CURSOR [ON | OFF] [, column] [, row [, style]]` (style 0 flashing, 1 solid: ESC F / ESC
E), `RCURSOR [column] [, row]` (into numeric variables), `WINDOW left, top, right, bottom [, clear]` (the ROM's
checks: `?ILLEGAL QUANTITY` beyond the screen, right of the right, below the bottom; a `clear` other than 0 / 1
after the window is set), `RWINDOW(0-3)` (the window's width, height, the screen's), `SPEED` / `FAST [1 | 3 | other:
40 MHz]`, `FAST ON` / `OFF` (the fast serial bus), `RSPEED(x)`, `PALETTE COLOR index, red, green, blue [, index,
red, green, blue ...]` (red 0-31, its bit 4 the foreground/background flag; green, blue 0-15), `PALETTE RESTORE`
and `VSYNC line` (0-511, below the frame's last line) set the VIC-IV's registers and the editor's variables as BASIC
65's (`lib/stdlib/basic_scr.c`), through the KERNAL's `PLOT` and the editor's `_set_window`, `_cursor` and
`_palette_init`. An error message (and `BREAK`) is in `HIGHLIGHT`'s colour, as the interpreter's. `PALETTE` of a graphics
screen is with the graphics below. Test: `test_basic_compile10_xemu.bas`.

**Graphics**: `SCREEN DEF / OPEN / SET / CLOSE / CLR` and the old `SCREEN [screen,] width, height, depth`,
`PEN [pen,] colour`, `DMODE`, `DPAT`, `DOT`, `LINE` (a chain of points; a value left out is the last point's), `BOX`
(two or four corners, solid), `CIRCLE` / `ELLIPSE` (`flag`: solid, no legs, angles from 12 o'clock; a sector from
`start` to `end` degrees), `POLYGON` (sides, the sides drawn, subtend lines, the start angle, solid), `PALETTE screen,
colour, red, green, blue`, `SCNCLR colour`, `GRAPHIC CLR`, `PAINT x, y [, mode [, colour]]` (mode 0: the area of
(x, y)'s colour, 1: up to `colour` or pen 0's, 2: up to any set dot), `VIEWPORT x, y, width, height` / `VIEWPORT CLR`,
`GCOPY` / `CUT` / `PASTE` (a brush of up to 1 KB a plane), `CHAR column, row, height, width, direction, string$
[, font address]` (BASIC's control codes: reverse, the next height and width), `RGRAPHIC`, `PIXEL` (a colour index),
`RPEN`, `RPALETTE` are BASIC 65's graphics kernel (V920413's `kg65.*`) written again (`lib/stdlib/basic_gfx.c`; the
dot, lines, `BOX`, the triangles, the ellipses and `PAINT` in `lib/stdlib_asm/basic_gfxa.s45`), not called: the VIC-III's bitplanes in 8 KB segments of banks 4 (even planes) and 5
(odd), each screen's palette in its segments' spare bytes, the same algorithms (Bresenham's lines from the kernel's
octants, its clipping with BASIC's floats, the ellipses' octant table from `SQR`, sectors by `ATN`, polygons'
triangles filled by columns), so the bitplanes hold the same bytes as the interpreter's. The state is the kernel's
own variables (bank 0 `$1F00`: its screen table, pens, the draw and view screens), so a screen opened before
`RUN` or left open after the program is the interpreter's too. An error shows the text screen first, as BASIC's.
Its quirks are kept: the clip's low edges one pixel off, a 640-wide screen's palette at `$7E80` of its bank,
`BOX` with fewer than four values doing nothing, `DOT`'s colour becoming pen 0's, the ellipses' `+ 0.35` that
adds nothing (the ROM reads the constant from the wrong bank), `VIEWPORT CLR` going through the brush's copy,
`PASTE` clamped to the screen, `PAINT` stopping when its stack is full. The ellipses' table and `PAINT`'s stack are in
attic RAM (`$8710000`, 8 KB; `$8700000`, as long as the kernel's after a short program), the brush in the kernel's
scratch pages (`$1800`-`$1BFF`). `SAVEIFF "[@]name" [,U#] [,D#]` writes the draw screen as an IFF ILBM file
(BMHD, CMAP from the screen's palette, a ByteRun1 BODY), `LOADIFF "name" [,U#] [,D#]` pastes one at (0, 0) of the
draw screen (its CMAP into the palette), both as the kernel does, quirks too: the chunks' pad bytes counted in
the lengths but not written, so an 8-plane screen's file (a one-colour CMAP) makes `LOADIFF` read past its end
and never return, in the interpreter as well; a file of another depth than the screen's sets the area to
pen 0 (a shallower file) or leaves it as it is (a deeper one); an error's number is what the kernel's A held (the I/O status, or
a byte of a chunk id: `?FILE DATA` is not given). Not compiled yet: `CHAR DEF`. The run time is about 20 KB
(a program using most of it at `-O0` or `--speed` can pass `$C000`); more of it is to become assembly. Tests:
`test_basic_compile11_xemu.bas`, `test_basic_compile12_xemu.bas`, `test_basic_compile13_xemu.bas`,
`test_basic_compile14_xemu.bas`.

**TRAP**: with a trap set, an error (not `STOP`'s BREAK) sets `ER` and `EL`, turns the trap off and goes on at
the trap's line with the CPU stack as at the start (BASIC's stack of `GOSUB`, `FOR` and `DO` as it was, as the ROM
does); `RESUME` turns it on again and goes back to the statement (`RESUME`), to the one after it (`RESUME NEXT`)
or to a line; `?CAN'T RESUME` without an error. A trap's line is a number (`TRAP 100`). A program with `RESUME`
without a line notes each statement's address as it runs (`__bas_spc` / `__bas_snext`).

**PEEK, POKE, WPEEK, WPOKE, WAIT** take BASIC 65's addresses: 0..2^32-1, above 65535 a 28-bit address (far memory,
`[zp],z`), below it in the bank `BANK` chose (128, the default, the CPU's map). A `POKE` value is 0..65535, its low
byte stored; several values fill the bytes after the address. An address known to be below 65536 in a program
without `BANK` is reached directly; others through `__bas_peek` / `__bas_poke` (`basic_rt.s45`).

**SYS, RREG, USR, DMA, EDMA, FRE** (`lib/stdlib/basic_sys.c`, `basic_usr.s45`): `SYS address [, a, x, y, z, s]`
calls the KERNAL's `_jsr_far` as BASIC 65 does: the address as `POKE`'s (a 16-bit one in `BANK`'s bank), the
ROM's base page and page-1 stack, BASIC's interrupt off meanwhile. The registers are the ones given, else those the
last `SYS` returned, which `RREG a, x, y, z, s` reads (an empty place skipped); the run time keeps them itself, as
its float code calls the ROM through the same base-page bytes. `USR(x)` calls the `JMP` at `$02F7` with x in the
ROM's FAC and the BASIC ROM mapped (the vector at the ROM's default: `?UNDEF'D FUNCTION`). `DMA` runs each job of
its list (a chained one: `?ILLEGAL QUANTITY`), `EDMA` an enhanced job with 28-bit addresses. `FRE(n)`: a negative n
gives the ROM's version (920413), 0 and 1 the heap's free bytes (a compiled program's strings and arrays are there),
4 and 5 65535, others 0. **CLR** closes the files, clears `ER` / `EL` and the `TRAP`, sets every variable to 0 or
`""`, forgets the arrays and the `DEF FN`s, empties the `GOSUB` / `FOR` / `DO` stack (a `FOR` loop with a `CLR` in it
is kept on that stack, so its `NEXT` gives `?NEXT WITHOUT FOR` as the interpreter's), sets the graphics kernel up
again and `RESTORE`s; `CLR` a variable, `ERR$`, `TI`, `ST`, `DS` as BASIC 65's. Test:
`test_basic_compile15_xemu.bas`.

**JOY, POT, LPEN, MOUSE, RMOUSE** (`lib/stdlib/basic_dev.c`) read the CIA and the paddle registers (`$D620`-`$D623`)
as BASIC 65 does: `JOY(n)` (3: both ports) the direction 0-8, 128 the fire button, 256 / 512 buttons 2 / 3 (the
paddle lines low), 1024 / 2048 buttons 4 / 5 (up + down, left + right, which leave no direction); `POT(n)` the
paddle, 256 more with its button; `LPEN(n)` the light pen's latched x (times 2) or y, then 0 (the interrupt hook
records the latch in BASIC's bytes, `$02E8`, as BASIC's IRQ does). `MOUSE ON [, port [, sprite [, hotspot x [, y
[, x, y]]]]]` / `MOUSE OFF` drive the KERNAL's mouse driver (the hotspot as its margins, the position as
`MOVSPR`'s); `RMOUSE x, y, buttons` reads the pointer sprite and the buttons (-1 each without a mouse). Test:
`test_basic_compile16_xemu.bas`.

**PLAY** (`src/main/BasicPlay.cpp`, `lib/stdlib/basic_snd.c`): each `PLAY`'s strings are read as BASIC 65 reads
them (notes A-G, `#` / `$`, the lengths W H Q I S, dots, `R`, `O` octave, `T` envelope, `L` back to the start; a
voice from octave 4, quarter notes, envelope 0) and turned, when the program is compiled, into a tune of the
library's tune player (`<audio_tune.h>`, tune45's data: a tick a 32nd note, the ten envelopes as its instruments,
voices 4-6 on SID 3 as BASIC's), which the program's interrupt hook plays each frame. A `PLAY` puts the envelopes'
current values (`ENVELOPE`, in BASIC's own tables at `$1289`) into the instruments and plays at `TEMPO`'s speed
(`tune_tempo`); `RPLAY(v)` is 1 until voice v has played its own length (the shorter voices end in rests: a tune's
voices are as long), for good with an `L`. `TEMPO`, `VOL`, `ENVELOPE`, `FILTER` and `PLAY` alone set BASIC's
variables (`$1273`, `$1278`-`$12BA`) as the ROM does, so the values carry over to and from the interpreter. The
lengths match the interpreter's to a frame (2.010 s for four quarter notes at `TEMPO 12`). Not yet, as the tune
player has no matching events: `U` volume, `X` filter, `M` modulation, `P` portamento (warned: #474-#477); a
`PLAY` starts all its voices again, and a voice it leaves out stops (BASIC's goes on: #478); a voice starts at
octave 4 and quarter notes in each `PLAY` (BASIC's keeps the last ones); strings built at run time (a compile error
for now). Test: `test_basic_compile17_xemu.bas`.

**SOUND** `voice, frequency, duration [, direction [, minimum [, step [, waveform [, pulse width]]]]]` and `SOUND
CLR` are BASIC 65's (`basic_snd.c`, its sweep in the interrupt hook as the ROM's `sound_irq`: `basic_spr.s45`), on
BASIC's own variables (`$12BD`-`$12FF`) and SIDs 2 and 4 (PLAY's are 1 and 3), the same frequencies and durations as
the interpreter's frame for frame. Test: `test_basic_compile20_xemu.bas`.

**PRINT USING** (`lib/stdlib/basic_using.c`) formats one value a `USING` as BASIC 65's (its own, not BASIC 7.0's):
`#` a digit or a character, `.` the point, `>` / `=` first a string right-justified / centred, `%[0]wD`, `%[0]wF`,
`%[0]w.pF`, `%[0]wX` fields (0: zero padding; X: the floor in hexadecimal), `*`s when a number does not fit; the
digits are FOUT's, rounded as the ROM rounds them (its 8-bit arithmetic and quirks kept); a comma after the value is
passed over. **FGOTO / FGOSUB** compute their line (0 to 2^32 - 1, its low 16 bits; none: `?UNDEF'D STATEMENT`)
through a dispatch over the program's lines (no `FOR` loop is then compiled as a counted one). **RUN** `[line]` is
`CLR` and a jump; `RUN "name"` loads the program as `DLOAD` does, without passing variables. **KEY** (`KEY` alone
lists them as the ROM does, `ON`, `OFF`, `RESTORE`, `LOAD`, `SAVE`, `n, string`) works on the editor's table at
`$1000` through the KERNAL's `_pfkey`; **FONT A-F**, **CHAR DEF** and **DECBIN** as the ROM's
(`lib/stdlib/basic_key.c`). Test: `test_basic_compile18_xemu.bas`.

**TYPE** `[P] "name"` prints a sequential file (P: page by page, `[MORE]`), **DISK** `"command"` sends a command
on the command channel (alone: prints the drive's status), **LOCK** / **UNLOCK** / **MKDIR** (CBDOS units only, 3
tracks at least) / **CHDIR** send BASIC 65's DOS commands (`F-L`, `F-U`, `/`); **SET DEF** / **SET DISK** `[TO new]`
(the drive renumbered) / **SET VERIFY** `ON | OFF` (`U0>V1`), **SET BIT** / **CLR BIT** `address, bit` (as `POKE`'s
address), and **BOOT** `"name"` (BLOAD, then the code at its start called as `SYS` calls) are the ROM's
(`basic_io.c`'s DOS commands 20-29). Not yet: `BOOT` alone (`AUTOBOOT.C65`), `BOOT SYS`, `MOUNT`, `CHDIR` on the SD
card (unit 12: the hypervisor). Test: `test_basic_compile19_xemu.bas`.

**DEF FN**: each `FN name(...)` is the function's expression compiled where it is used, its parameter the
argument (a variable of the same name keeps its value); `FN` before its `DEF` has run is `?UNDEFINED FUNCTION`. A
function defined twice is not compiled yet. `VAL` follows the ROM's number syntax (a second point or `E`, or an `E`
without digits: 0; it ends at another character), its value from `strtof` (correctly rounded). `INSTR`'s pattern
(a needle starting with a pound sign) searches the whole haystack from the position; the ROM without a position
argument searches as far as a value left in its float accumulator says. `RPT$` is in BASIC 65 releases after 920413.

**Counted loops**: a `FOR` loop that nothing can leave or enter is a plain loop: nothing on BASIC's stack, its
`NEXT` an inline step and compare. That is the case when the `FOR` and its `NEXT` are statements of their own (not
after `THEN`), the `NEXT` is the first one matching it in the text, and in between there is no `GOTO`, `GOSUB`,
`ON`, `RETURN`, `DO` / `LOOP` / `EXIT`, `BEGIN` / `BEND`, `THEN` / `ELSE` with a line number, `FOR` of the same
variable or line a jump goes to, no `IF` before the `NEXT` in its line, and only counted loops inside. Every other
loop uses the stack as the interpreter does.

**Arrays** are blocks on the heap (`malloc`): the number of dimensions, each one's size, the elements (floats
5 bytes, integers 2, strings a pointer). As in BASIC 65, a subscript is 0..65535 (`?ILLEGAL QUANTITY` outside
it), one past the `DIM` or a wrong number of subscripts is `?BAD SUBSCRIPT`, a second `DIM` of an array
`?REDIMENSIONED ARRAY`, and an array used before a `DIM` gets 11 elements in each dimension. An element of a
one-dimensional array is reached by `__bas_ael1` (inline with `--speed`) when the array exists and the
subscript is in range; anything else (making the array, more subscripts, the errors) is `__bas_aref`
(`basic_str.c`). Arrays and scalars of the same
name are different variables; array elements are not narrowed (a float array holds floats).

The run-time support is `lib/stdlib_asm/basic_rt.s45` (PRINT, the errors, BASIC's checks on numbers) and
`lib/stdlib/basic_str.c` (strings on the heap, arrays, the stack of `GOSUB` and `FOR`).

**Differences from the interpreter**:

- A float result is rounded to 32 bits after each operation; the interpreter keeps the accumulator's extra
  rounding byte between the operations of an expression, so a result can differ in its last bit.
- A number written with a point or an exponent is converted when compiling (correctly rounded); the
  interpreter's own conversion can differ in the last bit.
- A float result too large gives the largest float, not `?OVERFLOW ERROR` (except for `EXP` and `^`).
- `PEEK` of `$C000`-`$CFFF` reads RAM; the interpreter reads the ROM there.
- Sprites still moving when the program ends stop there (the interpreter's interrupt goes on moving them); a
  `COLLISION` trap turned off forgets a collision not yet taken.
- A number typed at `INPUT` or read from `DATA` is converted by the library's `strtof` (correctly rounded); the
  interpreter's own conversion can differ in the last bit, as for numbers in the program.
- Arrays and strings share the program's heap, from the end of the program to the stack's 1 KB share below `$BC00`
  (some 15–30 KB, as large as the program is not); the interpreter keeps them in bank 1 (about 60 KB). A `DIM` that does not fit is
  `?OUT OF MEMORY`.
- With `--speed`, `+ - * /` and comparisons are the library's RAM routines (`lib/stdlib_asm/float_fast.s45`,
  cc45's `-ffast-float`): their results are those of the BASIC 65 ROM with its float rounding fixed
  (correctly rounded products and quotients). The ROM of release 920413 rounds some products, quotients and
  sums differently in the last bit (`x * 1` is not always `x`), so the interpreter there can print a last
  digit differently. The functions (`SIN`, `LOG`, `^`, …) are the ROM's in both.

**Speed and size**, measured on xemu (frames of 1/50 s for 3000 iterations; `bench.bas`-style loops):

| Loop | Interpreted | Compiled | `--speed` |
|---|---|---|---|
| `for i = 1 to 3000 : s = s + i * 2` (`s` a float, `i` narrowed) | 18 | 10 | 8 |
| `s = s * .999 + x / 3 - x * x` (floats only) | 35 | 28 | 24 |
| `n% = n% + 1 : c% = c% + (n% and 1)` (`i` narrowed) | 41 | 2 | 2 |
| string `LEFT$` / `CHR$` / `+` (300 times) | 5 | 5 | 5 |
| `gosub` / `x = x + 1` / `return` (300 times) | 2 | 1 | 0 |
| sieve of Eratosthenes to 3000 (a `%` array) | 75 | 10 | 8 |
| `a(i) = a(i) + i`, 5 × 501 elements (a float array) | 23 | 10 | 8 |

Integer code is more than ten times as fast. Float arithmetic is not much faster: through the ROM each
operation calls it with `JSRFAR` (and once more to round the result); the RAM routines of `--speed` take about
half the time of that, but a float operation is still hundreds of cycles, as much as the interpreter's own. The
compiled program is about seven times the size of the tokenized one (`test_basic_compile_xemu.bas`: 2.7 KB →
18 KB): integers in 16 bits when their bounds allow, float constants as data and floats read and written in
place (cc45's code generator), `__bas_line` stored only where a line that can raise an error is entered,
`NEXT` one call (`__bas_next`: a FOR's entry holds its step code's address), conditions of comparisons
compiled as jumps where nothing in them has an effect. The next steps (#369): smaller code (the run time's arguments), strings and arrays in bank 1.

`test_basic_compile_xemu.bas` and `test_basic_compile2_xemu.bas` (input, loops, RND, DATA) are run interpreted and compiled (`-O2`, `-O0`, `--speed`) on xemu with the same results;
`test_basic45.sh` checks the output, the errors and a `.prg` input.

## Formatting

`basic45 --format prog.bas` prints the source with keywords in lower case, one space between words outside strings, code indented two spaces plus two per open block (`if … then`/`else`/`endif`, `proc`/`endproc`, `select`/`case`/`endselect`, `for`/`next`, `do`/`loop`) and labels in column 0. Strings, `REM` and `DATA` text, `#` lines, `'` comments (re-indented) and `asm` blocks are kept as written. With `-o` the result goes to a file. The formatted source builds to the same program, and formatting it again changes nothing.

## DATA Loaders (`--data-loader`)

`--data-loader` turns a machine-code program into a BASIC program that holds it in `DATA` lines, `POKE`s it into memory and `SYS`es it: the type-in listing of magazines and tutorials, one BASIC file with its machine code.

```bash
basic45 --data-loader game.prg                 # game-loader.prg, ready to run
basic45 --data-loader --checksum game.prg -o game.bas   # the listing, as source
basic45 --data-loader --load-at 7000 code.bin -o -      # a raw binary, to stdout
```

```basic
10 bank 0:a=28672:n=17
20 for i=0 to n-1:read v:poke a+i,v:next i
30 sys a
40 rem game.prg - $7000, 17 bytes
50 data 169,1,141,192,192,141,193,192
60 data 141,194,192,169,170,141,195,192
70 data 96
```

- The input is a `.prg` (its first two bytes the load address) or, with `--load-at <addr>` (hex: `7000`, `$7000`, `0x7000`), a raw binary; `--load-at` also moves a `.prg`'s code elsewhere.
- The output is built like any program (`-o x.prg`, default `<input>-loader.prg`; `--disk`, `--emulator`, `--list` work), or written as basic45 source with `-o x.bas` or `-o -`.
- The loader starts with `BANK 0`: BASIC 65's `POKE` and `SYS` use the current bank, and the default, 128, is the system mapping, where most addresses are not RAM.
- `--per-line <n>` bytes per `DATA` line (default 8, at most 30), lines from `--start <line>` (default: the increment) by `--increment <n>` (default 10). `--hex` writes the bytes in hex (`a9,01,60`, read with `DEC()`). `--no-sys` only loads the code.
- `--checksum` adds a checksum to each `DATA` line (the sum of its bytes `AND 255`), checked while loading: a mistyped line prints `DATA ERROR IN LINE n` and stops before the code runs.
- The code must not overwrite the loader itself, the BASIC program at `$2001`: that is an error (link the code elsewhere, e.g. `ln45 -prg -t 7000`). Code below `$2000` (BASIC 65's system and screen memory) is a warning.

`test_xemu.sh` runs loaders on BASIC 65 (decimal with checksums, hex, and a mistyped line that must stop before `SYS`); `src/test/test_basic45.sh` checks the source and the errors.

## Listing, Source Map and Decompiling

`--list` prints, for each BASIC line, its number, address, source file and line, and the source text:

```
   10  $2001  menu.bas:5         start:
   20  $2007  menu.bas:6         print "{clear}{white}basic 65 + c routines"
```

`--source-map <file>` writes the same mapping as JSON (`compiledLine`, `sourceLine`, `sourceFile`, `sourceCode` per line), to find the source of `?SYNTAX ERROR IN 230`.

`--decompile` (or a `.prg` input) turns a tokenized BASIC program back into basic45 source: keywords and names in lower case, strings with `{name}` escapes (`{name*n}` for runs, `{$xx}` for a byte without a name), and labels `l<number>` on the lines that `GOTO`, `GOSUB`, `ON ... GOTO`, `THEN`, `ELSE`, `RESTORE`, `TRAP`, `RESUME` or `RUN` refer to. Building the result with basic45 gives the same program (with `--keep-names` if the original's names were long).

## Converting BASIC 7.0 Programs (`--convert=basic7`)

```bash
basic45 --convert=basic7 game.prg -o game.bas      # a C128 program ($1C01)
basic45 --convert=basic7 listing.txt -o game.bas   # a text listing (10 PRINT ..., any case)
basic45 game.bas -o game65.prg                     # after reading the warnings
```

A separate, explicit step (basic45 never converts while building): it writes basic45 source for review, the
`GOTO`/`GOSUB`/`THEN` targets as labels (as `--decompile`), comments at the top naming what the conversion did, and a
warning on stderr, by line number, for everything that needs a hand. A `.prg` is read with BASIC 7.0's token tables
(its `$CE xx` / `$FE xx` numbers are not BASIC 65's: `$CE $08` is `XOR` there, `LOG10` here); a text listing is
tokenized as the C128 would (keywords anywhere outside strings, `REM` and `DATA` — a name like `TOTAL` holds `TO`,
as on the C128; strings may use basic45's `{name}` / `{$xx}` escapes).

BASIC 65 descends from BASIC 7.0, and most keywords are kept as they are (`DO`/`LOOP`, `TRAP`, the disk commands,
`SOUND`, `ENVELOPE`, `FILTER`, `VOL`, `COLLISION`, `BUMP`, `POT`, `JOY`, `XOR`, `INSTR`, `SLEEP`, `KEY`, …). What
differs — checked against BASIC 65's own source and the C128 System Guide, the rules run on xemu:

| | BASIC 7.0 (C128) | BASIC 65 | The conversion |
|---|---|---|---|
| Variable names | two letters count (`BASEX` and `BASEY` are one variable) | the same, but basic45 keeps long names apart | names that are one C128 variable are written as their two letters (a comment lists them) |
| `DT$` | a variable | the date, a reserved name (`?RESERVED NAME` when assigned) | `DT$` (and `DT…$`) renamed, with a warning |
| `TI` | 1/60 s | seconds (a 1 MHz timer) | `TI` → `int(ti * 60)` |
| `TI$` | `"HHMMSS"`, settable | `"HH:MM:SS"` from the real-time clock, not settable | read: its colons left out; `TI$ = "000000"` → `CLR TI`; other assignments warned |
| Colours | 1–16 | 0–15 | `COLOR`, `SPRITE`, `SPRCOLOR` take one less; `RCLR(`, `RSPCOLOR(`, `RSPRITE(n, 1)` give one more |
| `COLOR source, c` | sources 0 background, 1 foreground, 2/3 multicolour, 4 border, 5 characters, 6 the 80-column background | `BACKGROUND`, `BORDER`, `FOREGROUND`, `PEN` | 0 → `BACKGROUND`, 4 → `BORDER`, 5 → `FOREGROUND`, 1–3 kept for the drawing statements (below); 6 dropped |
| `RCLR(n)` | the colour of source n | `RCOLOR(n)`: 0 background, 1 text, 2 highlight, 3 border | renumbered (0 → 0, 4 → 3, 5 → 1; 1–3 from the kept sources) |
| Sprites | 1–8 | 0–7 | `SPRITE`, `MOVSPR`, `SPRSAV`, `RSPRITE(`, `RSPPOS(` take one less |
| Text screen | 40 columns (the VIC's); `GRAPHIC 5` the VDC's 80 | 80 columns at power-up; ESC X switches | 40 columns at the start (when the screen is in 80); `GRAPHIC 0` / `5` switch to 40 / 80 |
| Graphics | `GRAPHIC 1`–`4`: 320×200, two colours per 8×8 cell (multicolour 160×200, four); a colour source per drawing statement; a pixel cursor | screens of bitplanes (`SCREEN`), a colour per pixel from `PEN` | `GRAPHIC 1`–`4` → `SCREEN 320, 200, 4` (16 colours; multicolour x doubled; no split screen); `DRAW` → `LINE`; `BOX` and `CIRCLE` as below; `PAINT` → `PAINT x, y, 0, colour` (BASIC 65 fills the area of (x, y)'s colour: warned); `CHAR` → `CHAR` with the row in pixels (in text mode: `CURSOR` and `PRINT`); `LOCATE` and `RDOT(0/1)` → the kept pixel cursor; `RDOT(2)` → `PIXEL()` (a colour, not a source: warned) |
| `BOX [s], x1, y1 [, x2, y2] [, angle] [, paint]` | a rectangle by two corners, turned `angle` degrees clockwise about its centre (`box.src`) | `BOX` by four corners (any quadrangle) | unturned: its four corners; turned: a subroutine with BASIC 7.0's corners (the half sides rotated about the centre, rounded as the C128 does) |
| `CIRCLE [s], x, y, xr [, yr] [, sa] [, ea] [, angle] [, inc]` | an ellipse or arc from `sa` to `ea` (0° at the top, clockwise; an end not after the start + 360), turned by `angle`, drawn as segments every `inc` degrees (2): `inc` 120 is a triangle, 90 a diamond (`circle.src`) | `ELLIPSE` (whole, upright); `POLYGON x, y, xr, yr, sides, drawn, centre lines, angle, solid` (0° at 3 o'clock, anticlockwise — the argument order is the ROM's, not the MEGA65 Book's) | the default steps, a whole ellipse: `ELLIPSE`; steps dividing 360 and an arc of whole steps (a circle's rotation folded into it): `POLYGON` with BASIC 7.0's vertices (t = 90 − b); anything else (a turned ellipse, an arc between steps, expressions): a subroutine running BASIC 7.0's algorithm (its point formula, steps, last point at the end); the pixel cursor at the last point; in multicolour an omitted y radius is twice the x radius, as on the C128 |
| `SCNCLR n` | n: the screen mode | n: a colour to fill the graphics screen with | 0 / 5 → `SCNCLR`; 1–4 → `SCNCLR` with the background colour (dropped when no screen is open) |
| `SLOW` / `FAST` | 1 / 2 MHz | `SPEED 1`; `SPEED` alone is 40 MHz; `FAST` 3.5 MHz | `SLOW` → `SPEED 1`; `FAST` kept |
| `PEN(n)` | the light pen | `LPEN(0/1)` | renamed (`PEN(2..4)`, the 80-column screen's and the trigger, warned) |
| `FETCH` / `STASH` / `SWAP` | the RAM expansion (REU) | `EDMA` | `EDMA 0` / `0` / `2` with the REU's banks as attic RAM (`$8000000` + bank × 64 KB) |
| `TEMPO n` | a whole note: 1152 less n per frame (19.2/n s NTSC, 23.04/n s PAL); default 8 | 1200 (PAL) / 1440 (NTSC) less n per frame: 24/n s; default 12 | n × 1.04 (PAL, the default) or × 1.25 (`--ntsc`); an expression `int((e) * f + .5)`; `TEMPO 0` → 1; BASIC 7.0's default set first when the program plays music and sets none |
| `PLAY` | one string, voices switched by `V`; waits (in the statement) for a busy voice; `U` 0–15; `M` waits for the measure; octave and length shared (whole notes by default) | a string per voice (`PLAY v1$, v2$, v3$`; no `V`); returns at once, the next `PLAY` restarts the voices; `U` one digit (0–9, a table); `M` modulation; octave and length per voice (quarter notes) | a literal string split by voice, with rests where BASIC 7.0 would hold a voice back, the octave and length written out per voice, `U` mapped to the table (`U15` → `u9`), `M` dropped (the rests do its work); a wait (a subroutine: `RPLAY(v)` and the voice's counter at `$125E` + 2 × (v − 1)) before each `PLAY` |
| `RGR(n)` | the `GRAPHIC` mode | `RGRAPHIC(screen, parameter)`: a screen's settings | the mode each `GRAPHIC` sets, kept in a variable |
| `SCALE n [, xmax, ymax]` | coordinates 0–`xmax` / 0–`ymax` (default 1023) across the bit map | the screen's pixels | the drawing statements' coordinates (and radii) times two factors `SCALE` sets (320 or, in multicolour, 160 / (xmax + 1); 200 / (ymax + 1); 1 after `SCALE 0`) |
| `POKE 53265, v` (`$D011`) | bit 7 written: the raster compare's bit 8 (read, the raster line's; the C128's KERNAL runs on its own raster IRQ, the C64's on a CIA timer) | the KERNAL's IRQ is the raster IRQ at line 255: a compare of 256 or more (a `PEEK(53265)` written back while the raster is past line 255) is a line that never comes, and every IRQ stops — `PLAY`, `TI`'s clock, the keyboard | `POKE 53265, (v) and 127` (a number: its value and 127); the conversion's own `$D011` writes too |
| `SSHAPE a$, x1, y1 [, x2, y2]` / `GSHAPE a$ [, x, y] [, mode]` | an area saved in a string / drawn back (mode 0 as it is, 1 inverted, 2 OR, 3 AND, 4 XOR) | `GCOPY x, y, width, height` / `PASTE x, y`: one buffer, under 1024 pixels on a 16-colour screen (`?ILLEGAL QUANTITY`) | when the program keeps one shape (one string): `GCOPY` / `PASTE` (the pixel cursor for an omitted corner or place); a mode other than 0, an area known to be too large, several shapes warned |
| No counterpart | `WIDTH` (BASIC 65's `DMODE` thickness is not implemented), `PUDEF`, `SPRDEF`, `QUIT` | | left as written, warned |
| Other arguments | `BANK`, `WINDOW`, `RWINDOW(0/1)`, `MOVSPR n, distance;angle`, `PLAY` of a string variable | | warned |

The conversion keeps a little state where BASIC 7.0 has some and BASIC 65 has not, in names the program does not use
(two letters, as `zc`, `zx`, `zy`): the colour sources 0–6 (`zc(n)`, colours 0–15, set by `COLOR`, used by the drawing
statements' `PEN`) and the pixel cursor. Its subroutines — the waits before `PLAY`, the 40 / 80 columns, closing a
graphics screen, a turned `BOX`, a `CIRCLE` that `ELLIPSE` and `POLYGON` cannot draw — go after an `END` at the program's end.
Shapes match the C128's within a pixel: the C128 computes with an integer sine table (interpolated every 10°) and
rounds, BASIC 65's `POLYGON` truncates (`SIN(90)` is a hair under 1, so a vertex can land one pixel inside). Not converted: the hardware (`POKE`/`PEEK`/`SYS` to C128
addresses, the VDC 80-column screen's registers, the MMU).

Tests: `src/test/test_basic45.sh` (the rules), `src/test-resources/test_convert7_xemu.txt` (a C128 listing converted
and run by BASIC 65 on xemu: its music timed in frames, the variables, `TI`, the graphics statements, sprite and colour
numbers, the 40 columns, `$D011` written past raster line 255 with the IRQs still running, `SSHAPE`/`GSHAPE`).

### BASIC 3.5 (`--convert=basic35`)

```bash
basic45 --convert=basic35 game.prg -o game.bas     # a C16 / Plus/4 program ($1001)
```

BASIC 3.5 is BASIC 7.0's ancestor: its tokens are BASIC 7.0's `$CC`–`$FD` (`$CE` is `RLUM`, not a prefix), and its
graphics statements (`GRAPHIC` 0–4, `COLOR`, `DRAW`, `BOX`, `CIRCLE`, `PAINT`, `CHAR`, `LOCATE`, `RDOT`), `TI` / `TI$`
(60 per second on PAL too: the KERNAL adds a tick every fifth frame), variable names and reserved names are BASIC 7.0's,
converted by the rules above. It has no sprites, `PLAY` or `TEMPO`. What the TED makes different:

| | BASIC 3.5 (C16 / Plus/4) | BASIC 65 | The conversion |
|---|---|---|---|
| Colours | the TED's 16 hues (1–16) × 8 luminances (`COLOR source, colour [, luminance]`) | 16 colours (0–15) | the nearest: hues 1–10 the same, yellow-green and light green → light green, pink → light red, blue-green → cyan, dark blue → blue; white's luminances the greys (0–1 dark, 2–4 middle, 5–6 light, 7 white), red's, green's and blue's 5–7 their light colours; a colour expression through a table (`val(mid$("0001…", c * 2 - 1, 2))`, its luminance warned) |
| Colour codes in strings | `$96`–`$9B`: yellow-green, pink, blue-green, light blue, dark blue, light green; `$82` / `$84` flash on / off | light red, dark grey, grey, light green, light blue, light grey; `$82` underline off; flash `$0F` / `$8F` | rewritten to the nearest colour's code, the flash codes to BASIC 65's (`CHR$(n)` of a literal too) |
| `RLUM(n)` | a source's luminance | — | left as written, warned |
| `SOUND v, f, d` | voices 1–2 tones, 3 noise (on voice 2); f the TED's register (Hz = 110840.4375 / (1024 − f) on PAL, 111860.781 on NTSC); waits while the voice still sounds | voices 1–6, the SID's frequency (16.519 counts per Hz on PAL, 16.404 on NTSC: the clock of `b65.src`'s note tables); returns at once | the frequency converted (by the MEGA65's video mode, `$D06F` bit 7, read at the start), voice 3 → noise (waveform 3) on voice 2, a wait for the voice first (`SOUND`'s counter at `$12C4` + v − 1, busy below 128); `--ntsc`: durations × 5/6 |
| `VOL n` | 0–8 | 0–15 | n × 15/8 |
| `SCNCLR` | clears the screen in use (the bit map in graphics) | `SCNCLR colour` clears a graphics screen | `SCNCLR` with a screen open → `SCNCLR` with the background colour |
| The colour sources | background white, foreground black | | the kept sources start as the Plus/4's |

Tests: `src/test/test_basic45.sh`, `src/test-resources/test_convert35_xemu.txt` (a Plus/4 listing run by BASIC 65 on
xemu: a second `SOUND` waiting for the first, the frequency in BASIC 65's sound table, the colours, `TI`, graphics).

### Simons' BASIC (`--convert=simons`)

```bash
basic45 --convert=simons game.prg -o game.bas      # a C64 program ($0801) using Simons' BASIC
```

Simons' keywords are two bytes, `$64` and an index 1–127 (the table of TSB's source, `tsb_pt1.a`; TSB's own keywords —
`DO`, `CLS`, `X!`, `MAP`, `D!` — are read too: TSB's `CLS` / `X!` / `MAP` are `$64 $3C`–`$3E`, which BASIC V2's cruncher
makes `$64 $B3`–`$B1`). A text listing is tokenized as Simons' BASIC does: its keywords first, in its table's order, then
BASIC V2's (with `--convert=tsbneo`, TSB's own keywords too). Warnings name a message once, with the lines it applies to. The rules were checked against TSB's sources (Tuned Simons' BASIC, the debugged
superset) and the German C64-Wiki's pages, and run on xemu. BASIC V2's statements, `TI` / `TI$` (1/60 s, `"HHMMSS"`, as
the C128's) and the variable names are converted as for BASIC 7.0. The graphics are rewritten as BASIC 7.0's
statements and converted by its rules (above): a plot type is a colour source.

| Simons' BASIC | The conversion |
|---|---|
| `PROC name` / `EXEC name` / `CALL name` / `END PROC` | a label `proc_name:` / `GOSUB` / `GOTO` / `RETURN` (the name: the rest of the line, or up to a `:` as TSB allows) |
| `REPEAT` … `UNTIL c`; `LOOP` … `EXIT IF c` … `END LOOP` | `DO` … `LOOP UNTIL c`; `DO` … `IF c THEN EXIT` … `LOOP` (statements after the `EXIT IF` on a line of their own: BASIC 65 has no `EXIT IF`, and an `ELSE` there breaks the loop) |
| `IF c THEN a : ELSE : b` | `IF c THEN a : ELSE b` |
| `RCOMP a : ELSE : b` (the last `IF`'s result again) | each `IF`'s condition kept in a variable, `IF v THEN a : ELSE b` |
| `CGOTO e` | `GOTO n` for a number; else a table of the program's lines after the `END` (`IF v = 10 THEN 10` …): BASIC 65's `GOTO` takes a number only |
| `ON ERROR: handler` / `NO ERROR` / `ERRN` / `ERRLN` | `TRAP line` (a `GOTO` handler's line, or the handler moved to a line after the `END`) / `TRAP` / `ER` / `EL` (basic45 numbers the lines: `EL` is the new number) |
| `TRACE 10` / `TRACE n` / `RETRACE` | `TRON` / `TROFF` / dropped (it shows the traced lines again) |
| `RESET line` | `RESTORE line` |
| `DIV(a, b)`, `MOD(a, b)`, `FRAC(x)`, `EXOR(a, b)` | `INT(INT(a) / INT(b))`, `MOD(INT(a), INT(b))`, `SGN(x) * (ABS(x) - INT(ABS(x)))`, `((a) XOR (b))` (BASIC 65's `XOR` is an operator; also BASIC 7.0's `XOR(a, b)`) |
| `INST(n$, o$, p)` (overwrites), `INSERT(n$, o$, p)` (0-based), `PLACE(f$, s$)` | `LEFT$`/`MID$` expressions, `INSTR(s$, f$)` |
| `LIN`, `JOY`, `POT(n)`, `PENX` / `PENY`, `SOUND`, `GRAPHICS` | `RCURSOR , v` before the statement and `v`; `JOY(2)`; `POT(1)` / `POT(2)`; `LPEN(0)` / `LPEN(1)`; 54272; 53248 |
| `%bbbb bbbb`, `$hh hh` | the digits together (BASIC 65 reads both) |
| `PRINT AT(column, row) …` | `CURSOR column, row : PRINT …` (each `AT`) |
| `CENTRE s$`; `PAUSE ["text",] n`; `USE f$, s$` | `PRINT SPC((40 - POS(0) - LEN(s$)) / 2); s$;`; a wait of n seconds that RETURN ends (`TI`, `GET`); `PRINT USING f$; VAL(s$);` (BASIC 65 rounds) |
| `COLOUR border, background` | `BORDER`, `BACKGROUND` |
| `HIRES ink, paper` / `MULTI c1, c2, c3` / `NRM`, `CSET 0`/`1` | a 320×200 screen, cleared to the paper; the colour sources (0 paper or background, 1 ink or c1, 2 c2, 3 c3); `MULTI` keeps the screen, its x doubled from there on / the screen closed (and the character case) |
| `LOW COL` / `HI COL` | the colour sources changed / set back (BASIC 65's pixels have their own colours: no cell is recoloured) |
| `PLOT`, `LINE`, `REC x, y, w, h`, `BLOCK x1, y1, x2, y2`, `CIRCLE x, y, rx, ry`, `ARC x, y, start, end, step, rx, ry`, `ANGL x, y, angle, rx, ry` | `LINE`, `BOX` (outline, filled), `POLYGON` (30 sides: Simons' `CIRCLE` steps 12°) or BASIC 7.0's arc subroutine (0° up, clockwise, as BASIC 7.0's), a `LINE` from the centre; the plot type's colour by `PEN`, invert (2; `MULTI`'s 4) by XOR drawing (`DMODE 0, 1, 0, 0, 1`, the pen the two colours XORed) |
| `PAINT x, y, f`; `TEXT x, y, s$, f, height, spacing`; `TEST(x, y)` | `PAINT x, y, 1, colour` (the area of (x, y)'s colour); `CHAR` by 8-pixel columns (height kept, other spacings warned); the plot type from `PIXEL()` |
| `MOB SET n, block, colour, priority, multi`; `MMOB` / `RLOCMOB`; `CMOB`; `MOB OFF n`; `DETECT n` / `CHECK(a [, b])` | `SPRITE` (the data block warned: BASIC 65's sprite data is set with `SPRSAV`); `MOVSPR n, x1, y1 TO x2, y2, speed` and a wait for the move (`RSPPOS(n, 2)`; the speed approximate); `SPRCOLOR`; `SPRITE n, 0`; `BUMP(n + 1)` kept in a variable / 0 when the sprites (both) are in it, else 1 |
| `VOL n`; `DIR "$…"`; `DISK cmd$`; `DISPLAY` | `VOL` (the filter bits warned); `DIR`; `OPEN 15, 8, 15, cmd$ : CLOSE 15`; `KEY` |
| `FCHR`, `FCOL`, `FILL`, `INV`, `MOVE` (row, column, width, height …); `UPB` / `UPW` / `DOWNB` … `RIGHTW` | a subroutine each (its arguments in variables): loops of `PEEK` / `POKE` on the screen at `$0800` and the colours at `$D800`; the scrolls one character, the row or column that falls off a space (B) or put back at the other end (W) |
| `DESIGN type, address` and its `@` rows | the bytes computed from the rows when converting: a character (types 2, 3) → `CHAR DEF int(mod(address, 4096) / 8), b1, …, b8` (the C64's 4 KB set as `MEM` copies it: 0–255 upper case, 256–511 lower case, BASIC 65's numbering); a sprite (types 0, 1) → its 63 bytes (as hex) and its address in variables, copied by `MOB SET` to the sprite's data at `$0600` + 64 × n when the block's address (64 × block in text mode's VIC bank, `$C000` + 64 × block in the graphics one) is the `DESIGN`'s; the `@` rows `REM` lines |
| `WAVE voice, bits`; `ENVELOPE voice, a, d, s, r` | `POKE`s of the voice's control register / AD and SR (the MEGA65's first SID is at `$D400` too) |
| `MUSIC tempo, s$`; `PLAY 1` / `2` / `0` | a literal string's notes (letter, octave digit — the same octaves as BASIC 65's —, shifted letter sharp, C= letter flat, `Z` rest, `{f1}`–`{f8}` lengths, `{clear}1`–`3` the voice) as BASIC 65 `PLAY` strings, one per voice, rests keeping Simons' one note at a time; `TEMPO` and the unit (a whole note / 16, 8, … chosen so that BASIC 65's `TEMPO` is within 3 %) from the tempo / `PLAY` and a wait until it has played, `PLAY` alone, `SOUND CLR` (several `MUSIC`s: a variable says which) |
| `DUP(s$, n)`; `OUT` | the string built in front of the statement (a loop); `PRINT ERR$(ER)` |
| `LOCAL a, b$` / `GLOBAL` | the variables saved (then 0 / `""`) and a flag set / a subroutine putting back what each `LOCAL` saved, the latest first (not a stack: a procedure calling itself shares them) |
| `FETCH "allowed", length, v` | a subroutine: a `GETKEY` loop taking the allowed characters (`{home}` lower-case letters, `{down}` digits and punctuation, `{right}` all letters) up to the length, DEL, RETURN |
| `ROT r, size` / `DRAW "digits", x, y, f` | variables / a subroutine: 0–3 move, 5–8 draw a step right, up, down, left, turned by r × 45°; 9 ends |
| `CHAR x, y, code, f, zoom` | `TEXT` of the screen code's character (reverse codes and the lower-case set not shown) |
| `BCKGNDS b0, b1, b2, b3` | the extended background colour mode (`$D011` bit 6) and `$D021`–`$D024`; `NRM` turns it off |
| `SCRSV` / `SCRLD f, dev, 2, name$` | `OPEN` and a loop of `PRINT#` / `GET#`: TSB's file (a load address, then 1024 pairs: the colour, the screen code) |
| `COLD`; `DISAPA`, `SECURE`, `OPTION`, `PAGE`, `DELAY` | `END` (warned: Simons' erases the program); dropped (LIST's settings) |
| `FLASH colour [, speed]` / `OFF` | the VIC-III's blink attribute (colour RAM bit 4) set on the characters of that colour / cleared (warned: characters printed later do not flash, the speed is the MEGA65's) |
| `ON KEY "keys",: handler` / `DISABLE` / `RESUME` | a subroutine at each line's start (Simons' checks between statements): a key waiting in the queue (`$D619`, PETSCII) that the list holds is taken and the handler (the rest of the `ON KEY` line) run; keys not listed stay for the program's `GET`. `DISABLE` turns it off, `RESUME` returns and turns it on again (`RESUME NEXT` / `RESUME line`, TSB's after `ON ERROR`, are BASIC 65's). The key is in a variable, not `ST` (warned when the program reads `ST`) |
| `INKEY` | a subroutine in front of the statement: the keyboard's matrix row 0 (`$D614`/`$D613`: F1, F3, F5, F7) and the shift and C= keys (`$D611`): 1-8, 9-16 with C=, 0 none |
| `BFLASH n, c1, c2` / `0` (TSB: `OFF` / `ON`) | an IRQ routine at `$1600` (82 bytes; unused by BASIC 65's ROM, the DOS block buffer ends at `$15FF`) in front of the KERNAL's (`$0314`, changed by a `SYS` with interrupts off): the border between c1 and c2 every n frames (raster IRQs); `BFLASH 0` / `OFF` puts the KERNAL's back. It goes on after the program ends, as on the C64 |
| No counterpart (left as written, warned) | `CSET 2` (BASIC 65 closes the graphics screen), `SCRSV`/`SCRLD` of the bitmap, `OLD`, `DUMP`, `HRDCPY`, `COPY`, a `MUSIC` string that is not a literal |

**TSB and TSBneo.** TSB (Tuned Simons' BASIC) and TSBneo are supersets with forms of their own (checked against their
sources). `--convert=simons` converts them; `--convert=tsbneo` also takes TSBneo's `AT(row, column)` (TSB's and Simons'
are `AT(column, row)`), its `COLOUR b` (the border; TSB's: the background) and its `PAUSE` that any key ends:

| TSB | The conversion |
|---|---|
| `IF c THEN DO` … [`ELSE` alone at a line's end …] `DONE` | `IF c THEN BEGIN` … [`BEND : ELSE BEGIN` …] `BEND` (statements between `THEN` and `DO`, and after `DONE`, never run in TSB: dropped); `DO NULL` → `GETKEY` |
| `D!PEEK(a)` / `D!POKE a, v`; `D!` alone | `WPEEK(a)` / `WPOKE a, v` (16 bits, low byte first; a zero-page address warned); dropped (it deletes lines) |
| `CLS`, `DIV` as a statement; `CHECK` as a statement | `PRINT CHR$(147);`; dropped (it indexes the procedures) |
| `NRM(x$)`; `$$n` | `DEC()` of the hex digits (an 8- or 16-digit binary string is not converted: warned); `RIGHT$(HEX$(n), 2)` (4 digits above 255) |
| `MEM CLR a, n [, b]`; `MEM` | a `POKE` loop; dropped (`CHAR DEF` changes BASIC 65's own character set) |
| `SOUND v, f`; `WAVE v, c, pulse` | the voice's frequency registers (voice 4: the filter's cutoff); the control register and the pulse width |
| `INSERT f$, row, column, width, height, colour` | a subroutine: a box of the 9 characters (corners, edges, the fill) by `POKE`s, its colour |
| `MULTI ON` / `OFF` / `n`; `MOB [OFF] n`, `MOB ON n`; `MOBCOL n, c` (TSBneo) | the mode only (x 0–159 from there on, as TSB's); `SPRITE n, 0` / `1`; `SPRITE n, , c` |
| `COLOUR [b] [, p] [, i]` | `BORDER`, `BACKGROUND`, `FOREGROUND` for the colours given (`COLOUR , i`: the text's colour) |
| `MMOB n, x1, y1, x2, y2` / `RLOCMOB n, x, y` (size, speed left out) | as the full forms, the sprite's size kept |
| `AT(a, b)` before `CENTRE`'s, `USE`'s or `FETCH`'s text; `CENTRE s$, width` | `CURSOR` first; centred in the width |
| `USE f$, n1, n2 …[;]` | a `PRINT USING` per field of a literal format (BASIC 65's formats one value), the text between printed; a new line unless `;`; `USE n` (the drive) dropped |
| `SCRLD` / `SCRSV DEF hi, pages, mode, sa` … `RESTORE` | the next `SCRLD` / `SCRSV` of a channel of that parity: `BLOAD` / `BSAVE` at `hi` × 256 (warned: C64 machine code does not run on the MEGA65) |
| `KEY GET v$`; `VOL ON` / `OFF`; `FLASH OFF`; `ERROR`; `COLD` | `GETKEY v$`; `VOL 10` / `0`; as `OFF`; `PRINT DS$`; `END` |
| a procedure's name alone (keywords in it too: `.RESET`); `JOY(port)` | `GOSUB`; `JOY(port)` |
| Left as written | `X!`, `MAP` (TSB's hooks for extensions), the `DUP` and `MOD` statements, `MEM`'s RAM-expansion forms |

Tests: `src/test/test_basic45.sh`, `src/test-resources/test_simons_xemu.txt` (a Simons' listing run by BASIC 65 on
xemu: procedures, loops, the functions, `ELSE` / `RCOMP`, `ON ERROR`, `CGOTO`, `PRINT AT` and `LIN`, the graphics by
their pixels, `PAUSE` and `TI`, `COLOUR`, a sprite moved, the text-screen rectangles and scrolls, `DESIGN` of a character
and of a sprite, `FLASH`, `DUP`, `LOCAL`/`GLOBAL`, `DETECT`/`CHECK`, `ROT`/`DRAW`, `CHAR`, `BCKGNDS`, `MUSIC` timed, `INKEY` and
`ON KEY` with keys held through `$D615`/`$D616`, `BFLASH` counted in frames),
`src/test-resources/test_tsbneo_xemu.txt` (TSBneo's forms). TSB's example game (`REACTOR TSB`, from its `reactor_tsb.d64`,
a TSBneo program) converts with `--convert=tsbneo` and builds; what is warned is what does not carry over (its machine
code at `$7A00`, `D!PEEK` of the C64's zero page, `CSET 2`, `TEXT`'s spacing).

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

`--docs <file>` writes markdown listing the labeled sections of the input file:

```markdown
# program.bas

## Program Structure

### 1. main_loop (Line 10)

*No description*
```

A label is recognised when `name:` is followed by a space or the end of the line, with its BASIC line number. The comment lines (`'` or a `#` line that is not a directive) right before a label are its description; the input file is read as written, so `#include`d files are not documented. If there are no labels the file says `No labeled sections found.`

## Output Format

1. Load address (2 bytes, little-endian; default `$2001`)
2. One record per line, the first at the load address: link to the next line's address (2 bytes; the last line's link points at the end marker), line number (2 bytes), tokens, `$00`
3. Program end `$00 $00`

## Known Problems

- `--link` uses the ZP calling convention and `c45.lib`; for stack-convention C, declare the called functions `__fastcall__`.
- `--send`'s default commands (`etherload -r`, `m65 -F -r`) are untested; use `--send-cmd` with the options your tools need.
- `--compile` compiles a subset of BASIC 65 ([Compiling](#compiling-compile)), and not with `--link` or `asm` blocks yet.

`test_basic45_xemu.bas` (as is, `--minify` and `--preserve-spaces`), `test_basic45_features_xemu.bas` (as is, decompiled and rebuilt, and formatted), `test_basic45_link_xemu.bas` (C, asm, incbin, datainc) and `test_basic45_numbered_xemu.bas` (a numbered program; `MOD`, `LOG10`, `WPEEK`, `POINTER`, `WPOKE`, `<<`, `>>`) are run by BASIC 65 on xemu; `src/test/test_basic45.sh` (`make test`) checks the tokens and messages.

## See Also

- **cc45** — C compiler
- **ca45** — Assembler
- **ln45** — Linker (`-m` symbol map)
- **disk45** — put the `.prg` on a disk image
