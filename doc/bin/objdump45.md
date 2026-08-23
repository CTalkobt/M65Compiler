# objdump45 — Object File Inspector and Disassembler for .o45 / .o65

`objdump45` displays information from `.o45` and `.o65` relocatable object files, `.prg` executables, and raw binary files, including file headers, section layouts, symbol tables, relocation entries, hex dumps, and full 45GS02 disassembly with symbolic annotations. It follows the conventions of the GNU `objdump` tool.

Supported input formats:
- **`.o45`** — 32-bit relocatable objects (produced by `ca45 -c` or `cc45 -c`): full header, symbols, relocations, and disassembly.
- **`.o65`** — 16-bit relocatable objects (from other 6502 toolchains): same as above.
- **`.prg`** — PRG executables with 2-byte load address header: auto-detects base address, supports hex dump and disassembly.
- **`.bin`** — Raw binary files: supports hex dump and disassembly with a user-supplied base address (`-b`).

## Usage

```bash
objdump45 [options] <file> [...]
```

At least one display option (`-f`, `-h`, `-t`, `-r`, `-s`, `-d`, or `-a`) must be specified.

## Options

| Flag | Description |
|------|-------------|
| `-f` | Display file header (format, CPU, mode flags, options) |
| `-h` | Display section headers (name, size, VMA, type, relocation count) |
| `-t` | Display symbol table (imports and exports) |
| `-r` | Display relocation entries for each section |
| `-s` | Display full contents of all sections (hex dump) |
| `-d` | Disassemble executable sections |
| `-a` | Display all of the above |
| `-b ADDR` | Set base address for raw binary files (default: `$0000`). Accepts `$XXXX`, `0xXXXX`, or decimal. Overrides PRG load address when used with `.prg` files. |
| `-m FILE` | Load symbols and source line mappings from a linker map file (produced by `ln45 -M`) for symbolic disassembly of PRG binaries. |
| `-V` / `--version` | Display version and exit |
| `-?` | Display help message |

Flags can be combined: `-fdh` is equivalent to `-f -d -h`.

## Output Sections

### File Header (`-f`)

Shows the file format, CPU architecture, mode flags, and any embedded options (filename, OS, assembler, author).

```
main.o45:     file format o45-32
architecture: 45GS02, flags $8800 (SIZE32, CPUEXT)
  option: OS
  option: ASM "ca45"
```

### Section Headers (`-h`)

Lists each section with its index, name, size, virtual memory address, type, and number of relocations.

```
main.o45:
Sections:
Idx Name     Size       VMA        Type       Relocs
  0 TEXT     00000028   00000000   CODE       3
  1 DATA     0000000B   00000000   DATA       0
```

### Symbol Table (`-t`)

Lists imported and exported symbols in the traditional `nm` format.

```
SYMBOL TABLE (main.o45):
         U _printf
00000000 T _main
00000028 D _count
```

Type letters: `U` (undefined/import), `T` (text), `D` (data), `B` (BSS), `Z` (zero page), `W` (weak).

### Relocation Entries (`-r`)

Shows decoded relocation records for each section, including type, target segment, and external symbol references.

```
RELOCATION RECORDS FOR [TEXT] (main.o45):
OFFSET     TYPE         SEGMENT  SYMBOL
00000004   R_WORD       EXTERN   _printf
0000000A   R_WORD       DATA
```

### Section Contents (`-s`)

Hex dump of section bodies in a standard format with ASCII sidebar.

```
Contents of section TEXT:
 0000 a941208d 0200ae1c 00f003ca 60        .A .........`
```

### Disassembly (`-d`)

Full disassembly of code sections using the 45GS02 instruction set. Features:

- **Symbol labels**: Export symbols appear as labels at their addresses.
- **Branch annotations**: Branch and jump targets are annotated with symbol names when known.
- **Hex bytes**: Raw instruction bytes are shown alongside mnemonics.
- **All addressing modes**: Full support for 45GS02 modes including `($nn,SP),Y`, `[zp],Z`, 16-bit branches, `PHW #imm16`, `RTS #n`, and more.

```
Disassembly of section TEXT (main.o45):

00002000 <_main>:
    2000:	a9 41       	lda	#$41
    2002:	20 d2 ff    	jsr	$FFD2
    2005:	ae 28 20    	ldx	$2028
    2008:	f0 03       	beq	$200D <_done>
    200a:	ca          	dex

0000200b <_done>:
    200b:	60          	rts

0000200c <_irq_handler>:
    200c:	40          	rti
```

## Examples

### Quick disassembly of a compiled module

```bash
$ objdump45 -d puts.o45
```

### Full dump of an object file

```bash
$ objdump45 -a main.o45
```

### Check relocations before linking

```bash
$ objdump45 -rt main.o45
```

### Inspect multiple files

```bash
$ objdump45 -f lib/build/*.o45
```

### Disassemble a PRG file

The 2-byte load address is read automatically:

```bash
$ objdump45 -d program.prg

program.prg:     file format prg
base address: $0810, size: 42 bytes

Disassembly of section PRG (program.prg):
    0810:	a9 41       	lda	#$41
    0812:	20 d2 ff    	jsr	$FFD2
    ...
```

### Disassemble a raw binary with a custom base address

```bash
$ objdump45 -d -b '$C000' rom.bin
```

### Override the PRG load address

```bash
$ objdump45 -d -b '$0000' program.prg
```

### Analyze multiple object files before linking

```bash
# Check all object files for conflicts and relocations
$ objdump45 -rt *.o45

# Display headers and symbols for all files
$ objdump45 -fth *.o45
```

### Disassemble with linker map (symbolic disassembly)

```bash
# Generate linker map during linking
$ ln45 -M program.map -o program.prg main.o45 -l c45.lib

# Disassemble PRG with symbols from map
$ objdump45 -d -m program.map program.prg
```

Produces disassembly with function names and global symbols annotated.

## Supported Formats

| Format | Extension | Description |
|--------|-----------|-------------|
| `.o45` | `.o45`    | 32-bit MEGA65 relocatable objects (full symbol/reloc/section support) |
| `.o65` | `.o65`    | 16-bit Andre Fachat 6502 relocatable objects |
| `.prg` | `.prg`    | PRG executables (2-byte LE load address + code). Base address auto-detected from header. |
| `.bin` | `.bin`    | Raw binary files. Use `-b ADDR` to set the base address (default `$0000`). |

For `.o45`/`.o65` files, all display modes are available. For `.prg`/`.bin` files, `-f` (header summary), `-s` (hex dump), and `-d` (disassembly) are supported; `-h`, `-t`, and `-r` silently produce no output since raw binaries have no symbol or relocation tables.

Format detection: `objdump45` first attempts to parse the file as `.o45`/`.o65`. If that fails, it falls back to `.prg` (by extension) or raw binary (by `.bin` extension or when `-b` is specified).

## Debugging Workflows

### Verify object file integrity before linking

```bash
# Full dump of object file (all sections)
$ objdump45 -a main.o45

# Check for relocation issues
$ objdump45 -r main.o45 | grep -E "UNDEF|ERROR"
```

### Inspect generated code

```bash
# Look at disassembly of a specific function
$ objdump45 -d main.o45 | grep -A 20 "_main:"

# Find function that uses most code
$ objdump45 -d main.o45 | grep "^[0-9a-f].*<.*>:" | head -10
```

### Analyze relocation requirements

```bash
# See what relocation types are used
$ objdump45 -r main.o45 | cut -d' ' -f5 | sort | uniq -c

# Check external symbol references
$ objdump45 -r main.o45 | grep "EXTERN"
```

### Compare two compiled versions

Useful for understanding optimizer differences:

```bash
# Compile with different optimization levels
$ cc45 -O0 -c main.c -o main_o0.o45
$ cc45 -O2 -c main.c -o main_o2.o45

# Compare code sizes
$ objdump45 -s main_o0.o45 | grep "section TEXT" -A 1
$ objdump45 -s main_o2.o45 | grep "section TEXT" -A 1

# Disassemble both to spot differences
$ objdump45 -d main_o0.o45 > o0.asm
$ objdump45 -d main_o2.o45 > o2.asm
$ diff o0.asm o2.asm
```

### Analyze calling convention metadata

For ZP calling convention objects:

```bash
# Show function attributes
$ objdump45 -t main_zp.o45

# Inspect register clobber masks
# (May be embedded in symbol table for ZP-convention functions)
```

### Verify PRG binary correctness

```bash
# Check load address
$ objdump45 -f program.prg

# Disassemble and verify entry point
$ objdump45 -d program.prg | head -30

# Compare with expected linker map
$ objdump45 -d -m program.map program.prg | grep "_main" -A 10
```

### Extract and save disassembly for analysis

```bash
# Full disassembly to file
$ objdump45 -d program.o45 > program.asm

# With symbols
$ objdump45 -dt program.o45 > program_symbols.asm

# Hex dump for binary analysis
$ objdump45 -s program.o45 > program.hex
```

## See Also

- [nm45](nm45.md) — Symbol lister for `.o45` object files
- [ln45](ln45.md) — Linker
- [ar45](ar45.md) — Archiver for `.lib` archives
- [ca45](ca45.md) — Assembler (produces `.o45` with `-c`)
- [cc45](cc45.md) — C compiler (produces `.o45` with `-c`)
- [../architecture/lib45.md](../architecture/lib45.md) — `.o45` format specification
