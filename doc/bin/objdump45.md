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
| `-S FILE` | Name ROM routines and I/O registers from a symbol file (again for more; see [Symbol files](#symbol-files)): `jsr CHROUT ; $ffd2`. `-S c64` and `-S mega65` are the shipped sets, the CPU's by default; `-S none` uses none |
| `--source` | A PRG or binary as ca45 source that assembles back to its bytes (see [Assemblable source](#assemblable-source)); `--entry ADDR` (again for more) code a pointer reaches, `--data A-B` a range that is data |
| `--cpu=CPU` | The CPU to decode for (`--cpu CPU` too): `45gs02` (the default; `4510`), `65ce02`, `6502` or `6510` (see [CPU modes](#cpu-modes)). Without it a `.prg` loading at `$0801` (a C64 program) is decoded as `6510`, with a note |
| `-bas` | List the BASIC program of a `.prg` (line numbers, detokenised text) |
| `-x` | With `-bas`: also show each line's bytes in hex |
| `-D` | Binary diff of exactly two files (any format, compared byte by byte) |
| `-v` … `-vvvv`, `-v0` | Log on stderr as `[objdump45:LEVEL] …` (the toolchain's format, `include/Logger.hpp`); cumulative, at most `-vvvv`; `-v0` off: each file's format (object, PRG, raw binary, overlay file) and the map or debug file the disassembly takes symbols from (`-v`); why a file is not an object (`-vv`) |
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

In an object, a section's labels are its own exports (not the other sections' or absolute symbols), and an operand is named from its relocation, not from its bytes: an import (`; <__divu16>`, with the addend: `; <_x+2>`), or the export of the target segment at or before the address (`; <_pl+18>`; a static has no symbol of its own, so it shows as the export before it plus an offset, or `BSS+$4` with none before it); a byte of an address is `; <_pl> lo` / `; <_pl> hi`. Operands without a relocation (base-page bytes, I/O addresses) are not named.

```
    0016:	20 00 00    	jsr	$0000	; <__modu16>
    0019:	8d 00 00    	sta	$0000	; <_a>
    0020:	a9 00       	lda	#$00	; <_pl> lo
```

#### CPU modes

The 45GS02 uses the opcodes that are undocumented on the 6510 for its own instructions. A C64 program's `LAX $10` (`$A7 $10`) would be decoded as `smb2 $10`. `--cpu` selects the instruction set:

| CPU | Decodes |
|-----|---------|
| `45gs02` | The 45GS02 (the default): all 256 opcodes; `NEG NEG` before an instruction with a Q form is that form (`ldq`); `$5C` is `map`, `$EA` is `eom` |
| `65ce02` | As the 45GS02 without the quad (`$42 $42`) prefix: `$5C` is `aug` (four bytes), `$EA` is `nop` |
| `6502` | The 151 documented NMOS opcodes (those `ca45 --cpu 6502` accepts); any other byte is `.byte` |
| `6510` | The documented opcodes and the 105 undocumented ones, each marked `; undocumented`: `slo`, `rla`, `sre`, `rra`, `dcp`, `isc` (7 modes each), `sax`, `lax`, `anc`, `alr`, `arr`, `sbx`, `sbc #` (`$EB`), `ane`, `lxa`, `sha`, `shx`, `shy`, `tas`, `las`, `jam`, and the `nop`s with an operand |

A `.prg` that loads at `$0801` is decoded as `6510` code when `--cpu` (and `-b`) are not given. The disassembly says so: `; loaded at $0801: decoded as 6510 (C64) code; --cpu=45gs02 for MEGA65 code`.

```
$ objdump45 -d c64.prg
Disassembly of section PRG (c64.prg):
; loaded at $0801: decoded as 6510 (C64) code; --cpu=45gs02 for MEGA65 code
    0801:	a7 10       	lax	$10	; undocumented
    0803:	60          	rts
```

#### Symbol files

An operand that names an address a symbol file names is shown by that name, the address after it (`jsr CHROUT ; $ffd2`, `sta VIC_SPR0COL,x ; $d027`, `jmp (CINV) ; $0314`; not an immediate), and a disassembled address with a name is labelled with it. The program's own symbols (an object's, a map's or a debug file's) come first.

| `-S` | Symbols |
|------|---------|
| `c64` | `lib/symbols/c64.sym` (603): the KERNAL jump table under the names of Commodore's *Programmer's Reference Guide* (`CHROUT`, `CHKOUT` ...), the BASIC V2 and KERNAL routines a `JSR` or `JMP` in Commodore's ROM source reaches (its names, upper case; a routine a jump table entry reaches as `KRN_CINT`, BASIC's own `STOP` and `RESTOR` as `BAS_`), the RAM vectors `$0300`-`$0333` (`CINV` ...), the hardware vectors, the 6510's port (`D6510`, `R6510`) and the I/O registers: VIC-II (`VIC_BORDERCOL`, `VIC_SPR0X` ...), SID (`SID_V1FREQLO` ...), CIA 1 and 2 (`CIA1_PRA` ...), `COLORRAM`. The default for `--cpu=6502`/`6510` and a `$0801` PRG |
| `mega65` | `lib/symbols/mega65.sym` (469): the KERNAL jump table (the C65 KERNAL keeps the C64's: `CHROUT` ...) and the I/O registers as `<mega65.h>` names them, its struct overlays' fields (`VIC4_BORDER`, `SID1_VOICE0_FREQ_LO`, `DMA_ADDR_LSB_TRIG`, `CIA1_PRA`, `HYPER_TRAP0`, array elements numbered) and its register `#define`s (`ASCII_KEY_QUEUE`). The default for the 45GS02 and 65CE02 |
| a file | Lines of `$addr NAME [comment]` (also `0xaddr`, or plain hex), VICE's `al C:addr .name`, or `NAME = $addr`; `#` and `;` start comment lines. A line that is none of them is an error |

Several `-S` add up (a later name for an address wins). The shipped sets are in `lib/symbols` (installed in `lib/cc45/symbols`), made by `tools/symbols/gen_symbols.py`: the C64's from Commodore's source (github.com/mist64/c64rom), converted with asmCvt45 and assembled with `ca45 --cpu 6502`, written only when the bytes are the ROMs' (901226-01, 901227-03); the MEGA65's from `<mega65.h>`. BASIC 65's entry points are not in the MEGA65 set (no documented list).

#### Assemblable source

`objdump45 --source prog.prg > prog.s45` writes the PRG (or a binary, `-b ADDR`) as ca45 source; `ca45 prog.s45 -o same.prg` gives back its bytes (at any `-O`: the code is between `.noopt_start` and `.noopt_end`).

- **Code** is what the entry points reach: a BASIC stub's `SYS` target (else the start), and `--entry ADDR`, following branches, jumps and calls to the end of each path (`rts`, `rti`, `jmp`, `bra`, `brk`). Everything else, and `--data A-B` ranges, is `.byte` (8 a line), the BASIC stub too: code reached only through a pointer needs its `--entry`.
- **Labels** at the targets inside the file: the program's names (an object's, the map's `foo.map`, `-m`, or the debug file's), a symbol file's (`-S`), else `L_xxxx`; an operand into the middle of an instruction (self-modifying code) is `L_xxxx+n`. Names outside the file are equates before the code (`CHROUT = $FFD2`).
- **Exact bytes**: an absolute operand below `$100` is `lda.abs $0012` (ca45 would pick the base-page form); the 45GS02's NEG NEG before an instruction with a Q form is that instruction (`ldq $12`, ca45's names `cmpq`, `oraq`; not BITQ, ASRQ, which ca45 has not); written as `.byte` with the instruction in a comment: a 16-bit branch whose target an 8-bit one reaches (ca45 would shorten it), the 6510's undocumented opcodes (`; lax $10 (undocumented)`), the 65CE02's `aug`; the 65CE02's `nop` is `eom` (its byte, `$EA`). 6502/6510 code is under `.cpu 6502`.
- Not for objects (link them first) or overlay files.

Tested by `test_objdump45.sh`: every opcode of each CPU (and the 45GS02's after NEG NEG), random operands, each an entry, assembled back with `CA45_OPTIMIZE=0`; programs cc45 made, in both conventions; a C64 program with a BASIC stub, `--data` and `--entry`.

```
$ objdump45 --source c64.prg
; c64.prg as ca45 source (objdump45 --source, 6510): ca45 <this> -o <out>.prg assembles it back
        .cpu 6502
CHROUT = $FFD2
        * = $0801
        .noopt_start
        .byte $0b, $08, $0a, $00, $9e, $32, $30, $36
        .byte $31, $00, $00, $00
L_080D:
        ldx #$00
L_080F:
        lda L_081D,x            ; $081d
        beq L_081A              ; $081a
        jsr CHROUT              ; $ffd2
        inx
        bne L_080F              ; $080f
L_081A:
        .byte $a7, $10          ; lax $10 (undocumented)
        rts
L_081D:
        .byte $48, $49, $00
        .noopt_end
```

Limitations for objects: addresses are object-relative (the not yet relocated operand bytes are shown, usually `$0000` for external references), and the DATA section is disassembled as if it were code. Disassemble the linked PRG with its map for real addresses.

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
