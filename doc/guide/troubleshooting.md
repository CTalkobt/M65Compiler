# Troubleshooting

Messages and symptoms a new user meets, what causes them and what to do. Each is reproduced below with the
tools as they are (`src/test/test_guides.py` runs this page). Terms: [glossary.md](glossary.md).

- [ca45: "has no base-page mode"](#has-no-base-page-mode)
- [ROM calls from assembly](#rom-calls-from-assembly): the program hangs in the ROM
- [Plain `int` is unsigned](#plain-int-is-unsigned): `x < 0` is never true
- [ln45: no heap, or little room for the stack](#no-heap-or-little-room-for-the-stack)
- [xemu and the MEGA65 ROM](#xemu-and-the-mega65-rom)

## "has no base-page mode"

A cc45 program runs with a base page of its own (crt0 moves the B register to `__zp_page`; see
[the relocated base page](../architecture/calling-conventions.md#62-relocated-base-page)). An instruction
written with a one-byte address uses the base page, so `lda $40` reads `__zp_page + $40`. Some instructions
have no base-page form; ca45 assembles them with the absolute address `$0040`, which is page 0 of bank 0, not
the program's base page, and says so.

`bp.s45`:

<!-- guide: file bp.s45 -->
```
count = $40
        .org $2000
start:
        ldz count
        lda count,y
        jmp (count)
```

<!-- guide: run -->
```console
$ ca45 bp.s45 -o bp.bin
bp.s45:4:13: warning: 'ldz $40' has no base-page mode: assembled as absolute $0040, which ignores the base page register
bp.s45:5:13: warning: 'lda $40,y' has no base-page mode: assembled as absolute $0040,y, which ignores the base page register
bp.s45:6:14: warning: 'jmp ($40)' has no base-page mode: its pointer is read at absolute $0040, which ignores the base page register
```

The linter reports the same as the rule `mega65-no-bp-mode` (`al45 bp.s45`; [al45.md](../bin/al45.md)).

The instructions: `ldz $xx`, `lda $xx,y` / `sta $xx,y` (only `ldx`/`stx` have a base-page `,y`), `phw $xx` (pushes
the word at absolute `$00xx`), and `jmp ($xx)` / `jmp ($xx,x)` / `jsr ($xx)` / `jsr ($xx,x)` (the pointer read at
absolute `$00xx`, page 0 of bank 0). What to write instead:

| Instead of | Write |
|------------|-------|
| `ldz $40` | `lda $40` then `taz`, or `ldq $40` (all four registers) |
| `lda $40,y` | `lda $40,x` (with the index in X), or a pointer: `lda ($40),y` |
| `jmp ($40)` | the pointer copied to a pointer outside page 0 first, then `jmp (vec)` |

An address written with its leading zeros (`ldz $0040`, `phw $0040`, `jmp ($0040)`) is absolute on purpose (page
0 of bank 0, as the ROM's variables are) and is not warned about.

`.extern` symbols in the base page are assembled as absolute addresses too (ca45 does not know they are in the
base page): the library's `.s45` files define their base-page temporaries as constants (`tmp = $F6`).

## ROM calls from assembly

The ROM's routines (the KERNAL's `CHROUT` at `$FFD2`, BASIC's float routines...) expect the base page at 0.
Called with the program's base page, they read and write the program's variables as if they were the ROM's:
the program usually hangs in the ROM. A C program reaches the ROM through the library (`putchar`, `<cbm.h>`),
which switches; hand-written assembly must do it itself.

`say.s45`, a C-callable function (ZP convention: the `char` argument in `$10`) that forgets:

<!-- guide: file say.s45 -->
```
        .global _say
proc _say
        .func_flags zp_call
        lda $10
        jsr $FFD2
        rts
endproc
```

`rom.c`:

<!-- guide: file rom.c -->
```c
#include <stdio.h>

void say(char c);

int main(void)
{
    int i;
    for (i = 0; i < 3; i++) say('A' + i);
    printf("\ndone\n");
    return 0;
}
```

<!-- guide: run -->
```console
$ cc45 -O2 -c rom.c -o rom.o45
$ ca45 -c say.s45 -o say.o45
$ ln45 -basic -o rom.prg rom.o45 say.o45 -l c45.lib
Linked 2 objects to rom.prg (...)
```

ca45 and ln45 say nothing. Run in xemu, the screen shows nothing after `RUN:` and the exit status at
`$FD/$FE` stays `$FFFF` (`__exit` never reached). The assembly linter finds it:

<!-- guide: run -->
```console
$ al45 -c say.s45
say.s45:5:1: warning: 'jsr $FFD2' calls the ROM with the program's base page: bracket it with jsr __bp_rom / jsr __bp_prog, or use __rom_call [mega65-rom-call-bp]
```

The fix: `jsr __bp_rom` before the call and `jsr __bp_prog` after it (crt0's; both keep A, X, Y, Z, and
`__bp_prog` the flags, which ROM routines return status in):

<!-- guide: file say.s45 -->
```
        .global _say
        .extern __bp_rom
        .extern __bp_prog
proc _say
        .func_flags zp_call
        lda $10
        jsr __bp_rom
        jsr $FFD2
        jsr __bp_prog
        rts
endproc
```

<!-- guide: run -->
```console
$ al45 -c say.s45
$ ca45 -c say.s45 -o say.o45
$ ln45 -basic -o rom.prg rom.o45 say.o45 -l c45.lib
```

<!-- guide: xemu rom.prg screen=DONE -->

The KERNAL's disk routines also need the page-1 stack, which a cc45 program has moved (the 16-bit stack):
call them through `__rom_call` (`stdlib_asm/rom_call.s45`), which switches both. From C, `__jsr` does it
for you: `__jsr(0xFFD2, .A = c);` calls through `__rom_calli` ([cc45.md](../bin/cc45.md#calling-an-address-__jsr-__jsr_raw-__jmp)).
An `__interrupt` handler's ROM calls are reported by cl45's `mega65-isr-rom-call`.

## Plain `int` is unsigned

In cc45, plain `char`, `short`, `int` and `long` are **unsigned** unless declared `signed`. This is deliberate, but a program
written for standard C can go wrong without an error. `sign.c`:

<!-- guide: file sign.c -->
```c
#include <stdio.h>

int main(void)
{
    int t = 10 - 20;
    char c = 200;
    if (t < 0) printf("negative\n");
    else printf("t = %d, as unsigned %u\n", t, t);
    printf("c / 2 = %d\n", c / 2);
    return 0;
}
```

<!-- guide: run -->
```console
$ cc45 -O2 sign.c -o sign.prg
sign.c:7:11: warning: 't' is unsigned (plain int is unsigned in cc45): 't < 0' is always false; declare it 'signed' (or use -fsigned-default)
```

<!-- guide: xemu sign.prg screen=T = -10, AS UNSIGNED 65526 -->

It prints `T = -10, AS UNSIGNED 65526` (`%d` shows the bits as signed) and `C / 2 = 100`. cc45 warns about the
comparison it can prove always false; the division is just unsigned. cl45 reports the same as
`mega65-signedness`.

Two fixes:

- Declare what can be negative `signed` (`signed int t`), or use `<stdint.h>`'s `int16_t`, `int8_t`... (declared
  `signed`). The library is built this way.
- Compile as standard C: `-fsigned-default` (or `#pragma cc45 signed_default` in the file). `char` follows too;
  `-funsigned-char` keeps `char` unsigned.

<!-- guide: run -->
```console
$ cc45 -O2 -fsigned-default sign.c -o sign.prg
```

<!-- guide: xemu sign.prg screen=NEGATIVE -->

It prints `NEGATIVE` and `C / 2 = -28`. Keep one choice for all of a program's files: a header's `int` means
the same as the file including it.

## No heap, or little room for the stack

A program lives in bank 0, from `$2001` to the end of its BSS (with its 256-byte base page last). Above it is
the heap (`malloc`), up to the **stack share**, 4096 bytes below the stack top `$BFFF` by default: the heap ends
at `$B000`. ln45 checks what is left when it links (cc45 runs it in a one-step build;
[ln45.md](../bin/ln45.md#6-stack-room-check)). `heap.c`, a program with a large table that uses `malloc`:

<!-- guide: file heap.c -->
```c
#include <stdio.h>
#include <stdlib.h>

#ifndef N
#define N 36000
#endif
char table[N];

int main(void)
{
    char *p = malloc(100);
    table[0] = 1;
    printf("%s\n", p ? "got it" : "no memory");
    return 0;
}
```

<!-- guide: run -->
```console
$ cc45 -O2 heap.c -o heap.prg
ln45: warning: no heap: the program ends at $BA00, at or above the heap's end $B000 (4096 bytes below the CPU stack top for the stack); malloc returns NULL. A smaller stack share (--stack-size, cc45 -fstack-size) or a heap elsewhere (__heap_start/__heap_end) makes room
$ cc45 -O2 -DN=33000 heap.c -o heap.prg
ln45: warning: only 512 bytes for the heap between the program end ($AE00) and $B000
```

The first program prints `NO MEMORY`. A program that needs less stack can give the heap more: `-fstack-size=N`
(cc45; `#pragma cc45 stack_size(N)` in the source; ln45 `--stack-size=N`):

<!-- guide: run -->
```console
$ cc45 -O2 -fstack-size=1024 heap.c -o heap.prg
ln45: warning: only 512 bytes for the heap between the program end ($BA00) and $BC00
```

<!-- guide: xemu heap.prg screen=GOT IT -->

With a stack share chosen, a program that reaches into it is an error:

<!-- guide: fails -->
```console
$ cc45 -O2 -DN=37000 -fstack-size=1024 heap.c -o heap.prg
ln45: error: program ends at $BE00, inside the 1024-byte stack share ($BC00-$BFFF): make the program smaller, or the share smaller (--stack-size, cc45 -fstack-size)
```

A program that does not use `malloc` has no heap to check, but the stack must still fit: ln45 warns when
fewer than 512 bytes are left between the program's end and the stack top, and stops when the program
reaches it.

<!-- guide: file stack.c -->
```c
#include <stdio.h>

char table[40000];

int main(void)
{
    table[0] = 1;
    printf("hello\n");
    return 0;
}
```

<!-- guide: run -->
```console
$ cc45 -O2 stack.c -o stack.prg
ln45: warning: only 256 bytes left for the CPU stack between the program end ($BF00) and $BFFF
```

Where to find room: `ln45 -M prog.map` (in a one-step build `cc45 -Wl,-M,prog.map`) writes the map, with the
size of each segment and of each object's part in it; large tables can
go in banks 4-5 or attic RAM (`__bank(n)`, `far_malloc`: [cc45.md](../bin/cc45.md#__bankn),
[stdlib.md](../architecture/stdlib.md)); code in overlays or `__far` functions
([ln45.md](../bin/ln45.md#code-overlays)). The heap can also be put anywhere with `__heap_start` /
`__heap_end` ([cc45.md](../bin/cc45.md#heap)). A program can never end above `$BFFF`: `$C000`-`$CFFF` reads as
ROM in C65 mode, and ln45 stops with *"program ends at $C200, above the end of bank 0's RAM $BFFF"*.

## xemu and the MEGA65 ROM

The runtime tests, the tutorials' screens, dbg45 and `basic45 --emulator` run programs in xemu
(`xemu-xmega65`). xemu needs, in its preferences directory (`~/.local/share/xemu-lgb/mega65/`, or
`$XEMU_PREF_DIR` with the Docker images' and CTalkobt/xemu's `cc45-dev` build):

- **an SD card image**, `mega65.img`. Without one, a headless xemu stops at once (exit status 1):

  ```
  ERROR: Cannot find SD-card image (which is a must for MEGA65 emulation): @mega65.img
  UI: returning #0 (No) for choice in dialog box, as dialogs are NOT allowed!
  ```

  Started once with a window, xemu offers to make one.
- **the MEGA65 ROM**, `MEGA65.ROM` (128 KB; from your MEGA65's SD card, or [files.mega65.org](https://files.mega65.org);
  it cannot be distributed with the suite), on the SD card image or in the preferences directory. Without it
  xemu boots its built-in Open ROMs: the screen says `OPEN ROMS BASIC & KERNAL`, the program is never run
  and no test passes.

disk45 makes the image as the MEGA65's FDISK does, configured as after the first-boot onboarding (else the
hypervisor waits in it for keys), and puts the ROM on it (the image is a sparse 4 GB file):

```bash
P=~/.local/share/xemu-lgb/mega65
mkdir -p $P
disk45 sd create $P/mega65.img 4G --onboarded --pal
disk45 sd add $P/mega65.img /path/to/MEGA65.ROM MEGA65.ROM
cp /path/to/MEGA65.ROM $P/
```

Check it with a program:

```bash
cc45 -O2 hello.c -o hello.prg
timeout 15 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpscreen screen.txt -prg hello.prg
grep -i hello screen.txt
```

xemu exits at BASIC's `READY.`; `-dumpmem out.mem` also writes memory, where the exit status is at `$FD/$FE`
(0: success; `$FFFF`: `__exit` never reached; `$FFFE`: `abort()`).

dbg45 needs an xemu whose monitor has the breakpoint list (CTalkobt/xemu, branch `cc45-dev`); the Docker
image `ghcr.io/ctalkobt/m65compiler:runtime` has it and sets xemu up from a ROM you mount
(`-v /path/to/MEGA65.ROM:/rom/MEGA65.ROM:ro`; [DOCKER.md](../architecture/DOCKER.md)).
