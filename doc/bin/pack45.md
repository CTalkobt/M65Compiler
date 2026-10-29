# pack45 — Packer for unpack()

`pack45` packs a file into a stream that the library's `unpack()` ([`<unpack.h>`](../architecture/stdlib.md#packed-data-unpackh)) unpacks on the MEGA65, to any 28-bit address (chip RAM, attic RAM). The stream can be loaded inside the area it unpacks into and unpacked there ("in place"). `ln45 --pack-overlays` packs the overlay files with the same format ([ln45](ln45.md#packed-overlay-files)).

## Usage

```bash
pack45 [options] <input> <output>
pack45 -x <packed> <output>
pack45 --suggest-rate [--keep PCT] [--max-rate HZ] <sound> ...
pack45 --sound [--rate HZ] [--unsigned] [--normalise] [options] <sound> <output>
pack45 -V | --version
pack45 -? | --help
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
| `--rate HZ` | With `--sound`: the samples' rate |
| `--unsigned` | With `--sound`: unsigned samples (offset by 128) instead of signed |
| `--normalise` | With `--sound`: the loudest sample to full scale |
| `--keep PCT` | With `--suggest-rate`: the share of the energy to keep, per cent (default 99.9) |
| `--max-rate HZ` | With `--suggest-rate`: the highest rate it will be played at. Only the energy that rate can hold counts, and nothing above it is suggested |
| `-v` … `-vvvv` | Log on stderr as `[pack45:LEVEL] …` |

It prints the sizes, where the stream unpacks to and the in-place load address:

```
pack45: tiles.bin: 30000 -> 9812 bytes (32%), unpacks to $40000; in place: load it at $4502C (offset 20524)
```

Every stream is unpacked again by a model of the decoder (in place too) before it is written; a mismatch is an internal error (exit status 2).

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

`<unpack.h>` has two functions, both assembly (`lib/stdlib_asm/unpack.s45`), linked only when called:

| Function | Unpacks |
|----------|---------|
| `unsigned long unpack(unsigned long src)` | the stream at the 28-bit address `src` to the destination in its header |
| `unsigned long unpack_to(unsigned long src, unsigned long dst)` | the same to `dst`, the header's destination ignored |

Both return the number of bytes written, 0 for a stream of another format (nothing is written). Literals up to 16 bytes are copied by the CPU; longer ones, fills and repeats are each one DMA job.

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

The routines use the ZP convention in either library: the stream's address in `$10`-`$13` (for `___unpack_to` the destination in `$14`-`$17`), then `jsr ___unpack` / `jsr ___unpack_to`. The count comes back in Q (A, X, Y, Z: bits 0-7 … 24-31); `$08`-`$19` and the registers are changed. These are base-page addresses: in a cc45 program, the program's own base page.

```asm
.extern ___unpack_to

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

Source: `include/Pack45.hpp`, `src/main/Pack45.cpp` (also used by ln45 and nm45/objdump45), `src/main/pack45_main.cpp`, `include/PackAudio.hpp` and `src/main/PackAudio.cpp` (`--suggest-rate`, `--sound`; minimp3 in `third_party/minimp3`); the decoder `lib/stdlib_asm/unpack.s45`. Tests: `test_compiler.sh` (round trips; `--suggest-rate` and `--sound` on the WAV, AU and MP3 files in `src/test-resources/pack45/`), `src/test-resources/test_unpack_xemu.c` (chip RAM, in place, attic RAM, `unpack_to`, another format).
