# Getting Started with basic45

basic45 turns a BASIC 65 program written in a text editor into a MEGA65 program (`.prg`), the same tokens BASIC
itself stores. The source needs no line numbers: lines are named by labels, variables can have long names, and
there are block `IF`, procedures with parameters and `SELECT`. The same tool puts programs on disk images,
lists and converts old programs, compiles BASIC to machine code and links C functions into a BASIC program.

Every command here is run as written by `make test` (`src/test/test_guides.py`). The reference is
[basic45.md](../bin/basic45.md); BASIC 65 itself is described in the MEGA65 Book
([files.mega65.org](https://files.mega65.org)).

## 1. The tools

basic45 comes with the rest of the suite: a release archive, a build from source or the Docker image, as in
[Getting Started with C](getting-started-c.md#1-the-tools). Check it:

<!-- guide: run -->
```console
$ basic45 --version
basic45 v...
```

Programs run in the xemu emulator (`xemu-xmega65`, with the MEGA65 ROM) or on a MEGA65.

## 2. A first program

`hello.bas`:

<!-- guide: file hello.bas -->
```basic
' hello.bas: a first program
  print "hello from basic45"
  for count = 1 to 3
    print "line"; count
  next
```

A line starting with `'` is a comment, dropped from the program; indentation is only for reading.

<!-- guide: run -->
```console
$ basic45 hello.bas -o hello.prg
$ basic45 --decompile hello.prg -o -
print "hello from basic45"
for co = 1 to 3
print "line"; co
next
```

`--decompile` reads a `.prg` back as source: here it shows what BASIC 65 got. The lines were numbered 10, 20, 30,
40 and `count` became `CO`: BASIC 65 tells variables apart by their first two letters, so basic45 gives every
longer name two letters of its own.

Run it in xemu (it loads and runs the program), or let basic45 start xemu (`--emulator`):

```bash
xemu-xmega65 -prgmode 65 -prg hello.prg
basic45 hello.bas -o hello.prg --emulator
```

<!-- guide: xemu hello.prg screen=HELLO FROM BASIC45 LINE 1 LINE 2 LINE 3 -->

The screen shows `HELLO FROM BASIC45` and the three lines. Lower-case letters in the source are what BASIC 65's
screen shows as capitals; `{clear}`, `{red}`, `{home}` and the other `{...}` names in strings are the control
characters ([Strings and Escape Sequences](../bin/basic45.md#strings-and-escape-sequences)).

On a MEGA65 connected by Ethernet, `--send` loads and runs it there (mega65-tools' `etherload`;
[deploy45](../bin/deploy45.md) has the other ways).

## 3. What the source adds to BASIC 65

This program counts the primes below a limit with the sieve of Eratosthenes:

`primes.bas`:

<!-- guide: file primes.bas -->
```basic
#define LIMIT 200
' primes.bas: how many primes are there below LIMIT?
  dim sieve(LIMIT)
  found = 0
  for number = 2 to LIMIT
    if sieve(number) = 0 then
      call mark(number)
      found = found + 1
    endif
  next
  print "primes below"; LIMIT; ":"; found
  end

proc mark(prime)
  multiple = prime * 2
  do while multiple <= LIMIT
    sieve(multiple) = 1
    multiple = multiple + prime
  loop
endproc
```

- **`#define`**, `#include`, `#ifdef`: a preprocessor, as C's ([Preprocessor](../bin/basic45.md#preprocessor)).
- **Long names**: `sieve`, `number`, `multiple`.
- **Block `IF`**: `if ... then` with nothing after `THEN` opens a block, up to `endif` (with `else` if wanted).
- **Procedures**: `proc mark(prime)` ... `endproc`, called with `call mark(number)`; the arguments are assigned to
  the parameters, which are ordinary variables. `return <value>` and `call name(...) to v` return a result.
- **Labels**: `name:` at the start of a line names it, for `goto name`, `gosub name`, `then name` or
  `restore name` (this program needs none).
- BASIC 65's own `DO WHILE ... LOOP` and `BEGIN ... BEND` work as they are.

`--list` shows each line BASIC gets, its number and address, and the renamed variables:

<!-- guide: run -->
```console
$ basic45 --list primes.bas -o primes.prg
Variables: FOUND->FO MULTIPLE->MU NUMBER->NU PRIME->PR SIEVE->SI
   10  $2001  primes.bas:3      dim sieve(LIMIT)
...
   50  $203C  primes.bas:7      call mark(number)
...
$ basic45 --decompile primes.prg -o -
...
  if si(nu) <> 0 then l70
  pr = nu : gosub l100
...
```

The block `IF` became an `IF` on the opposite condition that jumps past the block, and `call` a `GOSUB` after the
argument's assignment.

<!-- guide: xemu primes.prg screen=PRIMES BELOW 200 : 46 -->

It prints `PRIMES BELOW 200 : 46`.

`--validate` checks a program (undefined labels, blocks left open) without writing anything, and `--report` shows
its memory use:

<!-- guide: run -->
```console
$ basic45 --validate primes.bas
$ basic45 --report primes.bas -o primes.prg
BASIC program $2001-$20... bytes, 15 lines
...
```

## 4. On a disk

`--disk` puts the program on a disk image as well (created when missing; a D81 is the MEGA65's own format):

<!-- guide: run -->
```console
$ basic45 primes.bas -o primes.prg --disk primes.d81
$ disk45 list primes.d81
0 "PRIMES" CC
1 "PRIMES" PRG
```

In xemu, `xemu-xmega65 -8 primes.d81`, then `DLOAD "PRIMES"` and `RUN` (`--emulator` with `--disk` does this
with the image as drive 8). On the MEGA65, copy the image to the SD card and `MOUNT "PRIMES.D81"`. `incbin` and
`datainc` take a program's data from files, and disk45 ([Getting started with the other
tools](getting-started-tools.md#disk-images-disk45)) adds data files to the image.

## 5. Old programs

A `.prg` from a disk, or saved from BASIC, becomes source with `--decompile`, as above. A program for another
Commodore BASIC is converted to BASIC 65 with `--convert`: `basic7` (the C128), `basic35` (the C16 and Plus/4),
`simons` (Simons' BASIC on the C64), `basic8`; from its `.prg` or a text listing:

`c128.txt`:

<!-- guide: file c128.txt -->
```text
10 scnclr
20 for i=1 to 5
30 print "c128 line";i
40 next
50 fast:sleep 1:slow
60 print "done"
```

<!-- guide: run -->
```console
$ basic45 --convert=basic7 c128.txt -o c128.bas
$ cat c128.bas
...
  scnclr
  for i = 1 to 5
  print "c128 line"; i
  next
  : fast : sleep 1 : speed 1
  print "done"
  end
l62: if rwindow(2) = 80 then print chr$(27) + "x";
  return
$ basic45 c128.bas -o c128.prg
```

<!-- guide: xemu c128.prg screen=C128 LINE 5 DONE -->

BASIC 7.0's `SLOW` is `SPEED 1` in BASIC 65, and the C128 program expects a 40-column screen, so the conversion
adds a subroutine that switches an 80-column screen to 40 (`chr$(27) + "x"`). What a conversion cannot do by
itself is warned about, and listed in comments at the top ([Converting](../bin/basic45.md#converting-basic-70-programs---convertbasic7)).

## 6. Compiling to machine code

`--compile` compiles the program to machine code, through cc45's code generator: it runs as the interpreter runs
it, only faster. The `.prg` is started with `RUN` like any other.

<!-- guide: run -->
```console
$ basic45 --compile primes.bas -o primes-fast.prg
```

<!-- guide: xemu primes-fast.prg screen=PRIMES BELOW 200 : 46 -->

With `LIMIT` at 5000, the interpreter takes 4.6 seconds on xemu, the compiled program 1.8, and 1.2 with
`--speed` (floating point by the library's routines in RAM instead of the ROM's). The compiled program is larger
(4.5 KB here, against 182 bytes of BASIC): it carries the run time it uses.

- `--checks` compiles the interpreter's checks of the program's values (`?ILLEGAL QUANTITY`, subscripts, `RETURN`
  without `GOSUB`...); without it, a compiled program takes them as tested. `--line-numbers` keeps the line
  numbers for error messages (` IN 110`) and `EL`.
- `proc name compiled` compiles one procedure and leaves the rest interpreted
  ([Compiled Procs](../bin/basic45.md#compiled-procs-proc--compiled)).
- Most of BASIC 65 compiles; what does not (`MERGE` and the other statements that change the program's text) is
  an error at build time ([Compiling](../bin/basic45.md#compiling---compile)).

## 7. Calling C

`--link` compiles C (or assembly) files with the program, and `SYS {name}` calls a C function:

`squares.c`:

<!-- guide: file squares.c -->
```c
/* squares.c: the sum of the squares 1..n, for BASIC */
unsigned int sum_squares(unsigned char n)
{
    unsigned int total = 0;
    unsigned char i;

    for (i = 1; i <= n; i++)
        total += i * i;
    return total;
}
```

`squares.bas`:

<!-- guide: file squares.bas -->
```basic
' squares.bas: BASIC calling a C function
  sys {sum_squares}, 10 : rreg lo, hi
  print "sum of squares 1-10:"; lo + 256 * hi
```

<!-- guide: run -->
```console
$ basic45 --link squares.c squares.bas -o squares.prg
```

<!-- guide: xemu squares.prg screen=SUM OF SQUARES 1-10: 385 -->

The SYS's A, X and Y registers (`sys addr, a, x, y`) are the function's first three parameters, and its result
comes back in A and X, read with `RREG`. `{name}` alone is the address of a C symbol (`peek({counter})`). See
[Calling C Functions](../bin/basic45.md#calling-c-functions---link) and
[Getting Started with C](getting-started-c.md).

## 8. Next

- [basic45.md](../bin/basic45.md): every option; `select` / `case`, local labels, `asm` blocks, `--minify`,
  `--watch` (rebuild on every change), the `--compile` details.
- [The examples](../examples/README.md): `examples/basic/c_routines` is a BASIC program calling C.
- [Getting Started with C](getting-started-c.md) and [the other tools](getting-started-tools.md).
