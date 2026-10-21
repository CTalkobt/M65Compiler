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
- **Listing, source map, decompiler** — `--list`, `--source-map`, `--decompile` ([Listing, Source Map and Decompiling](#listing-source-map-and-decompiling))
- **Disk image, emulator, hardware** — `--disk`, `--emulator` (xemu), `--send` (a MEGA65), `--watch`
- **Formatting** — `--format` ([Formatting](#formatting))
- **Markdown documentation** of labeled sections (`--docs`)

See [Known Problems](#known-problems) for the remaining limits.

## Syntax

```bash
basic45 [options] <input.bas> [-o <output.prg>]
basic45 --decompile <program.prg> [-o <output.bas>]
```

Without `-o` the output is the input name with its extension replaced by `.prg` (`.bas` when decompiling; `-o -` writes the source to standard output). A `.prg` input is decompiled. With no input file, the usage text is printed and the exit status is 1. Unrecognised options starting with `-` are ignored silently.

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
| `--list-tokens` | List all keywords with their token values and exit |
| `-v`, `--verbose` | Verbose output |
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

Keywords are recognised case-insensitively as whole words: `print`, `PRINT` and `Print` all tokenize, `printx` does not. `PRINT#1` and `INPUT#1` are `PRINT#` / `INPUT#` followed by `1`; `GETKEY` is the two tokens `GET KEY`, as BASIC stores it. Numbers keep their exponent (`1E5`, `2.5E-3`). Run `basic45 --list-tokens` for the table (115 single-byte tokens `$80`-`$FD`, 73 two-byte tokens `$FE xx` such as `BANK`, `SPRITE`, `DMA`, `FAST`, `PALETTE`).

The operators `+ - * / ^ > = <` are tokens (`$AA`-`$B3`), as BASIC stores them; `?` is `PRINT`, and `TAB(` / `SPC(` are tokens. After `REM` the rest of the line, and after `DATA` the items up to the next `:` outside quotes, are stored as text (not tokenized). Variable names and other text outside strings are converted to PETSCII upper case. A space is inserted after a keyword that is followed by a name, number or operator.

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

## Formatting

`basic45 --format prog.bas` prints the source with keywords in lower case, one space between words outside strings, code indented two spaces plus two per open block (`if … then`/`else`/`endif`, `proc`/`endproc`, `select`/`case`/`endselect`, `for`/`next`, `do`/`loop`) and labels in column 0. Strings, `REM` and `DATA` text, `#` lines, `'` comments (re-indented) and `asm` blocks are kept as written. With `-o` the result goes to a file. The formatted source builds to the same program, and formatting it again changes nothing.

## Listing, Source Map and Decompiling

`--list` prints, for each BASIC line, its number, address, source file and line, and the source text:

```
   10  $2001  menu.bas:5         start:
   20  $2007  menu.bas:6         print "{clear}{white}basic 65 + c routines"
```

`--source-map <file>` writes the same mapping as JSON (`compiledLine`, `sourceLine`, `sourceFile`, `sourceCode` per line), to find the source of `?SYNTAX ERROR IN 230`.

`--decompile` (or a `.prg` input) turns a tokenized BASIC program back into basic45 source: keywords and names in lower case, strings with `{name}` escapes (`{name*n}` for runs, `{$xx}` for a byte without a name), and labels `l<number>` on the lines that `GOTO`, `GOSUB`, `ON ... GOTO`, `THEN`, `ELSE`, `RESTORE`, `TRAP`, `RESUME` or `RUN` refer to. Building the result with basic45 gives the same program (with `--keep-names` if the original's names were long).

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

`test_basic45_xemu.bas` (as is, `--minify` and `--preserve-spaces`), `test_basic45_features_xemu.bas` (as is, decompiled and rebuilt, and formatted) and `test_basic45_link_xemu.bas` (C, asm, incbin, datainc) are run by BASIC 65 on xemu.

## See Also

- **cc45** — C compiler
- **ca45** — Assembler
- **ln45** — Linker (`-m` symbol map)
- **disk45** — put the `.prg` on a disk image
