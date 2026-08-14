# basic45 — MEGA65 BASIC Program Generator

**Version:** 1.0

## Overview

`basic45` converts plain-text BASIC source code into tokenized MEGA65/CBM BASIC program files (`.prg` format). It supports:

- **BASIC keywords** — Automatic tokenization (PRINT, SYS, FOR, IF, etc.)
- **PETSCII strings** — Quoted text with escape sequences and color codes
- **Symbol substitution** — Replace placeholders like `{myroutine}` with addresses from cc45 symbol tables
- **Custom load addresses** — Change starting address (default: $0801)

## Syntax

```bash
basic45 [options] <input.bas> -o <output.prg>
```

## Options

- **`-o <file>`** — Output file (default: `input.prg`)
- **`--symbols <file>`** — Load symbol table for address substitution
- **`--load <addr>`** — Load address in hex (default: 0x0801)
- **`-v, --verbose`** — Verbose output
- **`--version`** — Show version
- **`-h, --help`** — Show help

## Example

### Basic usage
```bash
basic45 mygame.bas -o mygame.prg
```

### With symbol substitution
```bash
cc45 routines.c -o routines.prg -E symbols.txt
basic45 program.bas --symbols symbols.txt -o program.prg
```

## Input Format

### Lines and Statements

Each line must start with a line number. Statements are separated by `:`.

```basic
10 print "hello"
20 sys 2000
30 goto 10
```

### Strings and Escape Sequences

String literals use double quotes. Inside strings, use escape sequences with curly braces:

**Color codes:**
```
{black} {white} {red} {cyan} {magenta} {purple} {green} {blue} {yellow}
```

**Control characters:**
```
{home} {clear} {esc} {back} {del} {insert} {reverse} {underline} {bell}
{up} {down} {left} {right} {crsr-up} {crsr-down} {crsr-left} {crsr-right}
{tab} {f1} {f2} {f3} {f4} {f5} {f6} {f7} {f8}
```

**Example:**
```basic
10 print "{clear}{white}hello{yellow} world"
```

### Symbol Substitution

Replace `{symbolname}` with the address loaded from a symbol table:

```basic
10 sys {myroutine}
20 call {print_string}
```

Symbol file format (one per line):
```
2000 myroutine
3000 print_string
```

The address is in hexadecimal; `basic45` will substitute the decimal equivalent.

## Output Format

The generated `.prg` file follows CBM BASIC format:

1. **Load address** (2 bytes, little-endian): Default $0801
2. **Tokenized program:**
   - Link to next line (2 bytes)
   - Line number (2 bytes)
   - Tokenized statements
   - End-of-line marker (0x00)
3. **Program end** (0x0000)

## Supported BASIC Keywords

All CBM BASIC v7 keywords are supported, including:

- Flow control: END, FOR, NEXT, IF, THEN, ELSE, GOTO, GOSUB, RETURN
- I/O: PRINT, INPUT, OPEN, CLOSE, GET, GETKEY
- Storage: READ, DATA, SAVE, LOAD, VERIFY
- Manipulation: LET, DIM, DEF, POKE, PEEK
- System: SYS, RUN, STOP, CONT, NEW, LIST, CLR
- Disk: DLOAD, DSAVE, DVERIFY, DCLEAR

Plus 40+ additional v7 tokens.

## Limitations

- BASIC programs must fit in available memory after the $0801 load address
- Symbol substitution only supports numeric addresses (not expressions)
- Line numbers must be in ascending order

## Integration with cc45

To compile C code and call it from BASIC:

```bash
# 1. Compile C to assembly (no linking)
cc45 routines.c -S -o routines.s45

# 2. Assemble to object file
ca45 routines.s45 -c -o routines.o45

# 3. Link to get executable and symbol table
ln45 routines.o45 -o routines.prg -E symbols.txt

# 4. Generate BASIC program that calls the compiled code
basic45 myprogram.bas --symbols symbols.txt -o myprogram.prg
```

Then load both `.prg` files and call compiled routines via SYS.

## See Also

- **cc45** — C compiler
- **ca45** — Assembler
- **ln45** — Linker
