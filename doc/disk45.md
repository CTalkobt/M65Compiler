# disk45 — CBM Disk Image Utility

## Synopsis

```
disk45 <command> [options]
```

## Description

**disk45** creates, reads, and manipulates Commodore disk images and archive files. It supports multiple disk image formats used by C64, C128, C65, and MEGA65 computers, as well as common archive formats from the Commodore BBS era.

All formats are auto-detected from the file extension. GZ compression is transparent — append `.gz` to any format (e.g., `.d81.gz`).

## Commands

### create

```
disk45 create <image> [-n name] [-i id]
```

Create a new, empty disk image or archive.

| Option | Description |
|--------|-------------|
| `-n name` | Disk name (default: "CC45 DISK") |
| `-i id` | 2-character disk ID (default: "CC") |

**Examples:**
```bash
disk45 create game.d81 -n "SPACE INVADERS" -i "SI"
disk45 create backup.d64
disk45 create assets.d81.gz -n "ASSETS"   # compressed
disk45 create files.ark                    # ARK archive
disk45 create readme.lnx                   # LNX archive
```

### list / ls / dir

```
disk45 list <image>
```

Display the disk directory in CBM format: file sizes in sectors, filenames, and file types.

**Example output:**
```
0 "SPACE INVADERS" SI
42   "GAME"        PRG
3    "SPRITES"     SEQ
15   "LEVELS"      USR
3140 blocks free.
```

### info

```
disk45 info <image>
```

Display detailed information about a disk image.

**Example output:**
```
Format:       D81 (1581)
Disk name:    SPACE INVADERS
Disk ID:      SI
Tracks:       80
Total sectors:3200
Free sectors: 3140
Image size:   819200 bytes
Files:        3
```

### add / write

```
disk45 add <image> <host_file> [cbm_name] [-p]
```

Add a file from the host filesystem to the disk image.

| Argument | Description |
|----------|-------------|
| `host_file` | Path to the file on the host system |
| `cbm_name` | CBM filename (default: derived from host filename, max 16 chars) |
| `-p`, `--petscii` | Convert ASCII text → PETSCII encoding (for SEQ files) |

The CBM file type is auto-detected from the host file extension:
- `.prg` → PRG (Program)
- `.seq` → SEQ (Sequential)
- `.usr` → USR (User)
- Default → PRG

**Examples:**
```bash
disk45 add game.d81 build/main.prg "GAME"
disk45 add game.d81 assets/sprites.bin "SPRITES"
disk45 add game.d64 README.seq
disk45 add game.d81 readme.seq "README" -p    # convert ASCII → PETSCII
```

### extract / read

```
disk45 extract <image> <cbm_name> <host_file> [-p]
```

Extract a file from the disk image to the host filesystem.

| Option | Description |
|--------|-------------|
| `-p`, `--petscii` | Convert PETSCII text → ASCII encoding |

**Examples:**
```bash
disk45 extract game.d81 "GAME" ./game.prg
disk45 extract backup.d64.gz "DATA" data.seq
disk45 extract archive.arc "PROGRAM" prog.prg   # decompresses automatically
disk45 extract game.d81 "README" readme.txt -p   # PETSCII → ASCII
```

### remove / delete / rm

```
disk45 remove <image> <cbm_name>
```

Delete a file from the disk image. Frees the sectors in the BAM (for disk images) or removes the entry (for archives).

**Examples:**
```bash
disk45 remove game.d81 "OLD_FILE"
disk45 rm game.d64 "TEMP"
```

### rename / mv

```
disk45 rename <image> <old_name> <new_name>
```

Rename a file within the disk image directory.

**Examples:**
```bash
disk45 rename game.d81 "README" "DOCS"
disk45 mv game.d64 "OLD" "NEW"
```

### lock

```
disk45 lock <image> <cbm_name>
```

Lock a file to prevent accidental deletion (scratch). Sets bit 6 of the directory entry's file type byte. Locked files display with a `<` suffix in directory listings (matching CBM convention).

**Example:**
```bash
disk45 lock game.d81 "GAME"
disk45 list game.d81
# 42   "GAME"        PRG<
```

### unlock

```
disk45 unlock <image> <cbm_name>
```

Remove the lock from a file, allowing it to be deleted.

**Example:**
```bash
disk45 unlock game.d81 "GAME"
```

### label

```
disk45 label <image> [-n name] [-i id]
```

Change the disk name and/or disk ID of an existing image.

| Option | Description |
|--------|-------------|
| `-n name` | New disk name (max 16 chars) |
| `-i id` | New 2-character disk ID |

**Examples:**
```bash
disk45 label game.d81 -n "FINAL VERSION"
disk45 label game.d81 -i "FV"
disk45 label game.d81 -n "RELEASE" -i "R1"
```

### validate / check

```
disk45 validate <image>
```

Check disk image integrity by comparing the BAM against actual file sector chains. Reports:
- **Cross-linked sectors** — sectors claimed by multiple files
- **Orphaned sectors** — marked used in BAM but not part of any file chain
- **Broken chains** — file chains pointing to invalid track/sector addresses

**Example output:**
```
Disk: "MY DISK" MD
Files found:           5
Sectors used by files: 42
Sectors marked used:   46
Free sectors (BAM):    3154
Orphaned sectors:      2
Status: OK
```

Returns exit code 0 if OK, 1 if errors found.

### bam / map

```
disk45 bam <image>
```

Display a visual track/sector allocation map showing which sectors are free (`.`) and allocated (`#`).

**Example output (D64):**
```
BAM: "MY DISK" MD  (664 blocks free)

Track  0         1         2
       012345678901234567890
   1   .....................  21/21
  17   .....................  21/21
  18   ##.................    17/19
  19   ###................    16/19
  35   .................      17/17

Legend: . = free, # = allocated
```

### dump / hex

```
disk45 dump <image> [track] [sector]
```

Hex dump raw sector data with ASCII display.

| Usage | Description |
|-------|-------------|
| `dump <image>` | Dump BAM/header sector |
| `dump <image> <track>` | Dump all sectors on a track |
| `dump <image> <track> <sector>` | Dump a single sector |

**Example output:**
```
Track 18, Sector 0:
00: 12 01 41 00 15 FF FF 1F  15 FF FF 1F 15 FF FF 1F  |..A.............|
10: 15 FF FF 1F 15 FF FF 1F  15 FF FF 1F 15 FF FF 1F  |................|
...
90: 4D 59 20 44 49 53 4B A0  A0 A0 A0 A0 A0 A0 A0 A0  |MY DISK.........|
```

## Supported Formats

### Disk Images

| Extension | Format | Capacity | Description |
|-----------|--------|----------|-------------|
| `.d64` | 1541 | 170 KB | Standard C64 disk. 35 tracks, variable sectors/track (17-21 per zone). BAM and directory at track 18. |
| `.d71` | 1571 | 340 KB | C128 double-sided disk. 70 tracks (35 per side), same zone layout as D64. BAM at tracks 18 and 53. |
| `.d81` | 1581 | 800 KB | C65/MEGA65 disk. 80 tracks × 40 sectors, uniform layout. BAM at track 40. **Recommended for MEGA65 development.** |
| `.d65` | MEGA65 | 1.6 MB | MEGA65 native extended format. 162 tracks (81 per side) × 40 sectors. BAM at track 40 across 4 sectors. |

### Archives

| Extension | Format | Compression | Description |
|-----------|--------|-------------|-------------|
| `.ark` | Arkive | None | Uncompressed collection of CBM files. Simple 29-byte-per-entry directory. Full read/write support. |
| `.arc` | ARC | Multiple | Compressed archive. Reads all modes (stored, RLE, Huffman, LZW). Writes in stored mode. |
| `.sda` | SDA | Multiple | Self-Dissolving Archive. Same as ARC with a BASIC loader header (auto-detected and skipped). |
| `.lnx` | Lynx | None | BASIC stub + ASCII header archive. 254-byte block-aligned data. Full read/write support. |

### Compression

| Extension | Description |
|-----------|-------------|
| `.gz` | GZ compression. Append to any format: `.d81.gz`, `.d64.gz`, `.ark.gz`, etc. Transparent on read (auto-detected via magic bytes `1F 8B`) and write (triggered by `.gz` extension). Uses zlib deflate. |

## ARC Compression Modes

When **reading** ARC archives, disk45 supports all compression modes:

| Mode | Name | Algorithm | Description |
|------|------|-----------|-------------|
| 0 | Stored | None | Uncompressed data |
| 1 | Packed | RLE | Run-Length Encoding with configurable control byte |
| 2 | Squeezed | Huffman | Huffman coding with inline code tree |
| 3 | Crunched | LZW | 12-bit Lempel-Ziv-Welch (codes 256=EOF, 257=reserved) |
| 4 | Squashed | Huffman+RLE | Huffman followed by RLE expansion |
| 5 | Crunched (1-pass) | LZW | LZW with trailing checksum and size |

When **writing** to ARC archives, disk45 uses stored mode (mode 0) only.

## PETSCII Handling

### Filenames

Filenames are automatically converted between ASCII and PETSCII for all operations:
- Lowercase `a-z` → PETSCII `$41-$5A` (unshifted uppercase)
- Uppercase `A-Z` → PETSCII `$C1-$DA` (shifted uppercase)
- Filenames are padded to 16 characters with shifted space (`$A0`)
- Maximum filename length: 16 characters

### File Content (-p flag)

The `-p` / `--petscii` flag converts SEQ file content between ASCII and PETSCII:

| Direction | Conversion |
|-----------|------------|
| ASCII → PETSCII (`add -p`) | `a-z` → `$41-$5A`, `A-Z` → `$C1-$DA`, LF → CR, strips `\r` |
| PETSCII → ASCII (`extract -p`) | `$41-$5A` → `a-z`, `$C1-$DA` → `A-Z`, CR → LF |

Special character mappings: `~` ↔ π (`$A8`), `|` ↔ bar (`$7D`), `\` ↔ £ (`$5C`)

**Example round-trip:**
```bash
echo "Hello World!" > readme.txt
disk45 add game.d81 readme.txt "README" -p     # ASCII → PETSCII
disk45 extract game.d81 "README" out.txt -p    # PETSCII → ASCII
diff readme.txt out.txt                         # identical
```

## Build Integration

### Makefile Example

```makefile
DISK45 = disk45

game.d81: build/main.prg build/sprites.bin build/levels.bin
	$(DISK45) create $@ -n "MY GAME" -i "MG"
	$(DISK45) add $@ build/main.prg "GAME"
	$(DISK45) add $@ build/sprites.bin "SPRITES"
	$(DISK45) add $@ build/levels.bin "LEVELS"
	$(DISK45) add $@ README.seq "README" -p
	$(DISK45) validate $@
```

### Compressed Distribution

```makefile
dist: game.d81
	$(DISK45) create game.d81.gz -n "MY GAME" -i "MG"
	$(DISK45) add game.d81.gz build/main.prg "GAME"
```

## Library API

The disk image library can be used programmatically from C++:

```cpp
#include "DiskImage.hpp"
#include "D81Image.hpp"

// Create a new D81
auto img = DiskImage::create(DiskFormat::D81);
img->format("MY DISK", "MD");

// Add a file
std::vector<uint8_t> data = loadFile("game.prg");
img->addFile("GAME", CbmFileType::PRG, data);

// Rename, relabel
img->renameFile("GAME", "MAIN");
img->setDiskName("FINAL DISK");
img->setDiskId("FD");

// Validate
auto result = img->validate();
if (!result.ok()) { /* handle errors */ }

// Save (with optional GZ compression)
img->saveToFile("game.d81");      // uncompressed
img->saveToFile("game.d81.gz");   // compressed

// Load and read
auto disk = DiskImage::load("game.d81.gz");  // auto-decompresses
auto files = disk->listFiles();
auto prg = disk->readFile("GAME");
```

### Supported Classes

| Class | Header | Description |
|-------|--------|-------------|
| `DiskImage` | `DiskImage.hpp` | Abstract base class |
| `D64Image` | `D64Image.hpp` | C64 1541 format |
| `D71Image` | `D71Image.hpp` | C128 1571 format |
| `D81Image` | `D81Image.hpp` | C65/MEGA65 1581 format |
| `D65Image` | `D65Image.hpp` | MEGA65 native format |
| `ArkImage` | `ArkImage.hpp` | Arkive archive format |
| `ArcImage` | `ArcImage.hpp` | ARC/SDA archive format |
| `LnxImage` | `LnxImage.hpp` | Lynx archive format |

## Command Summary

| Command | Aliases | Description |
|---------|---------|-------------|
| `create` | — | Create empty disk image or archive |
| `list` | `ls`, `dir` | List directory |
| `info` | — | Show disk metadata |
| `add` | `write` | Add file (with optional `-p` PETSCII conversion) |
| `extract` | `read` | Extract file (with optional `-p` PETSCII conversion) |
| `remove` | `delete`, `rm` | Delete file from image |
| `rename` | `mv` | Rename file in directory |
| `lock` | — | Lock file (prevent scratch) |
| `unlock` | — | Unlock file |
| `label` | — | Change disk name / ID |
| `validate` | `check` | Check BAM consistency |
| `bam` | `map` | Visual sector allocation map |
| `dump` | `hex` | Hex dump track/sector data |

## See Also

- `cc45(1)` — C compiler
- `ca45(1)` — Assembler
- `ln45(1)` — Linker
- `ar45(1)` — Static library archiver

## References

- [Peter Schepers' File Format Specifications](https://ist.uwaterloo.ca/~schepers/formats.html)
- [cbmconvert](https://github.com/dr-m/cbmconvert) — Reference implementation for CBM archive formats
- [Commodore file types](https://www.zimmers.net/anonftp/pub/cbm/FILETYPES.html)
