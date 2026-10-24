# tune45 — Music for the Tune Player

`tune45` turns music written as text (or a MIDI file) into the data of the library's tune player, `<audio_tune.h>`; plays it on the PC, in xemu or on a MEGA65; composes new tunes; and imports MIDI files. The player itself is in the standard library ([stdlib.md](../architecture/stdlib.md), `audio_tune.h`); the example `examples/c/tunes` plays the pfest game's music with it from a raster interrupt, and `examples/c/glamlambs` loads its band's music from the disk into bank 5.

## Usage

```bash
tune45 convert music.txt -o music.c          # C source and music.h (--bin, --asm)
tune45 list music.txt                        # each tune's size and length
tune45 play music.txt [--tune march]         # on the PC (reSIDfp), --emulator, --send, --wav
tune45 create --genre jazz --length 16 -o song.txt
tune45 import song.mid -o song.txt           # convert, list and play take a .mid too
tune45 -V | --version
tune45 -h | --help
```

In a makefile:

```make
music.c music.h: music.txt
	tune45 convert music.txt -o music.c
```

and in the program:

```c
#include <audio_tune.h>
#include "music.h"
...
tune_play(tune_march);            /* or tunes[TUNE_MARCH] */
for (;;) { wait_frame(); tune_update(); }   /* or in a raster __interrupt handler */
```

## The text format

The pfest game's `music.txt` format, which `tune45` accepts unchanged, with optional additions.

```
; a comment                        (a line ending in \ goes on)
instrument calliope pulse 09 a6 0600 1 1
instrument kick     pulse 08 00 0800 0 0 slide=-47
tune march 5 loop 8
v1: @calliope c5:2 e5:2 g5:2 e5:2 | f5:2 a5:2 g5:4 | ...
v2: @bass c3:4 g2:4 | ...
v3: @stab r:2 e4:2 r:2 g4:2 | ...
```

| Line | Meaning |
|------|---------|
| `instrument <name> <wave> <ad> <sr> <pulse> <vibrato> <gap> [pwm=n] [vdelay=n] [slide=n]` | `wave`: `tri`, `saw`, `pulse`, `noise` (or a hex control value). `ad`, `sr`: the envelope's nibble pairs, hex. `pulse`: pulse width, hex `0`-`fff`. `vibrato`: depth 0-15 (`(frequency >> 8) * depth * triangle / 2`, a triangle over 8 frames). `gap`: ticks the note is released before the next event (staccato). `pwm`: pulse-width change a frame, signed (bouncing between `$200` and `$E00`). `vdelay`: frames before the vibrato starts. `slide`: pitch change a frame, signed, in 16ths of the frequency register (stopping at 0 and `$FFFF`; a falling kick drum) |
| `tune <name> <frames per tick> loop\|once [ticks a bar]` | The name is a C identifier. Frames per tick may have a fraction (`6.25`: 8.8 fixed point). With ticks a bar, every bar between `\|` marks must be that long |
| `v1:` ... `v12:` | A voice, in order. Tokens: `note:ticks` (`c4` is MIDI 60, middle C; `c#4`, `eb4`, octaves -1 to 9), `r:ticks` a rest, `~:ticks` the note held longer (a tie), `@name` an instrument, `arp:a,b` an arpeggio (the note, +a, +b semitones, a frame each; `arp:off`), `note^s[,k]:ticks` a glide, `\|` a bar line |

A **glide** starts the note `s` semitones below its pitch (above when `s` is negative) and from its second frame moves (distance + 2^k − 1) >> k a frame towards it, then stops there: `g4^3:12` slides up into G4 from E4, an eighth of the way a frame (`k` 1-7, default 3); `c-1^-68,2:10` falls from G#4 a quarter of the way down a frame (a kick drum).

A voice plays instrument 0 (the file's first) until it names one. A duration over 255 ticks is split into a note and ties (or several rests). Errors carry the file and line: an unknown instrument, a note out of range, a glide out of range, a voice not `v1:`, `v2:`… in order, voices of one tune of different lengths (the ticks of each are listed), a bar that does not add up, a tune name that is not a C identifier.

## convert

| Option | Output |
|--------|--------|
| `-o music.c` (default) | C source: `const unsigned char tune_<name>[]` a tune, `tunes[]` of them all; `music.h` (or `--header <file>`) declares them with `#define TUNE_<NAME> <index>`, `TUNE_<NAME>_TICKS` (its ticks once through: `tune_seek` back to where a looping tune was) and `NTUNES` |
| `--bin -o music.bin` | One blob to load into far memory: the number of tunes, a 16-bit offset each, then the tunes; the header gives `TUNE_<NAME>_AT` (its offset) and `TUNES_SIZE`. `tune_play_far(<where it was loaded> + TUNE_MARCH_AT)` |
| `--load-at <addr>` | With `--bin`: the file starts with a 2-byte load address, as a KERNAL LOAD reads it (`cbm_load` skips it; offsets stay from the blob's start) |
| `--asm -o music.s45` | ca45 source of the same data (`_tune_<name>`, `_tunes`) |

The data of a tune (`lib/stdlib_asm/tune.s45` reads it): a header (flags: bit 0 loops; voices; frames per tick, 8.8; the number of instruments and their table's offset; a 16-bit offset per voice's stream), the instruments it uses (10 bytes: waveform, AD, SR, pulse width, vibrato, gap, pulse-width sweep, vibrato delay, slide; renumbered), and a byte stream a voice: `$00`-`$7F n d` note n for d ticks, `$80 d` a rest, `$81 i` instrument i, `$82 d` a tie, `$83 a b` an arpeggio, `$84 s k` the next note's glide, `$FE` back to the start (a tune that loops), `$FF` the end.

## list

For each tune: voices, bytes, ticks, its length in seconds (the same at PAL and NTSC: the player counts NTSC frames as 5/6 of a PAL one), frames per tick, loop or once, each voice's bytes and the instruments; then the total.

## play

| Option | Meaning |
|--------|---------|
| (none) | On the PC: a model of the player (`TunePlayer`, the same register writes as the library's, checked against it frame by frame in `test_xemu.sh`) drives four reSIDfp SID chips (SIDs 1-2 left, 3-4 right, voices mirrored on SIDs 3-4 as `sid_set_mirror`), and the sound goes to `pw-play`, `paplay` or `aplay` (the first on `PATH`; no audio library is linked) |
| `--tune <name>` | That tune (again for more; default: all, in order) |
| `--loops <n>` | Times through a looping tune (default once round) |
| `--wav <file>` | A 48 kHz stereo WAV file instead of the speakers |
| `--ntsc`, `--6581`, `--mono` | An NTSC machine; the MOS 6581 SID (default the 8580); SIDs 1-2 only |
| `--emulator`, `--emulator-path <f>` | On the MEGA65 in xemu: a player program (`lib/build/tuneplay.o45`, `lib/tools/tuneplay.c`) linked with the tunes by cc45 and ln45 from tune45's directory, run in xemu. On its screen: the tunes, the one playing, each voice's note, the ticks; keys: CRSR or 1-9 a tune, SPACE stop/play, R from the start, + - tempo, V vibrato, T G transpose, RUN/STOP |
| `--send`, `--send-cmd <cmd>` | The same program on a MEGA65: `etherload -r <prg>` (mega65-tools) when on `PATH`, else `m65 -F -r <prg>`, or the command given (`{prg}` the program) |
| `--prg <file>` | Keep the player program there |
| `--watch` | Again whenever the text file changes (Ctrl-C stops): on the PC the tunes start again; xemu is restarted; `--send` sends again |

reSIDfp emulates the 6581 and 8580 closely, filters included; the MEGA65's SIDs are an FPGA implementation, so the PC sound is very close, not identical. reSIDfp (libsidplayfp's libresidfp, GPL-2.0-or-later) is in `third_party/libresidfp` (`PROVENANCE.md`) and linked only into tune45.

## create

Composes a tune in the text format: the composer of `examples/c/musicgen`, ported. A genre's chords and the Markov table of how one follows another, AABA form (blues: 12 bars, call and response), melodies from the genre's rhythm cells and a motif with chord tones on the strong beats and steps between, voice-led chords, the genre's bass line (walking, boogie, Alberti...) and drums. A tick is a 16th note (a swung genre: a 12-tick beat, 4-4-2-2); bars are marked and checked. The text's first line says the genre, key, tempo, bars and seed.

| Option | Meaning |
|--------|---------|
| `--genre <g>` | `pop`, `rock`, `blues`, `jazz`, `classical`, `ambient`, `chiptune` (default: drawn from the seed) |
| `--tempo <t>` | Beats a minute, or `slow` / `medium` / `fast` (0.8, 1, 1.25 times the genre's); default the genre's |
| `--length <n>` | Bars, rounded up to whole sections of 8 (blues 12), ending on an A section; or seconds (`60s`). Default 32 |
| `--voices <n>` | `3` lead, bass and the chords as an arpeggio (one SID; default); `4` and drums (kick, snare, hats); `6` three chord voices |
| `--seed <n>` | The same tune again (default: from the clock) |
| `--key <k>` | `c`, `c#`, `d`, `eb` ... `b` (default: drawn) |
| `--once`, `--loop` | The tune ends (no pickup after its last cadence) / loops (default) |
| `--name <n>` | The tune's name (default the genre) |
| `-o <file>`, `--append <file>` | Write the text (default: to the output); or add the tune to a music file, with the instruments it does not have yet |
| `--play` | Play it afterwards (play's options apply) |
| `--check [--count n]` | Compose every genre (or `--genre`'s) with seeds 1 to n (default 20) and check each melody as `musicgen`'s `check_songs.py` does (every note in the scale or the chord, 80% chord tones on beats 1 and 3, 40% steps (the blues scale's minor thirds count), no leap over an octave, the range, the cadence on the tonic) and that the text parses |

## import

Reads a Standard MIDI File (format 0 or 1; ticks a quarter note, not SMPTE time) and writes tune45 text: up to `--voices` (default 3) monophonic voices from the first tracks with notes (format 1) or channels (format 0, or `--channels`); `--tracks a,b,c` / `--channels a,b` choose them. Where notes overlap the newest wins. Times are quantised to `--ticks-per-beat` (default 4: 16ths); the first tempo gives the frames per tick and the first time signature the bar length (notes across a bar line are tied). Channel 10 (drums) only with `--drums`. `--transpose <n>`, `--name <n>`, `--loop`. Instruments: `--instruments <file>` gives instrument lines (`voice1`, `voice2`... a voice's; `gm<program>` used at a program change); otherwise each voice gets one of three of tune45's. `convert`, `list` and `play` read a `.mid` file directly (by its `MThd` header).

## Exit Codes

0 on success, 1 on an error (the messages on stderr).

## Tests

`src/test/test_tune45.sh` (`make test`): the data of a tune byte by byte, glides, `TUNE_<NAME>_TICKS`, `--bin`, `--asm`, the errors, pfest's format, `list`, `create --check` and its options, `import` of a MIDI file made by the test, `play --wav`. `src/test/test_xemu.sh`: the player (`test_tune_xemu.c`) and `tune45 trace` (the model's registers) against the player's in xemu for 300 frames: a tune with every effect, and one of glides after a `tune_seek`. `examples/c/tunes` and `examples/c/glamlambs` (`test_examples.sh`).
