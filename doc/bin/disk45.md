# disk45 — CBM Disk Image Utility

## Synopsis

```
disk45 <command> [arguments] [options]
disk45 batch <scriptfile>
disk45 -a2p | -p2a
```

## Description

**disk45** creates, reads and manipulates Commodore disk images, tape images and archive files. It handles 25 container formats spanning C64, C128, PET, MEGA65 and CMD devices (read/write support varies, see [Supported Formats](#supported-formats)). It also offers FUSE mounting, an SQLite catalog of image collections, format conversion, batch scripts and PETSCII conversion.

The format is chosen from the file extension; an unrecognised extension is treated as D81. GZ compression is transparent: append `.gz` to any format (e.g. `.d81.gz`) and the image is decompressed on load and compressed on save.

Running `disk45` with no arguments or an unknown command prints the usage summary (exit status 1). `disk45 --version` (or `-v`) prints the version.

## Commands

### File Operations

#### create

```
disk45 create <image> [-n name] [-i id] [-t tracks]
```

Create a new, empty image or archive. An existing file is overwritten.

| Option | Description |
|--------|-------------|
| `-n name` | Disk name (default `CC45 DISK` for disk images; archives use a fixed name such as `ARK ARCHIVE`) |
| `-i id` | 2-character disk ID (default `CC`) |
| `-t tracks` | Track count larger than the format default (e.g. 36-40 for an extended D64). The image is padded with the extra tracks, but the BAM is not extended, so files are not allocated there. |

**Examples:**
```bash
# Standard MEGA65 disk
disk45 create game.d81 -n "MY GAME" -i "MG"

# 40-track C64 image (standard is 35 tracks)
disk45 create extended.d64 -n "SPEEDDOS" -t 40

# Compressed image
disk45 create backup.d64.gz -n "BACKUP"

# MEGA65 native double-size disk
disk45 create large.d65 -n "ARCHIVE" -i "AR"
```

#### list / ls / dir

```
disk45 list <image> [pattern]
disk45 list <image> SUBDIR/              # list a subdirectory
disk45 list <image> SUBDIR/pattern       # filter within a subdirectory
```

Display the directory. Patterns use `*` and `?` and match the CBM file name (case-insensitive); they do not match the file type.

Unclosed (splat) files show `*` before the type (`*PRG`); locked files show `<` after it (`PRG<`). Columns are tab-separated:

```
0 "MY GAME" MG
1	"ASSETS"	CBM
12	"GAME"	PRG<
1	"README"	SEQ
12	"LEVEL1"	*PRG
3145 blocks free.
```

**Examples:**
```bash
disk45 list game.d81
disk45 list game.d81 "LEVEL*"        # names starting with LEVEL
disk45 list game.d81 ASSETS/         # contents of subdirectory ASSETS
disk45 list game.d81 "ASSETS/S*"
disk45 list game.d81 | grep "<"      # locked files
```

#### add / write

```
disk45 add <image> <host_file> [cbm_name] [-p|--petscii]
```

Add a host file. The CBM type comes from the host file's extension: `.prg`→PRG, `.seq`→SEQ, `.usr`→USR, anything else→PRG. The CBM name defaults to the host file name without directory and extension; names are upper-cased and truncated to 16 characters. Adding fails if the name already exists or the image is full.

`-p` converts ASCII→PETSCII, but only when the file is added as SEQ (host extension `.seq`).

Files are always added to the root directory: a name such as `ASSETS/SPRITES` creates a root entry with a `/` in its name, not a file inside subdirectory `ASSETS`.

**Examples:**
```bash
disk45 add game.d81 build/main.prg GAME
disk45 add game.d81 build/main.prg          # CBM name "MAIN"
disk45 add game.d81 readme.seq README -p    # SEQ file, ASCII → PETSCII
disk45 add game.d81 assets/sprites.bin SPRITES

for f in build/*.prg; do disk45 add game.d81 "$f"; done
```

#### extract / read

```
disk45 extract <image> <cbm_name> <host_file> [-p|--petscii]
```

Extract a file (name lookup is case-insensitive). `-p` converts PETSCII→ASCII (applied whatever the file type). The host file name is required; there is no stdout mode.

```bash
disk45 extract game.d81 GAME recovered.prg
disk45 extract game.d81 README readme.txt -p
```

#### extract-all / extractall

```
disk45 extract-all <image> [output_dir] [pattern] [-p|--petscii]
```

Extract all files (or those matching `pattern`, default `*`) into `output_dir` (default `.`, created if missing). Host names get a type extension (`.prg`, `.seq`, `.usr`, `.rel`, `.del`); characters not allowed in host names become `_`. An existing host file is not overwritten: a numeric suffix is added (`GAME_1.prg`). `-p` converts PETSCII→ASCII for SEQ files only.

```bash
disk45 extract-all backup.d81                    # into the current directory
disk45 extract-all backup.d81 ./recovered/
disk45 extract-all backup.d81 ./levels/ "LEVEL*"
disk45 extract-all backup.d81 ./text/ "*" -p
```

#### copy / cp

```
disk45 copy <src_image> <pattern> <dst_image>
```

Copy files matching `pattern` between images of any formats, keeping the file type. The destination must already exist (create it first). Files whose name already exists in the destination are skipped with a warning.

```bash
disk45 create new.d81
disk45 copy old.d64 "*" new.d81
disk45 copy games.d81 "LEVEL*" backup.d64
```

Only the root directory is copied; with `*`, a subdirectory entry is copied as an empty `CBM` entry.

#### convert

```
disk45 convert <source> <target>
```

Create `target` (format from its extension) and copy every root-directory file of `source` into it, keeping the disk name and ID. Subdirectory entries are skipped; files that do not fit produce a warning.

If `source` is a single file (`.prg`, `.seq`, `.usr` or `.cvt`), a blank target named `CONVERTED` (ID `CV`) is created and the file is added under its upper-cased base name (a `.cvt` file is stored as-is, as a PRG).

```bash
disk45 convert game.d64 game.d81
disk45 convert game.d81 game.d64          # files that don't fit are skipped
disk45 convert main.prg release.d81       # one PRG → new disk
disk45 convert games.d64 archive.ark
for f in *.d64; do disk45 convert "$f" "${f%.d64}.d81"; done
```

#### remove / delete / rm

```
disk45 remove <image> <cbm_name>
```

Delete one file (exact name, no wildcards).

#### rename / mv

```
disk45 rename <image> <old_name> <new_name>
```

Works for files and for CBM subdirectory entries in the root directory.

#### lock / unlock

```
disk45 lock <image> <cbm_name>
disk45 unlock <image> <cbm_name>
```

Set or clear the locked flag (bit 6 of the file type; shown as `<`). disk45's own `remove` does not check this flag.

#### splat / unsplat

```
disk45 splat <image> <cbm_name>
disk45 unsplat <image> <cbm_name>
```

Clear or set the closed flag (bit 7 of the file type). Splat files show `*PRG` in listings. Sector-based images only.

#### label

```
disk45 label <image> [-n name] [-i id]
```

Change the disk name and/or ID (at least one option is required).

### Disk Operations

#### info

```
disk45 info <image>
```

Show metadata. TAP images show version, platform, data size and file count; GEOS CVT files show GEOS name, type, structure (VLIR/sequential, record count) and description; other images show format, name, ID, tracks, total/free sectors, image size and file count.

#### validate / check

```
disk45 validate <image>
```

Check BAM consistency of a sector-based image: files found, sectors used by files, sectors marked used, free sectors, and counts of cross-linked sectors, orphaned sectors and broken chains. Exit status 1 if errors are found (archives always report "Validation not supported").

#### bam / map

```
disk45 bam <image>
```

Visual sector allocation map per track (`.` = free, `#` = allocated) with free/total counts. Sector-based images only.

### Block-Level Operations

These work on sector-based images. Track, sector and offset are decimal; byte values (`bpoke`, `bfill`) accept decimal, `0x` hex or leading-`0` octal (not `$`).

#### dump / hex

```
disk45 dump <image> [track] [sector]
```

Hex dump with ASCII. No arguments: the header/BAM sector (track 18 or 40, sector 0). A track only: every sector of that track.

#### bread

```
disk45 bread <image> <track> <sector> [output_file]
```

Read a raw 256-byte sector into a file, or hex-dump it if no file is given.

#### bwrite

```
disk45 bwrite <image> <track> <sector> <input_file>
```

Write a raw sector from the first 256 bytes of a file.

#### bpeek

```
disk45 bpeek <image> <track> <sector> <offset> [count]
```

Print `count` (default 1) bytes as hex, starting at `offset`.

#### bpoke

```
disk45 bpoke <image> <track> <sector> <offset> <value> [value...]
```

Write byte(s) starting at `offset`.

#### bfill

```
disk45 bfill <image> <track> <sector> <value>
```

Fill a sector with one byte value.

#### chain

```
disk45 chain <image> <filename>
```

Display a file's sector chain (root directory files) with sector and byte counts.

```
File "LEVEL1" chain:
  19/0 → 19/1 → 19/2 → 19/3 → 19/4 → 19/5 → 19/6 → 19/7 → 19/8 → 19/9
   → 19/10 → 19/11
  12 sector(s), 3000 bytes
```

### REL File Operations

REL commands look for the directory at track 18/sector 1, so they only work on D64/D71 images (on a D81/D65 that sector is part of the BAM, and `rel-create` corrupts it).

#### rel-create

```
disk45 rel-create <image> <name> <record_len> [num_records]
```

Create a REL file with fixed-length records (1-254 bytes, default 10 records), zero-filled, with a side-sector chain.

#### rel-read

```
disk45 rel-read <image> <name> <record_num>
```

Print a record (1-based) as hex and as text.

#### rel-write

```
disk45 rel-write <image> <name> <record_num> <data...>
```

Write a record. Each data argument is a number (decimal, `0x` hex) or a string in literal double quotes (pass `'"HELLO"'` from the shell). Data is zero-padded or truncated to the record length.

#### rel-list

```
disk45 rel-list <image> <name>
```

Show the record count and all non-empty records.

```bash
disk45 rel-create data.d64 SCORES 20 30
disk45 rel-write data.d64 SCORES 3 '"HELLO"'
disk45 rel-read data.d64 SCORES 3
disk45 rel-list data.d64 SCORES
```

### Subdirectory Operations

CBM subdirectories (file type `CBM`) work on all sector-based images (D64, D71, D81, D65, ...). Paths use `/`.

#### mkdir

```
disk45 mkdir <image> <dirname>
disk45 mkdir <image> PARENT/NEWDIR       # nested
```

Create a subdirectory. The parent directory chain is extended by a sector when it is full.

#### rmdir

```
disk45 rmdir <image> <dirname>
```

Remove an empty subdirectory (`PARENT/DIR` for nested ones), free its sectors, and trim empty trailing sectors of the parent directory.

#### tree

```
disk45 tree <image>
```

Display the directory tree recursively.

```
MY GAME/
  ASSETS/
    SPRITES/
  GAME (PRG, 12 blocks)
  README (SEQ, 1 blocks)
```

#### dir-shrink

```
disk45 dir-shrink <image> [path]
```

Free empty trailing sectors of a directory chain (root, or the subdirectory `path`).

#### dir-ensure

```
disk45 dir-ensure <image> <min_free_slots> [path]
```

Make sure a directory has at least N free entry slots, adding sectors (8 slots each) as needed.

### FUSE Filesystem Mount

```
disk45 mount <image> <mountpoint> [-ro|--readonly] [fuse options...]
```

Mount an image as a Linux directory using FUSE3. Only available when disk45 was built with libfuse3 (the Makefile detects it with `pkg-config fuse3`; the command then appears in the usage text). disk45 stays in the foreground until unmounted; extra arguments are passed to FUSE.

- Only the root directory is shown. Files appear with type extensions (`GAME.prg`, `README.seq`); subdirectory entries appear as directories but cannot be listed.
- Read/write: create, write, truncate, rename and delete files; the image is saved after every change. A new file's type comes from its extension (`.prg`, `.seq`, `.usr`, `.rel`; others become PRG); names are limited to 16 characters.
- Images with no tracks (TAP, T64, ARK, ARC/SDA, LNX, CVT, P00) are always mounted read-only.
- `df` reports image blocks.

```bash
mkdir /tmp/cbm
disk45 mount game.d81 /tmp/cbm &
ls /tmp/cbm/                    # GAME.prg  README.seq
cp newfile.prg /tmp/cbm/        # add file to image
rm /tmp/cbm/OLD.prg             # remove file
fusermount -u /tmp/cbm          # unmount
```

### Catalog (Collection Indexing)

SQLite index of image collections with a CRC32 of each file's contents. disk45 is always linked with SQLite. Every subcommand accepts `--db <file>`; the default database is `~/.disk45-catalog.db`. Running `disk45 catalog` alone prints the subcommand list.

Indexed formats: `.d64 .d71 .d81 .d65 .d80 .d82 .ark .arc .sda .lnx .tap .t64 .g64 .g71 .cvt` (also with `.gz`).

#### catalog build / update

```
disk45 catalog build <dir|image> [more...] [--db file]
```

Scan directories recursively (or index single image files). Incremental: images whose modification time is unchanged are skipped; changed ones are re-indexed. `update` is a synonym.

#### catalog search

```
disk45 catalog search <pattern> [--db file]
```

Search file names (case-insensitive, `*` and `?` wildcards). Shows name, type, blocks, CRC32 and image path.

```
  LEVEL1            PRG     12 blk  99466F4E  coll/c.d64
  LEVEL1            PRG     12 blk  99466F4E  coll/g.d81
2 result(s)
```

#### catalog list

```
disk45 catalog list [--db file]
```

One line per indexed image: path, format, disk name, file count.

#### catalog duplicates

```
disk45 catalog duplicates [--db file]
```

Group files with identical CRC32 (empty files are ignored).

#### catalog stats

```
disk45 catalog stats [--db file]
```

Image and file counts with format and file-type breakdowns.

#### catalog prune

```
disk45 catalog prune [--db file]
```

Remove entries for images that no longer exist.

### Encoding Filters

```
disk45 -a2p    # or --ascii-to-petscii: ASCII → PETSCII (stdin → stdout)
disk45 -p2a    # or --petscii-to-ascii: PETSCII → ASCII (stdin → stdout)
```

These use the same conversion as `-p` (see [PETSCII Content](#petscii-content--p)).

```bash
disk45 -p2a < commodore_file > text.txt
echo "HELLO" | disk45 -a2p > hello.seq
```

### Batch Scripting

```
disk45 batch <scriptfile>        # or: disk45 --batch <scriptfile>
```

Run one disk45 command per line (without the `disk45` prefix). Blank lines and lines starting with `#` or `;` are ignored. Arguments are split on spaces; double quotes group words and are removed. A failing line is reported (`(script line N failed)`) and execution continues; the exit status is 1 if any line failed. `batch` cannot be nested.

```bash
# build_disk.script
create game.d81 -n "MY GAME" -i "MG"
add game.d81 build/main.prg GAME
add game.d81 assets/sprites.bin SPRITES
mkdir game.d81 ASSETS
validate game.d81
```

```bash
disk45 batch build_disk.script
```

## Common Use Cases

### Game Disk

```bash
disk45 create game.d81 -n "MY GAME" -i "MG"
cc45 -O2 src/main.c -o build/main.prg
disk45 add game.d81 build/main.prg GAME
disk45 add game.d81 assets/sprites.bin SPRITES
disk45 add game.d81 assets/music.bin MUSIC
disk45 validate game.d81
disk45 create backup.d81 && disk45 copy game.d81 "*" backup.d81
```

### Archival and Recovery

```bash
disk45 info old_disk.d64
disk45 validate old_disk.d64
disk45 convert old_disk.d64 archive.d81
disk45 extract-all archive.d81 recovered/
disk45 catalog build ./disk_collection/ --db games.db
disk45 catalog duplicates --db games.db
```

### Makefile Integration

```makefile
game.d81: build/main.prg build/sprites.bin
	disk45 create $@ -n "MY GAME" -i "MG"
	disk45 add $@ build/main.prg GAME
	disk45 add $@ build/sprites.bin SPRITES
	disk45 validate $@
```

## Supported Formats

R/W means `create`, `add`, `remove` etc. work and the image is written back in its own format. R means the format can be read (list, info, extract, copy/convert from).

### Commodore Floppy Disks

| Extension | Format | R/W | Size | Notes |
|-----------|--------|-----|------|-------|
| `.d64` | 1541 | R/W | 170 KB, 35 tracks | C64 standard. `-t` pads to 36-40 tracks. |
| `.d71` | 1571 | R/W | 340 KB, 70 tracks | C128 double-sided. |
| `.d81` | 1581 | R/W | 800 KB, 80 tracks | C65/MEGA65. **Recommended.** |
| `.d65` | MEGA65 | R/W | 1.6 MB, 162 tracks | MEGA65 native extended. |
| `.d80` | 8050 | R/W | 533 KB, 77 tracks | CBM PET 8050. |
| `.d82` | 8250 | R/W | 1 MB, 154 tracks | CBM PET 8250. Free-block count and allocation cover only the first 77 tracks. |
| `.x64` | Extended D64 | R/W | D64 + 64-byte header | |

### Hard Drives & CMD Devices

| Extension | Format | R/W | Size | Notes |
|-----------|--------|-----|------|-------|
| `.d90` | D9090 | R | 7.5 MB | CBM hard drive (6 heads). |
| `.d60` | D9060 | R | 5 MB | CBM hard drive (4 heads). |
| `.d1m` | FD-2000 DD | R | 207 KB | CMD FD-2000 double density. |
| `.d2m` | FD-2000 HD | R | 414 KB | CMD FD-2000 high density. |
| `.d4m` | FD-4000 ED | R | 829 KB | CMD FD-4000 extra density. |
| `.dnp` | CMD Native | R | ≤ 16 MB | CMD Native Partition. |

`create` on these extensions produces a blank formatted image, but files added to it are not read back correctly; treat them as read-only.

### Raw/GCR Formats (read-only)

| Extension | Format | Notes |
|-----------|--------|-------|
| `.g64` | GCR-1541 | Decoded to D64 sectors on load. |
| `.g71` | GCR-1571 | Decoded to D71 sectors on load. |
| `.nib`, `.nbz` | Nibble dump | Decoded to D64 sectors on load. |
| `1!name` | Zipcode | 4-file RLE pack; give the `1!` file, `2!`-`4!` are found automatically. |

There is no encoder: `create` writes plain D64 data that cannot be loaded back, and a modifying command (`add`, `remove`, ...) overwrites the file with plain D64 data, destroying the original encoding. Convert to `.d64`/`.d71` before changing anything.

### Tape Formats

| Extension | Format | R/W | Notes |
|-----------|--------|-----|-------|
| `.tap` | TAP | R | Raw tape pulses, decoded with CBM ROM loader rules. |
| `.t64` | T64 | R/W | Tape container. |

### Archives & Containers

| Extension | Format | R/W | Notes |
|-----------|--------|-----|-------|
| `.ark` | Arkive | R/W | Uncompressed archive. |
| `.arc` / `.sda` | ARC | R/W | Reads stored/RLE/Huffman/LZW members. |
| `.lnx` | Lynx | R/W | Block-aligned archive. |
| `.cvt` | GEOS Convert | R | `info`, `list`, `extract`. |
| `.p00` | PC64 | R/W | Single-file container; also `.s00`/`.u00`/`.r00` and `.p01`-`.p99` etc. |

### Compression

| Extension | Description |
|-----------|-------------|
| `.gz` | Transparent gzip on any format (e.g. `.d81.gz`). |

## Filename Handling

- CBM names are matched case-insensitively and stored upper-case, at most 16 characters.
- `rename`, `splat`/`unsplat`, `chain`, `mkdir`/`rmdir`, `dir-shrink`/`dir-ensure` and the `rel-*` commands also accept `@"NAME"` (explicit ASCII, same as plain) and `p"NAME"` (raw PETSCII, no case conversion).
- Wildcards (`*`, `?`) are accepted by `list`, `extract-all`, `copy` and `catalog search`.

### PETSCII Content (-p)

| Direction | Conversion |
|-----------|------------|
| ASCII→PETSCII (`add -p`, `-a2p`) | `a-z`→`$41-$5A`, `A-Z`→`$C1-$DA`, LF→CR, CR dropped, `~`→`$A8`, `\|`→`$7D`, `\`→`$5C` |
| PETSCII→ASCII (`extract -p`, `extract-all -p`, `-p2a`) | `$41-$5A`→`a-z`, `$C1-$DA`→`A-Z`, CR→LF, `$A8`→`~`, a `$00` byte ends the output |

## Docker Usage

The repository `Dockerfile` builds an image containing all tools (with FUSE and SQLite):

```bash
docker build -t cc45 .
docker run --rm -v $(pwd):/work cc45 disk45 list /work/game.d81
docker run --rm -v $(pwd):/work -v catalog:/catalog cc45 \
  disk45 catalog build /work --db /catalog/disk45.db
```

## Library API

The image classes live in `lib45-tools.a` (headers in `include/`):

```cpp
#include "DiskImage.hpp"

auto img = DiskImage::create(DiskFormat::D81);   // D64, D71, D81, D65
img->format("MY DISK", "MD");
img->addFile("GAME", CbmFileType::PRG, data);
img->saveToFile("game.d81.gz");

auto disk = DiskImage::load("game.d81.gz");      // any format, by extension
auto files = disk->listFiles();
auto prg = disk->readFile("GAME");
```

`DiskImage::createFromExtension(path)` returns an empty image of the class matching an extension.

| Class | Header | Formats |
|-------|--------|---------|
| `D64Image` | `D64Image.hpp` | D64 |
| `D71Image` | `D71Image.hpp` | D71 |
| `D81Image` | `D81Image.hpp` | D81 |
| `D65Image` | `D65Image.hpp` | D65 |
| `D80Image`, `D82Image` | `D80Image.hpp` | D80, D82 |
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
| `create` | — | Create image (`-n`, `-i`, `-t`) |
| `list` | `ls`, `dir` | List directory (wildcards, subdirs) |
| `info` | — | Show disk/tape/GEOS metadata |
| `add` | `write` | Add file (`-p` for SEQ) |
| `extract` | `read` | Extract file (`-p`) |
| `extract-all` | `extractall` | Extract all files to a directory (`-p`) |
| `copy` | `cp` | Copy files between existing images |
| `convert` | — | Convert between formats |
| `remove` | `delete`, `rm` | Delete file |
| `rename` | `mv` | Rename file or directory |
| `lock` / `unlock` | — | Set/clear locked flag |
| `splat` / `unsplat` | — | Mark file unclosed/closed |
| `label` | — | Change disk name/ID |
| `validate` | `check` | Check BAM consistency |
| `bam` | `map` | Visual sector map |
| `dump` | `hex` | Hex dump sector(s) |
| `bread` / `bwrite` | — | Read/write raw sector |
| `bpeek` / `bpoke` | — | Read/write byte(s) in a sector |
| `bfill` | — | Fill sector |
| `chain` | — | Show file sector chain |
| `rel-create` / `rel-read` / `rel-write` / `rel-list` | — | REL files (D64/D71) |
| `mkdir` / `rmdir` | — | Create/remove subdirectory |
| `tree` | — | Directory tree |
| `dir-shrink` | — | Reclaim empty directory sectors |
| `dir-ensure` | — | Ensure min free directory slots |
| `mount` | — | FUSE mount (when built with libfuse3) |
| `catalog` | — | Collection index: `build`/`update`, `search`, `list`, `duplicates`, `stats`, `prune` |
| `batch` | `--batch` | Run commands from a file |
| `-a2p` | `--ascii-to-petscii` | ASCII→PETSCII filter |
| `-p2a` | `--petscii-to-ascii` | PETSCII→ASCII filter |
| `--version` | `-v` | Print version |

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
