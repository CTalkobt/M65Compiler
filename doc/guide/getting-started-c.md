# Getting Started with C

This page takes you from nothing to a C program running on the MEGA65: in the xemu emulator, from a disk image and
on the real machine. Then a program in several files, what cc45's C does differently, the hardware, and
debugging. Every command here is run as written by `make test` (`src/test/test_guides.py`), so what you see is
what the tools print.

The reference for everything below is [cc45.md](../bin/cc45.md); the other tools have their pages in
[../bin/](../bin/).

## 1. The tools

Any one of these:

- **A release archive** (Linux x86-64 or ARM64, macOS on Apple silicon) from the
  [release page](https://github.com/CTalkobt/M65Compiler/releases): unpack it anywhere and put its `bin/` on your
  `PATH`. The tools find their headers and libraries next to themselves.

  ```bash
  tar -xzf m65compiler-<version>-linux-x86_64.tar.gz
  export PATH="$PWD/m65compiler-<version>-linux-x86_64/bin:$PATH"
  ```

- **Built from source** (a C++17 compiler and GNU make; nothing else is needed):

  ```bash
  git clone --recurse-submodules https://github.com/CTalkobt/M65Compiler.git
  cd M65Compiler
  make && make install_local          # into ~/.local/bin; or make install (PREFIX=/usr/local)
  ```

- **Docker**, with nothing else installed: `docker run --rm -v "$PWD":/work --user $(id -u):$(id -g)
  ghcr.io/ctalkobt/m65compiler -O2 hello.c -o hello.prg` runs cc45 on the files of the current directory
  ([DOCKER.md](../architecture/DOCKER.md); the `:runtime` image has xemu too).

Check that it works:

<!-- guide: run -->
```console
$ cc45 --version
cc45 v...
```

To run programs on the PC you need **xemu** (`xemu-xmega65`, [github.com/lgblgblgb/xemu](https://github.com/lgblgblgb/xemu))
and the MEGA65 ROM (`MEGA65.ROM`, from your MEGA65's SD card or [files.mega65.org](https://files.mega65.org); xemu
looks for it in `~/.local/share/xemu-lgb/mega65/`).

## 2. A first program

`hello.c`:

<!-- guide: file hello.c -->
```c
#include <stdio.h>

int main(void)
{
    printf("hello, mega65!\n");
    return 0;
}
```

One command compiles, assembles and links it with the standard library:

<!-- guide: run -->
```console
$ cc45 -O2 hello.c -o hello.prg
$ ls hello.prg
hello.prg
```

`-O2` is the optimization level (`-O0` to `-O4`, `-Os` for size; `-O2` is the default, but say it: a personal
`~/.config/m65/cc45.conf` can change the default). `hello.prg` is a MEGA65 program: it starts with a BASIC line
`10 SYS ...`, so `RUN` starts it.

## 3. Running it

In xemu:

```bash
xemu-xmega65 -prgmode 65 -prg hello.prg
```

xemu boots the MEGA65, loads the program and runs it: `HELLO, MEGA65!` is on the screen, then `READY.`. cc45 writes
strings in PETSCII, the Commodore character set, where lower-case letters are the ones the screen shows as
capitals in the upper-case mode BASIC starts in (press SHIFT and the MEGA65 key together, or print `"\x0e"`, for
the lower-case font; [cc45.md](../bin/cc45.md#string-and-character-encodings) has the encodings).

<!-- guide: xemu hello.prg screen=HELLO, MEGA65! -->

On a MEGA65 connected by Ethernet, with mega65-tools' `etherload` installed, deploy45 sends the program and runs
it ([deploy45.md](../bin/deploy45.md) has the serial cable and the Ultimate 64 too). `--dry-run` shows what it
would run:

<!-- guide: run -->
```console
$ deploy45 --dry-run m65ether hello.prg
etherload -5 -r hello.prg
```

## 4. On a disk image

A program the MEGA65 loads from its drive goes on a D81 image (the MEGA65's 3.5" disk; it mounts images from
its SD card):

<!-- guide: run -->
```console
$ disk45 create hello.d81 -n HELLO
$ disk45 add hello.d81 hello.prg HELLO
$ disk45 list hello.d81
0 "HELLO" CC
2 "HELLO" PRG
3158 blocks free.
```

In xemu, with the image as drive 8: `xemu-xmega65 -8 hello.d81`, then at the `READY.` prompt
`DLOAD "HELLO"` and `RUN`. On the MEGA65, copy `hello.d81` to the SD card, `MOUNT "HELLO.D81"`, then the same.

## 5. A program in several files

A larger program is several C files compiled one at a time to objects (`.o45`), then linked by ln45. A header
declares what the files share:

`score/score.h`:

<!-- guide: file score/score.h -->
```c
#ifndef SCORE_H
#define SCORE_H

void score_add(unsigned int points);
unsigned int score_get(void);

#endif
```

`score/score.c`:

<!-- guide: file score/score.c -->
```c
#include "score.h"

static unsigned int score;

void score_add(unsigned int points)
{
    score += points;
}

unsigned int score_get(void)
{
    return score;
}
```

`score/main.c`:

<!-- guide: file score/main.c -->
```c
#include <stdio.h>
#include "score.h"

int main(void)
{
    unsigned char i;

    for (i = 1; i <= 10; i++)
        score_add(i * 10);
    printf("score: %u\n", score_get());
    return 0;
}
```

<!-- guide: run -->
```console
$ cd score
$ cc45 -O2 -c main.c -o main.o45
$ cc45 -O2 -c score.c -o score.o45
$ ln45 -basic -o game.prg main.o45 score.o45 -l c45.lib
```

`-c` stops at the object; `ln45 -basic` writes the BASIC start line and links the objects with the standard
library, `c45.lib` (`-l` finds it next to the tools), which holds the start-up code too.

<!-- guide: xemu score/game.prg screen=SCORE: 550 -->

The program prints `score: 550`. A Makefile keeps the steps:

`score/Makefile`:

<!-- guide: file score/Makefile -->
```make
CFLAGS = -O2

game.prg: main.o45 score.o45
	ln45 -basic -o $@ $^ -l c45.lib

%.o45: %.c score.h
	cc45 $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o45 game.prg
```

<!-- guide: run -->
```console
$ make clean && make
...
cc45 -O2 -c main.c -o main.o45
cc45 -O2 -c score.c -o score.o45
ln45 -basic -o game.prg main.o45 score.o45 -l c45.lib
```

The examples in [`examples/c/`](../examples/README.md) are built the same way, from `hello_world` and
`multi_module` up to games.

## 6. What is different from other C compilers

cc45 compiles C for a CPU with 8-bit registers and 64 KB banks, and a few of its choices follow from that:

- **`char`, `short`, `int` and `long` are unsigned unless declared `signed`.** Unsigned arithmetic is shorter and
  faster on the 45GS02. `int x = -1;` stores 65535, and `x < 0` is never true. Write `signed int`, use
  `<stdint.h>`'s `int16_t` (declared `signed`), or compile with `-fsigned-default` for standard C.
- **Sizes**: `char` 1 byte, `short` and `int` 2, `long` 4, pointers 2; `float`, `double` and `long double` are
  the 5-byte (40-bit) floating point of Commodore BASIC, computed by the MEGA65's ROM.
- **Structs are packed**: no padding between members (`_Alignof(int)` is 1).
- **Calling conventions**: arguments go in a block of zero page by default (`-fzpcall`), on the stack with
  `-fno-zpcall` (with the library `c45_stack.lib`); functions that are not recursive keep their locals in static
  storage. You rarely need to think about it, but it matters when you mix C with assembly:
  [calling-conventions.md](../architecture/calling-conventions.md).

The program below shows the first three:

`diff.c`:

<!-- guide: file diff.c -->
```c
#include <stdio.h>

struct point { char tag; int x, y; };

int main(void)
{
    int u = -1;
    signed int s = -1;

    printf("u<0:%d s<0:%d\n", u < 0, s < 0);
    printf("int:%u long:%u float:%u point:%u\n",
           (unsigned)sizeof(int), (unsigned)sizeof(long), (unsigned)sizeof(float), (unsigned)sizeof(struct point));
    return 0;
}
```

<!-- guide: run -->
```console
$ cd ..
$ cc45 -O2 diff.c -o diff.prg
```

<!-- guide: xemu diff.prg screen=U<0:0 S<0:1 -->
<!-- guide: xemu diff.prg screen=INT:2 LONG:4 FLOAT:5 POINT:5 -->

It prints `u<0:0 s<0:1` and `int:2 long:4 float:5 point:5`.

## 7. The hardware

`<mega65.h>` names the MEGA65's registers as C structs (`vic4->border` is `$D020`), and `<conio.h>` is cc65's
console library: colours, cursor positioning, keys.

`colours.c`:

<!-- guide: file colours.c -->
```c
#include <conio.h>
#include <mega65.h>

int main(void)
{
    unsigned char c;

    clrscr();
    vic4->border = COLOR_BLACK;
    for (c = 0; c < 16; c++) {
        textcolor(c);
        gotoxy(c * 2, 5);
        cputs("##");
    }
    textcolor(COLOR_WHITE);
    gotoxy(0, 7);
    cputs("sixteen colours");
    gotoxy(0, 9);
    return 0;
}
```

<!-- guide: run -->
```console
$ cc45 -O2 colours.c -o colours.prg
```

<!-- guide: xemu colours.prg screen=SIXTEEN COLOURS -->

The library's headers are listed in [stdlib.md](../architecture/stdlib.md): files (`<stdio.h>` on the disk drive,
`<cbm.h>`), far memory beyond 64 KB (`<far_ptr.h>`, `__far` pointers), DMA, sprites, graphics (`<tgi.h>`),
sound and music (`<audio_tune.h>`), the real-time clock and the joystick (`<joystick.h>`, `<input.h>`).

## 8. Debugging

`-g` writes a debug file next to the program (`game.dbg`), and dbg45 debugs the program by its C source in xemu:
breakpoints, stepping, the call stack and variables. It needs an xemu with a breakpoint list (the `cc45-dev`
branch of [CTalkobt/xemu](https://github.com/CTalkobt/xemu), which the Docker images build; see
[dbg45.md](../bin/dbg45.md)).

<!-- guide: run -->
```console
$ cd score
$ cc45 -O2 -g -c main.c -o main.o45
$ cc45 -O2 -g -c score.c -o score.o45
$ ln45 -g -basic -o game.prg main.o45 score.o45 -l c45.lib
$ ls game.dbg
game.dbg
```

`dbg45 game.prg` starts xemu with the program and gives a `(dbg45)` prompt, where the commands are gdb's:
`break`, `run`, `bt`, `print`, `next`, `step`, `continue`, `info locals`, `quit`. The same commands can be given
on the command line, as here (`--headless`: no xemu window; `--batch`: quit after them):

<!-- guide: run xemu -->
```console
$ dbg45 --headless --fast --batch -ex "break score_add" -ex run -ex bt -ex "print points" -ex continue game.prg
(dbg45) break score_add
Breakpoint 1 at $...: score_add
(dbg45) run
Breakpoint 1, score_add (points=10) at score.c:7
7 score += points;
(dbg45) bt
#0 score_add (points=10) at score.c:7
#1 main () at main.c:9
(dbg45) print points
points = 10
(dbg45) continue
Breakpoint 1, score_add (points=20) at score.c:7
```

In VS Code, the extension in `tools/ide/vscode-extension` debugs through dbg45 (`--dap`).

When the program goes wrong without a debugger, `cc45 -S` shows the assembly it made, and
`CC45_DUMP_IR=1 cc45 ...` the intermediate code: [cc45.md](../bin/cc45.md).

## 9. Next

- **The linter**: `cl45` checks C as cc45 compiles it, with rules for the MEGA65 ([cl45.md](../bin/cl45.md)):

  <!-- guide: run -->
  ```console
  $ cd ..
  $ cl45 -O2 diff.c
  diff.c:7:13: warning: 'u' is unsigned (plain int is unsigned in cc45): -1 becomes 65535; declare it 'signed' (or use -fsigned-default) [mega65-signedness]
  diff.c:10:33: warning: 'u' is unsigned (plain int is unsigned in cc45): 'u < 0' is always false; declare it 'signed' (or use -fsigned-default) [mega65-signedness]
  ```

  It found the unsigned `int` of section 6. Warnings do not fail the run (exit status 0); `--checks` chooses
  the rules, `.cc45-lint.yml` configures them.

- [The examples](../examples/README.md), with their screens and sources.
- [cc45.md](../bin/cc45.md): every option, the language extensions (far pointers, `__fixed` point, `__bit`,
  interrupts, overlays for programs larger than 64 KB).
- [The other tools](getting-started-tools.md): the assembler and linker, disk images, fonts, music.
- [BASIC with basic45](getting-started-basic45.md), which can call your C functions.
