# cvt_asm — Assembler Format Converter

`cvt_asm` converts 6502/45GS02 assembly source between assembler syntaxes. Each input format has a parser that builds a common semantic representation (`AsmIR`), and each output format has a writer that prints it.

## Formats

| Name (`-f`/`-t`) | Assembler | Auto-detected extensions | Output extension |
|------------------|-----------|--------------------------|------------------|
| `ca45` | ca45 (this toolchain) | `.s45`, `.s`, `.asm` | `.s45` |
| `ca65` | cc65's ca65 | `.s65`, `.c65` | `.s65` |
| `acme` | ACME | `.a65` | `.a65` |
| `kickassembler` | KickAssembler | `.ka` | `.ka` |
| `merlin64` | Merlin 64 | `.m64`, `.m6502` | `.m64` |
| `oscar` | Oscar (Commodore 6502 assembler) | `.s6502` | `.s6502` |
| `x65` | x65 | `.x65` | `.x65` |

`cvt_asm -l` (or `--list-formats`) prints the registered parsers and writers. See [Readers and writers](#readers-and-writers) for what each format's reader takes and [Limitations](#limitations) for what does not convert.

## Usage

```bash
cvt_asm [options] <input-file> [<input-file> ...]
```

| Option | Description |
|--------|-------------|
| `-f <format>` | Input format (default: auto-detect, see below). Must be a canonical name from the table |
| `-t <format>` | Target format (default: `ca45`, or `default_format` from `.cvtasmrc`) |
| `-o <path>` | Output file for one input (unless `<path>` is an existing directory); for several inputs a directory (created if needed) receiving `<basename><target extension>` |
| `-l`, `--list-formats` | List supported formats and exit |
| `-v` … `-vvvv`, `-v0` | Log on stderr as `[cvt_asm:LEVEL] …` (the toolchain's format, `include/Logger.hpp`); cumulative, at most `-vvvv`; `-v0` off: detection, parse/write progress and the batch summaries; with `-v` the performance summary too (stderr) |
| `--stats` | Print conversion statistics (source/target counts, losses, preservation %) instead of the output |
| `--validate` | Convert to the target and back, and report how much was preserved (`LOSSLESS` at ≥95%) instead of the output |
| `--metrics` | Print the performance summary (time, files, include-cache hits) after converting |
| `--parallel` | Convert several files in parallel |
| `--threads <n>` | Worker threads for `--parallel` (0 = number of cores) |
| `--clear-cache` | Clear the include-file cache before the batch |
| `--init-config` | Write a default `.cvtasmrc` in the current directory and exit |
| `--show-config` | Print the configuration in effect and exit |
| `--help` | Show usage |
| `--version` | Show version |

With one input and no `-o`, the result goes to stdout. With several inputs and no `-o`, each result is written to `<basename><target extension>` in the current directory. The exit status is 1 if any file failed.

### Input format detection

Without `-f`, cvt_asm looks for an editor modeline in the first or last five lines — `vim: ft=<name>` / `vim: filetype=<name>` or `-*- mode: <name> -*-` — then falls back to the file extension. Modeline names may also be aliases: `45gs02`/`mega65` (ca45), `cc65`/`c65` (ca65), `a65` (acme), `merlin`/`m64`/`m6502` (merlin64), `commodore` (oscar), `kickass`/`ka` (kickassembler).

```bash
cvt_asm input.s45                       # ca45 -> ca45 (normalise) to stdout
cvt_asm -t kickassembler input.s45 -o out.ka
cvt_asm -f ca65 -t acme *.s65           # batch: writes <name>.a65 files
cvt_asm -t ca65 -o converted/ a.s45 b.s45
cvt_asm --stats -t acme input.s45
cvt_asm --validate -t ca65 input.s45
```

## Configuration File (`.cvtasmrc`)

cvt_asm looks for `.cvtasmrc` in the current directory and then in each parent directory, and uses the first one found. Lines starting with `;` or `#` are comments; settings are `key = value` in sections. Command-line options override them.

```ini
[general]
default_format = ca45          ; default -t
default_threads = 0            ; --threads (0 = auto)
output_dir =                   ; default -o
validate_round_trip = false    ; act as if --validate were given
clear_cache_per_batch = false  ; --clear-cache
parallel_default = false       ; --parallel
metrics_default = false        ; --metrics

[paths]
library_paths = .:./include:./macros

[performance]
enable_profiling = false
show_timing_breakdown = false
```

`library_paths`, `enable_profiling` and `show_timing_breakdown` are read and shown by `--show-config` but not used by the conversion.

## Semantic IR (AsmIR)

Defined in `include/AsmIR.hpp`.

- **Statement types**: `INSTRUCTION`, `DIRECTIVE`, `LABEL`, `COMMENT`, `BLANK`.
- **Addressing modes**: `IMPLIED`, `IMMEDIATE`, `ZERO_PAGE`, `ZERO_PAGE_X`, `ZERO_PAGE_Y`, `ABSOLUTE`, `ABSOLUTE_X`, `ABSOLUTE_Y`, `INDIRECT`, `INDIRECT_X`, `INDIRECT_Y`, `STACK_RELATIVE`, `FRAME_RELATIVE`, `INDEXED_PAIR` (45GS02 `$addr,XY`).
- **Directive types**: `BYTE_DATA`, `WORD_DATA`, `LONG_DATA`, `FILL`, `ALIGN`, `ORG`, `SEGMENT`, `GLOBAL`, `EXTERN`, `WEAK`, `CONST`, `CPU`, `FUNCTION`, `ZP_USES`, `ZP_CLOBBERS`, `REG_CLOBBERS`, `FLAG_CLOBBERS`, `LOC`, `OTHER`.

The ca45 parser runs the real ca45 preprocessor and parser (`Preprocessor`, `AssemblerLexer`, `AssemblerParser::pass1`), so macros and conditionals are expanded and the IR holds the expanded statements. The other parsers are hand-written for their syntax.

## Architecture

Parsers and writers are registered by name in `registerFormats()` in `src/main/cvt_asm_main.cpp`:

```cpp
ParserRegistry::getInstance().registerParser("ca65", []() -> AsmParser* { return new Ca65Parser(); });
WriterRegistry::getInstance().registerWriter("ca65", []() -> AsmWriter* { return new Ca65Writer(); });
```

A parser derives from `AsmParser` (`include/AsmParser.hpp`) and implements `AsmIR::Module parse(const std::string& source)`; a writer derives from `AsmWriter` and implements `std::string write(const AsmIR::Module& module)`. Both collect errors and warnings (`addError`/`addWarning`, `hasErrors`, `getErrors`, `getWarnings`).

To add a format:
1. Add `include/XyzParser.hpp`/`src/main/XyzParser.cpp` and `include/XyzWriter.hpp`/`src/main/XyzWriter.cpp`.
2. Register both in `registerFormats()`.
3. Add `XyzParser.o XyzWriter.o` to `LIB45_TOOLS_OBJS` in the `Makefile` (cvt_asm links `lib45-tools.a`).
4. Optionally map its extension and aliases in `src/main/FormatDetection.cpp` and its output extension in `getFormatExtension()`.

## Conversion Examples

Output of the current build.

**ca45 → ca45** (normalisation; instructions are re-indented):

```asm
main:
    lda #$42
    sta $d020
    rts
```
becomes
```asm
main:
  lda #$42
  sta $d020
  rts
```

**ca45 → kickassembler / ca65**: the same lines with four-space indentation. **merlin64 / oscar**: labels without a colon, tab-indented instructions.

**KickAssembler namespaces** are flattened to dotted labels:

```asm
!namespace game
    main:
        lda #$42
        rts
!endnamespace
```
→ (`cvt_asm n.ka`)
```asm
game.main:
  LDA #$42
  RTS
```

## Readers and writers

The six non-ca45 readers share one line-based reader (`DialectReader`), set up per dialect, which reads a line as ca45's own reader does: a label, then a constant, an origin, a directive or an instruction, with its comment. An instruction's addressing mode comes from its operand (`#e`, `e,x`, `e,y`, `(e,x)`, `(e),y`, `(e)`, `e`); another operand (and any operand of an instruction that is not a 6502/65C02/45GS02 opcode, such as ca45's simulated `push a` or `add.16`) is kept as written. A directive the dialect's table does not know is passed on under its own name, as the writers pass on ca45-only directives, and macro definitions are read into the module (their calls are expanded).

| Reader | Labels | Constants, origin | Data and strings | Symbols, other |
|--------|--------|-------------------|------------------|----------------|
| `ca65` | `name:`, `@local:` | `n = e`, `n := e`, `.org` | `.byte`/`.byt` (strings ASCII), `.word`/`.addr`, `.dword`, `.res` | `.import`/`.export` (also `zp`), `.segment`, `.code`/`.data`/`.bss`/`.rodata`, `.setcpu`; `.p02`, `.p4510`, ... dropped |
| `acme` | `name:` or a word in column 1 | `n = e`, `*= e` | `!byte`/`!by`/`!8`, `!word`/`!wo`/`!16`, `!32`, `!text`/`!tx` (ASCII), `!pet` (PETSCII), `!scr` (screen codes), `!fill`, `!align`, `!binary` | `!cpu m65` (45GS02); `!to`, `!zone`, `!sl` dropped; `!macro n { }` called as `+n` |
| `kickassembler` | `name:` | `.const`/`.label n = e`, `.var`, `*=` / `.pc =` | `.byte`, `.word`, `.dword`, `.text` (in the current `.encoding`: screencode_mixed by default, petscii_mixed/upper, screencode_upper, ascii), `.fill`, `.align` | `.cpu _45gs02`; `//` and `/* */` comments; `.macro n(a) { }` called as `n(a)` |
| `merlin64` | column 1, no `:` | `n EQU e`, `n = e`, `ORG` | `DFB`/`DB`, `HEX`, `DW`/`DA`, `ASC` (ASCII), `DS` | `ENT`/`EXT`; `*` comment lines; `LST`, `MX`, `XC`, ... dropped |
| `oscar` | column 1, no `:` | `n = e`, `ORG` | `BYTE`/`DB`, `WORD`/`DW` | `GLOBAL`/`EXTERN` |
| `x65` | `name:` or a word in column 1 | `n = e`, `.org` | `.byte`, `.word`, `.long`, `dc.b`/`dc.w`/`dc.l`, `.text` | `.import`/`.export`, `xref`/`xdef` |

Strings and character constants keep their bytes: a reader writes a dialect's strings as ca45 `.ascii` (plain ASCII) or `.byte` lists in that dialect's encoding, and `'c'` constants as numbers; the writers write ca45's `.text` (PETSCII), `.screencode` and `.ascii` as `.byte` lists of the bytes ca45 makes (the string in the comment), and its `'c'` (PETSCII: `'a'` is `$41`) as numbers. The Merlin 64 writer uses Merlin's keywords (`DFB`, `DW`, `ASC`, `DS`, `ENT`, `EXT`, directives indented out of the label column); a ca45 directive with no keyword in Merlin or Oscar is written `.name`.

`test_cvt_asm.sh` (in `make test`) converts every test source that ca45 assembles as a flat binary to each dialect and back, and checks that it assembles to the same bytes; and converts a program written in each dialect's own syntax (ca65, ACME, Merlin 64, KickAssembler, x65) and checks it against the same program written for ca45.

## Limitations

**ca45 input** is read from the (preprocessed) source as written: labels keep their spelling, operands and directive arguments their expressions and commas, constants (`name = expr`) and comments stay in place, and a label and a statement on one line both survive. ca45's own parse supplies the addressing modes. What only ca45 understands — simulated instructions (`add.16`), `proc`/`endproc`, `.repeat`, `(zp),z`, `[zp],z`, stack-relative operands — is written out as it was, with a warning (`'lda ($20),z' is ca45-specific and kept as written`). `#define` macros are expanded by the preprocessor; their definitions are not reproduced. ca45 → ca45 is lossless: `test_cvt_asm.sh` (in `make test`) converts every test source that ca45 assembles as a flat binary and checks that it assembles to the same bytes.

**Writers**: `.global`/`.extern`/`.weak` are written once, from the symbol table (`.export`/`.import` for ca65 and x65, `GLOBAL`/`EXTERN` for merlin64 and oscar). ACME and KickAssembler assemble one program without a linker, so they get comments (`; global: start`, `; extern: putc (define it before assembling)`). Constants are `name = expr` (`.const name = expr` for KickAssembler); the 45GS02 is `!cpu m65` for ACME and `.cpu _45gs02` for KickAssembler.

Known problems:

- A reader knows the common directives of its assembler, not all of them: scoping (`.proc`, `!zone` blocks, KickAssembler namespaces), conditional assembly in the dialect's own syntax, KickAssembler's script language and `.fill` with an expression of the index are not translated; an unknown directive reaches ca45 under its own name, where ca45 reports it.
- Merlin's `ASC` is taken as ASCII as written (the Apple II versions set bit 7 for some delimiters).
- ca65's `.asciiz` and ACME's `!24` have no ca45 equivalent yet and are passed on unchanged.

Warnings (unknown directives, constructs kept as written) always go to stderr; `-v`'s and `--metrics`' performance summary goes to stderr too, so it never mixes with converted source on stdout. Use `--validate` and `--stats` to see how much of a file survives a conversion, and check the result by assembling it.

## Error Handling

Parse errors are printed per file and the file counts as failed:

```
$ cvt_asm bad.s45
  Parse error: :2:1: error: Error parsing instruction 'lda': :2:11: error: Expected )
```

Other errors: `Error: Could not auto-detect input format for 'X'. Specify with -f <format>`, `Error: Unsupported input format 'X'` / `Unsupported target format 'X'`, `Unknown option: X`.

## Build

```bash
make bin/cvt_asm
```

## See Also

- [ca45](ca45.md) — ca45 assembler
- [CONFIGURATION](CONFIGURATION.md) — configuration files of the toolchain
- [lib45](../architecture/lib45.md) — `.o45` relocatable object format
