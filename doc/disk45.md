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
disk45 add <image> <host_file> [cbm_name]
```

Add a file from the host filesystem to the disk image.

| Argument | Description |
|----------|-------------|
| `host_file` | Path to the file on the host system |
| `cbm_name` | CBM filename (default: derived from host filename, max 16 chars) |

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
```

### extract / read

```
disk45 extract <image> <cbm_name> <host_file>
```

Extract a file from the disk image to the host filesystem.

**Examples:**
```bash
disk45 extract game.d81 "GAME" ./game.prg
disk45 extract backup.d64.gz "DATA" data.seq
disk45 extract archive.arc "PROGRAM" prog.prg   # decompresses automatically
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

## PETSCII Filename Handling

Filenames are automatically converted between ASCII and PETSCII:
- Lowercase `a-z` → PETSCII uppercase `$41-$5A`
- Uppercase `A-Z` → PETSCII shifted `$C1-$DA`
- Filenames are padded to 16 characters with shifted space (`$A0`)
- Maximum filename length: 16 characters

## Build Integration

### Makefile Example

```makefile
DISK45 = disk45

game.d81: build/main.prg build/sprites.bin build/levels.bin
	$(DISK45) create $@ -n "MY GAME" -i "MG"
	$(DISK45) add $@ build/main.prg "GAME"
	$(DISK45) add $@ build/sprites.bin "SPRITES"
	$(DISK45) add $@ build/levels.bin "LEVELS"
```

### Compressed Distribution

```makefile
dist: game.d81
	cp game.d81 game.d81.tmp
	$(DISK45) create game.d81.gz -n "MY GAME" -i "MG"
	# Or simply: gzip -k game.d81
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

## See Also

- `cc45(1)` — C compiler
- `ca45(1)` — Assembler
- `ln45(1)` — Linker
- `ar45(1)` — Static library archiver

## References

- [Peter Schepers' File Format Specifications](https://ist.uwaterloo.ca/~schepers/formats.html)
- [cbmconvert](https://github.com/dr-m/cbmconvert) — Reference implementation for CBM archive formats
- [Commodore file types](https://www.zimmers.net/anonftp/pub/cbm/FILETYPES.html)
