# tune45 — Music for the Tune Player

`tune45` turns music written as text (or a MIDI file) into the data of the library's tune player, `<audio_tune.h>`; plays it on the PC, in xemu or on a MEGA65; composes new tunes; and imports MIDI files. The player itself is in the standard library ([stdlib.md](../architecture/stdlib.md), `audio_tune.h`); the example `examples/c/tunes` plays the pfest game's music with it from a raster interrupt, and `examples/c/glamlambs` loads its band's music from the disk into bank 5.

## Usage

```bash
tune45 convert music.txt -o music.c          # C source and music.h (--bin, --asm)
tune45 list music.txt                        # each tune's size and length
tune45 play music.txt [--tune march]         # on the PC (reSIDfp), --emulator, --send, --wav
tune45 create --genre jazz --length 16 -o song.txt
tune45 import song.mid -o song.txt           # convert, list and play take a .mid too
tune45 import song.mod -o song.txt           # a MOD as text: SID instruments, and the samples kept
tune45 import song.mod --report              # a MOD's samples weighed for the SID; --extract <dir>
tune45 import song.mod --audition [n]        # each sample: the original, the SID, both; then sid or sample?
tune45 sample growl.pak --rate 8000          # sampled sound as the audio DMA plays it
tune45 -V | --version
tune45 -h | --help
```

`-v` … `-vvvv` anywhere among the arguments log on stderr as `[tune45:LEVEL] …` (the toolchain's format, `include/Logger.hpp`); cumulative, at most `-vvvv`; `-v0` off: the tunes and instruments read, the file written, the player program built (`-v`); each tune's voices and data size, the commands run (`-vv`).

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
wavetable snare noise pulse:-12 pulse:-19 pulse:-24
instrument calliope pulse 09 a6 0600 1 1
instrument kick     pulse 08 00 0800 0 0 slide=-47
instrument drum     pulse 00 f0 0800 0 0 hr=1 table=snare
instrument fbass    saw   00 f8 0800 0 0 hr=2 filter=lp res=12 cutoff=200 fsweep=12
sample kick  drums/kick.wav rate=8000
sample snare drums/snare.wav
sample hat   drums/hat.wav gain=60
tune march 5 loop 8
v1: @calliope c5:2 e5:2 g5:2 e5:2 | f5:2 a5:2 g5:4 | ...
v2: @bass c3:4 g2:4 | ...
v3: @stab r:2 e4:2 r:2 g4:2 | ...
d1: kick:2 hat:2 kick+snare:2 hat:2 | ...
```

| Line | Meaning |
|------|---------|
| `instrument <name> <wave> <ad> <sr> <pulse> <vibrato> <gap> [pwm=n] [vdelay=n] [slide=n] [hr=n] [filter=m res=n cutoff=x fsweep=n] [table=name]` | `wave`: `tri`, `saw`, `pulse`, `noise`, joined by `+` (`pulse+saw`), or a hex control value. `ad`, `sr`: the envelope's nibble pairs, hex. `pulse`: pulse width, hex `0`-`fff`. `vibrato`: depth 0-15 (`(frequency >> 8) * depth * triangle / 2`, a triangle over 8 frames). `gap`: ticks the note is released before the next event (staccato). `pwm`: pulse-width change a frame, signed (bouncing between `$200` and `$E00`). `vdelay`: frames before the vibrato starts. `slide`: pitch change a frame, signed, in 16ths of the frequency register (stopping at 0 and `$FFFF`; a falling kick drum). `hr`: hard restart, 0-3 frames: that many frames before the voice's next note (not a tie or a rest) its gate goes off and AD/SR to 0, so the envelope is at rest when the note starts (no ADSR bug: every note's attack the same). `filter`: `lp`, `bp`, `hp`, `notch`, or joined by `+`: a note of the instrument sets its chip's filter (`cutoff` hex `0`-`7ff`, default `400`; `res` 0-15; `fsweep` the cutoff's change a frame, signed, stopping at `0` and `7ff`) and routes the voice through it; a note of an unfiltered instrument takes its voice out. The filter is the chip's (three voices): the last filtered note on it sets it. `table`: a wave table |
| `sample <name> <file> [rate=hz] [gain=percent] [from=s] [len=s] [note=n] [loop[=s]]` | A sound for the digi voices: WAV, AU, MP3 (MP4, OGG, FLAC ... by `ffmpeg`), the path from the text file's directory; mixed to mono and made 8-bit (unsigned) at `rate` (default: the lowest that keeps 99.9% of it, at most 12000 Hz: `pack45 --suggest-rate`; a hi-hat or cymbal is nearly all high frequencies: give it 16000 or so); `gain` scales it (100). `from` and `len` (seconds, with a fraction) cut a part out (one hit of a kit in one file), its end faded over 5 ms. `note`: the note it sounds at its rate (default `c4`), for pitched hits. `loop`: it loops from its start to its end until a stop or the next hit (`loop=0.25`: from 0.25 s in; a mix of several samples plays once) |
| `d1:` ... `d4:` | A digi voice, in order: samples on the MEGA65's audio DMA, a channel each. Tokens: `name:ticks` a hit, `name:note:ticks` a hit at a note (`piano:g4:4`: the same bytes at a rate 2^(semitones/12) times the sample's, from its `note=`; every pitch shares one copy in the bank), `a+b:ticks` samples hit together, `r:ticks` / `~:ticks` (the sample rings on), `-:ticks` the channel stopped (a loop ended, a sample cut), `vol:n` the voice's volume from here, 0-64 (no time; also the sample sounding: a slide is a `vol:` a tick), `\|` a bar line. A channel plays one sample: the next hit cuts the one before short (tune45 warns of each hit that is cut, and by how much), so samples hit together are mixed by tune45 into one (`kick+snare`: the samples at the highest of their rates, added, clipped; each combination once, in any order). A tune's digi voices are as long as its SID voices; a tune may have only digi voices |
| `wavetable <name> <step> ... [loop <n>]` | A step a frame from the note's start (1-64): a waveform (as an instrument's) and `:semitones` from the note (`pulse:-12`; signed). After the last step, step `n` (from 1) again, or the last held. A step's waveform replaces the instrument's; semitones other than 0 set the pitch from the note (with an arpeggio, added to it), 0 leaves the pitch to the glide, slide and vibrato. A drum: `noise pulse:-12 pulse:-19`; a chord buzz: `pulse saw pulse:12 saw:12 loop 1`. Defined before an instrument names it |
| `pattern <name>: <tokens>` | A part a voice plays where it says `*name` (`*name*3`: three times running, 1-255): its tokens as a voice's (a SID voice's or a digi voice's, as the voice calling it; no calls in a pattern). In the data once (for each kind of voice that calls it), called and returning: a repeated phrase costs its bytes once. Ticks and bars are counted through the calls. Defined before a voice calls it |
| `tune <name> <frames per tick> loop\|once [ticks a bar]` | The name is a C identifier. Frames per tick may have a fraction (`6.25`: 8.8 fixed point). With ticks a bar, every bar between `\|` marks must be that long |
| `v1:` ... `v12:` | A voice, in order. Tokens: `note:ticks` (`c4` is MIDI 60, middle C; `c#4`, `eb4`, octaves -1 to 9), `r:ticks` a rest, `~:ticks` the note held longer (a tie), `@name` an instrument, `arp:a,b` an arpeggio (the note, +a, +b semitones, a frame each; `arp:off`), `note^s[,k]:ticks` a glide, `\|` a bar line, `tempo:f` the tune's frames per tick from there (as the `tune` line's; a fraction allowed) |

A **tempo change** (`tempo:5`, in any `v` voice, taking no time) holds from that tick on, for all the voices; one of a later voice on the same tick wins. A looping tune starts again at its `tune` line's tempo (tune45 puts it at the start of `v1`). `list` and `play` count the changes in a tune's length; the digi voices' cut-short warning too.

A **glide** starts the note `s` semitones below its pitch (above when `s` is negative) and from its second frame moves (distance + 2^k − 1) >> k a frame towards it, then stops there: `g4^3:12` slides up into G4 from E4, an eighth of the way a frame (`k` 1-7, default 3); `c-1^-68,2:10` falls from G#4 a quarter of the way down a frame (a kick drum).

A voice plays instrument 0 (the file's first) until it names one. A duration over 255 ticks is split into a note and ties (or several rests). Errors carry the file and line: an unknown instrument, a note out of range, a glide out of range, a voice not `v1:`, `v2:`… in order, voices of one tune of different lengths (the ticks of each are listed), a bar that does not add up, a tune name that is not a C identifier.

## convert

| Option | Output |
|--------|--------|
| `-o music.c` (default) | C source: `const unsigned char tune_<name>[]` a tune, `tunes[]` of them all; `music.h` (or `--header <file>`) declares them with `#define TUNE_<NAME> <index>`, `TUNE_<NAME>_TICKS` (its ticks once through: `tune_seek` back to where a looping tune was) and `NTUNES` |
| `--bin -o music.bin` | One blob to load into far memory: the number of tunes, a 16-bit offset each, then the tunes; the header gives `TUNE_<NAME>_AT` (its offset) and `TUNES_SIZE`. `tune_play_far(<where it was loaded> + TUNE_MARCH_AT)` |
| `--load-at <addr>` | With `--bin`: the file starts with a 2-byte load address, as a KERNAL LOAD reads it (`cbm_load` skips it; offsets stay from the blob's start) |
| `--asm -o music.s45` | ca45 source of the same data (`_tune_<name>`, `_tunes`) |
| `--samples <file>` | With `--bin` and digi voices (needed then): the samples, one bank (with `--load-at`, its load address first too). In C and `--asm` they are `tune_sample_bank` (`TUNE_SAMPLES_SIZE` bytes). The program says where they are with `tune_samples(addr)`: chip RAM (the audio DMA reads nothing else), not across a 64 KB boundary; at most 64 KB |

The data of a tune (`lib/stdlib_asm/tune.s45` reads it): a header (flags: bit 0 loops; voices; frames per tick, 8.8; the number of instruments, an instrument's bytes (17; 0 in data of before hard restart, filters and wave tables, whose instruments are 10 bytes and still play) and their table's offset; a 16-bit offset per voice's stream), the instruments it uses (17 bytes: waveform, AD, SR, pulse width, vibrato, gap, pulse-width sweep, vibrato delay, slide, hard restart, filter (mode bits 4-6, resonance 0-3), cutoff, cutoff sweep, the wave table's offset; renumbered), the wave tables they use (steps, the step to loop to or `$FF`, a waveform and semitones a step), with digi voices (their count in the flags' bits 1-3; their streams' offsets after the SID voices', then the sample table's) the sample table (9 bytes a hit, flags bit 4: its offset in the samples, its bytes, the audio DMA's rate value, its loop's start from it or `$FFFF`; 7 bytes, without the loop, in data of before loops), and a byte stream a voice (a digi voice's: `$00`-`$7F h d` hit h for d ticks, `$84 h d` any hit 0-255, `$80 d` / `$82 d` ringing on, `$81 d` stopped, `$83 v` its volume 0-255, `$FE`, `$FF`): `$00`-`$7F n d` note n for d ticks, `$80 d` a rest, `$81 i` instrument i, `$82 d` a tie, `$83 a b` an arpeggio, `$84 s k` the next note's glide, `$85 lo hi` the frames per tick from then (8.8), `$86 lo hi n` a pattern called n times (its offset in the tune; the voice's stream on after it), `$87` a pattern's end, `$FE` back to the start (a tune that loops), `$FF` the end.

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

Digi voices are mixed in as the audio DMA plays them (each sample held to the next, its channel's side; at half scale beside the SIDs, so the balance is near, not exact). reSIDfp emulates the 6581 and 8580 closely, filters included; the MEGA65's SIDs are an FPGA implementation, so the PC sound is very close, not identical. reSIDfp (libsidplayfp's libresidfp, GPL-2.0-or-later) is in `third_party/libresidfp` (`PROVENANCE.md`) and linked only into tune45.

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

### A MOD

`tune45 import song.mod -o song.txt` writes a MOD (ProTracker's `M.K.`, `4CHN`, `6CHN`, `8CHN`, `FLT4`... and the old 15-sample Soundtracker files) as tune45 text: each sample a SID instrument or kept as a sample, by its verdict (below: the best guess), or by the choices of `--audition` (`--choices song.choices`).

- **The rows**, in the order the song plays them (position jumps, pattern breaks, pattern loops and delays followed), a tick each; it stops where it would play a position again, and the tune loops there. The frames a tick are the speed * 125 / BPM (`tempo:` where they change); a pattern a line, between `|` marks.
- **A channel** is a `v` voice for its notes on SID instruments and a `d` voice for its samples kept (a channel with both has one of each: a sample stops at the channel's SID note, by a silent hit, `cut`). More than 4 channels of samples are folded onto `d1`-`d4` (hits at once mixed).
- **A SID note** sounds at the sample's own pitch (its measured period, the finetune too); its instrument is the report's (`s06`: sample 6), with `hr=1`.
- **A kept sample** is a WAV a note it is played at (`s08_c2`: at C-2, its rate the note's, so `sample` keeps the pitch), in `song_samples/` beside the text (without `-o`: beside the MOD); a loop is unrolled to the longest the song holds it (at most 2 s). Hi-hats at 16000 Hz.
- **Patterns**: a voice's part of a MOD pattern (a line) played more than once is a `pattern` (`p1`...), called where it plays (`*p2*3`: three times running): the data and the text a good deal smaller (the four test MODs': to 27-63% of the data).
- **Effects**: arpeggio (`arp:`, a row's), tone portamento (a glide on the note), vibrato (an instrument of that depth, `s06v4`), note cut and `C00` (a rest; the silent hit for a sample), speed and BPM. Not carried over, and counted in a comment: volume (a SID voice has none), pitch slides, tremolo, retriggers, note delays, sample offsets.
- **64 KB**: tune45's samples are one bank. When the kept samples are more (about: their notes, at the rate tune45 will choose), the best guess makes SID instruments of the biggest, the melodic ones first, the drums last, until they fit (said, and in the instrument's comment). With `--choices` they are kept as chosen: the text is written, with convert's error and the kept samples' sizes.

The text then converts, lists and plays as any other (`tune45 play song.txt`); it is a start to edit.

### A MOD: which samples the SID can play

`tune45 import song.mod --report` reads a MOD (ProTracker's `M.K.`, `4CHN`, `6CHN`, `8CHN`, `FLT4`... and the old 15-sample Soundtracker files) and weighs each sample for the SID: whether one of tune45's instruments can stand in for it, or it is too far from the SID and is best kept as a sample on a digi voice.

Each sample is measured at the period the song plays it at most (the Amiga's PAL clock: 3546894.6 / period Hz):

| Measured | How | Counts against the SID |
|----------|-----|------------------------|
| Pitch | the normalised square difference function's first key maximum (McLeod): its height is the *clarity*; a loop of up to 512 bytes is read round (a single cycle) | no steady pitch (clarity under 0.75): a drum or a noise |
| Repetition | the periods folded into one (up to 16), and the share of the energy not in it | breath, noise, chorus, a chord or a voice |
| Timbre | the folded period's harmonics against the SID's waveforms' (triangle, saw, pulse at 5%-50%, each also through a 12 dB low-pass at 1-8 times the note): the nearest by cosine, the *fit* | a timbre none comes near |
| Change | the harmonics after the attack against the loop's (or later on) | a timbre that changes as it sounds |
| Pitch over time | the pitch every half window | vibrato or a slide in the sample |
| Loudness | 20 ms frames: attack, decay, sustain (the loop's level) as the SID's nearest AD and SR; swells after the peak | several hits, tremolo or an echo in the sample |
| The song | notes, periods, channels; vibrato, slides, arpeggios, volume changes and sample offsets on it | many volume changes (a SID voice has no volume of its own) |
| The name | drum words make a sound with a doubtful pitch a drum; voice words count against it | |

A score from 0 (easy) to 100 gives the verdict: `sid` (up to 25), `sid~` (approximately, up to 55), `sample` above, and every unpitched sound (`kick`, `snare`, `hat` by its name or brightness, `texture` when it is held in a loop). The table gives each sample's bytes, loop, notes, the notes it is played at (as the tracker names them), the verdict and score, and for the SID its waveform, pulse width (`pulse 400`: 25%), low-pass, AD and SR and the notes it sounds at; then each sample's measurements and the reasons, and a summary: the instruments, the samples kept and their bytes, how many of them sound at once (the digi voices they need: the audio DMA has 4 channels), and those played at several notes (a digi voice plays a hit at one rate: a hit a note until the player has pitched hits).

`--extract <dir>` writes the samples kept (`--all`: every one the song uses) as WAV files, `NN_name.wav`: 8-bit at the rate of the period played most, the loop and the note it sounds at in a `smpl` chunk; a `sample` line reads them.

`--audition [n]` plays each sample the song uses (or sample `n`) to compare by ear: the original as the audio DMA plays it (8-bit, its loop, each sample held to the next), then the SID's stand-in (the report's waveform, pulse width, low-pass and envelope through the player's model and reSIDfp; a drum's: noise, or a falling pulse for a kick), then both at once, **the original on the left and the SID on the right**. Each at up to four of the notes the song plays it at (the lowest, the highest, between), as long as the sample sounds (a looped one 0.8 s); each part scaled to the same loudness (the SID's offset and the click of its volume left out), so the sound is compared, not the level.

At a terminal it then asks, after each sample:

```
 6  in sample bank 1f      sid~    35  SID: pulse a1 c3 733 0 0
  6: [sid] d s r 1 2 3 b q >
```

| Key | |
|-----|--|
| Enter | as shown in brackets: the verdict (`sid` for `sid` and `sid~`), or the choice made before |
| `d` / `s` | a SID instrument / keep the sample |
| `r`, `1`, `2`, `3` | all three again; the original, the SID, both alone |
| `b`, `q` | back a sample; quit (the choices made are kept) |

The choices are saved as they are made to `--choices <file>` (default: beside the MOD, `song.choices`), a line a sample (`6 sid`, `8 sample`; `;` comments), and are the defaults the next time, so an audition can be taken up again or redone. `--report --choices <file>` and `--extract <dir> --choices <file>` use them as the verdicts (marked `*`; the measured verdict stays among the reasons). `--wav <file>` writes the audition to a file instead (and asks nothing); `--ask` asks without a terminal, the answers read from the input (with `--wav`: the file written first, nothing played).

## sample

Plays sampled sound as the MEGA65's audio DMA plays it: 8-bit samples at their rate, each held until the next (the DMA does not interpolate), on the PC through `pw-play`, `paplay` or `aplay` as `play` does, or into a 48 kHz WAV (`--wav`). What it plays:

| The file | Played |
|----------|--------|
| A pack45 stream (with its load address or `--raw`) | Unpacked: the samples `pack45 --sound -d ...` packed |
| A sound: WAV, AU, MP3; MP4, M4A, OGG, FLAC ... by `ffmpeg` | Made into samples first as `pack45 --sound` does: mixed to mono, low-passed, resampled to `--rate` or the rate `pack45 --suggest-rate` names (`--max-rate <hz>` its cap) |
| Anything else | The bytes as samples |

| Option | Meaning |
|--------|---------|
| `--rate <hz>` | The samples' rate (needed for samples: they do not say it; `pack45 --sound` prints it) |
| `--max-rate <hz>` | A sound file: the highest rate to suggest |
| `--unsigned` | Unsigned samples, offset by 128 (default signed, as `pack45 --sound` makes them) |
| `--wav <file>` | A 48 kHz WAV file instead of the speakers |

So a sound can be heard as it will be, its rate and 8 bits and all, before it goes on a disk: `tune45 sample --max-rate 12000 dragon.wav` plays it at the rate pack45 would choose. See [pack45](pack45.md#sounds---suggest-rate---sound).

## Exit Codes

0 on success, 1 on an error (the messages on stderr).

## Tests

`src/test/test_tune45.sh` (`make test`): the data of a tune byte by byte, glides, `TUNE_<NAME>_TICKS`, `--bin`, `--asm`, the errors, pfest's format, `list`, `create --check` and its options, the data of hard restart, a filter and a wave table and their errors, digi voices (the data, `--samples`, the cut-short warning, play's mix, the errors), `import` of a MIDI file made by the test, `import` of MODs made by the test (the rows, tempo, effects, a kept sample stopped, the notes' pitch, fitting 64 KB), `--report`, `--extract` and `--audition` (a saw, a pulse and a triangle the SID plays, a noise and a bell it does not), `play --wav`, `sample` (a sound and the same samples packed by pack45, played into a WAV, and the error without `--rate`). `src/test/test_xemu.sh`: the player (`test_tune_xemu.c`, its tunes in the 10-byte instruments of older data) and `tune45 trace` (the model's registers, voices and filter) against the player's in xemu for 300 frames: a tune with every effect, one of glides after a `tune_seek`, one of the filter, wave tables and hard restart, and one of digi voices (`digi.txt`: the audio DMA's registers too, from channel 2, a third voice left out). `examples/c/tunes` and `examples/c/glamlambs` (`test_examples.sh`).
