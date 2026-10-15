# Examples

The programs in [`examples/`](https://github.com/CTalkobt/M65Compiler/tree/main/examples),
each with what it shows and its screen 2.5 seconds after it starts (xemu-xmega65,
real speed). Build them all with `make -C examples` (or `make` in one example's
directory; most have `make run` to start xemu). The pictures are made by
[`examples/capture_screens.py`](https://github.com/CTalkobt/M65Compiler/blob/main/examples/capture_screens.py)
(run from the project root after building the examples).

The MEGA65's own boot screen and `READY.` prompt show in the pictures of the
programs that print text and return to BASIC.

## The examples disk

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/disk) —
`make -C examples/disk` builds every example and puts them on one D81 disk
image, `examples/disk/examples.d81`, with
[disk45](../bin/disk45.md): `create`, an `add` for each program (named as
in [`programs.txt`](https://github.com/CTalkobt/M65Compiler/blob/main/examples/disk/programs.txt)),
`validate` and `list`. The menu,
[`menu.bas`](https://github.com/CTalkobt/M65Compiler/blob/main/examples/disk/menu.bas),
is a BASIC 65 program written in [basic45](../bin/basic45.md)'s source form
(labels, no line numbers). It is saved as `AUTOBOOT.C65`, so a MEGA65 that
starts with the disk mounted runs it, and as `MENU`. A key loads and starts an
example with `DLOAD`: inside a running program it loads the next one and runs
it, and every example is a BASIC `SYS` line at `$2001` followed by its code.
When an example ends, `RUN "MENU"` brings the menu back. `make -C examples/disk run`
starts xemu with the disk in drive 8; on a MEGA65, copy the D81 to the SD card
and mount it.

![The examples disk's menu](images/disk_menu.png)

## Games and demos

### invaders

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/invaders) —
A Space Invaders game. The game objects are structs with single inheritance
and virtual `update()`/`draw()`, positions an operator-overloaded `Vec2`. The
character set is copied to bank 4 with `dma_copy_far` and redefined (aliens,
shields); the cannon and UFO are 16-colour VIC-IV sprites; SID sound effects (through `<audio_sid.h>`)
and the marching beat. An attract mode plays by itself (shown); FIRE or SPACE
starts a game, Z/X, `,`/`.`, the cursor keys or a joystick move, RUN/STOP quits.
The high score is kept on disk: `INVADERS.HI`, a sequential file on device 8
holding the score in decimal, read with `fopen`/`fread` at the start and
written with `fwrite` after a game that beat it (a missing file or a disk
error leaves it at 0; the attract mode's score does not count).

![invaders](images/invaders.png)

### mandelbrot

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/mandelbrot) —
A Mandelbrot set viewer: the VIC-IV's full-colour character mode as a
320x192 bitmap (a byte per pixel in bank 4), the iteration in 45GS02
assembly (the 32-bit Q register and the hardware multiplier, 4.28 fixed
point; points in the main cardioid and the period-2 bulb are recognised
without iterating), progressive rendering with border checks (an 8x8 cell
whose border is one colour is filled; otherwise its four 4x4 quadrants are
checked the same way; the picture is still filling in at 2.5 seconds),
palette cycling. Cursor keys or a joystick move,
Z/X zoom, `?` shows the keys.

![mandelbrot](images/mandelbrot.png)

### musicgen

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/musicgen) —
A random music generator in seven styles (pop, rock, blues, jazz,
classical, ambient, chiptune): Markov chord tables with fixed cadences, AABA
or 12-bar blues form, melodies built from a motif with chord tones on the
strong beats, voice-led chords, walking/boogie/Alberti bass and drums with
swing, played by 14 SID instruments on four SIDs (`<audio_sid.h>`: SIDs 1-2 mirrored to 3-4 for stereo). Keys 1-7 choose the style,
`+`/`-` the tempo, N a new song. `make check` composes songs of every style
and checks them against the chords and scales.

![musicgen](images/musicgen.png)

### calc_graph

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/calc_graph) —
A scientific calculator that plots. An expression (+ - * / ^, parentheses,
implicit multiplication like `2x` or `3sin(x)`, `x`, `pi`, `e`, `ans`, sin cos
tan atn sqrt log exp abs int) is compiled to bytecode by a shunting-yard
parser and run by an evaluator that dispatches with computed goto
(`goto *ops[*pc++]`), in cc45's ROM-backed floats; with `x` in it, it is
plotted (up to four curves) on a full-colour 320x128 graph. A raster
interrupt colours the graph area's border and background and counts frames:
an `__interrupt` handler behind a stub at the KERNAL's `$0314` vector, which
also handles interrupts that arrive while the float routines have the BASIC
ROM mapped over the program. Shows GCC extensions: computed goto, nested
functions, statement expressions with `typeof`, case ranges, designated
initializers, `?:`, `__attribute__`. The demo at the start computes two
values and plots three curves (shown); a marker runs along the newest one.
(xemu's exit screenshot is rendered without the CPU, so it shows no raster
split.)

![calc_graph](images/calc_graph.png)

### rtc_clock

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/rtc_clock) —
The real-time clock in big digits (a 5x7 font drawn with reverse spaces),
the weekday and date, a bar of the seconds and a stopwatch counted in video
frames. The RTC's BCD registers at `$FFD7110`-`$FFD7116` lie outside the
16-bit address space, so a `dma_copy_far` copies them into a buffer each
frame. H switches 12/24 hours, S starts and stops the stopwatch, R resets
it, C changes the colours. `make check` reads the RTC twice, a second
apart, and checks the values.

![rtc_clock](images/rtc_clock.png)

### sample_player

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/sample_player) —
A drum machine on the audio DMA (`<audio_digi.h>`). It needs no sample
files: at start it synthesizes a kick (a falling sine sweep), a snare (a
sine under noise), a hi-hat (high-passed noise) and a one-cycle sine tone
as 8-bit samples at 8 kHz, then plays them on the four audio DMA channels,
placed in the stereo field with `digi_set_volume`. The 16-step pattern
starts playing at once (SPACE stops and restarts it; four patterns, P); keys
1-3 trigger the drums, + and - set the
tempo, T loops the tone and Q/W change its pitch (the playback rate).
`make check` synthesizes the samples, starts each channel and checks the
data and the registers.

![sample_player](images/sample_player.png)

### keyboard_viewer

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/keyboard_viewer) —
The MEGA65 keyboard drawn with conio; every frame each key of the 8x8
matrix is read with `key_pressed()` (several at once, SHIFT and CTRL too)
and a held key shows reversed in yellow. Below, conio's `kbhit()` and the
characters `getch()` returns, with their PETSCII codes. It is the
interactive test of `kbhit`/`getch` (headless xemu can't press keys): run it
in a windowed xemu or on a MEGA65. RUN/STOP quits.

![keyboard_viewer](images/keyboard_viewer.png)

### rogue

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/rogue) —
A mini rogue-like: each level is a new random dungeon (a 3x3 grid of rooms,
some only a corridor crossing, joined along a random spanning tree plus a
few extra corridors), rooms light up when entered and what you have seen
stays on the map. Monsters (rats to dragons, by depth) wake when they see
you and chase you; walk into one to fight. Potions heal, weapons and armour
improve attack and defence, experience raises your level; the amulet waits
on depth 8. Monsters and items are structs with single inheritance
(`struct Monster : Thing`) and a method. Cursor keys or h/j/k/l and y/u/b/n
move, `>` goes down. `make check` tests levels from fixed seeds
(everything reachable, nothing overlapping), scripted moves, a fight and a
3000-step random walk.

![rogue](images/rogue.png)

### forth

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/forth) —
A tiny Forth: a calculator you can program. Numbers go on a 16-bit stack,
words act on it (`2 3 + 4 * .` prints 20); `: sq dup * ;` defines a new word,
with `if`/`else`/`then`, `do`/`loop`/`i`, `begin`/`until`, variables and
constants; `*/`, `sqrt`, `hex`/`decimal`. A definition compiles to threaded
code that the inner interpreter runs with GCC's computed goto, one jump per
word (`goto *ops[code[ip++]]`); statement expressions, case ranges and a
nested function appear too. The picture shows the demo lines it runs at
start; type more at the keyboard, BYE or RUN/STOP returns to BASIC.

![forth](images/forth.png)

### far_scroller

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/far_scroller) —
A 400x25 landscape map allocated with `far_malloc` in bank 4, outside the
program's 64 KB, scrolled with one `dma_copy_far` per row each frame (the sky
at half speed: parallax); colours written straight into colour RAM at
`$FF80000` with `dma_fill_far`.

![far_scroller](images/far_scroller.png)

### text_scroller

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/text_scroller) —
A horizontal text scroller in C and assembly (arguments through shared
variables, so both calling conventions work), with a bouncing ball (hardware
sprite 0, VIC-IV 16-bit sprite pointers) that passes in front of and behind
the reverse-video message.

![text_scroller](images/text_scroller.png)

### game_of_life

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/game_of_life) —
Conway's Game of Life on the 80x25 text screen: 2D arrays, signed neighbour
offsets, direct screen RAM writes.

![game_of_life](images/game_of_life.png)

## Graphics and hardware registers

### palette_fade

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/palette_fade) —
C and assembly together: an assembly routine scales the VIC-IV palette's R/G/B
values by a fade level the C code steps up and down (shown mid-fade).

![palette_fade](images/palette_fade.png)

### vic4_colors

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/vic4_colors) —
A volatile struct overlaid on the border and background colour registers
(`$D020`/`$D021`), stepped through a few colours (shown part way), ending on
white with black text.

![vic4_colors](images/vic4_colors.png)

### bitfield_registers

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/bitfield_registers) —
VIC-III/VIC-IV control registers (`$D031`, `$D054`) as bitfield structs: reads
and changes single bits by name, enables sprites, writes characters with
VIC-III attributes.

![bitfield_registers](images/bitfield_registers.png)

### memset_screen

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/memset_screen) —
Fills screen RAM with each character code 0-255 in turn with `memset`.

![memset_screen](images/memset_screen.png)

## Programs that print

### hello_world

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/hello_world) —
The smallest program: `puts("hello world")`, built in one step with cc45.

![hello_world](images/hello_world.png)

### hello_linked

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/hello_linked) —
The same with separate compile and link steps (`cc45 -c`, `ln45`) and a few
library calls (`itoa`, `strlen`).

![hello_linked](images/hello_linked.png)

### multi_module

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/multi_module) —
A program in three C files with a shared header, compiled separately and
linked with ln45; prints the results of its vector functions and its own
checks.

![multi_module](images/multi_module.png)

### unit_convert

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/unit_convert) —
Miles/kilometres conversion in 32-bit fixed point (`long`), with `sprintf` and
`sscanf`.

![unit_convert](images/unit_convert.png)

### ctype_demo

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/c/stdlib/ctype_h) —
`<ctype.h>` on PETSCII: classifies a few characters and converts their case.

![ctype_demo](images/ctype_demo.png)

## BASIC and C

### c_routines

[Source](https://github.com/CTalkobt/M65Compiler/tree/main/examples/basic/c_routines) —
A BASIC 65 program (basic45 source with labels) that calls C functions:
`basic45 --link routines.c menu.bas` compiles the C file with cc45 and links it
after the BASIC program. The picture shows its results and the prompt for the
next part.

![c_routines](images/c_routines.png)

## Library checks

These programs check standard library functions and return 0 (in A, and as
the exit status at `$FD/$FE`) when they all pass; they print nothing, so
their screen is BASIC's `READY.`.

| Program | Checks | Source |
|---------|--------|--------|
| test_atoi | `atoi` | [stdlib_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/stdlib_h/test_atoi.c) |
| test_itoa | `itoa` | [stdlib_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/stdlib_h/test_itoa.c) |
| test_memops | `memset`, `memcpy`, `memcmp` | [string_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/string_h/test_memops.c) |
| test_strchr | `strchr`, `strrchr` | [string_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/string_h/test_strchr.c) |
| test_strcmp | `strcmp` | [string_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/string_h/test_strcmp.c) |
| test_strcpy | `strcpy` | [string_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/string_h/test_strcpy.c) |
| test_strlen | `strlen` | [string_h](https://github.com/CTalkobt/M65Compiler/blob/main/examples/c/stdlib/string_h/test_strlen.c) |

![test_strlen](images/test_strlen.png)

`math_h` (`gcd`/`lcm`, `min`/`max`) is built and run by its own Makefile, not
by `make -C examples`.

## Assembly

[`examples/asm/zpcall_prototype.s45`](https://github.com/CTalkobt/M65Compiler/blob/main/examples/asm/zpcall_prototype.s45)
is a stand-alone assembly prototype of the ZP parameter-block calling
convention from early in the project (parameters at `$03`-`$12`; cc45's
convention now uses `$10`-`$2F`, see
[calling conventions](../architecture/calling-conventions.md)): nested calls,
a 32-bit result in A/X/Y/Z, an ltoa-style conversion.
