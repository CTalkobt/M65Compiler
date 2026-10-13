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

`cvt_asm -l` (or `--list-formats`) prints the registered parsers and writers. The conversion quality differs a lot between formats — see [Limitations](#limitations).

## Usage

```bash
cvt_asm [options] <input-file> [<input-file> ...]
```

| Option | Description |
|--------|-------------|
| `-f <format>` | Input format (default: auto-detect, see below). Must be a canonical name from the table |
| `-t <format>` | Target format (default: `ca45`, or `default_format` from `.cvtasmrc`) |
| `-o <path>` | Output file for one input; for several inputs a directory (created if needed) receiving `<basename><target extension>` |
| `-l`, `--list-formats` | List supported formats and exit |
| `-v` | Verbose: detection, parse/write progress and warnings on stderr, plus a performance summary on stdout |
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

## Limitations

Known problems of the current version (found while checking this page; ca45 input):

- A label and an instruction or directive on the same line (`loop: lda counter`, `counter: .byte 0`) keep only the label — the rest of the line is lost.
- Data directives lose their commas (`.byte 1, 2` → `.byte 1 2`).
- A label whose name is an opcode in another case (`tab:`) is upper-cased (`TAB:`).
- `.global`/`.extern` of names: the ca45 writer keeps them, the ACME writer prints bare `!global`/`!extern` without names, the KickAssembler writer swaps them (`!import` for exports, `!export` for imports) and adds bare `!global`/`!extern`, the ca65 writer adds bare `.export`/`.import`.
- Directives the ca45 parser does not know are passed through unchanged instead of being reported.
- Macros are expanded by the ca45 parser; the macro definitions are not reproduced in the output.
- With `-v` the performance summary goes to stdout, mixed with the converted source when that also goes to stdout.

Use `--validate` and `--stats` to see how much of a file survives a conversion, and check the result by assembling it.

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
