# ln45 — Linker for .o45 Object Files

`ln45` links multiple `.o45` relocatable object files into a single flat binary or PRG executable. It resolves symbol references, merges segments, and applies relocations.

## Usage

```bash
ln45 [options] <file1.o45> [file2.o45 ...]
```

## Options

| Flag | Description |
|------|-------------|
| `-o <file>` | Output filename (default: `a.out`, or `a.prg` if `-prg`) |
| `-prg` | Output as PRG with 2-byte load address header (auto-detected from `.prg` extension) |
| `-basic` | Output as PRG with BASIC `SYS` stub at `$0801` (code starts at `$080D`; auto-run on MEGA65/C64) |
| `-t <addr>` | Set text segment base address (hex, default: `2000`) |
| `-d <addr>` | Set data segment base address (hex, default: after text) |
| `-b <addr>` | Set BSS segment base address (hex, default: after data) |
| `-z <addr>` | Set zero page base address (hex, default: `02`) |
| `-m` | Print linker map (symbol addresses) after linking |
| `-?` | Display help |

## Linking Process

The linker performs three passes:

### 1. Layout
Segments from all input objects are concatenated in input order:
- All text segments are merged into one contiguous text block
- All data segments are merged into one contiguous data block
- BSS and ZP lengths are summed
- Each object gets an offset within the merged segment

### 2. Symbol Resolution
- **Exports** from all objects are collected into a global symbol map
- Each export's final address is computed: `segment_base + object_offset + symbol_offset`
- **Imports** are verified — every imported symbol must have a matching export
- Duplicate exports are rejected with an error

### 3. Relocation
- Each object's relocation table is decoded
- For external references (`SEG_EXTERNAL`), the import name is looked up in the global symbol map
- For internal cross-segment references, the target segment's final base + object offset is computed
- The patched address is written into the merged segment body

## Relocation Types

| Type | Patch | Description |
|------|-------|-------------|
| `R_WORD` | 2 bytes | 16-bit absolute address |
| `R_LOW` | 1 byte | Low byte of 16-bit address |
| `R_HIGH` | 1 byte | High byte of 16-bit address |
| `R_LINEAR24` | 3 bytes | 24-bit linear address (MEGA65) |
| `R_LINEAR32` | 4 bytes | 32-bit linear address |
| `R_SEGADR` | 3 bytes | Segment address (bank:addr) |

## Examples

### Basic linking

```bash
# Compile two C files to objects
cc45 -c main.c -o main.o45
cc45 -c math.c -o math.o45

# Link into a PRG
ln45 -prg -o program.prg main.o45 math.o45
```

### Auto-running BASIC PRG (recommended for MEGA65/C64)

```bash
cc45 -c main.c -o main.o45

# Link with BASIC SYS stub — auto-runs on load
ln45 -basic -o program.prg crt0.o45 main.o45 stdlib.lib
```

This produces a PRG that loads at `$0801` with a `10 SYS 2061` BASIC stub. Code starts at `$080D`. The program auto-runs when loaded via `LOAD"PROGRAM",8,1` then `RUN` (or auto-start).

### Assembly object linking

```bash
# Assemble to objects
ca45 -c main.s -o main.o45
ca45 -c utils.s -o utils.o45

# Link with explicit text base at $0800
ln45 -t 0800 -o program.bin main.o45 utils.o45
```

### Inspect before linking

```bash
# Check symbols
nm45 main.o45 utils.o45

# Link with map output
ln45 -prg -m -o program.prg main.o45 utils.o45
```

Example map output:
```
Symbol map:
  00002000 _main
  0000203c _add
  00002068 _mul2
  0000208b _result
```

### Custom memory layout

```bash
ln45 -t 0800 -d 4000 -b 6000 -z 02 -o rom.bin boot.o45 kernel.o45
```

## Error Messages

| Error | Cause |
|-------|-------|
| `undefined symbol '_foo'` | An object imports `_foo` but no object exports it |
| `duplicate symbol '_foo'` | Two objects both export `_foo` |
| `relocation offset out of range` | A relocation entry points past the end of a segment |
| `invalid symbol index` | A relocation references a non-existent import table entry |

## Output Format

Without `-prg`: raw flat binary (text + data, gap-filled if needed).

With `-prg`: 2-byte little-endian load address header followed by the binary. The load address is the text base (`-t` value). This format is directly loadable on the MEGA65/C64.

BSS is not included in the output — it must be zeroed at runtime (by `crt0` or the startup code).

## See Also

- [ca45](ca45.md) — Assembler (produces `.o45` with `-c`)
- [cc45](cc45.md) — C Compiler (produces `.o45` with `-c`)
- [nm45](nm45.md) — Symbol lister for `.o45` files
- [lib.md](../lib.md) — `.o45` format specification
