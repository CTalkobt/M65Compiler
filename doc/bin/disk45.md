# disk45 — CBM Disk Image Utility

## Synopsis

```
disk45 <command> [options]
```

## Description

**disk45** creates, reads, and manipulates Commodore disk images, tape images, and archive files. It supports 25 formats spanning C64, C128, PET, MEGA65, and CMD devices. Features include FUSE filesystem mounting, SQLite-based collection cataloging, format conversion, batch scripting, and full PETSCII handling.

All formats are auto-detected from the file extension. GZ compression is transparent — append `.gz` to any format (e.g., `.d81.gz`).

## Commands

### File Operations

#### create

```
disk45 create <image> [-n name] [-i id] [-t tracks]
```

Create a new, empty disk image or archive.

| Option | Description |
|--------|-------------|
| `-n name` | Disk name (default varies by format) |
| `-i id` | 2-character disk ID (default: "CC") |
| `-t tracks` | Track count (for extended D64: 36-40, SpeedDOS/DolphinDOS) |

**Examples:**
```bash
# Standard MEGA65 disk (recommended)
disk45 create game.d81 -n "MY GAME" -i "MG"

# 40-track C64 disk (standard capacity is 35 tracks)
disk45 create extended.d64 -n "SPEEDDOS" -t 40

# Compressed disk (adds .gz transparently)
disk45 create backup.d64.gz -n "BACKUP"

# Multi-disk project setup
for i in 1 2 3; do
  disk45 create project_$i.d81 -n "PROJECT $i" -i "P$i"
done

# Large capacity MEGA65 disk
disk45 create large.d65 -n "ARCHIVE" -i "AR"
```

**Common Use Cases:**
- Create blank disk for archiving project files
- Create bootable disk with game code and assets
- Create data disk for large collections
- Create multi-disk set for complex projects

#### list / ls / dir

```
disk45 list <image> [pattern]
disk45 list <image> SUBDIR/              # list subdirectory
disk45 list <image> SUBDIR/pattern       # filter within subdirectory
```

Display directory. Supports wildcard patterns (`*`, `?`) and subdirectory navigation.

Splat (unclosed) files show `*` before the type: `*PRG`. Locked files show `<` after.

```
0 "MY GAME" MG
42   "GAME"        PRG
3    "SPRITES"     SEQ
1    "UTILS"       CBM
15   "OLD"         *PRG
3140 blocks free.
```

**Examples:**
```bash
# List entire disk
disk45 list game.d81

# List with wildcard pattern
disk45 list game.d81 "LEVEL*"      # All files starting with LEVEL
disk45 list game.d81 "*.prg"       # Files with PRG extension (if named that way)

# List subdirectory
disk45 list game.d81 "ASSETS/"
disk45 list game.d81 "ASSETS/*.seq"

# Check for locked files
disk45 list game.d81 | grep "<"    # Find locked files

# Count files
disk45 list game.d81 | grep -c "PRG"
```

**File Status Indicators:**
- `*PRG` — Splat (unclosed) PRG file
- `<` suffix — Locked file (protected)
- `.` prefix — Deleted (partial, should be purged)

#### add / write

```
disk45 add <image> <host_file> [cbm_name] [-p]
```

Add a file. Type auto-detected from extension (`.prg`→PRG, `.seq`→SEQ, `.usr`→USR). `-p` converts ASCII→PETSCII for SEQ content.

**Examples:**
```bash
# Add compiled program
disk45 add game.d81 build/main.prg GAME

# Add with custom name (defaults to filename without path)
disk45 add game.d81 /src/utils.prg UTILS

# Add text file, converting ASCII to PETSCII
disk45 add game.d81 readme.txt README -p

# Add multiple files in batch
for f in build/*.prg; do
  disk45 add game.d81 "$f"
done

# Add binary/data file
disk45 add game.d81 assets/sprites.bin SPRITES

# Add with subdirectory
disk45 mkdir game.d81 ASSETS/
disk45 add game.d81 sprites.bin ASSETS/SPRITES
```

#### extract / read

```
disk45 extract <image> <cbm_name> <host_file> [-p]
```

Extract a file. `-p` converts PETSCII→ASCII.

**Examples:**
```bash
# Extract program to host
disk45 extract game.d81 GAME recovered.prg

# Extract text file, converting PETSCII to ASCII
disk45 extract game.d81 README readme.txt -p

# Extract to stdout (no file specified)
disk45 extract game.d81 DATA_FILE | hexdump -C

# Extract with subdirectory
disk45 extract game.d81 ASSETS/SPRITES sprites.bin

# Quick examination of file
disk45 extract game.d81 MYFILE /tmp/temp.bin && hexdump -C /tmp/temp.bin
```

#### extract-all

```
disk45 extract-all <image> [output_dir] [pattern] [-p]
```

Extract all files (or matching pattern) to a directory. Files get type extensions (`.prg`, `.seq`). Handles name collisions with numeric suffixes.

**Examples:**
```bash
# Extract entire disk to current directory
disk45 extract-all backup.d81

# Extract to specific directory
disk45 extract-all backup.d81 ./recovered/

# Extract only PRG files
disk45 extract-all backup.d81 ./prgs/ "*.prg"

# Extract with pattern matching
disk45 extract-all backup.d81 ./levels/ "LEVEL*"

# Extract and convert PETSCII text files
disk45 extract-all backup.d81 ./text/ "*.txt" -p

# Backup entire disk
mkdir backup_contents
disk45 extract-all old_disk.d64 backup_contents/ ""
```

#### copy / cp

```
disk45 copy <src_image> <pattern> <dst_image>
```

Copy files between images (cross-format). Preserves file types.

**Examples:**
```bash
# Copy all files D64 → D81
disk45 copy old.d64 "*" new.d81

# Copy matching files between formats
disk45 copy games.d81 "LEVEL*" backup.d64

# Copy specific file type
disk45 copy project.d81 "*.prg" archive.d81

# Copy from subdirectory
disk45 copy game.d81 "ASSETS/*" backup.d81

# Multi-disk consolidation
disk45 copy disk1.d64 "*" archive.d81
disk45 copy disk2.d64 "*" archive.d81
disk45 copy disk3.d64 "*" archive.d81

# Copy with rename (manual per-file)
disk45 copy old.d64 "OLDNAME" work.d81
disk45 rename work.d81 "OLDNAME" "NEWNAME"
```

**Cross-format Compatibility:**
- D64 → D81: 170KB → 800KB (lossless)
- D81 → D64: 800KB → 170KB (fails if files exceed capacity)
- D64 ↔ D71 ↔ ARK: All compatible (verify with `disk45 info` before copying)

#### convert

```
disk45 convert <source> <target>
```

Convert between formats, copying all files. For single-file sources (`.prg`, `.cvt`), creates a blank target image and adds the file.

**Examples:**
```bash
# Standard format conversion
disk45 convert game.d64 game.d81     # D64 → D81 (C64 → MEGA65)
disk45 convert game.d81 game.d64     # D81 → D64 (if content fits)

# PRG to disk
disk45 convert game.prg release.d81  # Single PRG → bootable disk
disk45 convert main.prg project.d81 -n "MY PROJECT"  # With disk name

# Archive conversion
disk45 convert archive.ark disk.d64  # ARK archive → D64 disk
disk45 convert games.d64 archive.ark # D64 → ARK archive (compact)

# Batch conversion with loop
for f in *.d64; do
  disk45 convert "$f" "${f%.d64}.d81"
done

# Upgrade to newer format
disk45 convert old.d64 new.d81        # C64 disk → MEGA65 disk
disk45 convert old.d81 new.d65        # D81 → native MEGA65 format

# Create bootable disk from compiled program
cc45 -o main.prg main.c
disk45 convert main.prg bootable.d81 -n "GAME"
```

**Format Compatibility Matrix:**
- D64, D71, D81, D65: Full read/write, lossless if capacity allows
- ARK, T64: Archive formats (consolidation, distribution)
- G64, NIB, TAP: Read-only formats (GCR-encoded, raw)

#### remove / delete / rm

```
disk45 remove <image> <cbm_name>
```

#### rename / mv

```
disk45 rename <image> <old_name> <new_name>
```

Works for both files and CBM subdirectory entries.

#### lock / unlock

```
disk45 lock <image> <cbm_name>
disk45 unlock <image> <cbm_name>
```

Toggle the locked flag (bit 6 of file type).

#### splat / unsplat

```
disk45 splat <image> <cbm_name>
disk45 unsplat <image> <cbm_name>
```

Toggle the closed/splat flag (bit 7 of file type). Splat files show `*PRG` in listings.

#### label

```
disk45 label <image> [-n name] [-i id]
```

Change disk name and/or ID.

### Disk Operations

#### info

```
disk45 info <image>
```

Show format-specific metadata. Detects TAP (version, platform), GEOS CVT (type, structure, description), and standard disk info.

#### validate / check

```
disk45 validate <image>
```

Check BAM consistency. Reports cross-linked sectors, orphaned sectors, and broken chains.

#### bam / map

```
disk45 bam <image>
```

Visual sector allocation map (`.` = free, `#` = allocated).

### Block-Level Operations

#### dump / hex

```
disk45 dump <image> [track] [sector]
```

Hex dump with ASCII display.

#### bread

```
disk45 bread <image> <track> <sector> [output_file]
```

Read a raw 256-byte sector. Outputs hex if no file specified.

#### bwrite

```
disk45 bwrite <image> <track> <sector> <input_file>
```

Write a raw sector from file.

#### bpeek

```
disk45 bpeek <image> <track> <sector> <offset> [count]
```

Read byte(s) at a specific offset within a sector.

#### bpoke

```
disk45 bpoke <image> <track> <sector> <offset> <value> [value...]
```

Write byte(s) at a specific offset.

#### bfill

```
disk45 bfill <image> <track> <sector> <value>
```

Fill entire sector with a byte value.

#### chain

```
disk45 chain <image> <filename>
```

Display a file's sector chain with byte counts.

```
File "GAME" chain:
  17/0 → 17/10 → 17/20 → 18/1 → 18/4
  5 sector(s), 1270 bytes
```

### REL File Operations

#### rel-create

```
disk45 rel-create <image> <name> <record_len> [num_records]
```

Create a REL (relative/record-based) file with fixed-length records and side-sector chain.

#### rel-read

```
disk45 rel-read <image> <name> <record_num>
```

Read a specific record (1-based). Shows hex and text.

#### rel-write

```
disk45 rel-write <image> <name> <record_num> <data...>
```

Write data to a record. Accepts hex values or quoted strings.

#### rel-list

```
disk45 rel-list <image> <name>
```

List all non-empty records.

### Subdirectory Operations (D71/D81)

#### mkdir

```
disk45 mkdir <image> <dirname>
disk45 mkdir <image> PARENT/NEWDIR       # nested
```

Create a CBM subdirectory (file type CBM/$85). Directory auto-expands when full.

#### rmdir

```
disk45 rmdir <image> <dirname>
```

Remove an empty subdirectory. Frees sectors and shrinks parent directory chain.

#### tree

```
disk45 tree <image>
```

Display recursive directory tree.

```
MY DISK/
  GAME (PRG, 42 blocks)
  UTILS/
    EDITOR (PRG, 15 blocks)
  DATA/
    LEVELS/
```

#### dir-shrink

```
disk45 dir-shrink <image> [path]
```

Reclaim trailing empty directory sectors.

#### dir-ensure

```
disk45 dir-ensure <image> <min_free_slots> [path]
```

Ensure a directory has at least N free entry slots, allocating new sectors as needed.

### FUSE Filesystem Mount

```
disk45 mount <image> <mountpoint> [-ro]
```

Mount a disk image as a Linux directory using FUSE3. Files appear with type extensions (`.prg`, `.seq`). Full read/write support.

```bash
disk45 mount game.d81 /mnt/cbm &
ls /mnt/cbm/                    # GAME.prg  SPRITES.seq
cat /mnt/cbm/GAME.prg           # read file contents
cp newfile.prg /mnt/cbm/        # add file to image
rm /mnt/cbm/OLD.prg             # remove file
df /mnt/cbm/                    # show free space
fusermount -u /mnt/cbm          # unmount
```

Requires `libfuse3-dev` at build time (auto-detected). Read-only for TAP/G64/CVT/NIB.

### Catalog (Collection Indexing)

SQLite-based indexing with CRC32 content hashing for large collections.

#### catalog build

```
disk45 catalog build <dir1> [dir2...] [--db file]
```

Scan directories recursively, index all disk/tape/archive images. Incremental: skips unchanged files (by mtime).

#### catalog search

```
disk45 catalog search <pattern> [--db file]
```

Search indexed files by name. Supports `*` and `?` wildcards.

```
  BOULDER DASH      PRG    42 blk  51C4CE0B  /collections/games.d64
  BOULDER DASH      PRG    42 blk  51C4CE0B  /collections/demo.d64
2 result(s)
```

#### catalog list

```
disk45 catalog list [--db file]
```

Summary of all indexed images.

#### catalog duplicates

```
disk45 catalog duplicates [--db file]
```

Find duplicate files by CRC32 hash.

#### catalog stats

```
disk45 catalog stats [--db file]
```

Collection statistics: image count, file count, format and type breakdown.

#### catalog prune

```
disk45 catalog prune [--db file]
```

Remove entries for images that no longer exist on disk.

Default database: `~/.disk45-catalog.db`. Override with `--db path`.

### Encoding Filters

```
disk45 -a2p    # ASCII → PETSCII (stdin → stdout)
disk45 -p2a    # PETSCII → ASCII (stdin → stdout)
```

Pipe filters for use with FUSE mounts and shell pipelines:

```bash
cat commodore_file | disk45 -p2a              # read PETSCII as ASCII
echo "HELLO" | disk45 -a2p > file.seq         # write ASCII as PETSCII
```

### Batch Scripting

```
disk45 batch <scriptfile>
```

Execute commands from a file. One command per line, `#` for comments, quoted arguments supported.

**Basic Example:**
```bash
# build_disk.script
create game.d81 -n "MY GAME" -i "MG"
add game.d81 build/main.prg GAME
add game.d81 assets/sprites.bin SPRITES
validate game.d81
```

```bash
disk45 batch build_disk.script
```

**Advanced Examples:**

**Multi-disk project build:**
```bash
# project_build.script
# Create main disk
create project.d81 -n "PROJECT" -i "P1"
mkdir project.d81 ASSETS/
mkdir project.d81 DATA/

# Add compiled files
add project.d81 build/main.prg MAIN
add project.d81 build/kernel.prg KERNEL

# Add data in subdirectories
add project.d81 assets/sprites.bin ASSETS/SPRITES
add project.d81 assets/music.sid ASSETS/MUSIC
add project.d81 data/levels.dat DATA/LEVELS

# Validate and finish
validate project.d81
info project.d81
```

**Disk backup with directory structure:**
```bash
# backup.script
create backup.d81 -n "BACKUP" -i "BK"
mkdir backup.d81 SRC/
mkdir backup.d81 BUILD/
mkdir backup.d81 DOC/

add backup.d81 src/main.c SRC/MAIN.C
add backup.d81 src/util.c SRC/UTIL.C
add backup.d81 build/main.prg BUILD/MAIN.PRG
add backup.d81 doc/readme.txt DOC/README

validate backup.d81
```

**Deployment script:**
```bash
# deploy.script
# Copy to multiple distributions
copy project.d81 "*" dist_v1.d81
copy project.d81 "*" dist_v2.d81
copy project.d81 "*" dist_v3.d81

# Verify all copies
validate dist_v1.d81
validate dist_v2.d81
validate dist_v3.d81
```

## Common Use Cases

### Game Development Workflow

**Scenario:** Develop a game with code, sprites, and audio assets.

```bash
# 1. Create blank game disk
disk45 create game.d81 -n "MY GAME" -i "MG"

# 2. Create asset directories
disk45 mkdir game.d81 ASSETS/
disk45 mkdir game.d81 ASSETS/SPRITES/
disk45 mkdir game.d81 ASSETS/AUDIO/

# 3. Compile and add code
cc45 -O2 src/main.c -o build/main.prg
disk45 add game.d81 build/main.prg GAME

# 4. Add compiled assets
disk45 add game.d81 assets/sprites.bin ASSETS/SPRITES/DATA
disk45 add game.d81 assets/music.sid ASSETS/AUDIO/MUSIC

# 5. Verify disk integrity
disk45 validate game.d81
disk45 info game.d81

# 6. Create backups
disk45 copy game.d81 "*" backup.d81
```

### Data Archival & Recovery

**Scenario:** Archive old C64 disks, recover files, create distributions.

```bash
# 1. Analyze original disk
disk45 info old_disk.d64
disk45 list old_disk.d64
disk45 validate old_disk.d64

# 2. Convert to modern format
disk45 convert old_disk.d64 archive.d81

# 3. Extract and recover files
mkdir recovered/
disk45 extract-all archive.d81 recovered/ ""

# 4. Check for duplicates across collection
disk45 catalog build ./disk_collection/
disk45 catalog duplicates

# 5. Create distribution copies
disk45 copy archive.d81 "*" dist1.d81
disk45 copy archive.d81 "*" dist2.d81
```

### Build Automation

**Scenario:** Automatically build game disk from source files.

Create `build_game.sh`:
```bash
#!/bin/bash
set -e

# Compile
cc45 -O2 src/main.c -o build/main.prg
cc45 -O2 src/utils.c -o build/utils.o45

# Assemble data
ca45 assets/data.s45 -o build/data.o45

# Create disk
disk45 create game.d81 -n "GAME" -i "GM"
disk45 mkdir game.d81 ASSETS/

# Add files
disk45 add game.d81 build/main.prg MAIN
disk45 add game.d81 build/utils.o45 UTILS
disk45 add game.d81 assets/sprites.bin ASSETS/SPRITES

# Verify
disk45 validate game.d81
disk45 info game.d81

echo "Build complete: game.d81"
```

### FUSE Mount Workflow

**Scenario:** Edit files on disk image like a normal filesystem.

```bash
# Mount disk
mkdir /tmp/disk_mount
disk45 mount game.d81 /tmp/disk_mount &

# Edit files normally
echo "PRINT \"HELLO WORLD\"" > /tmp/disk_mount/hello.bas

# Copy files from CLI
cp build/main.prg /tmp/disk_mount/

# Remove files
rm /tmp/disk_mount/old_file.prg

# Unmount
fusermount -u /tmp/disk_mount

# Verify changes
disk45 list game.d81
```

### Catalog Management

**Scenario:** Manage large collection of game disks, find duplicates.

```bash
# Build catalog of all games
mkdir ~/my_collection
# ... copy/download disks to ~/my_collection ...

# Index the collection
disk45 catalog build ~/my_collection/ --db games.db

# Find all Boulder Dash copies
disk45 catalog search "BOULDER DASH" --db games.db

# Find duplicates (by CRC32)
disk45 catalog duplicates --db games.db

# Get statistics
disk45 catalog stats --db games.db

# Clean up missing images
disk45 catalog prune --db games.db
```

### Format Conversion Pipeline

**Scenario:** Convert mixed-format collection to standard D81.

```bash
#!/bin/bash
# Convert all disks in directory to D81

for f in *.d64 *.d71 *.ark *.t64; do
  [ -f "$f" ] || continue
  echo "Converting $f..."
  
  # Remove extension, add .d81
  base="${f%.*}"
  disk45 convert "$f" "$base.d81"
done

# Verify all conversions
for d in *.d81; do
  disk45 validate "$d"
done

echo "Conversion complete"
```

### Batch File Processing

**Scenario:** Add multiple programs from build directory to disk.

```bash
# Create disk
disk45 create tools.d81 -n "TOOLS" -i "TL"

# Add all PRG files with their source names
for f in build/*.prg; do
  name=$(basename "$f" .prg)
  disk45 add tools.d81 "$f" "$name"
done

# Verify
disk45 list tools.d81
```

## Supported Formats (25)

### Commodore Floppy Disks

| Extension | Format | R/W | Capacity | Description |
|-----------|--------|-----|----------|-------------|
| `.d64` | 1541 | R/W | 170 KB | C64 standard. 35 tracks (extendable to 40). |
| `.d71` | 1571 | R/W | 340 KB | C128 double-sided. |
| `.d81` | 1581 | R/W | 800 KB | C65/MEGA65. **Recommended.** |
| `.d65` | MEGA65 | R/W | 1.6 MB | MEGA65 native extended. |
| `.d80` | 8050 | R/W | 533 KB | CBM PET 8050. |
| `.d82` | 8250 | R/W | 1 MB | CBM PET 8250. |

### Hard Drives & CMD Devices

| Extension | Format | R/W | Capacity | Description |
|-----------|--------|-----|----------|-------------|
| `.d90` | D9090 | R/W | 7.5 MB | CBM hard drive (6-head). |
| `.d60` | D9060 | R/W | 5 MB | CBM hard drive (4-head). |
| `.d1m` | FD-2000 DD | R/W | 207 KB | CMD FD-2000 double density. |
| `.d2m` | FD-2000 HD | R/W | 414 KB | CMD FD-2000 high density. |
| `.d4m` | FD-4000 ED | R/W | 829 KB | CMD FD-4000 extra density. |
| `.dnp` | Native | R/W | ≤16 MB | CMD Native Partition. |

### Raw/GCR Formats

| Extension | Format | R/W | Description |
|-----------|--------|-----|-------------|
| `.g64` | GCR-1541 | R | GCR-encoded 1541 (decoded to D64). |
| `.g71` | GCR-1571 | R | GCR-encoded 1571 (decoded to D71). |
| `.x64` | Extended | R/W | D64 with 64-byte header. |
| `.nib` | Nibble | R | Raw nibble dump (decoded to D64). |

### Tape Formats

| Extension | Format | R/W | Description |
|-----------|--------|-----|-------------|
| `.tap` | TAP | R | Raw tape pulses (CBM ROM decoding). |
| `.t64` | T64 | R/W | Tape container (file-level). |

### Archives & Containers

| Extension | Format | R/W | Description |
|-----------|--------|-----|-------------|
| `.ark` | Arkive | R/W | Uncompressed archive. |
| `.arc`/`.sda` | ARC | R | Compressed (RLE/Huffman/LZW). |
| `.lnx` | Lynx | R/W | Block-aligned archive. |
| `.cvt` | GEOS | R | GEOS Convert (info/list/extract). |
| `.p00` | PC64 | R/W | PC64 single-file container (.s00/.u00/.r00). |
| `1!name` | Zipcode | R | 4-pack RLE (auto-finds 2!-4!). |

### Compression

| Extension | Description |
|-----------|-------------|
| `.gz` | Transparent GZ on all formats. |

## Filename Handling

### Input Convention

- `NAME` — ASCII, auto-converted to uppercase PETSCII
- `@"NAME"` — explicit ASCII (same as default)
- `p"NAME"` — raw PETSCII, no conversion
- Case-insensitive lookups throughout

### PETSCII Content (-p flag)

| Direction | Conversion |
|-----------|------------|
| ASCII→PETSCII (`-p` on add) | `a-z`→`$41-$5A`, `A-Z`→`$C1-$DA`, LF→CR |
| PETSCII→ASCII (`-p` on extract) | `$41-$5A`→`A-Z`, `$C1-$DA`→`A-Z`, CR→LF |

## Docker Usage

```bash
# Compile
docker run --rm -v $(pwd):/work cc45 cc45 -c /work/main.c -o /work/main.o45

# Disk operations
docker run --rm -v $(pwd):/work cc45 disk45 list /work/game.d81

# Catalog with persistent database
docker run --rm -v $(pwd):/work -v catalog:/catalog cc45 \
  disk45 catalog build /work --db /catalog/disk45.db
```

## Build Integration

```makefile
game.d81: build/main.prg build/sprites.bin
	disk45 create $@ -n "MY GAME" -i "MG"
	disk45 add $@ build/main.prg "GAME"
	disk45 add $@ build/sprites.bin "SPRITES"
	disk45 validate $@
```

Or using batch scripts:

```bash
disk45 batch build_disk.script
```

## Library API

```cpp
#include "DiskImage.hpp"

auto img = DiskImage::create(DiskFormat::D81);
img->format("MY DISK", "MD");
img->addFile("GAME", CbmFileType::PRG, data);
img->saveToFile("game.d81.gz");

auto disk = DiskImage::load("game.d81.gz");
auto files = disk->listFiles();
auto prg = disk->readFile("GAME");
```

### Supported Classes

| Class | Header | Formats |
|-------|--------|---------|
| `D64Image` | `D64Image.hpp` | D64 (35-40 tracks) |
| `D71Image` | `D71Image.hpp` | D71 |
| `D81Image` | `D81Image.hpp` | D81 |
| `D65Image` | `D65Image.hpp` | D65 |
| `D80Image` | `D80Image.hpp` | D80, D82 |
| `D90Image` | `D90Image.hpp` | D90, D60 |
| `CmdImage` | `CmdImage.hpp` | D1M, D2M, D4M, DNP |
| `G64Image` | `G64Image.hpp` | G64, G71 |
| `X64Image` | `X64Image.hpp` | X64 |
| `NibImage` | `NibImage.hpp` | NIB |
| `TapImage` | `TapImage.hpp` | TAP |
| `T64Image` | `T64Image.hpp` | T64 |
| `ArkImage` | `ArkImage.hpp` | ARK |
| `ArcImage` | `ArcImage.hpp` | ARC, SDA |
| `LnxImage` | `LnxImage.hpp` | LNX |
| `GeosCvtImage` | `GeosCvtImage.hpp` | CVT |
| `P00Image` | `P00Image.hpp` | P00/S00/U00/R00 |
| `ZipcodeImage` | `ZipcodeImage.hpp` | Zipcode 4-pack |

## Command Summary

| Command | Aliases | Description |
|---------|---------|-------------|
| `create` | — | Create disk image (`-n`, `-i`, `-t`) |
| `list` | `ls`, `dir` | List directory (wildcards, subdirs) |
| `info` | — | Show disk/tape/GEOS metadata |
| `add` | `write` | Add file (`-p` for PETSCII) |
| `extract` | `read` | Extract file (`-p` for PETSCII) |
| `extract-all` | `extractall` | Extract all files to directory |
| `copy` | `cp` | Copy files between images |
| `convert` | — | Convert between formats |
| `remove` | `delete`, `rm` | Delete file |
| `rename` | `mv` | Rename file or directory |
| `lock` | — | Lock file |
| `unlock` | — | Unlock file |
| `splat` | — | Mark file as unclosed |
| `unsplat` | — | Mark file as closed |
| `label` | — | Change disk name/ID |
| `validate` | `check` | Check BAM consistency |
| `bam` | `map` | Visual sector map |
| `dump` | `hex` | Hex dump sector(s) |
| `bread` | — | Read raw sector |
| `bwrite` | — | Write raw sector |
| `bpeek` | — | Read byte(s) from sector |
| `bpoke` | — | Write byte(s) to sector |
| `bfill` | — | Fill sector |
| `chain` | — | Show file sector chain |
| `rel-create` | — | Create REL file |
| `rel-read` | — | Read REL record |
| `rel-write` | — | Write REL record |
| `rel-list` | — | List REL records |
| `mkdir` | — | Create subdirectory |
| `rmdir` | — | Remove subdirectory |
| `tree` | — | Directory tree |
| `dir-shrink` | — | Reclaim empty dir sectors |
| `dir-ensure` | — | Ensure min free dir slots |
| `mount` | — | FUSE mount (if libfuse3) |
| `catalog` | — | Collection indexing (SQLite) |
| `batch` | — | Run commands from file |
| `-a2p` | — | ASCII→PETSCII filter |
| `-p2a` | — | PETSCII→ASCII filter |

## See Also

- `cc45(1)` — C compiler
- `ca45(1)` — Assembler
- `ln45(1)` — Linker
- `objdump45(1)` — Disassembler

## References

- [Peter Schepers' File Format Specifications](https://ist.uwaterloo.ca/~schepers/formats.html)
- [VICE c1541 Manual](https://vice-emu.sourceforge.io/vice_14.html)
- [DirMaster](https://style64.org/dirmaster)
- [cc1541](https://manpages.ubuntu.com/manpages/noble/man1/cc1541.1.html)
