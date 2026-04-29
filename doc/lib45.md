# `.o45` Relocatable Object Format Specification

The `.o45` format is a 32-bit extension of Andre Fachat's `.o65` relocatable object format for 6502-family systems. It reuses the `.o65` header structure, magic bytes, mode flags, segment IDs, and relocation type encoding, adding only what is needed for the MEGA65's 45GS02 CPU and its 28-bit linear address space.

References: [Andre Fachat o65 file format](http://www.6502.org/users/andre/o65/fileformat.html)

---

## 1. Header [IMPLEMENTED]

The header is a direct extension of the `.o65` header. When mode bit 15 (`SIZE32`) is set, all address and length fields widen from 16-bit to 32-bit.

### 1.1 Fixed Header Fields

| Offset | Field        | Size    | Value / Description                          |
|--------|--------------|---------|----------------------------------------------|
| 0      | marker       | 1       | `$01` (non-C64 marker, per `.o65` spec)      |
| 1      | marker2      | 1       | `$00`                                        |
| 2-4    | magic        | 3       | `$6F $36 $35` ("o65")                        |
| 5      | version      | 1       | `$00` (format version 0)                     |
| 6-7    | mode         | 2 (LE)  | Mode bit field (see 1.2)                     |

### 1.2 Mode Word (16-bit, little-endian)

Reuses all standard `.o65` mode bits. New bits are marked with **(ext)**.

| Bit(s) | Name          | `.o65` Value | `.o45` Usage                                    |
|--------|---------------|-------------|--------------------------------------------------|
| 0      | `PAGED`       | `$0001`     | 1 = segments are page-aligned                    |
| 1-2    | `OBJ_TYPE`    | `$0006`     | 00 = object, 01 = executable, 10 = library       |
| 3      | `SIMPLE`      | `$0008`     | 1 = simple load (no undefined refs)              |
| 4      | `CHAIN`       | `$0010`     | 1 = another file follows in stream               |
| 5      | `BSSZERO`     | `$0020`     | 1 = BSS segment must be zeroed                   |
| 6-7    | reserved      |             | Must be 0                                        |
| 8-10   | `CPU_BITS`    | `$0700`     | CPU class (see 1.3)                              |
| 11     | `CPU_EXT`     | `$0800`     | 1 = extended CPU ID byte follows header **(used)**|
| 12-14  | reserved      |             | Must be 0                                        |
| 15     | `SIZE32`      | `$8000`     | 1 = all address/length fields are 32-bit **(required for .o45)** |

**Canonical `.o45` mode value**: `$8800` minimum (SIZE32 + CPU_EXT). Additional bits set as needed (e.g. `$8820` for BSSZERO + SIZE32 + CPU_EXT).

### 1.3 CPU Class (`CPU_BITS`, bits 8-10)

Standard `.o65` values, reused:

| Value | CPU Family            |
|-------|-----------------------|
| 0     | 6502                  |
| 1     | 65C02                 |
| 2     | 65SC02                |
| 3     | 65CE02                |
| 4     | Reserved (was 6502 undocumented) |
| 5     | 65816                 |
| 6     | Reserved              |
| 7     | Extended (see CPU_EXT) |

When `CPU_BITS = 7` and `CPU_EXT = 1`, the assembler writes one additional byte after the header's address fields (but before options) to identify the exact CPU.

### 1.4 Extended CPU ID Byte

| Value  | CPU                    |
|--------|------------------------|
| `$01`  | 6502                   |
| `$02`  | 65C02                  |
| `$03`  | 65CE02                 |
| `$04`  | 65816                  |
| `$45`  | **45GS02 (MEGA65)**    |

`.o45` files MUST set this byte to `$45`.

### 1.5 Address / Length Fields (32-bit mode)

With `SIZE32` set, all segment base addresses and lengths are 4 bytes (little-endian) instead of 2. The header continues after the mode word:

| Offset | Field            | Size | Description                          |
|--------|------------------|------|--------------------------------------|
| 8-11   | `tbase`          | 4    | Text segment base address            |
| 12-15  | `tlen`           | 4    | Text segment length (bytes)          |
| 16-19  | `dbase`          | 4    | Data segment base address            |
| 20-23  | `dlen`           | 4    | Data segment length (bytes)          |
| 24-27  | `bbase`          | 4    | BSS segment base address             |
| 28-31  | `blen`           | 4    | BSS segment length (bytes)           |
| 32-35  | `zbase`          | 4    | Zero/Direct Page segment base address|
| 36-39  | `zlen`           | 4    | Zero/Direct Page segment length      |
| 40     | `cpu_id`         | 1    | `$45` (extended CPU ID)              |

**Total fixed header**: 41 bytes.

### 1.6 Option Headers

Immediately after the fixed header (and CPU ID byte), zero or more option records appear, terminated by a zero-length byte. This is identical to `.o65`.

```
option_length (1 byte)   ; total length including this byte; 0 = end of options
option_type   (1 byte)   ; see table below
option_data   (N bytes)  ; length - 2 bytes of payload
```

| Type   | Name         | Description                                          |
|--------|--------------|------------------------------------------------------|
| `$00`  | (end)        | Length byte = 0 terminates option list                |
| `$01`  | `OPT_FNAME`  | Filename (NUL-terminated string)                     |
| `$02`  | `OPT_OS`     | Target OS identifier                                 |
| `$03`  | `OPT_ASM`    | Assembler name/version (NUL-terminated)              |
| `$04`  | `OPT_AUTHOR` | Author string (NUL-terminated)                       |
| `$05`  | `OPT_CREATED`| Creation timestamp (NUL-terminated)                  |
| `$80`+ | user-defined | Available for toolchain-specific extensions           |

Recommended `.o45` options:
- `OPT_ASM` = `"ca45 <version>\0"`
- `OPT_OS` with byte `$05` = MEGA65/C65 (user-defined OS ID, outside the standard o65 OS range)

---

## 2. Segment Bodies [IMPLEMENTED]

After the option list, the segment bodies appear in fixed order:

1. **Text body** (`tlen` bytes of machine code)
2. **Text relocation table** (see Section 3)
3. **Data body** (`dlen` bytes of initialized data)
4. **Data relocation table** (see Section 3)

BSS and ZP segments have no body (they are uninitialized); only their base and length are recorded in the header.

---

## 3. Relocation Tables [IMPLEMENTED]

Each relocation table (text, data) is a stream of relocation entries terminated by a `$00` byte. The encoding reuses the `.o65` relocation byte format exactly.

### 3.1 Relocation Entry Format

```
offset_byte(s)    ; 1 byte offset delta, or $FF escape (see 3.2)
type_and_seg      ; 1 byte: type (bits 7-5) | segment (bits 4-0)
[extra_byte]      ; present only for certain types (HIGH with underflow, etc.)
[symbol_id]       ; present only if segment bits = $00 (external reference)
```

### 3.2 Offset Encoding

The offset byte is a delta from the previous relocation site (starting from the segment base). This is standard `.o65`:

| Byte Value | Meaning                                           |
|------------|---------------------------------------------------|
| `$00`      | End of relocation table                           |
| `$01-$FE` | Add this value to the current offset              |
| `$FF`      | Add 254 to offset, read next offset byte (escape) |

Multiple `$FF` bytes chain to skip large gaps.

### 3.3 Type/Segment Byte

```
  7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+
| type (3 bits) | segment (5 bits)|
+---+---+---+---+---+---+---+---+
```

#### Segment IDs (bits 4-0)

Reused from `.o65` without modification:

| ID   | Name          | Constant              | Description                        |
|------|---------------|-----------------------|------------------------------------|
| `0`  | External      | `SEG_EXTERNAL`        | Refers to an imported symbol (symbol ID follows) |
| `1`  | Absolute      | `SEG_ABSOLUTE`        | Absolute value (no relocation)     |
| `2`  | Text          | `SEG_TEXT`            | Offset relative to text segment    |
| `3`  | Data          | `SEG_DATA`            | Offset relative to data segment    |
| `4`  | BSS           | `SEG_BSS`             | Offset relative to BSS segment     |
| `5`  | Zero Page     | `SEG_ZP`              | Offset relative to ZP/Direct Page  |

#### Relocation Types (bits 7-5)

Standard `.o65` types are reused. `.o45` extends with two new types using previously undefined encodings.

| Bits 7-5 | Value  | Name             | Constant          | Patch Size | Description                                |
|----------|--------|------------------|-------------------|------------|--------------------------------------------|
| `001`    | `$20`  | LOW              | `R_LOW`           | 1 byte     | Low byte of 16-bit address                 |
| `010`    | `$40`  | HIGH             | `R_HIGH`          | 1 byte     | High byte of 16-bit address (+extra byte for borrow) |
| `011`    | `$60`  | **LINEAR24**     | `R_LINEAR24`      | 3 bytes    | **(ext)** 24-bit linear address            |
| `100`    | `$80`  | WORD             | `R_WORD`          | 2 bytes    | Full 16-bit address (little-endian)        |
| `101`    | `$A0`  | **LINEAR32**     | `R_LINEAR32`      | 4 bytes    | **(ext)** 32-bit linear address            |
| `110`    | `$C0`  | SEGADR           | `R_SEGADR`        | 3 bytes    | Segment address (bank:addr, o65 standard)  |
| `111`    | `$E0`  | reserved         |                   |            | Reserved for future use                    |
| `000`    | `$00`  | (unused/end)     |                   |            | Not a valid type (offset $00 = table end)  |

### 3.4 External Symbol References

When the segment ID is `SEG_EXTERNAL` (`0`), the relocation entry is followed by a symbol index (encoded as a variable-length unsigned integer per `.o65` spec):

- If SIZE32 is set: 4-byte little-endian symbol index
- Else: 2-byte little-endian symbol index

The symbol index refers to the **Imports table** (see Section 4.2), zero-based.

### 3.5 Relocation Examples

**Example 1**: `JSR external_func` in the text segment at offset +$0012

```
; offset delta to reach $0012 from previous site
$12                     ; offset delta = 18
$80 | $00 = $80         ; type=WORD, segment=EXTERNAL
$03 $00 $00 $00         ; symbol index 3 (32-bit, SIZE32 mode)
```
Linker patches 2 bytes at text+$0012 with the resolved 16-bit address of import #3.

**Example 2**: `LDAL [linear_ptr]` — 24-bit linear relocation to a data symbol at offset +$0030

```
$1E                     ; offset delta ($0030 - $0012 = $1E)
$60 | $03 = $63         ; type=LINEAR24, segment=DATA
```
Linker patches 3 bytes at text+$0030 with (data_base + stored offset).

**Example 3**: 32-bit pointer in data segment referencing a text label

```
$08                     ; offset delta
$A0 | $02 = $A2         ; type=LINEAR32, segment=TEXT
```
Linker patches 4 bytes at data+offset with (text_base + stored value).

---

## 4. Symbol Tables [IMPLEMENTED]

Two symbol tables appear after the last relocation table, in order:

1. **Imports** (undefined/external references)
2. **Exports** (globally visible definitions)

### 4.1 Table Structure

Each table begins with a symbol count:

| SIZE32=0 | SIZE32=1 | Description             |
|----------|----------|-------------------------|
| 2 bytes  | 4 bytes  | Number of entries (LE)  |

### 4.2 Import Entry

Each import is a NUL-terminated symbol name. Imports are referenced by zero-based index from relocation entries.

```
name_bytes... $00       ; NUL-terminated ASCII symbol name
```

The import table corresponds to `.extern` directives in `ca45`.

### 4.3 Export Entry

Each export contains the symbol name, the segment it belongs to, and its offset within that segment.

```
name_bytes... $00       ; NUL-terminated ASCII symbol name
segment_id    (1 byte)  ; SEG_TEXT, SEG_DATA, SEG_BSS, SEG_ZP, or SEG_ABSOLUTE
value         (2 or 4 bytes) ; offset within segment (width depends on SIZE32)
```

The export table corresponds to `.global` directives in `ca45`.

---

## 5. Complete File Layout (Summary)

```
+-------------------------------+
| Fixed Header (41 bytes)       |   marker, magic, version, mode,
|   marker:  $01 $00            |   tbase/tlen, dbase/dlen,
|   magic:   "o65"              |   bbase/blen, zbase/zlen,
|   version: $00                |   cpu_id ($45)
|   mode:    $xx $xx            |
|   tbase/tlen (4+4)            |
|   dbase/dlen (4+4)            |
|   bbase/blen (4+4)            |
|   zbase/zlen (4+4)            |
|   cpu_id: $45                 |
+-------------------------------+
| Option Headers                |   zero or more; terminated by $00
+-------------------------------+
| Text Segment Body             |   tlen bytes of machine code
+-------------------------------+
| Text Relocation Table         |   stream terminated by $00
+-------------------------------+
| Data Segment Body             |   dlen bytes of initialized data
+-------------------------------+
| Data Relocation Table         |   stream terminated by $00
+-------------------------------+
| Import Table                  |   count + NUL-terminated names
+-------------------------------+
| Export Table                  |   count + (name + segment + value)
+-------------------------------+
```

---

## 6. C++ Constants [IMPLEMENTED]

These constants and enums are defined in `include/O45Types.hpp`. They reuse `.o65` values directly.

```cpp
#pragma once
#include <cstdint>

// --- File markers ---
constexpr uint8_t  O45_MARKER1       = 0x01;
constexpr uint8_t  O45_MARKER2       = 0x00;
constexpr uint8_t  O45_MAGIC[3]      = { 0x6F, 0x36, 0x35 }; // "o65"
constexpr uint8_t  O45_VERSION       = 0x00;

// --- Mode word bits (reused from .o65) ---
constexpr uint16_t O45_MODE_PAGED    = 0x0001;
constexpr uint16_t O45_MODE_OBJ      = 0x0000; // bits 1-2 = 00
constexpr uint16_t O45_MODE_EXE      = 0x0002; // bits 1-2 = 01
constexpr uint16_t O45_MODE_LIB      = 0x0004; // bits 1-2 = 10
constexpr uint16_t O45_MODE_SIMPLE   = 0x0008;
constexpr uint16_t O45_MODE_CHAIN    = 0x0010;
constexpr uint16_t O45_MODE_BSSZERO  = 0x0020;
constexpr uint16_t O45_MODE_CPUEXT   = 0x0800;
constexpr uint16_t O45_MODE_SIZE32   = 0x8000;

// Canonical .o45 mode: 32-bit addresses + extended CPU byte
constexpr uint16_t O45_MODE_DEFAULT  = O45_MODE_SIZE32 | O45_MODE_CPUEXT;

// --- CPU IDs (extended byte, when MODE_CPUEXT is set) ---
constexpr uint8_t  O45_CPU_6502      = 0x01;
constexpr uint8_t  O45_CPU_65C02     = 0x02;
constexpr uint8_t  O45_CPU_65CE02    = 0x03;
constexpr uint8_t  O45_CPU_65816     = 0x04;
constexpr uint8_t  O45_CPU_45GS02    = 0x45;

// --- Segment IDs (bits 4-0 of relocation type/seg byte) ---
enum O45Segment : uint8_t {
    SEG_EXTERNAL  = 0,   // external symbol (index follows)
    SEG_ABSOLUTE  = 1,   // absolute / no relocation
    SEG_TEXT      = 2,   // .text (code)
    SEG_DATA      = 3,   // .data (initialized)
    SEG_BSS       = 4,   // .bss  (uninitialized)
    SEG_ZP        = 5,   // .zp   (zero/direct page)
};

// --- Relocation types (bits 7-5 of relocation type/seg byte) ---
enum O45RelocType : uint8_t {
    R_LOW         = 0x20, // low byte of 16-bit address
    R_HIGH        = 0x40, // high byte of 16-bit address
    R_LINEAR24    = 0x60, // 24-bit linear address (3 bytes) [.o45 extension]
    R_WORD        = 0x80, // full 16-bit address (2 bytes)
    R_LINEAR32    = 0xA0, // 32-bit linear address (4 bytes) [.o45 extension]
    R_SEGADR      = 0xC0, // segment address (bank:addr, 3 bytes)
};

// Masks for splitting the type/seg byte
constexpr uint8_t  O45_RTYPE_MASK    = 0xE0; // bits 7-5
constexpr uint8_t  O45_RSEG_MASK     = 0x1F; // bits 4-0

// --- Relocation offset encoding ---
constexpr uint8_t  O45_RELOC_END     = 0x00; // end of relocation table
constexpr uint8_t  O45_RELOC_ESCAPE  = 0xFF; // add 254, read next byte

// --- Option types (reused from .o65) ---
constexpr uint8_t  OPT_END           = 0x00;
constexpr uint8_t  OPT_FNAME         = 0x01;
constexpr uint8_t  OPT_OS            = 0x02;
constexpr uint8_t  OPT_ASM           = 0x03;
constexpr uint8_t  OPT_AUTHOR        = 0x04;
constexpr uint8_t  OPT_CREATED       = 0x05;

// OS identifier for MEGA65
constexpr uint8_t  OPT_OS_MEGA65     = 0x05;

// --- Patch sizes per relocation type ---
inline int o45RelocPatchSize(uint8_t rtype) {
    switch (rtype & O45_RTYPE_MASK) {
        case R_LOW:       return 1;
        case R_HIGH:      return 1;
        case R_WORD:      return 2;
        case R_LINEAR24:  return 3;
        case R_SEGADR:    return 3;
        case R_LINEAR32:  return 4;
        default:          return 0;
    }
}
```

---

## 7. Mapping to Existing Assembler Structures [IMPLEMENTED]

The `.o45` segments map directly to the `ca45` segment infrastructure already in `AssemblerParser`:

| `.o45` Segment | `ca45` `.segment` name | `Segment` struct field |
|----------------|------------------------|------------------------|
| `SEG_TEXT`     | `"text"`               | `segments["text"]`     |
| `SEG_DATA`     | `"data"`               | `segments["data"]`     |
| `SEG_BSS`      | `"bss"`                | `segments["bss"]`      |
| `SEG_ZP`       | `"zp"`                 | `segments["zp"]`       |

The existing `Symbol` struct in `AssemblerTypes.hpp` already carries `value`, `isAddress`, and `size` — sufficient for populating export entries. Import tracking requires a new list of unresolved `.extern` names, indexed in declaration order (matching the symbol ID used in relocation entries).

---

## 8. Symbol Naming Convention (C Linkage) [IMPLEMENTED]

The `.o45` format adopts the traditional C symbol naming convention: all C-linkage symbols are stored with a single leading underscore (`_`) prefix. This ensures cross-toolchain compatibility with cc65, ACME, and other 6502-family tools.

### 8.1 Convention

| Symbol Kind      | C Source Name   | `.o45` Export/Import Name |
|------------------|-----------------|---------------------------|
| Function         | `main`          | `_main`                   |
| Function         | `printf`        | `_printf`                 |
| Global variable  | `count`         | `_count`                  |
| Global array     | `buffer`        | `_buffer`                 |

Local variables and parameters are never exported and have no mandated naming — they remain internal to the assembler.

Assembly-only symbols (not originating from C) that are exported via `.global` are stored **as-is** with no automatic prefix. The `_` prefix is a compiler convention, not a format requirement.

### 8.2 Changes Made in `cc45` (`CodeGenerator.cpp`)

The compiler previously used `_g_` prefix for globals. The following changes were applied:

#### A. Global Variables: `_g_` -> `_`

All `"_g_" + name` patterns changed to `"_" + name` (~15 sites), including:
- `resolveVarName()` lookup and prefix detection
- `getExprType()` global variable resolution
- `emitAddress()` struct member access
- `visit(VariableDeclaration)` global declaration
- `visit(Assignment)` global store paths
- `visit(MemberAccess)` and `visit(SizeofExpression)` lookups
- `emitData()` label emission (`.data` and `.bss` sections)

#### B. `isGlobal` Detection

Changed from prefix-based (`rName.substr(0,3) == "_g_"`) to lookup-based (`globalVariableTypes.count(rName)`). This is more robust and independent of naming convention. Applied at 5 sites (4x `isGlobal`, 1x `sourceGlobal`).

#### C. Function Labels: bare name -> `_` prefix

- Startup stub: `jsr main` -> `jsr _main`
- Proc declaration: `"proc " + node.name` -> `"proc _" + node.name`
- Function calls: `"jsr " + node.name` -> `"jsr _" + node.name`

#### D. Local Variables (`_l_`) and Parameters (`_p_`): No change

These are never exported and remain internal to the assembler.

#### E. `resolveVarName()` Prefix Detection

The early-return check was simplified: only `_p_` and `_l_` are checked (3-char prefixes). The global lookup uses `globalVariableTypes.count()` which naturally handles the `_` prefix.

### 8.3 Changes in `ca45`

No assembler changes needed — the assembler treats labels as opaque strings.

### 8.4 Summary

| Scope     | Old Prefix | New Prefix | Exported? | Status |
|-----------|-----------|------------|-----------|--------|
| Global var| `_g_`     | `_`        | Yes       | Done |
| Function  | (none)    | `_`        | Yes       | Done |
| Local var | `_l_`     | `_l_`      | No        | Unchanged |
| Parameter | `_p_`     | `_p_`      | No        | Unchanged |

---

## 9. Differences from Standard `.o65`

| Feature                | `.o65`                  | `.o45`                              |
|------------------------|-------------------------|-------------------------------------|
| Address width          | 16-bit (default)        | 32-bit (`SIZE32` always set)        |
| CPU identification     | `CPU_BITS` only         | `CPU_EXT` byte = `$45`             |
| `R_LINEAR24`           | Not defined             | `$60` — 24-bit linear patch        |
| `R_LINEAR32`           | Not defined             | `$A0` — 32-bit linear patch        |
| Symbol index width     | 2 bytes                 | 4 bytes (due to SIZE32)            |
| Fixed header size      | 26 bytes (16-bit mode)  | 41 bytes (32-bit + CPU ext byte)   |
