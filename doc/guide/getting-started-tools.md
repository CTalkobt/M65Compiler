# Getting Started with the Other Tools

cc45 and basic45 have their own tutorials ([C](getting-started-c.md), [basic45](getting-started-basic45.md)).
This page is a short tour of the rest: what each tool is for and the commands you will use first, then a small
project that uses several of them together. Every command here is run as written by `make test`
(`src/test/test_guides.py`); each tool's page in [../bin/](../bin/) has all its options.

| Tool | For |
|------|-----|
| [ca45](../bin/ca45.md), [ln45](../bin/ln45.md), [ar45](../bin/ar45.md) | Assembling, linking, libraries |
| [nm45](../bin/nm45.md), [objdump45](../bin/objdump45.md) | Looking inside objects and programs |
| [al45](../bin/al45.md), [af45](../bin/af45.md), [cl45](../bin/cl45.md) | Linting assembly, formatting it, linting C |
| [disk45](../bin/disk45.md) | Disk images (D81 and 24 other formats) |
| [pack45](../bin/pack45.md) | Packed data, unpacked by the program |
| [font45](../bin/font45.md) | Fonts and character sets |
| [tune45](../bin/tune45.md) | Music for the library's tune player |
| [asmCvt45](../bin/asmCvt45.md) | Other assemblers' source as ca45's |
| [dbg45](../bin/dbg45.md), [deploy45](../bin/deploy45.md) | Debugging in xemu; sending to a MEGA65 or an Ultimate 64 |

## Assembly: ca45, ln45 and ar45

**ca45** assembles 45GS02 code (the MEGA65's CPU: the 6502's instructions and the 65CE02's and its own). A
program in assembly alone:

`hello.s45`:

<!-- guide: file hello.s45 -->
```asm
; hello.s45: a program in assembly alone
CHROUT = $ffd2

    .basicUpstart start

start:
    ldx #0
loop:
    lda message,x
    beq done
    jsr CHROUT
    inx
    bra loop
done:
    rts

message:
    .text "hello from ca45"
    .byte 13, 0
```

<!-- guide: run -->
```console
$ ca45 hello.s45 -o hello.prg
Assembled to hello.prg (45 bytes)
```

<!-- guide: xemu hello.prg screen=HELLO FROM CA45 -->

`.basicUpstart` writes the `10 SYS` line, so the program starts with `RUN`; it prints `HELLO FROM CA45`.

More often assembly is part of a C program: `ca45 -c` makes a relocatable object (`.o45`), like `cc45 -c`, and
**ln45** links the objects with the library. A C function's argument arrives in the zero page at `$10` (cc45's
default calling convention, [calling-conventions.md](../architecture/calling-conventions.md)) and its result goes
back in A (low byte) and X:

`times.s45`:

<!-- guide: file times.s45 -->
```asm
; times.s45: unsigned int times_ten(unsigned int x), for C
; (cc45's default convention: the argument in $10-$11, the result in A/X)

    .global _times_ten

_times_ten:
    lda $10
    asl                     ; x * 2 in $08-$09
    sta $08
    lda $11
    rol
    sta $09
    lda $08                 ; x * 8 in A/X
    ldx $09
    asl
    pha
    txa
    rol
    tax
    pla
    asl
    pha
    txa
    rol
    tax
    pla
    clc                     ; x * 8 + x * 2
    adc $08
    pha
    txa
    adc $09
    tax
    pla
    rts
```

`main.c`:

<!-- guide: file main.c -->
```c
#include <stdio.h>

unsigned int times_ten(unsigned int x);     /* in times.s45 */

int main(void)
{
    printf("times_ten(42) = %u\n", times_ten(42));
    return 0;
}
```

<!-- guide: run -->
```console
$ ca45 -c times.s45 -o times.o45
Object file: times.o45 (129 bytes)
$ cc45 -O2 -c main.c -o main.o45
$ ln45 -basic -o main.prg main.o45 times.o45 -l c45.lib
Linked 2 objects to main.prg (2547 bytes)
```

<!-- guide: xemu main.prg screen=TIMES_TEN(42) = 420 -->

C's `times_ten` is `_times_ten` in assembly (cc45 puts `_` in front of C names). `ln45 -M main.map` also writes a
map of where everything went.

**ar45** keeps objects in a library (`.lib`); ln45 takes from a library only the members the program uses:

<!-- guide: run -->
```console
$ ar45 c mymath.lib times.o45
Created mymath.lib (1 member, 2 symbols, 187 bytes)
$ ar45 t mymath.lib
times.o45 (129 bytes)
$ ln45 -basic -o main.prg main.o45 mymath.lib -l c45.lib
```

## Looking inside: nm45 and objdump45

**nm45** lists an object's symbols: `T` code, `D` data, `B` uninitialized data, `U` needed from elsewhere, `W`
weak. `-f` adds what cc45 recorded about each function (its calling convention, the registers it changes):

<!-- guide: run -->
```console
$ nm45 times.o45
00000000 T _times_ten
...
$ nm45 -f main.o45
...
00000000 T _main [zp_call static_alloc ...
...
         U _printf
         U _times_ten
...
```

**objdump45** disassembles objects and programs (and shows a program's BASIC line):

<!-- guide: run -->
```console
$ objdump45 -d hello.prg
...
00002001 <BASIC stub>:
    2001: 0b 20 0a 00 9e 38 32 30 35 00 00 00 ; 10 SYS 8205 ($200d)
...
    200d: a2 00 ldx #$00
    200f: bd 1b 20 lda $201b,x
...
    2014: 20 d2 ff jsr CHROUT ; $ffd2
...
```

`objdump45 --html prog.html prog.prg` writes everything it knows about a program as a page to browse,
`--tree` the same in the terminal, `--cycles` the time each instruction takes.

## Linting and formatting: al45, af45 and cl45

**al45** checks assembly for mistakes ca45 accepts: a register written and never read, a flag tested that the
instruction before did not set, a stack left unbalanced. Its style rules (`mega65-style-*`, off by default) check
the layout:

`messy.s45`:

<!-- guide: file messy.s45 -->
```asm
; messy.s45: correct, but laid out by hand
  .global _clear_line
_clear_line:   ldx #39
        lda #32 ; a space
loop:  sta $0800,x
 dex
     bpl loop
   rts
```

<!-- guide: run -->
```console
$ al45 --checks='mega65-style-*' messy.s45
messy.s45:2:3: warning: '.global' at column 3, not 5 (or 1) [mega65-style-indent]
messy.s45:4:9: warning: instruction 'lda' at column 9, not 5 [mega65-style-indent]
...
messy.s45:8:4: warning: instruction 'rts' at column 4, not 5 [mega65-style-indent]
```

**af45** lays the file out as those rules say (only the white space changes; the code assembles to the same
bytes). Without `-w` it prints the result; `-w` rewrites the file and `--check` only says whether it would:

<!-- guide: run -->
```console
$ af45 messy.s45
; messy.s45: correct, but laid out by hand
    .global _clear_line
_clear_line:   ldx #39
    lda #32                 ; a space
loop:  sta $0800,x
    dex
    bpl loop
    rts
$ af45 -w messy.s45
$ af45 --check messy.s45
```

Instructions and directives move to column 5, the trailing comment to column 29; a label with an instruction
after it on its line keeps them as written.

**cl45** is the C linter, run with cc45's options:
[Getting Started with C](getting-started-c.md#9-next) shows it. Both linters read their settings from
`.cc45-lint.yml` ([cl45.md](../bin/cl45.md), [al45.md](../bin/al45.md)).

## Disk images: disk45

**disk45** makes and changes disk images: the MEGA65's D81, the C64's D64, the C128's D71 and 22 other formats,
by the file's extension.

<!-- guide: run -->
```console
$ disk45 create game.d81 -n "MY GAME" -i 01
Created D81 (1581) image: game.d81 (80 tracks, 819200 bytes)
$ disk45 add game.d81 main.prg GAME
Added "GAME" (2547 bytes, PRG)
$ disk45 list game.d81
0 "MY GAME" 01
11 "GAME" PRG
3149 blocks free.
$ disk45 extract game.d81 GAME copy.prg
Extracted "GAME" (2547 bytes)
```

`disk45 info`, `rename`, `remove`, `convert` (between formats), `mount` (an image as a directory, with FUSE),
`basic` (a BASIC program listed), `sd` (MEGA65 SD card images): `disk45` alone lists them all.

## Packed data: pack45

**pack45** packs a file for the library's `unpack()` (`<unpack.h>`), which unpacks it by DMA to any address of
the MEGA65's memory, banks 4 and 5 and attic RAM too. `-d` is where it will unpack to:

`message.txt`:

<!-- guide: file message.txt -->
```text
======================================
  PACKED, LOADED FROM DISK, UNPACKED
======================================
```

<!-- guide: run -->
```console
$ tr '\n' '\r' < message.txt > message.seq
$ pack45 -d '$40000' message.seq message.pak
pack45: message.seq: 115 -> ... bytes (...%), unpacks to $40000; in place: load it at $4... (offset ...)
$ pack45 -x message.pak check.seq && cmp message.seq check.seq
pack45: message.pak: 115 bytes, unpacking to $40000
```

`tr` first gives the text the MEGA65's line ends (a carriage return, 13, where the PC has a line feed). `-x`
unpacks on the PC, as a check. A packed file can be loaded and unpacked inside the area it unpacks into, at
the address pack45 prints. `--image` packs pictures, `--sound` turns sound files (WAV, MP3...) into samples for
the audio DMA ([pack45.md](../bin/pack45.md)). The program at the end of this page loads and unpacks
`message.pak`.

## Fonts: font45

**font45** turns fonts (BDF, PSF, TrueType, C64 character sets, images, or glyphs drawn in a text file) into 8x8
character sets for the text screen, or into fonts for `<font.h>`, which draws proportional text into bitmaps:

`arrows.txt`:

<!-- guide: file arrows.txt -->
```text
# arrows.txt: two hand-drawn 8x8 characters
baseline: 8
'<': ...##... ..##.... .##..... ######## ######## .##..... ..##.... ...##...
'>': ...##... ....##.. .....##. ######## ######## .....##. ....##.. ...##...
```

<!-- guide: run -->
```console
$ font45 list arrows.txt
glyphs: 2 (codes 60-62)
height: 8 (baseline 8)
...
$ font45 convert arrows.txt --charset screencode -o arrows.bin
font45: arrows.txt: an 8x8 charset, 256 characters, screencode order, 2048 bytes -> arrows.bin
$ font45 preview arrows.txt -o arrows.png
font45: arrows.txt: 2 glyphs -> arrows.png
```

`preview` draws the glyphs into a PNG, to look at them on the PC. `examples/c/fonts` shows `<font.h>` at work.

## Music: tune45

**tune45** makes music for the library's tune player (`<audio_tune.h>`) from a text of notes or from MIDI, MOD
and C64 SID files, and plays it on the PC (a SID emulation) or in xemu:

`tune.txt`:

<!-- guide: file tune.txt -->
```text
; tune.txt: a short tune for the tune player
instrument lead pulse 09 a6 0600 1 1
instrument bass tri   0a 00 0000 0 1
tune ditty 6 once
v1: @lead c5:4 e5:4 g5:4 c6:8 r:4 | g5:4 e5:4 c5:12
v2: @bass c3:12 g2:12 | c3:20
```

An `instrument` is a waveform and an envelope; a `tune` has a speed (frames per tick) and voices, each a list of
`note:ticks` (`r` a rest, `|` only for reading).

<!-- guide: run -->
```console
$ tune45 list tune.txt
ditty 2 voices 74 bytes 44 ticks 5.3 s 6.00 frames/tick once
...
$ tune45 convert tune.txt -o tune.c
$ tune45 play tune.txt --wav tune.wav
playing ditty: 5.3 s (via tune.wav)
```

`convert` writes `tune.c` and `tune.h` (`tunes[TUNE_DITTY]`) to compile with the program; `play` without
`--wav` plays through the PC's speakers. The player runs once a frame, usually from a raster interrupt:
`examples/c/tunes` is a complete program.

## Converting assembly: asmCvt45

**asmCvt45** converts source between ca45, ca65, ACME, KickAssembler, Merlin 64, Oscar, x65 and BSA, to bring
code written for another assembler over:

`wait.s65`:

<!-- guide: file wait.s65 -->
```asm
; a ca65 routine
.export _wait_frame
.proc _wait_frame
@loop:  lda $d012
        cmp #$f8
        bne @loop
        rts
.endproc
```

<!-- guide: run -->
```console
$ asmCvt45 -f ca65 wait.s65 -o wait.s45
$ cat wait.s45
.global _wait_frame
...
_wait_frame:
@loop:
...
$ ca45 -c wait.s45 -o wait.o45
Object file: wait.o45 (103 bytes)
```

`.export` became `.global`, and ca65's `.proc` a plain label. What a dialect has and ca45 has not is warned about;
[the conversion guide](../architecture/asmCvt45-guide.md) lists what each one keeps.

## Debugging and real hardware: dbg45 and deploy45

**dbg45** debugs a cc45 program by its C source in xemu: [Getting Started with C](getting-started-c.md#8-debugging)
has a session. **deploy45** sends a program or a disk image to a MEGA65 (over Ethernet or the JTAG adapter's
serial port, through mega65-tools) or to an Ultimate 64 (over the network). `--dry-run` prints what it would do:

<!-- guide: run -->
```console
$ deploy45 --dry-run m65ether main.prg
etherload -5 -r main.prg
$ deploy45 --dry-run m65mount game.d81
mega65_ftp -c 'put game.d81 GAME.D81' -c 'mount GAME.D81' -c exit
$ deploy45 --dry-run u64 -i u64.local main.prg
POST http://u64.local/v1/runners:run_prg (2547 bytes)
```

## Putting it together

A program in C and assembly that loads `message.pak` from its disk, unpacks it to bank 4 and prints it:

`title.c`:

<!-- guide: file title.c -->
```c
#include <stdio.h>
#include <load.h>
#include <unpack.h>

unsigned int times_ten(unsigned int x);         /* times.s45 */

int main(void)
{
    const char __far *text = (const char __far *)0x40000UL;
    unsigned long n, i;

    n = load_unpack("message", 7, 0, 0x48000UL);
    if (n == UNPACK_ERROR) {
        printf("no message: error %u\n", load_error);
        return 1;
    }
    for (i = 0; i < n; i++)
        putchar(text[i]);
    printf("%u bytes; times ten: %u\n", (unsigned)n, times_ten((unsigned)n));
    return 0;
}
```

`load_unpack` loads the file to `$48000` (bank 4) and unpacks it to the address pack45 recorded, `$40000`; a
`__far` pointer reads it there.

<!-- guide: run -->
```console
$ al45 times.s45
$ cc45 -O2 -c title.c -o title.o45
$ ln45 -basic -o title.prg title.o45 times.o45 -l c45.lib
...
$ disk45 create title.d81 -n TITLE
...
$ disk45 add title.d81 title.prg TITLE
...
$ disk45 add title.d81 message.pak MESSAGE
...
$ deploy45 --dry-run m65mount title.d81
mega65_ftp -c 'put title.d81 TITLE.D81' -c 'mount TITLE.D81' -c exit
```

<!-- guide: xemu title.prg disk=title.d81 screen=PACKED, LOADED FROM DISK, UNPACKED ====================================== 115 BYTES; TIMES TEN: 1150 -->

In xemu, `xemu-xmega65 -8 title.d81`, then `DLOAD "TITLE"` and `RUN` (or `xemu-xmega65 -8 title.d81 -prg
title.prg`); on a MEGA65, the `deploy45 m65mount` above puts the disk on its SD card and mounts it. The screen:

```text
======================================
  PACKED, LOADED FROM DISK, UNPACKED
======================================
115 BYTES; TIMES TEN: 1150
```

## Next

- Each tool's reference page in [../bin/](../bin/), and [../README.md](../README.md) for the rest of the
  documentation.
- [The examples](../examples/README.md): games and demos using the library and these tools.
