# `.o45` Relocatable Object Format Specification

The `.o45` format is a 32-bit extension of Andre Fachat's `.o65` relocatable object format for 6502-family systems. It reuses the `.o65` header structure, mode flags, segment IDs and relocation encoding, and adds 32-bit address fields, an extended CPU byte, extra relocation types, option records and function attribute records for the MEGA65's 45GS02. The magic bytes are `"o45"` instead of `"o65"`.

Writers: `ca45 -c` / `cc45 -c` (`src/main/O45Emitter.cpp` → `O45Writer`). Readers: `O45Reader` (ln45, nm45, objdump45, ar45). Constants: `include/O45Types.hpp`. Archives (`.lib`) are described in [ar45](../bin/ar45.md#archive-format-lib).

References: [Andre Fachat o65 file format](http://www.6502.org/users/andre/o65/fileformat.html)

---

## 1. Header

When mode bit 15 (`SIZE32`) is set, all address and length fields are 32-bit instead of 16-bit. The tools always write `SIZE32`.

### 1.1 Fixed Header Fields

| Offset | Field        | Size    | Value / Description                          |
|--------|--------------|---------|----------------------------------------------|
| 0      | marker       | 1       | `$01` (non-C64 marker, per `.o65` spec)      |
| 1      | marker2      | 1       | `$00`                                        |
| 2-4    | magic        | 3       | `$6F $34 $35` ("o45")                        |
| 5      | version      | 1       | `$00`                                        |
| 6-7    | mode         | 2 (LE)  | Mode bit field (see 1.2)                     |

The reader rejects any other magic (including the standard `"o65"`) and any other version. A cc65 object (`"uz\0\0"`) gets a specific error.

### 1.2 Mode Word (16-bit, little-endian)

| Bit(s) | Name          | Mask        | Meaning |
|--------|---------------|-------------|---------|
| 0      | `PAGED`       | `$0001`     | Segments are page-aligned |
| 1-2    | `OBJ_TYPE`    | `$0006`     | 00 = object, 01 = executable, 10 = library |
| 3      | `SIMPLE`      | `$0008`     | Simple load |
| 4      | `CHAIN`       | `$0010`     | Another file follows |
| 5      | `BSSZERO`     | `$0020`     | BSS must be zeroed |
| 8-10   | `CPU_BITS`    | `$0700`     | `.o65` CPU class (used only when `CPU_EXT` is clear) |
| 11     | `CPU_EXT`     | `$0800`     | An extended CPU ID byte follows the address fields |
| 15     | `SIZE32`      | `$8000`     | Address/length fields, counts and symbol indices are 32-bit |

ca45 writes `$8800` (`SIZE32 | CPU_EXT`, object, `CPU_BITS` = 0). The other bits are displayed by nm45/objdump45 but not acted upon. With `SIZE32` clear the reader uses 16-bit fields, counts and symbol indices (widened internally); no tool writes such files.

### 1.3 Extended CPU ID Byte (`CPU_EXT`)

| Value  | CPU |
|--------|-----|
| `$01`  | 6502 |
| `$02`  | 65C02 |
| `$03`  | 65CE02 |
| `$04`  | 65816 |
| `$45`  | **45GS02 (MEGA65)** — always written by ca45 |

Without `CPU_EXT`, the reader takes the CPU from `CPU_BITS` (the `.o65` classes: 0 = 6502, 1 = 65C02, 2 = 65SC02, 3 = 65CE02, 5 = 65816).

### 1.4 Address / Length Fields (32-bit mode)

| Offset | Field     | Size | Description |
|--------|-----------|------|-------------|
| 8-11   | `tbase`   | 4    | Text base address used when assembling |
| 12-15  | `tlen`    | 4    | Text length |
| 16-19  | `dbase`   | 4    | Data base address |
| 20-23  | `dlen`    | 4    | Data length |
| 24-27  | `bbase`   | 4    | BSS base address |
| 28-31  | `blen`    | 4    | BSS length |
| 32-35  | `zbase`   | 4    | Zero-page base address |
| 36-39  | `zlen`    | 4    | Zero-page length |
| 40     | `cpu_id`  | 1    | `$45` (present because `CPU_EXT` is set) |

**Total fixed header**: 41 bytes. ca45 writes the segment start addresses of the assembly (normally 0); the linker subtracts them from internal relocation values (Section 3).

### 1.5 Option Records

After the header come zero or more option records, terminated by a `$00` byte:

```
option_length (1 byte)   ; total length including this byte and the type; 0 = end of options
option_type   (1 byte)
option_data   (option_length - 2 bytes)   ; at most 253 bytes
```

| Type   | Name               | Written by ca45 | Read by | Description |
|--------|--------------------|-----------------|---------|-------------|
| `$01`  | `OPT_FNAME`        | no | displayed | File name (NUL-terminated) |
| `$02`  | `OPT_OS`           | yes, `[$05]` | displayed | Target OS; `$05` = MEGA65/C65 (outside the `.o65` range) |
| `$03`  | `OPT_ASM`          | yes, `"ca45\0"` | displayed | Assembler name |
| `$04`  | `OPT_AUTHOR`       | no | displayed | Author (NUL-terminated) |
| `$05`  | `OPT_CREATED`      | no | displayed | Creation time (NUL-terminated) |
| `$10`  | `OPT_SEGATTR`      | yes | ln45 | Text sub-segment (1.6) |
| `$11`  | `OPT_LINEINFO`     | yes, with `.loc` info | ln45, objdump45 | Source line table (1.7) |
| `$12`  | `OPT_DEBUG_SYMBOLS`| yes, for `proc`s | displayed | Procedure parameter names/offsets (1.8) |
| `$13`  | `OPT_SAC_PARAMS`   | no | no | Reserved for SAC parameter metadata |
| `$50`  | `OPT_IPO_HINTS`    | no | ln45 | Cross-module optimisation hints (1.9) |

Unknown options are skipped. `OPT_IPO_HINTS`, when a writer has hints, is written before the other options.

### 1.6 Sub-Segment Attributes (`OPT_SEGATTR`)

ca45 maps every assembler segment except `data`, `bss` and `zp` to TEXT, concatenating them into one text body. Each part gets a record so the linker can reorder them:

```
seg_id   (1 byte)   ; $02 = SEG_TEXT
offset   (4 bytes)  ; LE offset within the text body
length   (4 bytes)  ; LE byte count
name     (N+1)      ; NUL-terminated assembler segment name
```

Records are written when the text body has more than one part, or one part not named `code`, `text` or `default`. The linker places all `init` parts first (CRT startup), then `code`, then the other names alphabetically. Text not covered by a record, and objects without records, count as `code`.

### 1.7 Debug Line Info (`OPT_LINEINFO`)

The first record:

```
fileCount   (2 bytes LE)
fileNames   fileCount NUL-terminated strings
entryCount  (2 bytes LE)   ; total entries of all records (readers ignore it)
entries     N × 6 bytes    ; as many as fit in the record
```

Further records hold only entries. Each entry:

```
textOffset  (2 bytes LE)   ; offset within the text body
fileIndex   (2 bytes LE)   ; index into fileNames
line        (2 bytes LE)   ; 1-based line number
```

The file table is limited to about 220 bytes; longer paths are shortened to their base name, and files that still do not fit are mapped to entry 0. The linker merges the tables into the `Source Lines` section of its map.

### 1.8 Debug Symbols (`OPT_DEBUG_SYMBOLS`)

```
procCount   (2 bytes LE)   ; number of procedures in the object (repeated in every record)
records:
  name      NUL-terminated procedure name
  count     (1 byte)       ; parameters in this record
  count × { name NUL-terminated, offset (2 bytes LE, signed frame offset) }
  $FF
```

Records are kept under ~240 bytes; a procedure with more parameters than fit is split into several records with the same name.

### 1.9 IPO Hints (`OPT_IPO_HINTS`)

```
version     (1 byte)  ; $01 (other versions are ignored)
funcCount   (2 bytes LE)
per function:
  name               NUL-terminated
  callCount          (2 bytes LE)
  estimatedCodeSize  (2 bytes LE)
  flags              (1 byte, FUNC_FLAG_*)
  externalCallCount  (1 byte)
  specCount          (1 byte)
  specCount × { argCount (1), argCount × int64 LE, frequency (1, percent) }
```

Defined in the writer API and parsed by ln45; no tool currently writes it.

---

## 2. Segment Bodies

After the option list, in fixed order:

1. Text body (`tlen` bytes)
2. Text relocation table (Section 3)
3. Data body (`dlen` bytes)
4. Data relocation table

BSS and ZP have no body; only base and length are recorded.

---

## 3. Relocation Tables

Each table is a stream of entries terminated by `$00`:

```
offset_delta   1 byte (or $FF escapes, 3.1)
type_and_seg   1 byte: type (bits 7-5) | segment (bits 4-0)
[extra]        1 byte, R_HIGH: low byte of the full target value;
                       R_LOWX: high byte of the full target value
[symbol_id]    4 bytes LE (2 without SIZE32), external references only
```

### 3.1 Offset Encoding

The offset is a delta from the previous relocation site (the first from segment offset 0):

| Byte Value | Meaning |
|------------|---------|
| `$00`      | End of table |
| `$01-$FE`  | Add this value to the current offset; an entry follows |
| `$FF`      | Add 254 and read another delta byte |

A relocation at segment offset 0 cannot be encoded (the writer drops it).

### 3.2 Segment IDs (bits 4-0)

| ID   | Constant       | Description |
|------|----------------|-------------|
| `0`  | `SEG_EXTERNAL` | Imported symbol; symbol index follows |
| `1`  | `SEG_ABSOLUTE` | Absolute (no relocation) |
| `2`  | `SEG_TEXT`     | Text |
| `3`  | `SEG_DATA`     | Data |
| `4`  | `SEG_BSS`      | BSS |
| `5`  | `SEG_ZP`       | Zero page |

### 3.3 Relocation Types (bits 7-5)

| Bits 7-5 | Value  | Constant      | Patch | Description |
|----------|--------|---------------|-------|-------------|
| `001`    | `$20`  | `R_LOW`       | 1 | Low byte of the address |
| `010`    | `$40`  | `R_HIGH`      | 1 | High byte; the extra byte holds the low byte so the full value (addend) can be rebuilt |
| `011`    | `$60`  | `R_LINEAR24`  | 3 | **(ext)** 24-bit linear address |
| `100`    | `$80`  | `R_WORD`      | 2 | 16-bit address, little-endian |
| `101`    | `$A0`  | `R_LINEAR32`  | 4 | **(ext)** 32-bit linear address (`.o65` uses `$A0` for SEG) |
| `110`    | `$C0`  | `R_SEGADR`    | 3 | 24-bit address (patched like `R_LINEAR24`) |
| `000`    | `$00`  | `R_IMM8`      | 1 | **(ext)** 8-bit immediate operand (self-modifying-code parameters) |
| —        | `$01`  | `R_IMM16`     | 2 | **(ext)** 16-bit immediate. Its value lies in the segment bits, so it cannot be represented in this encoding |
| `111`    | `$E0`  | `R_LOWX`      | 1 | **(ext)** Low byte; the extra byte holds the high byte so the full value can be rebuilt. ca45 writes it for `#<sym` and its simulated ops (`R_LOW` is still read: the linker needs the full value only to move SAC storage, see [ln45](../bin/ln45.md#sac-overlay)) |

### 3.4 Patch Values

- **Internal** (segment 2-5): the bytes at the patch site hold the assembly-time address. The linker subtracts the object's base for that segment (`tbase`/`dbase`/`bbase`/`zbase`) and adds the segment's final address plus the object's offset in the merged segment.
- **External** (segment 0): the bytes at the patch site are an addend (sign-extended) added to the symbol's address, e.g. `jsr _f+3`. An import named `X__ar_N` resolves to `X__ar` + `N`.

### 3.5 Examples

`JSR external_func` with the address bytes at text offset `$0012`, first relocation:

```
$12                 ; delta 18
$80                 ; R_WORD | SEG_EXTERNAL
$03 $00 $00 $00     ; import #3
```

`lda #<msg` / `ldx #>msg` / `jmp print` (ca45 output, `msg` in DATA at offset 0, `print` external):

```
$01 $E3 $00         ; offset 1: R_LOWX | SEG_DATA, high byte $00
$02 $43 $00         ; offset 3: R_HIGH | SEG_DATA, low byte $00
$02 $80 $00 $00 $00 $00   ; offset 5: R_WORD | SEG_EXTERNAL, import #0
$00                 ; end
```

---

## 4. Symbol Tables

After the data relocation table: the import table, then the export table. Each starts with a count (4 bytes LE; 2 without `SIZE32`).

### 4.1 Import Entry

```
name $00            ; NUL-terminated
```

Relocations refer to imports by zero-based index. Imports come from `.extern` (an undefined symbol that is not declared `.extern` is an assembly error).

### 4.2 Export Entry

```
name $00            ; NUL-terminated
segment   (1 byte)  ; bits 6-0: SEG_TEXT/DATA/BSS/ZP/ABSOLUTE; bit 7 ($80): weak
value     (4 bytes LE, 2 without SIZE32)   ; offset within the segment, or the value if absolute
[function attribute record, 4.3]
```

Exports come from `.global` and `.weak` (weak bit set).

### 4.3 Function Attribute Record (optional)

Follows an export when the function's `proc` carries metadata (`.func_flags`, `.zp_uses`, `.zp_clobbers`, `.zp_release`, `.reg_clobbers`, `.flag_clobbers`). cc45 emits it for every function.

```
$FA                         ; marker
flags         (1 byte)      ; FUNC_FLAG_* below
regClobbers   (1 byte)      ; bit 0=A, 1=X, 2=Y, 3=Z
flagClobbers  (1 byte)      ; bit 0=C, 1=N, 2=Z, 3=V
zpUses        (4 bytes LE)  ; ZP slots read as parameters
zpClobbers    (4 bytes LE)  ; ZP slots written
zpRelease     (4 bytes LE)  ; ZP slots consumed (caller need not restore)
paramSize     (1 byte)      ; parameter bytes (used for thunks)
frameSize     (2 bytes LE)  ; activation record / frame size
```

**Total size**: 19 bytes including the marker. ZP masks: bit *n* = address `cc45.zeroPageStart + 1 + n` (`zeroPageStart` defaults to `$02`; cc45 objects define it as `$08`, so bit 0 = `$09`). Addresses at or below `zeroPageStart` cannot be expressed.

| Bit | Constant | Meaning |
|-----|----------|---------|
| 0 | `FUNC_FLAG_LEAF` (`$01`) | Makes no calls |
| 1 | `FUNC_FLAG_REENTRANT` (`$02`) | Re-entrant |
| 2 | `FUNC_FLAG_ZP_CONV` (`$04`) | ZP calling convention (clear = stack) |
| 3 | `FUNC_FLAG_STATIC_ALLOC` (`$08`) | Uses a static activation record (SAC) |
| 4 | `FUNC_FLAG_ISR` (`$10`) | `__interrupt` handler |
| 5 | `FUNC_FLAG_ZERO_ALLOC` (`$20`) | SAC leaf with no locals and all-constant parameters |

A record means the convention is known; exports without one are never convention-checked by the linker. The linker bridges ZP/stack mismatches in **both** directions with thunks (an error with `ln45 --no-thunks`), and rejects SAC functions that are recursive or reachable from an ISR — see [ln45](../bin/ln45.md#calling-convention-thunks).

The writer API can also put a content-flag record (`$FB`, flags byte; `O45_CONTENT_FLAG_*`) before the attribute record. No tool writes it and the reader does not understand it.

---

## 5. Complete File Layout

```
+-------------------------------+
| Fixed header (41 bytes)       |  marker $01 $00, "o45", version $00, mode,
|                               |  tbase/tlen dbase/dlen bbase/blen zbase/zlen, cpu $45
+-------------------------------+
| Option records                |  terminated by $00
+-------------------------------+
| Text body                     |  tlen bytes
| Text relocation table         |  terminated by $00
+-------------------------------+
| Data body                     |  dlen bytes
| Data relocation table         |  terminated by $00
+-------------------------------+
| Import table                  |  count + NUL-terminated names
| Export table                  |  count + (name, segment, value [, $FA record])
+-------------------------------+
| [debug sections]              |  writer API only: .debug_info/.debug_line/.debug_str,
|                               |  each as size (4 LE, incl. itself) + data; not written
|                               |  by the tools, ignored by the reader
+-------------------------------+
```

---

## 6. C++ Constants

`include/O45Types.hpp` defines the format: file markers (`O45_MARKER1/2`, `O45_MAGIC`, `O45_VERSION`, `O45_HEADER_SIZE` = 41), mode bits (`O45_MODE_*`, `O45_MODE_DEFAULT` = `SIZE32 | CPUEXT`), CPU IDs (`O45_CPU_*`), `O45Segment`, `O45_EXPORT_WEAK`, `O45RelocType` (including `R_IMM8`/`R_IMM16`), `O45_RTYPE_MASK`/`O45_RSEG_MASK`, `O45_RELOC_END`/`ESCAPE`/`SKIP`, option types (`OPT_*`, `OPT_OS_MEGA65`), `O45_FUNCATTR_MARKER`/`O45_FUNCATTR_SIZE` (19), `O45_CONTENTFLAG_MARKER`, `O45FuncAttr` with the `FUNC_FLAG_*` bits, `o45RelocPatchSize()`, and the IPO hint structures. `include/O45Writer.hpp` has `O45Writer`, `O45Reloc` and `O45RelocEncoder`; `include/O45Reader.hpp` has `O45File` and `O45Reader`; `include/O45Linker.hpp` has `O45RelocDecoder`.

---

## 7. Assembler Segments

| ca45 segment | `.o45` segment |
|--------------|----------------|
| `code`, `text`, `init`, any other name | `SEG_TEXT` (parts described by `OPT_SEGATTR`) |
| `data` | `SEG_DATA` |
| `bss` | `SEG_BSS` |
| `zp` | `SEG_ZP` |

Export offsets are relative to their segment (for TEXT, to the merged text body).

---

## 8. Symbol Naming (C Linkage)

C symbols carry a leading underscore: function `main` → `_main`, global `count` → `_count`. Hand-written assembly symbols are stored as written (`print`), so C code calling them must use the underscored name in the assembly (`_print`).

cc45 objects also contain:

| Symbol | Kind | Meaning |
|--------|------|---------|
| `_f__sac` | TEXT | Body of SAC function `f` after its parameter copy |
| `_f__param_x` | BSS | Static parameter slot `x` of SAC function `f` |
| `cc45.mode_zp` / `cc45.mode_stack` | weak absolute (1) | Calling convention of the object (used by ln45) |
| `cc45.zeroPageStart` | weak absolute | First ZP address of the code generator (`$08`); base of the ZP masks |
| `cc45.align.text` / `.data` / `.bss` | weak absolute | Largest alignment in that segment (only when > 1) |
| `__zp_scratch`..`__zp_scratch4`, `__static_chain`, `__sp_base` | weak absolute | ZP temporaries and stack base used by the code |

---

## 9. Differences from Standard `.o65`

| Feature | `.o65` | `.o45` |
|---------|--------|--------|
| Magic | `"o65"` | `"o45"` |
| Address width | 16-bit (32-bit optional) | 32-bit (`SIZE32` always set) |
| CPU identification | `CPU_BITS` | `CPU_EXT` byte = `$45` |
| `R_LINEAR24` | — | `$60` |
| `R_LINEAR32` | `$A0` is SEG | `$A0` — 32-bit patch |
| `R_IMM8` | — | type bits `000` |
| Symbol index / counts | 2 bytes | 4 bytes |
| Weak exports | — | bit 7 of the export segment byte |
| Function attributes | — | `$FA` record after an export |
| Extra options | — | `$10`, `$11`, `$12`, `$50` |
| Fixed header size | 26 bytes (16-bit mode) | 41 bytes |
