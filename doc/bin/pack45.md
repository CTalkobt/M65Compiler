# pack45 — Packer for unpack()

`pack45` packs a file into a stream that the library's `unpack()` ([`<unpack.h>`](../architecture/stdlib.md#packed-data-unpackh)) unpacks on the MEGA65, to any 28-bit address (chip RAM, attic RAM). The stream can be loaded inside the area it unpacks into and unpacked there ("in place"). `ln45 --pack-overlays` packs the overlay files with the same format ([ln45](ln45.md#packed-overlay-files)).

## Usage

```bash
pack45 [options] <input> <output>
pack45 -x <packed> <output>
pack45 --suggest-rate [--keep PCT] [--max-rate HZ] <sound.wav> ...
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
| `--suggest-rate` | For each WAV: the lowest sample rate that keeps its sound (below) |
| `--keep PCT` | With `--suggest-rate`: the share of the energy to keep, per cent (default 99.9) |
| `--max-rate HZ` | With `--suggest-rate`: the highest rate it will be played at. Only the energy that rate can hold counts, and nothing above it is suggested |
| `-v` … `-vvvv` | Log on stderr as `[pack45:LEVEL] …` |

It prints the sizes, where the stream unpacks to and the in-place load address:

```
pack45: tiles.bin: 30000 -> 9812 bytes (32%), unpacks to $40000; in place: load it at $4502C (offset 20524)
```

Every stream is unpacked again by a model of the decoder (in place too) before it is written; a mismatch is an internal error (exit status 2).

## Sounds: --suggest-rate

Sampled sound does not pack: LZ finds no repeats in it, and a coder (8-bit PCM, ADPCM) spends the same bytes on every sample however smooth the wave. What makes a sound smaller is fewer samples, a lower rate, and a rate keeps only what lies below half of it. `--suggest-rate` measures where a sound's energy lies (its spectrum) and names the lowest of the usual rates (4000, 5000, 6000, 8000, 10000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 Hz) whose pass band, 0.45 of the rate (what a practical low-pass filter keeps), holds `--keep` per cent of it:

```
$ pack45 --suggest-rate --max-rate 12000 dragon-idle.wav chest-open.wav
pack45: dragon-idle.wav: 44100 Hz, 16-bit, 2 channels, 1.48 s; 99.9% of its energy below 3106 Hz: 8000 Hz keeps it (11840 bytes at 8 bits, 18% of 44100 Hz); audio DMA rate $000CF2
pack45: chest-open.wav: 44100 Hz, 16-bit, 2 channels, 1.76 s; 99.9% of its energy below 5340 Hz: 12000 Hz keeps it (21120 bytes at 8 bits, 27% of 44100 Hz); audio DMA rate $00136B
```

A growl, a bell or a thud keeps nearly all of itself at 4000-8000 Hz; a hit, a swing or a splash of water needs the top. Give `--max-rate` the rate the program plays at: a recording's faint hiss far above it otherwise counts as sound to keep. The audio DMA rate is the value for the MEGA65's audio DMA channel's frequency registers (Hz × 2²⁴ / 40.5 MHz). Reads PCM WAVs of 8, 16, 24 or 32 bits and 32-bit float, any number of channels (mixed to one). It suggests only: resample with the tool of your choice (`sox in.wav -r 8000 out.wav`).

## On the MEGA65

```c
#include <cbm.h>
#include <unpack.h>

cbm_load_far("TILES", 5, 8, 0x4502CUL);   /* the in-place address pack45 printed */
unpack(0x4502CUL);                         /* to $40000, the destination in the stream */
```

`cbm_load_far` skips the two-byte load address. A stream for attic RAM (`-d 0x8000000`) is loaded into chip RAM and unpacked from there.

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

A repeat is never closer than it is long, so one forward DMA copy does it. The packer is greedy with hash chains (256 candidates a position) and looks one byte ahead. With `--best` it parses from the end instead: at each position the cheapest way on to the end, over a fill, a repeat of any length up to the longest near (1-byte offset) and far (2-byte offset) one found, or literals. On a game's art (760 KB of character sets and screens) that is 2.7% smaller than the greedy stream, in about five times the time. The format byte keeps the door open to denser formats later; `unpack()` returns 0 for a stream of another format.

**In place:** while unpacking, the bytes written must stay behind the bytes still to be read. The packer computes the least offset from the destination at which the stream can start for that to hold (its largest lead of writing over reading); loaded there or further in, `unpack()` reads each literal block before overwriting it (a forward copy, downwards).

Source: `include/Pack45.hpp`, `src/main/Pack45.cpp` (also used by ln45 and nm45/objdump45), `src/main/pack45_main.cpp`, `include/PackAudio.hpp` and `src/main/PackAudio.cpp` (`--suggest-rate`); the decoder `lib/stdlib/unpack.c`. Tests: `test_compiler.sh` (round trips, `--suggest-rate` on `src/test-resources/pack45/`), `src/test-resources/test_unpack_xemu.c` (chip RAM, in place, attic RAM).
