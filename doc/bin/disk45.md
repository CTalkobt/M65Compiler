# disk45 — CBM Disk Image Utility

## Synopsis

```
disk45 <command> [arguments] [options]
disk45 batch <scriptfile>
disk45 sd <command> <card.img> [arguments]
disk45 -a2p | -p2a
```

## Description

**disk45** creates, reads and manipulates Commodore disk images, tape images and archive files. It handles 25 container formats spanning C64, C128, PET, MEGA65 and CMD devices (read/write support varies, see [Supported Formats](#supported-formats)). It also offers FUSE mounting, an SQLite catalog of image collections, format conversion, batch scripts and PETSCII conversion.

The format is chosen from the file extension; an unrecognised extension is treated as D81. GZ compression is transparent: append `.gz` to any format (e.g. `.d81.gz`) and the image is decompressed on load and compressed on save.

`disk45 sd` works on MEGA65 SD card images (FAT32 with the MEGA65 system partition, see [MEGA65 SD Card Images](#mega65-sd-card-images)).

Running `disk45` with no arguments or an unknown command prints the usage summary (exit status 1). `disk45 --version` (or `-v`) prints the version.

## Commands

### File Operations

#### create

```
disk45 create <image> [-n name] [-i id] [-t tracks] [--geos]
```

Create a new, empty image or archive. An existing file is overwritten.

| Option | Description |
|--------|-------------|
| `-n name` | Disk name (default `CC45 DISK` for disk images; archives use a fixed name such as `ARK ARCHIVE`) |
| `-i id` | 2-character disk ID (default `CC`) |
| `--geos` | Format for GEOS (D64/D71): `GEOS format V1.0` at `$AD` of the BAM sector and a border sector (searched from 19/0 upwards, then from 1/0), as [GEOS.TXT](https://ist.uwaterloo.ca/~schepers/formats/GEOS.TXT) describes. |
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

A **GEOS Convert file** (a `.cvt`, recognised by its signature whatever its extension) is added as a GEOS file: its info block, the record block and the record chains of a VLIR file, or the data chain of a sequential one, with the directory entry from the CVT (GEOS type, structure, date). The name is the one in the CVT unless `cbm_name` is given. See [GEOS Files](#geos-files).

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

Extract a file (name lookup is case-insensitive). `-p` converts PETSCII→ASCII (applied whatever the file type). The host file name is required; there is no stdout mode. A GEOS file is written as its Convert (CVT) form, the only form that keeps its info block and VLIR records; `extract-all` names GEOS files `<name>.cvt`.

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

If `source` is a single file (`.prg`, `.seq`, `.usr` or `.cvt`), a blank target named `CONVERTED` (ID `CV`) is created and the file is added under its upper-cased base name (a `.cvt` file becomes a GEOS file on a disk image target, and is stored as-is, as a PRG, elsewhere). Between disk images, GEOS files are rebuilt as GEOS files and a GEOS disk's signature is kept (D64/D71 targets).

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

Show metadata. TAP images show version, platform, data size and file count; GEOS CVT files show GEOS name, type, structure (VLIR/sequential, record count), class, author and description; disk images formatted for GEOS also show the `GEOS format` signature; other images show format, name, ID, tracks, total/free sectors, image size and file count.

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

REL files work on the sector-based images (D64, D71, D81, D65, D80, D82), laid out as CBM DOS does: the records packed one after another across the data sectors (254 data bytes each; a record can span two), an empty record being `$FF` and `$00`s, side sectors listing the data sectors (6 per group, 120 data sectors each), and on D81/D65 the 1581's super side sector in front of the groups. `rel-create` on a D81 gives the same sectors as BASIC 65's DOS creating the file, and BASIC 65 reads, writes and extends a REL file disk45 made (`test_disk45_rel_xemu.bas` on xemu). The D64/D71 layout follows the 1541/1571 format; D80/D82 use it too (the REL layout is not checked against an 8050/8250).

#### rel-create

```
disk45 rel-create <image> <name> <record_len> [num_records]
```

Create a REL file with fixed-length records (1-254 bytes, default 10 records). As DOS does, the file fills its last data sector, so it can hold more records than asked (`rel-create x 20 30` makes 38); all of them are empty. Without a super side sector (D64, D71, D80, D82) a REL file holds at most 720 data sectors.

#### rel-read

```
disk45 rel-read <image> <name> <record_num>
```

Print a record (1-based) as hex and as text.

#### rel-write

```
disk45 rel-write <image> <name> <record_num> <data...>
```

Write a record. Each data argument is a number (decimal, `0x` hex) or a string in literal double quotes (pass `'"HELLO"'` from the shell). Data is zero-padded to the record length; more data than the record holds is an error, as is a record past the end (DOS extends a REL file when a program writes past it; `rel-write` does not).

#### rel-list

```
disk45 rel-list <image> <name>
```

Show the record count and the records that are not empty.

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

### MEGA65 SD Card Images

```
disk45 sd create <card.img> <size> [--label NAME] [--no-syspart] [--syspart SIZE]
                                   [--onboarded] [config options] [-f]
disk45 sd info <card.img>
disk45 sd list <card.img> [dir]
disk45 sd add <card.img> <file> [card-path] [-f]
disk45 sd extract <card.img> <card-path> [file]
disk45 sd rm <card.img> <card-path>
disk45 sd mkdir <card.img> <dir>
disk45 sd mv <card.img> <from> <to>
disk45 sd check <card.img>
disk45 sd config <card.img> [config options]
```

An SD card image is what xemu uses (`~/.local/share/xemu-lgb/mega65/mega65.img`, or `-sdimg`) and what `dd` reads from or writes to a real MEGA65 card: an MBR partition table, a FAT32 data partition (type `$0B`/`$0C`) and the MEGA65 system partition (type `$41`). The image is read and written sector by sector, so multi-gigabyte cards are fine. `sd create` makes a sparse file.

**`sd create`** lays the card out as the MEGA65's FDISK utility does (mega65-fdisk; xemu's `sdcontent.c` copies the same algorithm). Apart from card sector 1 (the configuration, which xemu writes during onboarding), xemu's signature block in sector 2 and the backup FS information sector (which xemu's own erase step overwrites), the first 4 MB of a 4 GB disk45 card are identical to xemu's card:

- **MBR**: disk signature `$A6CB7D83`. Partition 1 is FAT32 (`$0C`, LBA) from sector 2048. Partition 2 is the system partition (`$41`) at the end of the card. FDISK makes it half the card, at most 2 GB, in whole MB; `--syspart SIZE` sets the size and `--no-syspart` leaves it out.
- **FAT32**: FDISK's boot sector ("MEGA65r1", a "MEGA65 HYPPOBOOT" message for PCs), 568 reserved sectors, 2 FATs, 4 KB clusters (as many as fit beside their FATs), the root directory in cluster 2, FS information sector 1 and the backup boot sector 6 (+7). The volume label in the root directory is FDISK's `M.E.G.A.65!`; `--label` changes it. fsck.vfat rejects `.` in labels, and disk45 warns about such characters.
- **System partition**: the `MEGA65SYS00` header with the freeze and service slot areas (512 KB slots, sized as FDISK does), then 1 MB reserved and the slot directories.
- **Configuration sector**: written to **card sector 1**, where current hypervisors read it. They moved it out of the system partition so PAL/NTSC can be chosen without one. A copy goes to the system partition's sector 1, which older hypervisors read. FDISK's values: version 1.1, NTSC, audio amplifier on, mono, Amiga mouse as 1351, MAC `41:41:41:41:41:41`, default disk `mega65.d81`, DMAgic F018B. Onboarding is pending, so the MEGA65 (and xemu) run the onboarding utility on the first boot.
- **`--onboarded`** writes what that utility sets: onboarding done (`$0E` bit 7), long file names on, SID 8580, DMAgic F018B, a random locally administered MAC. Use it for cards that should boot straight to BASIC, for example under xemu.

Cards below 512 MB have fewer than 65525 clusters (FDISK formats them like that too). Tools such as fsck.vfat warn about it, and disk45 prints a note.

**Files.** `sd add` copies a host file onto the card. The default name is the file's own; a card path ending in `/` or naming a directory puts the file inside it; `-f` replaces an existing file. Every file is stored in **one contiguous run of clusters**, because the MEGA65 mounts only unfragmented D81 images. Names that are not valid 8.3 names get a long file name (VFAT) and a `NAME~N.EXT` short name. All-lower-case 8.3 names keep their case through the NT case flags, as Linux does. Names compare case-insensitively against both forms. Paths use `/`. `sd rm` removes files and empty directories. `sd mv` renames and moves (a moved directory's `..` is updated). Changes write both FATs and update the FS information sector's free count.

**`sd check`** walks the directory tree and reports:
- errors: cross-linked clusters, chains that run into free or bad clusters, chain lengths that don't match file sizes, lost clusters, FAT copies that differ
- warnings: fragmented files (for D81/D64/D71/D65 also "the MEGA65 cannot mount it"), names longer than the 64 characters the MEGA65 handles, a stale free count

**`sd info`** shows the partition table, the FAT32 volume (label, cluster size, free space, FAT size), the system partition's slots and the configuration.

**`sd config`** shows or changes the configuration sector:

| Option | Byte | Meaning |
|--------|------|---------|
| `--pal` / `--ntsc` | `$02` bit 7 | video standard |
| `--hdmi-audio on\|off` | `$0D` bit 1 | HDMI with digital audio, or pure DVI |
| `--audio-amp on\|off`, `--swap-stereo`, `--mono`, `--dac pdm\|pwm` | `$03` bits 0, 5, 6, 7 | audio |
| `--floppy sd\|drive` | `$04` bit 0 | F011 uses SD card disk images or the 3.5" drive |
| `--amiga-mouse on\|off` | `$05` bit 0 | Amiga mice as 1351 |
| `--mac XX:XX:XX:XX:XX:XX\|random` | `$06`-`$0B` | Ethernet MAC |
| `--super-sfx on\|off` | `$0C` | Super SFX cartridge emulation |
| `--onboarding done\|pending`, `--onboarded` | `$0E` bit 7 | whether onboarding has run |
| `--lfn on\|off` | `$0F` bit 7 | long file names in the hypervisor |
| `--default-disk NAME` | `$10`-`$1F` | disk image mounted at boot |
| `--dmagic old\|new` | `$20` | DMAgic F018A or F018B |
| `--crt-emulation on\|off`, `--blur on\|off` | `$21` bits 5, 3 | scanlines, horizontal blur |
| `--sid 6581\|8580` | `$22` | SID type |

The hypervisor applies the sector only if its version is 1.1. A card without one (version 0, for example a card formatted elsewhere) runs onboarding. When the card has no configuration yet, `sd config` starts from the system partition's copy, else from FDISK's values.

```bash
# a card for xemu: boots straight to BASIC with the card's ROM and D81
disk45 sd create card.img 1G --onboarded --pal
disk45 sd add card.img ~/.local/share/xemu-lgb/mega65/MEGA65.ROM
disk45 sd add card.img game.d81 MEGA65.D81
xemu-xmega65 -sdimg $PWD/card.img -defd81fromsd -romfromsd

disk45 sd list ~/.local/share/xemu-lgb/mega65/mega65.img        # xemu's own card
disk45 sd extract card.img MEGA65.D81 saved.d81 && disk45 list saved.d81
```

xemu mounts its own external default D81 instead of the card's `MEGA65.D81` unless it is given `-defd81fromsd`. `test_xemu.sh` boots a disk45 card this way (`test_disk45_sd_xemu.bas`): BASIC reads a file disk45 put on the card's D81 and writes one, which disk45 then finds. `test_disk45.sh` checks the layout bytes, the file operations and the configuration, and runs `fsck.vfat` when it is installed.

### GEOS Files

GEOS files on D64, D71, D81, D65 and D80/D82 images ([GEOS.TXT](https://ist.uwaterloo.ca/~schepers/formats/GEOS.TXT)) use directory bytes `$15`-`$1D`: the info block's T/S, the structure (`0` sequential, `1` VLIR), the GEOS file type and a time stamp. A VLIR file's first T/S is its record block: up to 127 sector chains (`00/00` ends the list, `00/FF` is an absent record).

- `list` marks them (`GEOS Application, VLIR`); names keep their mixed-case ASCII and are matched case-insensitively.
- `extract` writes the [CVT](https://ist.uwaterloo.ca/~schepers/formats/CVT.TXT) form, `add` takes one; `copy`/`convert` move GEOS files between disk images as GEOS files. The CVT layout is the one cbmconvert reads and writes: the directory entry and `PRG formatted GEOS file V1.0`, the info block, for VLIR the record block (block count and last-byte index per record) and the records padded to 254-byte blocks. An empty record takes one block, as cbmconvert's writer places it.
- `remove` and `validate` count a GEOS file's info block, record block and records (and a REL file's side sectors), and `validate` the border sector of a GEOS disk.
- `create --geos` formats a D64/D71 for GEOS; `info` shows the signature.

`test_disk45.sh` builds CVT files with `src/test/mkcvt.py` and checks that they survive add/extract, copy to a D81 and convert to a D71 byte for byte. The FUSE mount shows GEOS files but reads them as plain chains.

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
| `.d80` | 8050 | R/W | 533 KB, 77 tracks | CBM PET 8050. Header 39/0 → BAM 38/0, 38/3 → directory 39/1 ([D80-D82.TXT](https://ist.uwaterloo.ca/~schepers/formats/D80-D82.TXT)). |
| `.d82` | 8250 | R/W | 1 MB, 154 tracks | CBM PET 8250. BAM 38/0, 38/3, 38/6, 38/9 (50 tracks each); all 154 tracks are used. |
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

`create` on these extensions produces a blank formatted image. disk45 does not write their BAM correctly, so every command that changes such an image (`add`, `remove`, `rename`, `bwrite`, a FUSE mount, ...) is refused with an error and the image stays as it was.

### GCR Formats

| Extension | Format | R/W | Notes |
|-----------|--------|-----|-------|
| `.g64` | GCR-1541 | R/W | Decoded to D64 sectors; written back as GCR. |
| `.g71` | GCR-1571 | R/W | The same for the 1571 (168 track entries, side 2 = tracks 36-70). |
| `.nib`, `.nbz` | nibtools read | R | Decoded to D64 sectors; `.nbz` is the `.nib` compressed with LZ77 (Geelnard's lz.c). |
| `1!name` | Zipcode | R | 4-file RLE pack; give the `1!` file, `2!`-`4!` are found automatically. |

**Decoding** reads a track as a bit stream ([G64.TXT](https://ist.uwaterloo.ca/~schepers/formats/G64.TXT)): a sync is 10 or more 1 bits and what follows need not be byte-aligned, a G64 track wraps around (a sector may cross its end), and a sector counts only when its header ($08, checksum, sector, track, ID) and data block ($07, 256 bytes, checksum) checksums are right. A NIB holds one 8 KB read per listed half track after a 256-byte header (`MNIB-1541-RAW`, then half-track/density pairs from $10); a headerless file of 84 reads is accepted too. Half tracks and tracks past 35 (G64) are not decoded but are kept.

**Writing a G64/G71** (any change, `create`, `convert` to `.g64`/`.g71`): a track whose sectors are unchanged since loading keeps its original GCR (copy protection, extra syncs, odd gaps, its speed zone or speed block); a changed or new track is encoded as a standard 1541 track: sectors 0..n-1, 5-byte syncs, the header with the format ID found on the disk (or the BAM's disk ID), a 9-byte header gap, the data block, gaps spread evenly to MNIB's track lengths (7692/7142/6666/6250 bytes for speed zones 3-0), every track slot 7928 bytes padded with $FF. A G64 disk45 writes from a D64 is byte for byte what `src/test/mkgcr.py`, an independent encoder, writes; `test_disk45.sh` also checks that a byte in an untouched track survives adding a file.

NIB, NBZ and Zipcode are read-only: `create` refuses them and changes are refused (convert to `.d64` or `.g64`).

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
| `.cvt` | GEOS Convert | R | One GEOS file: `info`, `list` (the file, or a VLIR file's records `RECORD_n`), `extract`; `add`/`convert` put it on a disk image. |
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
| `G64Image`, `G71Image` | `G64Image.hpp` (`GcrTracks.hpp`, `Gcr.hpp`) | G64, G71 |
| `X64Image` | `X64Image.hpp` | X64 |
| `NibImage` | `NibImage.hpp` | NIB |
| `TapImage` | `TapImage.hpp` | TAP |
| `T64Image` | `T64Image.hpp` | T64 |
| `ArkImage` | `ArkImage.hpp` | ARK |
| `ArcImage` | `ArcImage.hpp` | ARC, SDA |
| `LnxImage` | `LnxImage.hpp` | LNX |
| `GeosCvtImage` | `GeosCvtImage.hpp` | CVT |
| `SdCardImage` | `SdCardImage.hpp` | MEGA65 SD card (MBR, FAT32, system partition; `disk45 sd`) |
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

- [Peter Schepers' File Format Specifications](https://ist.uwaterloo.ca/~schepers/formats.html) — the reference for the CBM disk layouts disk45 writes: [D64](https://ist.uwaterloo.ca/~schepers/formats/D64.TXT), [D71](https://ist.uwaterloo.ca/~schepers/formats/D71.TXT), [D81](https://ist.uwaterloo.ca/~schepers/formats/D81.TXT) (also the 1581 super side sector), [D80/D82](https://ist.uwaterloo.ca/~schepers/formats/D80-D82.TXT) (header → BAM 38/0, 38/3, 38/6, 38/9 → directory 39/1; `test_disk45.sh` checks these bytes), [REL files](https://ist.uwaterloo.ca/~schepers/formats/REL.TXT), [G64](https://ist.uwaterloo.ca/~schepers/formats/G64.TXT), [D2M-DNP](https://ist.uwaterloo.ca/~schepers/formats/D2M-DNP.TXT), [T64](https://ist.uwaterloo.ca/~schepers/formats/T64.TXT), [X64](https://ist.uwaterloo.ca/~schepers/formats/X64.TXT)
- MEGA65 SD card layout: mega65-core's `docs/sdcard-and-floppy-drive.md` and `docs/MEGA65_System_Partition.md` (the system partition; the configuration sector has since moved to card sector 1, see `src/hyppo/syspart.asm`), `src/utilities/onboard.c`, and [mega65-fdisk](https://github.com/MEGA65/mega65-fdisk); xemu's `targets/mega65/sdcontent.c`
- [VICE c1541 Manual](https://vice-emu.sourceforge.io/vice_14.html)
- [DirMaster](https://style64.org/dirmaster)
- [cc1541](https://manpages.ubuntu.com/manpages/noble/man1/cc1541.1.html)
