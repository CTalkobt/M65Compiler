# pack45 — Packer for unpack()

`pack45` packs a file into a stream that the library's `unpack()` ([`<unpack.h>`](../architecture/stdlib.md#packed-data-unpackh)) unpacks on the MEGA65, to any 28-bit address (chip RAM, attic RAM). The stream can be loaded inside the area it unpacks into and unpacked there ("in place"). `ln45 --pack-overlays` packs the overlay files with the same format ([ln45](ln45.md#packed-overlay-files)).

## Usage

```bash
pack45 [options] <input> <output>
pack45 -x <packed> <output>
pack45 --suggest-rate [--keep PCT] [--max-rate HZ] <sound> ...
pack45 --sound [--rate HZ] [--unsigned] [--normalise] [options] <sound> <output>
pack45 --image [--width W] [--palette FILE] [--palette-out FILE] [--predict above|left] [options] <image> <output>
pack45 -V | --version
pack45 -? | -h | --help
```

| Option | Description |
|--------|-------------|
| `-d`, `--dest ADDR` | Where it unpacks to: a 28-bit address (`$40000`, `0x8000000`, decimal). A `.prg` input's load address is the default |
| `--load ADDR` | The load address written in front of the stream. Default: the destination plus the in-place offset, so the file can be loaded and unpacked in place |
| `--raw` | No load address: the stream alone (to include in a program) |
| `--best` | The shortest stream: an optimal parse instead of the greedy one. A few per cent smaller, about five times slower; the same format, for the same `unpack()` |
| `--prg` | The input starts with a two-byte load address (also when its name ends in `.prg`); it is left out |
| `-x` | Unpack `<packed>` (with its load address, or `--raw`) to `<output>`: a check on the host |
| `--suggest-rate` | For each sound: the lowest sample rate that keeps it (below) |
| `--sound` | Convert `<sound>` to 8-bit samples for the audio DMA, at `--rate` or the rate `--suggest-rate` names. With `-d` they are packed like any file; without, the samples alone are written (`--load ADDR` puts that load address in front) |
| `--rate HZ` | With `--sound`: the samples' rate, 1000-192000 |
| `--unsigned` | With `--sound`: unsigned samples (offset by 128) instead of signed |
| `--normalise` (or `--normalize`) | With `--sound`: the loudest sample to full scale |
| `--keep PCT` | With `--suggest-rate`: the share of the energy to keep, per cent (default 99.9) |
| `--max-rate HZ` | With `--suggest-rate`: the highest rate it will be played at. Only the energy that rate can hold counts, and nothing above it is suggested |
| `--image` | `<image>`, in up to 16 colours, as a [format 2](#format-2) stream: each pixel coded against the one above or the one to its left (both are tried, the smaller kept), unpacking to 4-bit pixels. A PNG (or BMP, GIF, ... : what font45 reads, by stb_image), or raw 4-bit pixels with `--width` |
| `--width W` | With `--image`: the input is raw 4-bit pixels, two a byte (the left one the low nibble), `W` (even, 2-256) a row; the rows follow each other |
| `--palette FILE` | With `--image`: the palette, `R`, `G`, `B` bytes an entry from entry 0. An image's colours are found among its first 16 entries (one that is not is an error), and the order of the colours is dark to light by it |
| `--palette-out FILE` | With `--image`, an image without `--palette`: its colours, numbered 0 up dark to light, written as `R`, `G`, `B` bytes (they are also printed) |
| `--predict above\|left` | With `--image`: that prediction, not the smaller of the two |
| `-v` … `-vvvv` | Log on stderr as `[pack45:LEVEL] …` |

It prints the sizes, where the stream unpacks to and the in-place load address:

```
pack45: tiles.bin: 30000 -> 9812 bytes (32%), unpacks to $40000; in place: load it at $4502C (offset 20524)
```

Every stream is unpacked again by a model of the decoder (in place too) before it is written; a mismatch is an internal error (exit status 2). Other errors (an unknown option, a file that cannot be read, no destination: `-d` or a `.prg` input) give exit status 1. `--load` writes the low 16 bits of its address (a load address is two bytes); pack45 warns when a `--load` address above `$FFFF` lies in the area it unpacks to but not far enough in to unpack in place.

## Sounds: --suggest-rate, --sound

Sampled sound does not pack: LZ finds no repeats in it, and a coder (8-bit PCM, ADPCM) spends the same bytes on every sample however smooth the wave. What makes a sound smaller is fewer samples, a lower rate, and a rate keeps only what lies below half of it. `--suggest-rate` measures where a sound's energy lies (its spectrum) and names the lowest of the usual rates (4000, 5000, 6000, 8000, 10000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 Hz) whose pass band, 0.45 of the rate (what a practical low-pass filter keeps), holds `--keep` per cent of it:

```
$ pack45 --suggest-rate --max-rate 12000 dragon-idle.wav chest-open.wav
pack45: dragon-idle.wav: 44100 Hz, 16-bit, 2 channels, 1.48 s; 99.9% of its energy below 3106 Hz: 8000 Hz keeps it (11840 bytes at 8 bits, 18% of 44100 Hz); audio DMA rate $000CF2
pack45: chest-open.wav: 44100 Hz, 16-bit, 2 channels, 1.76 s; 99.9% of its energy below 5340 Hz: 12000 Hz keeps it (21120 bytes at 8 bits, 27% of 44100 Hz); audio DMA rate $00136B
```

A growl, a bell or a thud keeps nearly all of itself at 4000-8000 Hz; a hit, a swing or a splash of water needs the top. Give `--max-rate` the rate the program plays at: a recording's faint hiss far above it otherwise counts as sound to keep. The audio DMA rate is the value for the MEGA65's audio DMA channel's frequency registers (Hz × 2²⁴ / 40.5 MHz). `--sound` makes the samples, at the suggested rate unless `--rate` says otherwise: the sound mixed to one channel, low-passed at 0.45 of the new rate (a 255-tap windowed sinc) and resampled, then 8-bit:

```
$ pack45 --sound --max-rate 12000 dragon-idle.wav growl.raw
pack45: dragon-idle.wav: 44100 Hz, 2 channels, 1.48 s -> 8000 Hz (suggested), 11840 signed 8-bit samples; audio DMA rate $000CF2
```

Packing them (`-d`) gains little (silence and fades, as fills), but loads and unpacks like any other file.

| Format | Read |
|--------|------|
| WAV | PCM of 8, 16, 24 or 32 bits, 32-bit float |
| AU (`.au`, `.snd`) | mu-law, A-law, PCM of 8, 16, 24 or 32 bits, 32 or 64-bit float |
| MP3 | by [minimp3](../../third_party/minimp3/PROVENANCE.md) (CC0), built in |
| MP4, M4A and anything else | by `ffmpeg`, when it is on the PATH (otherwise an error says so) |

The format is told by the file's first bytes (an MP3 also by its name), any number of channels mixed to one.

## On the MEGA65

`<unpack.h>` has two functions, both assembly (`lib/stdlib_asm/unpack.s45`, the dispatch on the stream's format), linked only when called:

| Function | Unpacks |
|----------|---------|
| `unsigned long unpack(unsigned long src)` | the stream at the 28-bit address `src` to the destination in its header |
| `unsigned long unpack_to(unsigned long src, unsigned long dst)` | the same to `dst`, the header's destination ignored |

Both return the number of bytes written, 0 for a stream of another format (nothing is written). Literals up to 16 bytes are copied by the CPU; longer ones, fills and repeats are each one DMA job.

**The decoders are chosen** (#666): each format's decoder is a library member of its own, linked only when the program names it, at file scope in any one of its files: `UNPACK_FORMAT1;` (`lib/stdlib_asm/unpack1.s45`, 315 bytes) and `UNPACK_FORMAT2;` (`unpack2.s45`, [below](#format-2); `UNPACK_IMAGES;` is the same). `unpack()` returns 0 for a stream whose decoder is not linked (the weak defaults in `unpack.s45`). A program that names none gets format 1, as before: `<unpack.h>` (and `<load.h>`, which includes it) names it itself, unless the program defines `UNPACK_EXPLICIT_FORMATS` first, in every file that includes them (or `cc45 -DUNPACK_EXPLICIT_FORMATS`). A program of format 2 streams alone then links no format 1 decoder (315 bytes of code and 5 of BSS fewer):

```c
#define UNPACK_EXPLICIT_FORMATS
#include <unpack.h>

UNPACK_FORMAT2;             /* format 2 only: a format 1 stream gives 0 */
```

The library's own users take what they need: `font_load` (fonts are packed in format 1) and the overlay loader of `ln45 --pack-overlays` (`ovl_unpack.s45`) link format 1 themselves; `load_unpack` unpacks whatever formats the program linked.

### A file on the disk, unpacked in place

```bash
pack45 -d 0x40000 tiles.bin TILES       # prints: in place: load it at $4502C
```

```c
#include <cbm.h>
#include <unpack.h>

cbm_load_far("TILES", 5, 8, 0x4502CUL);   /* the in-place address pack45 printed */
unpack(0x4502CUL);                         /* to $40000, the destination in the stream */
```

`cbm_load_far` skips the two-byte load address. The packed file takes no memory of its own: it is loaded inside the area it unpacks into. A stream for attic RAM (`-d 0x8000000`) is loaded into chip RAM and unpacked from there.

### Data inside the program

`--raw` writes the stream alone; ca45's `.incbin` puts it in the program:

```bash
pack45 -d 0x40000 --raw level1.bin level1.pk
```

```asm
; levels.s45
.global _level1_packed
.segment "data"
_level1_packed:
    .incbin "level1.pk"         ; (relative to the current directory)
```

```c
#include <unpack.h>
extern const unsigned char level1_packed[];

unpack((unsigned long)(unsigned int)level1_packed);           /* to $40000 */
unpack_to((unsigned long)(unsigned int)level1_packed, 0x50000UL);  /* or anywhere */
```

```bash
ca45 -c levels.s45 -o levels.o45
cc45 -O2 -c game.c -o game.o45
ln45 -basic -o game.prg crt0_zp.o45 game.o45 levels.o45 c45.lib    # (the library's directory: lib/build, or lib/cc45 installed)
```

### From assembly

Assembly names the decoders it needs itself, `.extern ___unpack_format1` / `___unpack_format2` (without one `___unpack` returns 0 for that format). The routines use the ZP convention in either library: the stream's address in `$10`-`$13` (for `___unpack_to` the destination in `$14`-`$17`), then `jsr ___unpack` / `jsr ___unpack_to`. The count comes back in Q (A, X, Y, Z: bits 0-7 … 24-31); `$08`-`$19` and the registers are changed. These are base-page addresses: in a cc45 program, the program's own base page.

```asm
.extern ___unpack_to
.extern ___unpack_format1   ; (format 1's decoder: in C <unpack.h> names it)

proc _unpack_level1
    lda #<_level1_packed
    sta $10
    lda #>_level1_packed
    sta $11
    lda #0
    sta $12
    sta $13
    sta $14                 ; to $50000
    sta $15
    sta $17
    lda #5
    sta $16
    jmp ___unpack_to        ; the count in Q
    .func_flags zp_call     ; (callable from C: __fastcall__ unsigned long unpack_level1(void))
endproc
```

Linked as above, with the library.

### Overlays

`ln45 --pack-overlays` packs the overlay files with the same format and the start-up code unpacks them ([ln45](ln45.md#packed-overlay-files)); nm45 and objdump45 read packed overlay files.

## Format 1

```
1 byte    the format (1)
4 bytes   where it unpacks to: a 28-bit address, low byte first
tokens, n = t & $3F:
  $00-$3F   n + 1 bytes follow as they are             (a DMA copy)
  $40-$7F   n + 3 bytes of the one byte that follows   (a DMA fill)
  $80-$BF   n + 3 bytes repeat from o + 1 back (1 byte o follows)
  $C0-$FF   n + 3 bytes repeat from o back (2 bytes o follow, o >= 1)
  $C0 $00 $00  the end
```

A repeat is never closer than it is long, so one forward DMA copy does it. The tokens are bytes, so the decoder is small (about 400 bytes) and most of the work is DMA. The packer is greedy with hash chains (256 candidates a position) and looks one byte ahead. With `--best` it parses from the end instead: at each position the cheapest way on to the end, over a fill, a repeat of any length up to the longest near (1-byte offset) and far (2-byte offset) one found, or literals. On a game's art (760 KB of character sets and screens) that is 2.7% smaller than the greedy stream, in about five times the time. The format byte keeps the door open to denser formats later; `unpack()` returns 0 for a stream of another format.

**In place:** while unpacking, the bytes written must stay behind the bytes still to be read. The packer computes the least offset from the destination at which the stream can start for that to hold (its largest lead of writing over reading); loaded there or further in, `unpack()` reads each literal block before overwriting it (a forward copy, downwards).

## Format 2

An image (#666; the delta coding of texture_demo, a project beside this one), `w` x `h` pixels in `N` colours:

```
1 byte    the format (2)
4 bytes   where it unpacks to: a 28-bit address, low byte first
1 byte    w / 2 (1-128: w is even, at most 256)
2 bytes   h, low byte first
1 byte    N, the colours (1-16)
1 byte    the prediction: 0 the pixel above, 1 the pixel to its left
N bytes   the order: rank r's palette entry (0-15)
N bytes   the code lengths of the symbols 0 .. N-1 (0: not used; at most 8)
the codes, row by row, MSB first; then zero bytes up to the last the decoder reads
```

A pixel's *rank* is its colour's place in the order (pack45 puts the colours dark to light, by the palette when it has one, so neighbours' ranks are close). What is coded is the rank minus its prediction, mod `N`: the rank above (row 0's "above" is 0) or the rank to its left (column 0 from above). The code is a canonical Huffman code (by length, then symbol) of at most 8 bits: the decoder finds a code by one lookup on the next 8 bits, in tables of 256 entries. pack45 makes each code with the counts halved until no code is longer. It unpacks to `h` rows of `w / 2` bytes, the left pixel the low nibble, each pixel's byte the palette entry: for the VIC-IV's nibble-colour characters (NCM) or a 16-colour bitmap, a row at a time to any 28-bit address. The order table is part of the stream, so `unpack()` takes no palette.

The decoder decodes a row into a buffer and then copies it to its place by DMA, reading the stream forwards. **In place**, a row must not reach the bytes still to be read: pack45 computes the least offset from the destination for that (the most a row's end gets ahead of the reading) and checks the stream unpacks there, as for format 1. The decoder holds up to two bytes ahead, so the stream ends with the zero bytes it reads ahead.

**On the MEGA65**, the decoder (`lib/stdlib_asm/unpack2.s45`, 581 bytes) is a member of its own, with 0.9 KB of BSS (the two lookup tables, a row of ranks, a row of bytes), linked when the program says `UNPACK_FORMAT2;` (or `UNPACK_IMAGES;`) at file scope in one of its files; `unpack()`, `unpack_to()` and `load_unpack()` then take format 2 as well as format 1 (by default), or format 2 alone with `UNPACK_EXPLICIT_FORMATS` ([above](#on-the-mega65)):

```c
#include <unpack.h>

UNPACK_FORMAT2;             /* links the format 2 decoder */

unpack(0x41234UL);          /* a format 1 or a format 2 stream */
```

Without `UNPACK_FORMAT2` they return 0 for a format 2 stream (nothing is written); pack45 `--image` prints the reminder. ln45 `--pack-overlays` writes format 1 only; nm45 and objdump45 read either in an overlay file.

```
$ pack45 --image -d 0x40000 --palette-out brick.pal brick.png BRICK
pack45: brick.png: 128x128, 16 colours, from the left: 8192 -> 5789 bytes (70%; format 1: 7639), unpacks to $40000; in place: load it at $40963 (offset 2403)
pack45: its colours, entry 0 up: #… (16 of them)
pack45: the program unpacks it with UNPACK_FORMAT2; at file scope (<unpack.h>; with UNPACK_EXPLICIT_FORMATS, UNPACK_FORMAT1; too if it unpacks format 1 streams)
```

**What it gains** is on images with texture and noise, where LZ finds few repeats; on flat art with long repeats format 1 is smaller, and pack45 prints both sizes. texture_demo's photographs (128x128, quantised to 16 colours, no dithering):

| Image | 4-bit bytes | Format 1 | Format 2 |
|-------|-------------|----------|----------|
| red brick | 8192 | 7639 | 5789 |
| cobblestones | 8192 | 8112 | 6774 |
| rocky trail | 8192 | 8318 | 8093 |
| bands of 4 pixels, 16 colours (64x40, the runtime test's) | 1280 | 132 | 363 |

Decoding takes about 5 frames for a 128x128 image (a tenth of a second; format 1 about 0.6 frames, its work mostly DMA): a code a pixel, by table, and a DMA copy a row.

Source: `include/Pack45.hpp`, `src/main/Pack45.cpp` (also used by ln45 and nm45/objdump45), `src/main/pack45_main.cpp`, `include/PackAudio.hpp` and `src/main/PackAudio.cpp` (`--suggest-rate`, `--sound`; minimp3 in `third_party/minimp3`); the dispatch `lib/stdlib_asm/unpack.s45` and the decoders `unpack1.s45` and `unpack2.s45`. Tests: `test_pack45_image.sh` (format 2: round trips through the model and an independent decoder, PNG with and without a palette, the errors), `test_compiler.sh` (round trips; `--suggest-rate` and `--sound` on the WAV, AU and MP3 files in `src/test-resources/pack45/`), `src/test-resources/test_unpack_xemu.c` (chip RAM, in place, attic RAM, `unpack_to`, another format), `test_unpack2_xemu.c` (format 2: the same, and both formats in one program), `test_unpack2only_xemu.c` (format 2 alone: a format 1 stream gives 0, no format 1 decoder in the map).
