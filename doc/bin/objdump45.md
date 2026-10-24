# objdump45 — Object File Inspector and Disassembler

`objdump45` displays information from `.o45` relocatable object files, `.prg` executables and raw binaries: file headers, section layouts, symbol tables, relocation entries, hex dumps and 45GS02 disassembly with symbol labels. It also dumps the BASIC lines of a PRG and compares two binaries. Its options follow GNU `objdump`.

Supported inputs:
- **`.o45`** — relocatable objects (`ca45 -c`, `cc45 -c`): header, sections, symbols, relocations, contents, disassembly.
- **`.prg`** — 2-byte load address + image: header summary, hex dump, disassembly (BASIC `SYS` stub recognised, `ln45 -M` map used for symbols), BASIC listing.
- **raw binaries** — `.bin` extension or any file with `-b`: header summary, hex dump, disassembly.

The disassembly shows the 45GS02's 32-bit instructions as such: `NEG NEG` ($42 $42) before an instruction with a Q form is `ldq`, `stq`, `adcq`, `sbcq`, `andq`, `orq`, `eorq`, `cpq`, `bitq`, `aslq`, `lsrq`, `rolq`, `rorq`, `asrq`, `inq` or `deq`.
- **`.ovd`, `.ov1`** — ln45's code overlay files, the description and the code (see [ln45 Code Overlays](ln45.md#code-overlays)); either one, with the other next to it: one section per overlay at the address it runs at (VMA) and where it is kept in far memory (LMA); `-f`, `-h`, `-t`, `-s` and `-d` (with the overlays' symbols as labels).

## Usage

```bash
objdump45 [options] <file> [...]
```

At least one display option (`-f`, `-h`, `-t`, `-r`, `-s`, `-d`, `-a` or `-bas`) is required, or `-D` with two files.

## Options

| Flag | Description |
|------|-------------|
| `-f` | File header (format, CPU, mode flags, options; for PRG/binaries the base address and size) |
| `-h` | Section headers (name, size, VMA, type, relocation count); for a sectioned object also its link sections (segment, offset, size, alignment, name) |
| `-t` | Symbol table (imports and exports, with function attributes) |
| `-r` | Relocation entries per section |
| `-s` | Full contents of all sections (hex dump) |
| `-d` | Disassemble (an object's TEXT and DATA sections; the code of a PRG/binary) |
| `-g` | An object's debug information: its source files and line table, the struct / union layouts and the variables (kind, scope, name, type, location; lib45.md 1.12) |
| `-a` | All of `-f -h -t -r -s -d -g` |
| `-b ADDR` | Base address for raw binaries (default `$0000`); overrides the load address of a `.prg`. `$C000` and `0xC000` are hex; a plain number is decimal (`2048`), and a leading `0` makes it octal |
| `-m FILE` | Load symbols and source lines from an `ln45 -M` map file for PRG/binary disassembly. For `foo.prg`, `foo.map` is used automatically when it exists, else `foo.dbg` (ln45 `-g`: function names and source lines). With code overlays, a call into an overlay is labelled with the function's name at its stub; the overlays' own symbols are in the `.ovd` file |
| `-bas` | List the BASIC program of a `.prg` (line numbers, detokenised text) |
| `-x` | With `-bas`: also show each line's bytes in hex |
| `-D` | Binary diff of exactly two files (any format, compared byte by byte) |
| `-V` / `--version` | Display version and exit |
| `-?` / `--help` | Display help |

Single-letter flags can be combined: `-fdh` is `-f -d -h`, `-rt` is `-r -t`.

## Output Sections

### File Header (`-f`)

```
amain.o45:     file format o45-32
architecture: 45GS02, flags $8800 (SIZE32, CPUEXT)
  option: OS
  option: ASM "ca45"
```

Options are named `FNAME`, `OS`, `ASM`, `AUTHOR`, `CREATED`, `LINEINFO` (`$11`), `DEBUG_SYMBOLS` (`$12`); others (e.g. `$10` sub-segment) show as `USER`. For a PRG:

```
program.prg:     file format prg
base address: $2001, size: 10590 bytes
```

### Section Headers (`-h`)

```
amain.o45:
Sections:
Idx Name     Size       VMA        Type       Relocs
  0 TEXT     00000007   00000000   CODE       3
  1 DATA     00000003   00000000   DATA       0
```

BSS and ZP sections appear with type `NOBITS`; empty sections are omitted.

### Symbol Table (`-t`)

```
SYMBOL TABLE (math.o45):
         U __bp_rom
         U __bp_prog
00000101 W __sp_base
...
00000000 T _add  [zp_call leaf uses:- clobbers:- releases:- regs:AX flags:CNZV]
00000000 B _add__param_a
```

Imports, then exports in file order. Letters as in [nm45](nm45.md): `U`, `T`, `D`, `B`, `Z`, `A`, `W` (weak); functions with an attribute record show it in brackets.

### Relocation Entries (`-r`)

```
RELOCATION RECORDS FOR [TEXT] (amain.o45):
OFFSET     TYPE         SEGMENT  SYMBOL
00000001   R_LOWX       DATA      extra=$00
00000003   R_HIGH       DATA      extra=$00
00000005   R_WORD       EXTERN   print
```

### Section Contents (`-s`)

```
Contents of section TEXT:
 0000 a900a200 4c0000                      ....L..

Contents of section DATA:
 0000 484900                               HI.
```

### Disassembly (`-d`)

All 45GS02 addressing modes are decoded (including `($nn,SP),Y`, `[zp],Z`, 16-bit branches, `PHW #imm16`). Export symbols become labels, branch targets inside the section are resolved, and `; file:line` comments come from the object's line info (or the map's Source Lines).

```
Disassembly of section TEXT (autil.o45):

00000000 <print>:
    0000:	85 fb       	sta	$fb
    0002:	86 fc       	stx	$fc
    0004:	a0 00       	ldy	#$00
    0006:	b1 fb       	lda	($fb),y
    0008:	f0 06       	beq	$0010
    000a:	20 d2 ff    	jsr	$ffd2
    000d:	c8          	iny
    000e:	d0 f6       	bne	$0006
    0010:	60          	rts
```

Limitations for objects: addresses are object-relative (the not yet relocated operand bytes are shown, usually `$0000` for external references), the label table mixes the exports of all segments (so a BSS or absolute symbol whose offset matches a code address is shown as a label/annotation in TEXT), and the DATA section is disassembled as if it were code. Disassemble the linked PRG with its map for real addresses and names.

## Examples

### Inspect an object

```bash
objdump45 -a main.o45        # everything
objdump45 -rt main.o45       # relocations and symbols
objdump45 -d main.o45        # code
objdump45 -f lib/build/*.o45 # headers of many objects
```

### Disassemble a linked PRG

```bash
ln45 -basic -M program.map -o program.prg main.o45 -l c45.lib
objdump45 -d program.prg            # program.map is found automatically
objdump45 -d -m other.map program.prg
```

```
Disassembly of section PRG (program.prg):

00002001 <BASIC stub>:
    2001:	0b 20 0a 00 9e 38 32 30 35 00 00 00 	; 10 SYS 8205 ($200d)

Disassembly of section code (program.prg):

0000200d <__init>:
    200d:	ba          	tsx
    200e:	8e 5d 20    	stx	$205d
    ...
```

With a map, disassembly stops at the end of TEXT (data is not decoded).

### Raw binary

```bash
ln45 -t 0800 -o prog.bin main.o45 utils.o45
objdump45 -d -b '$0800' prog.bin
objdump45 -d -b '$0000' program.prg    # override the PRG load address
```

### BASIC listing

```
$ objdump45 -bas program.prg

BASIC Program: program.prg (1 lines)
─────────────────────────────────
Format: LineNum [TOKENS...]

   10  SYS 8205
```

### Compare two builds

```
$ objdump45 -D old.prg new.prg
Binary Diff: old.prg vs new.prg
Size: 10592 bytes vs 10656 bytes

Offset $008C: [61→A1] [62→A2] [63→A3] [64→A4] ...

Total differences: 9939 bytes
```

Identical files print `Files are identical`; offsets are file offsets (a PRG's load-address bytes included).

### Compare optimisation levels

```bash
cc45 -O0 -c main.c -o main_o0.o45
cc45 -O2 -c main.c -o main_o2.o45
objdump45 -h main_o0.o45 main_o2.o45          # section sizes
objdump45 -d main_o0.o45 > o0.asm; objdump45 -d main_o2.o45 > o2.asm; diff o0.asm o2.asm
```

## Format Detection

ln45's overlay description (a file that starts and ends with `OVL45`) and a file named `.ov1`–`.ov9` are read as code overlay files (the code needs the `.ovd` next to it; the description without its code files gives the sections and symbols, not `-s`/`-d`). Otherwise `objdump45` tries to read each file as `.o45`. If that fails, a `.prg` extension selects PRG mode, a `.bin` extension or `-b` selects raw-binary mode; otherwise the read error is reported. For PRG/binaries `-h`, `-t` and `-r` print nothing. `.o65` files (magic `"o65"`) are not accepted as objects (see [nm45](nm45.md#supported-formats)).

## See Also

- [nm45](nm45.md) — Symbol lister for `.o45` object files
- [ln45](ln45.md) — Linker (map files with `-M`)
- [ar45](ar45.md) — Archiver for `.lib` archives
- [ca45](ca45.md) — Assembler (produces `.o45` with `-c`)
- [cc45](cc45.md) — C compiler (produces `.o45` with `-c`)
- [lib45](../architecture/lib45.md) — `.o45` format specification
