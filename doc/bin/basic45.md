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
- **`--labels`** — Enable label support (use symbolic labels instead of line numbers)
- **`--label-table <file>`** — Output label→line number mapping table
- **`--docs <file>`** — Generate markdown documentation to file
- **`--increment <n>`** — Line number increment (default: 10)
- **`-I <path>`** — Add include search path for `#include` directives
- **`--list-tokens`** — List all supported BASIC keywords and exit
- **`-v, --verbose`** — Verbose output
- **`--version`** — Show version
- **`-h, --help`** — Show help

## Examples

### Basic usage (normal mode with line numbers)
```bash
basic45 mygame.bas -o mygame.prg
```

### Label mode
```bash
basic45 mygame.bas --labels -o mygame.prg
```

### Label mode with mapping table
```bash
basic45 mygame.bas --labels --label-table labels.txt -o mygame.prg
cat labels.txt
```

Output:
```
Label           Line Number     Memory Address
main_loop       20              0x0803
sub1            60              0x0835
```

### With symbol substitution
```bash
cc45 routines.c -o routines.prg -E symbols.txt
basic45 program.bas --symbols symbols.txt -o program.prg
```

### Combined: labels + symbol substitution
```bash
cc45 routines.c -o routines.prg -E symbols.txt
basic45 program.bas --labels --symbols symbols.txt -o program.prg
```

### With documentation generation
```bash
basic45 program.bas --labels --docs program.md -o program.prg
```

## Documentation Generation

The `--docs` option automatically generates markdown documentation from your BASIC source code, extracting labeled sections, subroutines, and comments.

### How It Works

When you enable documentation generation with `--docs <file>`, basic45:

1. **Extracts labeled sections** — Identifies all labels (in label mode) and subroutines
2. **Collects comments** — Associates comments (lines starting with `#`) with the following label/section
3. **Generates markdown** — Outputs a structured markdown document with program sections

### Expected Output Format

The generated markdown file follows this structure:

```markdown
# Program Name

## Program Structure

### 1. main_loop (Line 10)
This is the main loop that handles game input and rendering.

### 2. update_physics (Line 50)
Updates object positions and handles collision detection.

### 3. render_screen (Line 100)
Draws all objects to the MEGA65 screen.
```

**Key features:**
- Automatically numbered sections
- Line numbers for each labeled section
- Comments from the BASIC source preserved and associated with sections
- Blank lines and preprocessor directives ignored

### Comment Association

Comments placed immediately before a label are associated with that section:

```basic
# Initialize the game state
# and enter the main loop
main_loop:
print "game started"
sys {game_init}

# Handle user input and update game state
update:
print "waiting for input"
```

This generates documentation like:

```markdown
### 1. main_loop (Line 10)
Initialize the game state and enter the main loop

### 2. update (Line 50)
Handle user input and update game state
```

### Workflow Example

Complete workflow with documentation:

```bash
# 1. Compile C routines to executable and generate symbols
cc45 game_logic.c -o game_logic.prg -E symbols.txt

# 2. Generate BASIC program with documentation
basic45 game.bas --labels --symbols symbols.txt --docs game.md -o game.prg

# 3. View the generated documentation
cat game.md

# 4. Keep documentation up-to-date when source changes
basic45 game.bas --labels --symbols symbols.txt --docs game.md -o game.prg
```

## Input Format

### Preprocessor Features

basic45 includes a full C-style preprocessor that processes source before compilation:

**Macro definitions:**
```basic
#define MAX_HEALTH 100
#define SCREEN_ADDR 0xD000

print MAX_HEALTH
poke SCREEN_ADDR, 0
```

**Conditional compilation:**
```basic
#define DEBUG 1

#ifdef DEBUG
print "debug mode active"
#endif

#ifndef RELEASE
print "not a release build"
#endif

#ifdef NDEBUG
#else
print "assertions enabled"
#endif
```

**File inclusion:**
```basic
#include "sprite_data.bas"
#include "common_routines.bas"
```

Use `-I <path>` to add include search paths:
```bash
basic45 program.bas -I ./includes/ -I ./lib/ -o program.prg
```

**Common patterns:**
```basic
#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 16
#define MAX_SPRITES 10

# Create reusable library files with #include
#include "sprite_lib.bas"

#ifdef ENABLE_SOUND
#include "sound.bas"
#endif
```

### Lines and Statements

Each line must start with a line number (in normal mode) or a label/statement (in label mode). Statements are separated by `:`.

**Normal mode (with line numbers):**
```basic
10 print "hello"
20 sys 2000
30 goto 10
```

**Label mode** (with `--labels` flag):
```basic
start:
print "hello"
sys 2000
goto start
```

### Line Increment Control

By default, auto-generated line numbers increment by 10 (10, 20, 30, ...). Use `--increment` to change this:

**Default (10):**
```bash
basic45 program.bas --labels -o program.prg
# Generated line numbers: 10, 20, 30, 40, ...
```

**Increment by 1:**
```bash
basic45 program.bas --labels --increment 1 -o program.prg
# Generated line numbers: 1, 2, 3, 4, ...
```

**Increment by 5:**
```bash
basic45 program.bas --labels --increment 5 -o program.prg
# Generated line numbers: 5, 10, 15, 20, ...
```

This is useful when:
- You need tightly packed line numbers (increment 1)
- You want extra space for future edits (increment 100)
- You're porting code from existing BASIC programs with specific line number spacing

### Labels and Comments (Label Mode)

When `--labels` is enabled:

- **Labels** — Identifier followed by `:` at the start of a line marks a label
  ```basic
  main_loop:
  print "in loop"
  goto main_loop
  ```

- **Comments** — Lines starting with `#` are stripped (not emitted as REM)
  ```basic
  # This comment is silently removed
  print "hello"
  # Another comment
  ```

- **Empty lines** — Blank lines are skipped and not included in output
  ```basic
  print "line 1"
  
  print "line 2"  # These become consecutive lines
  ```

- **Line number generation** — Line numbers are auto-generated (10, 20, 30, ...)
- **Label resolution** — GOTO/GOSUB with label names are automatically resolved to line numbers

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
- **Normal mode**: Line numbers must be in ascending order
- **Label mode**: Undefined labels in GOTO/GOSUB will cause runtime errors (no compile-time validation)

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
