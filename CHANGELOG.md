# Changelog

All notable changes to the cc45 / ca45 suite will be documented in this file.

## [Unreleased]

### cc45

- **Parameter narrowing** (`-fparam-narrowing`, from `-O1`): an `int`
  parameter of a `static` function that every call passes 0-255 (a
  constant, an `unsigned char`, `x & 0xFF`) is passed as a byte, when each of
  its uses costs nothing zero-extended (a byte use, a 16-bit add/sub,
  and/xor or comparison). A parameter used otherwise (multiplied, stored,
  passed on, ...) keeps its type, so the function does not grow. Test
  `test_param_narrowing_xemu.c`.
- **Zero-extended bytes read in place**: a byte variable or parameter
  zero-extended for one 16-bit add/sub, and/xor or comparison is read where
  it lives, with 0 for its high byte (`lda x; clc; adc c; bcc *+3; inx`,
  `cpx #0; bne +; cmp c`), not first widened into a 16-bit temporary. LICM
  leaves such an extension at its use. 1115 bytes smaller over the test
  programs and examples (ZP convention), 1330 with parameter narrowing.
- **Char sums keep their carry**: `+ - * <<` of chars are ints, also as
  the operand of another char operation: `a + (b + c)` and `a + b + c` of
  unsigned chars cut `b + c` (or `a + b`) back to 8 bits (200 + 200 + 200
  gave 344).
- **Narrow arguments widen by their own signedness**: an `unsigned char`
  passed to a `signed int` or `signed long` parameter was sign-extended
  (200 arrived as -56); this hit every `int` parameter with
  `-fsigned-default` (the library is built with it). A constant still
  takes the parameter's signedness (`f(-1)` to a `signed long` is -1).
  Test `test_char_promotion_xemu.c`.

### ca45 / ln45

- **Redundant loads kept for their flags** (Issue #282): the
  redundant-load pass removed an `lda`/`ldx`/`ldy`/`ldz` whose register
  already held the value although the load's N/Z were read: `cr = c == 13;
  if (c == 13 ...)` reloaded the boolean for a `bne` after an `ldx #0`, and
  the branch tested X (a CR was never recognised at `-O2`). Its guard
  against a following branch compared lowercase mnemonics with uppercase
  names and never applied. A load now goes only when the flags already
  reflect the register or are set again before anything can read them (a
  branch on N/Z, `php`, a jump, a return, a call to a function not known
  to set them). The flag tracking also records that `pla`/`plx`/`ply`/`plz`
  set N/Z, and that `plp`, `bit` and the instructions it does not model
  change them. Tests `test_ca45_redundant_load_xemu.s45`,
  `test_redundant_load_flags_xemu.c`.
- **Weak absolute symbols**: `.weak x` / `x = value` is used at its value
  in the defining object; such an object now records the ones its code
  used (`OPT_WEAKUSE`), and ln45 refuses a program that gives one of them
  another value (before, the object silently kept its own value while the
  rest of the program got the new one). Overriding a default nothing uses
  stays possible.
- **`.extern` expressions**: an imported symbol in a form a relocation
  cannot hold (`ext * 3`, `.word ext / 2`), where its value is needed while
  assembling (`.res ext`, `x = ext + 1`), or in a form the assembler does
  not relocate (`#(ext + 2)`, `.byte <ext`) is an error; these were
  assembled with the import's placeholder 0 or a relocation of the wrong
  value, without a word.
- **Data after a return**: `.byte`/`.word`/... (and the code after them)
  right after an `rts`/`rti` with no label in between was skipped as dead
  code, leaving zeros (or nothing at a segment's end) where the parser had
  placed it.

### objdump45 / nm45 / ln45 / cc45

- **A directory as input**: `objdump45 -h <directory>` (any option, and
  `-D`), `nm45 <directory>` and `ln45 <directory>` (also `-l <directory>`)
  ended with an uncaught `std::ios_failure`; they now report "is a
  directory" and exit with status 1 (objdump45 and nm45 go on with the
  other files). ln45's library search skips a directory with the library's
  name. `objdump45 -D` also exits with 1 when it cannot open its files (it
  exited with 0).
- **cc45 with a directory as input**: `cc45 <directory>` (and cp45) read
  it as an empty source file (a one-step build then failed with "undefined
  symbol '_main'"); it is now an error ("Input file is a directory"). The
  `#include` search skips a directory with the header's name.

## [v1.1.0] - 2026-10-16

Programs can be larger than bank 0: code overlays (`#pragma cc45
overlay(name)`, copied into a region or mapped into a window with MAP) and
`__far` functions (kept in far memory and mapped in when called, packed by
the call graph), with their file inspected by nm45 and objdump45 and shown
in the linker map. Much of the standard library was rewritten in assembly
(conio, the scanf engine, strtol/strtoul, strtok, qsort/bsearch, clock,
rand); cc45 generates smaller code for constant addresses and byte joins;
ca45, ln45 and cp45 fixes. xemu runtime tests 220 → 295, GCC torture tests
537 → 552 in each convention. Details: `doc/bin/*.md`, `doc/architecture/`.

### cp45

- A function-like macro call whose arguments go on over the next lines is
  expanded whole: the lines are joined first (a blank line out for each,
  line numbers kept). It used to expand with the arguments of the first
  line only, or not at all.

### `__far` functions

- `__far` on a function keeps its code in far memory: cc45 emits it in the
  text sub-segment `far`, and ln45 packs each file's far code into automatic
  mapped overlays (`far1`, `far2`, …; 8 KB windows, 16 KB when a file needs
  more) reached through the overlay stubs. The packing follows the call
  graph: files whose far code calls each other share an overlay when they
  fit. No pragma or option is needed;
  the specifier can come anywhere among the others, and only the definition
  needs it. Far functions are not inlined; a far `__interrupt` handler is a
  link error, and so is any overlay function (far or `#pragma cc45 overlay`)
  an interrupt handler reaches through its calls, directly or through
  function pointers (the overlay runtime is not reentrant; ln45 names the
  chain of calls). New example `examples/c/far_functions` (`far functions`
  on the examples disk). xemu tests in both conventions, with and without function
  sections (`src/test-resources/far/`).
- `setjmp`/`longjmp` work across code overlay calls: `jmp_buf` grows to 8
  bytes; `setjmp` saves the overlay return stack's depth and the overlay in
  the region, and `longjmp` drops the overlay calls it leaves and brings
  that overlay back (hooks `__ovl_jsave`/`__ovl_jrestore`: `ovl.s45`, empty
  weak ones in crt0). Before, each `longjmp` out of overlay calls left
  entries on the 32-deep overlay return stack, and a `longjmp` back into
  overlay code could run in the wrong overlay.
- `far_malloc` leaves the code overlays' store alone: ln45 defines where
  the overlay file lies in far memory (`__ovl_store_bank`/`_first`/`_last`)
  and the allocator never hands out that part of its bank (bank 5 by
  default; it used to overwrite the overlays once it reached bank 5).
- ln45: a code overlay is now a property of each text chunk, not of a whole
  object, so one object can have resident code and code in overlays.
- **ca45 fixes** (found with `__far`): a reference into a later text
  sub-segment (`far`, `init`) held the target's address in its own segment
  instead of its place in the object's text; a `JSR`/`JMP` into another
  segment could become a relative `BSR`/`BRA`; line info in a later text
  sub-segment, or before a function's static storage, was placed at wrong
  offsets; two `proc`s at the same address in different segments lost one
  function attribute record.

### Code overlays

- A program can be larger than bank 0: a file with `#pragma cc45 overlay(name)`
  (cc45 `-foverlay=name`, ca45 `.overlay name`) has its code in that overlay.
  ln45 links every overlay to run at one region below the stack's share (the
  heap ends there), writes their code to `<program>.ovl` (loaded from the
  program's drive as `<PROGRAM>.OVL` into bank 5 at start: `--overlay-store`,
  `--overlay-name`, `--overlay-out`), and sends every reference from outside
  an overlay through a resident stub: `__ovl_call` (`stdlib_asm/ovl.s45`)
  copies the overlay in by DMA and swaps the return address, so both calling
  conventions, thunks, function pointers and calls between overlays work.
  `<overlay.h>`: `__ovl_current`, `__ovl_region`. New example
  `examples/c/overlays` (on the examples disk as `overlays`); xemu tests in
  both conventions and link errors (an interrupt handler in an overlay, an
  overlay too large).
- New example `examples/c/mapped_overlays` (`map overlays` on the examples
  disk) checks that mapped overlays run in place. The CPU sees each
  overlay's code in the window, and the overlay reports what MAP maps there.
  DMA reads and writes the same bytes in bank 5, a store through the window
  is visible there and the other way round, and an overlay calling another
  gets the window back. With copied overlays only 2 of the 6 checks pass.
- The examples disk's menu has two pages of 13 examples (space turns the
  page): 23 examples no longer fit on one screen.
- Mapped overlays (ln45 `--overlay-map`): each overlay runs where it is
  kept, through a window of `$4000`–`$7FFF` that `MAP` points at it (only
  the map's lower half changes: the KERNAL stays mapped at `$E000`); no
  copying. The program ends below the window, the heap starts above it.
- crt0 calls `__ovl_init` (weak, empty without overlays) after
  `_init_features`, `__ovl_rom_leave` after ROM calls (`__bp_prog`) and
  `__ovl_exit` in `__exit`.
- The linker map (`-M`) shows where the overlays run and where each is
  kept (bank and addresses). It lists overlay objects at their run
  addresses and tags each stub with its overlay. New sections, Overlay
  Symbols and Overlay Lines, list each overlay's symbols and source lines
  separately; each symbol shows its run address, where it is kept and
  its stub.
- The overlay file ends with a description of its overlays and symbols:
  nm45 lists the symbols at their run addresses with their overlay (`-h`
  the overlays), objdump45 shows a section per overlay (`-f -h -t -s -d`,
  disassembly with labels).
- **ca45 fix**: a value loaded with `lda #>sym` and read back as an
  immediate (`cpx #>sym`, from the known zero-page value) kept the wrong
  byte for the linker's carry, so the high byte could be off by one with
  some layouts (seen in `test_parse_fixes`).
- **ca45 fix**: `lda #sym` of an external symbol assembled as `#0` without
  a relocation; it is now relocated as `#<sym`.

### cc45

- **Fixes**: a `__naked` function returning a value assembled with a
  branch to an epilogue it does not have (`undefined local symbol
  '@__return'`); `static` after `__naked`, `__interrupt`, `__fastcall__`,
  `_Noreturn` or `__regparm` was dropped (the function was exported under
  its plain name: two files with the same static function did not link);
  these specifiers may now come in any order.
- **Code size**: loads through a constant address are absolute
  (`lda $D012`, was through a zero-page pointer); a value put together from
  bytes (`a | b << 8 | (long)c << 16`, byte swaps, `(x & 0xFF00) | lo`) is
  built byte by byte from where each byte lives; an operation on two
  constants, also a variable assigned a constant once, is folded. The
  library is 1.35 KB smaller (`clock` 464 → 218 bytes, conio's `scr_addr`
  430 → 220, `__builtin_bswap16` 65 → 11). `test_bytejoin_xemu.c`.
- **Fix**: `volatile T *p` made the variable `p` volatile (kept in memory,
  its constant value hidden); now only what it points to is, and
  `T * volatile p` the variable (parameters too).
- **Fix**: the DMA job starts (`dma_copy_far` / `dma_fill_far`, crt0's BSS
  fill, ca45's `__dma_copy` / `__dma_fill` expansion) set the DMA registers
  with interrupts held off: an interrupt in between could leave the list
  address of its own, so a far copy failed depending on where code landed
  (`test_far_string_xemu.c` failed after unrelated size changes).
- **Speed** (benchmark `src/test/bench/run_bench.sh`, ZP convention: 1344 ms
  at v1.0.6 → 763 ms): leaf `char`/`int` locals in ZP; shifts for element
  strides 2/4/8; constant element stores; stack frames made by moving SP;
  `a[i + c]` as element `i` of a shifted base; the element address kept in
  `__zp_scratch` and reused by later loads and stores of the same array and
  index (across ZP stores, pointer accesses, compares, branches and into a
  block with a single predecessor).
- **Warnings**: a variable of a plain integer type (unsigned by cc45's
  default) warns when a negative constant is stored in it or compared with
  it, and on `x < 0` / `x >= 0`: `'r' is unsigned (plain int is unsigned in
  cc45): -1 becomes 65535; declare it 'signed' (or use -fsigned-default)`.
  Checked before constant folding, which used to remove such comparisons.
- **Fixes**: `_Alignas` anywhere among the declaration specifiers
  (`_Alignas(8) static int x;`); the comma operator in `for` clauses; the
  preprocessor substitutes each macro parameter once (an argument's text was
  scanned for later parameters' names: `F(dst, src, len)` with
  `src = g(len)` passed `g(<len's argument>)`); `<float.h>` constants exact;
  `_Complex int` division; operators on small-struct temporaries; a struct
  operand without an operator is an error; no false "unused" warning;
  `x op= y` on a struct calls its `operator op=` (a struct of 4 bytes or
  less was added as a 32-bit integer, carrying from one member into the
  next), and is an error without one; `sizeof x[i]` / `sizeof *p` in
  constant expressions (`int a[sizeof t / sizeof t[0]]` got 2 bytes, so
  writing it overran into the next object); a non-constant array size at
  file scope, `static` or in a later declarator of a declaration
  (`int a, b[n]`) is an error (it was a 2-byte object or 1 element).
  A struct member that is an array of pointers (`const char *d[3]`):
  `s->d[i]` loaded one byte and `s->d[i][k]` scaled `k` by 2.
- `__int(8)`, `__int(16)`, `__int(32)` and `__uint(...)` are the native
  `char`, `int` and `long` (`__int(32)` was an "Unknown struct" error); only
  wider ones include `<intwide.h>`.
- Bitfields over 16 bits in a `long`: a narrower value stored in one is
  widened first (a `short` -55 in a `signed long : 29` read back positive);
  a struct passed by value to an inlined function keeps a memory copy (its
  members were read through the address of a register temporary).
- Struct methods: a method called on a global object (`g.set(9)`) gets the
  object's address as `this` (it got its contents); a method with an
  `unsigned`/`signed` return type parses; a base struct's method is found
  through a derived struct (`struct D : B`).
- Nested functions: the enclosing function's variables keep their types (a
  `char *` was read 16 bits wide, an array was treated as a pointer); a
  nested function's return type can be `unsigned char`, `unsigned long`.
- The preprocessor expands `F()` of a macro without parameters.
- Code size: 16-bit shifts by a constant and array index scaling shift the
  high byte in a scratch byte (`asl; rol s`) instead of `asl; pha; txa;
  rol; tax; pla` per bit; a byte index times 2 is `asl; bcc; inx`; a byte
  index times another constant (3, 5, 10, 13...) is shifts and adds instead
  of the hardware multiplier (24 bytes). The examples are 802 bytes smaller.
- Division by powers of two: 16-bit right shifts by a constant use a
  scratch byte (`lsr`/the 45GS02's `asr` on base page, `ror`) instead of
  pushes and pulls per bit, 9-15 bits move the high byte; a signed `x / 2^k`
  and `x % 2^k` add the bias 2^k - 1 to a negative x (from its sign) and
  shift or mask instead of calling the division routine; a `char` divided
  by a constant is reduced too (strength reduction runs again after the
  passes that make the promoted divisor a constant). The examples are
  1112 more bytes smaller.
- A block reached only by falling through from the conditional branch
  before it gets no label (a switch's next comparison): the switch value is
  not reloaded before every case. A `char` parameter of a statically
  allocated function gets one byte at the call (not the slot's unused high
  byte, from a stale X). ca45 drops an `ldx #imm` whose X is written again
  before it is read (cc45's zero extension of a byte used as a byte). The
  examples are 1060 more bytes smaller.
- An element of a global array of at most 256 bytes (1- or 2-byte elements,
  strides 1, 2, 4) is `lda name+off,y` / `sta name+off,y` with the scaled
  index in Y, instead of an address built in `__zp_scratch` and `(zp),y`.
  The examples are 4554 more bytes smaller (forth by 16%).
- `goto *table[i]` through a static label table of at most 256 bytes is
  `asl; tax; jmp (table,x)` (the 45GS02's indexed indirect jump) instead of
  the table entry patched into a self-modified `jmp`: about 16 bytes less
  per dispatch and much faster (forth 570 bytes smaller, calc_graph 207).
- A comparison used as a value is built from the flags without branches:
  `lda #0; rol` (and `eor #1`) for `<` / `>=`, `php; pla; lsr; and #1` for
  `==` / `!=`. `g += c` on a global (`count++`, `sp += 2`) is done in place:
  `inc g` / `inc g; bne; inc g+1`, or an add on the variable, not through
  temporaries. Loop-invariant code motion leaves constants and global
  addresses (`&g + c`) where they are used (hoisted they were variables live
  across the loop, kept in memory across its calls and copied to zero page
  for each use); pointer temporaries dereferenced across calls are kept in
  the function's base-page block. Loop-invariant code motion moves only
  instructions with a variable operand (`k * 7`): constants, global
  addresses (`&g + c`) and what is computed from them alone stay where they
  are used (they cost nothing there; hoisted they were variables live across
  the loop, kept in memory across its calls). CSE carries multiplies, element
  addresses and divisions into a block whose one predecessor is the block
  before it. A function's base-page block holds up to 16 bytes (the library
  is built with 8). The examples are 2263 more bytes smaller.
- ln45: calls after a computed-goto label (cc45 exports `f__lbl_x` for
  `&&x` in a table) are the function's: the static-allocation overlay gave
  a computed-goto interpreter and the functions it calls the same storage.
- Block scope: a variable declared in an inner block, a `for` header or a
  statement expression hides an outer one of the same name only there
  (after `{ int a = 7; }` the outer `a` was the inner one).
- Array designators take any integer constant expression (`[ENUM]`,
  `[(2)]`, `[1 + 1]`); an unsized array is as long as its highest
  designated element (`{ [2] = 5, [0] = 1 }` had 2 elements).
- Nested functions: a recursive one ran forever (its static link shared a
  frame slot with its parameter); a forward-declared one (`int h(int);`
  or `auto int h(int);`) is the nested function, not an external; no
  "unused variable `__static_link`" warning.
- Struct layout: `sizeof(type)`, `sizeof` in an array dimension, a
  variable's storage and the member offsets now come from one layout
  (`include/StructLayout.hpp`). An `__unpacked` struct aligns members to
  their natural alignment (a nested struct to its own, not its size) and is
  padded at the end (`{char a; long l; char c;}` is 12 bytes, was 9);
  `sizeof` of a bitfield struct was the sum of its member types (5 for 3
  bytes); `char buf[sizeof(struct S)]` got 2 bytes for any struct; a
  flexible array member took one element in a variable but not in `sizeof`.
  `struct __unpacked S v;` declares a variable (was a parse error).
- `#pragma encoding` (and `asm(".encoding ...")`) also sets the encoding
  of the unprefixed character constants that follow: `'A'` is `$41` under
  `ascii` and 1 under `screencode` (was always PETSCII).
- A struct member `const T *p` (pointer to const) was a const member:
  `s->p++` and `s->p = q` were errors. Only `T *const p` is.
- An undeclared name in a static initializer is an error (it was stored
  as 0). The preprocessor treated `"\\"` as ending in an escaped quote: the
  macros after such a string on the same line were not expanded.
- A commutative operation (`+ * & | ^`) with the constant first is
  simplified like one with it second: `0 ^ x` and `1 * x` were kept (`1 * x`
  called the multiply routine), `4 * x` is a shift like `x * 4`. A
  constant of another width (`x ^ 0` on a `char` or `long`: the 0 is
  converted first) is seen as a constant; all ones in `&`/`|` is recognized
  also when written `-1`; the simplification runs again after type
  narrowing (`x & x` on a `char`).
- A local or parameter assigned a constant exactly once, in the entry block
  before anything reads it, is that constant to the IR passes (`one = 1;
  one * x` is `x`, `n = 4; ... n * i` a shift). Not for variables whose
  address is taken, volatile ones, ones a nested function reaches, register
  variables, or in functions with inline assembly (`test_constprop`).
- scanf with floats is linked like printf's: `_scanf_float` is referenced
  when `scanf`/`fscanf`/`sscanf` gets a `float *`, or a unit with float code
  calls `vscanf`/`vfscanf`/`vsscanf`. `clock`, `rand` and `srand` are no
  longer treated as assembly ZP routines (they are C now).

### Standard library

- **The scanf engine in assembly** (`stdlib_asm/scanf_core.s45`): 4.8 KB →
  1.7 KB; a program using `sscanf("%d")` 9972 → 6794 bytes. Its callbacks
  (`__scan_getc`, `__scan_ungetc`, `__scan_float`) are `__fastcall__`, so no
  bridge thunks in the stack library. New `test_scanf_xemu.c`: integers and
  their prefixes, sizes (`hh`…`ll`, `%n`), widths, sets, suppression, the
  counts returned, floats (the same results as the C engine gave).
- **More of the library in assembly**: `clock` (222 → 32 bytes), `rand` /
  `srand` (336 → 114; the LCG's multiply on the hardware multiplier),
  `strtoul` / `strtol` (1733 → 677), `strtok` / `strtok_r` (584 → 136),
  `qsort` / `bsearch` (2.2 KB → 1.2 KB, about 16% faster; the comparator is
  called through a small C function compiled in each calling convention).
  All `__fastcall__` in their headers. `test_qsort_xemu.c` now tests
  `bsearch`, `test_stdlib2_xemu.c` the reference `rand` sequence and
  `strtok_r`.
- **conio in assembly** (`stdlib_asm/conio.s45`; `cprintf`/`cprintfxy` stay
  in C as `conio_printf.c`): a row's screen and colour RAM addresses are
  32-bit pointers (row × stride by the hardware multiplier) and characters
  go out with flat stores (`sta [$08],z`) instead of a long multiply and a
  DMA job per character. 4000 characters with scrolling: 16 frames → 1; a
  program using `cputs` 8374 → 1848 bytes. Clearing and scrolling are DMA
  jobs. `test_conio_xemu.c` also checks a 40-column stride and an 80x50
  screen in bank 4.

- Library functions without tests got runtime tests (`test_stdlib2`,
  `test_math2`, `test_time2`, `test_cbmio`, `test_stdio3`), which found:
  - `strtol`/`strtoul`: upper-case hex digits in a C string (`"0x1F"`: cc45
    writes `F` as `$66`) and `0X` were not accepted; no `ERANGE` on
    overflow; `strtoul` ignored a `-`.
  - `<ctype.h>`: `'A'`–`'Z'` as cc45 writes them (`$61`–`$7A`) were not
    upper-case letters (`isupper('A')` was 0); `tolower` maps them too.
  - `atof`/`strtof`: fractions were summed from 0.1, 0.01, … (`"-0.25"` was
    not -0.25); now digits form an exact integer scaled by exact powers of
    ten; `endptr` is the start when there are no digits.
  - `srand` did nothing: after it, `rand` is C's reference generator (a
    seed repeats its sequence); before it, the hardware RNG as before.
    `RAND_MAX` is defined.
  - `atexit` handlers never ran: `exit()` and a return from `main` call
    them (up to 32, in reverse order). A failed `assert`/`assert_msg` calls
    `abort()` (exit status `$FFFE`) instead of `BRK`.
  - `div_t`/`ldiv_t`/`div`/`ldiv` are declared in `<stdlib.h>`.
  - `ldexpf`/`frexpf` set the exponent byte (exact, `ERANGE` out of range);
    `log2f` is exact for powers of two.
  - `clock()` returned BCD minutes:hours: the C65 KERNAL's `RDTIM` reads the
    CIA time-of-day clock, it has no jiffy clock. `clock` counts video
    frames (the KERNAL's counter at `$110C`), `clock_t` is `unsigned long`
    and `CLOCKS_PER_SEC` the frame rate (50 PAL / 60 NTSC). `time()` is
    the RTC in seconds since 2000 (was the jiffy value); `gmtime`/
    `localtime` convert their argument (they returned the RTC); `mktime`
    normalizes out-of-range fields and dates before 2000; `time_t` is
    `signed long`.
  - `cbm_seek`/`cbm_tell` were stubs and `cbm_eof` read the last status:
    files track their position, seek forward by reading and back by
    reopening. `cbm_readdir` parsed the directory listing as raw 32-byte
    entries (nothing matched): it reads the listing's lines. `CBM_T_USR`/
    `REL`/`CBM`/`DIR` are the directory codes (`$83`–`$86`). REL functions
    on a never-opened handle used fd 0 before the first `cbm_rel_open`.
    `cbm_k_chrin`/`cbm_k_chrout` are declared.
  - `fscanf`/`vfscanf` were stubs and `sscanf` minimal: one scanf engine
    (`scanf_core.c`) for `sscanf`/`vsscanf`/`fscanf`/`vfscanf` and new
    `scanf`/`vscanf`: widths, `*`, `%i %n %p %[set]`, `hh h l ll`
    modifiers, `EOF` before the first conversion; float conversions in
    `scanf_float.c`, linked only when needed.
  - `setvbuf` was a stub, `tmpfile` returned NULL and `tmpnam` an empty
    name: `setvbuf`/`setbuf` set the buffering, `tmpfile` is a memory
    stream (never on disk), `tmpnam` gives `TMPnnnn` names no file has.
  - Far pointers and DMA (`test_far2`): the allocation policies only set
    a variable; they now choose `far_malloc_current`'s bank (fixed, first
    fit 4→5, round robin), whose default bank is 4 (was the unusable 0).
    `bank_reset` no longer uses `assert` (which linked printf).
  - The far-pointer versions of `memcpy`/`memset`/`strlen`/`strcpy`/
    `strcmp`/`strcat`/`memcmp` (`memcpy__fpvst` …, `stdlib_overloads.c`)
    were declared nowhere (the comment's "compiler selects the variant" is
    not so): `<far_string.h>` declares them. Far destinations return the
    far pointer (was a `far_ptr_t` returned as `char *`), strings are no
    longer cut at 256 characters, 256-512-byte buffers are gone, and the
    typed far array helpers take any index (`test_far_string`).
  - **`<input.h>`** (new, `test_input`): one way to read the keyboard and
    joysticks. `input_key` reads the keyboard's hardware queue (`$D610`,
    where typed keys wait) and gives letters as cc45 writes them, PETSCII
    cursor/RETURN/DEL codes, RUN/STOP 3 and F1-F14 `$F1`-`$FE`;
    `input_pressed`/`input_modifiers` read held keys; `input_joy` reads
    either port with 3-button sticks (C64GS style) or Sega pads (B, C) on
    the paddle lines. conio's `kbhit`/`getch` and the invaders, musicgen,
    sample_player and keyboard_viewer examples use it. The test presses
    keys through the virtual key registers (`$D615`), the path of real
    typing (the earlier tests wrote the KERNAL's buffer, which typed keys
    never reach).
  - **Audio** (`test_audio`): `<audio_sid.h>`, `<audio_digi.h>` and
    `<audio_midi.h>` were a software synthesizer model written for a 32-bit
    `int` that never touched the sound hardware (no program made a sound
    with them). New APIs drive it: `audio_sid` the four SIDs as 12 voices
    (notes, instruments, envelopes, filter, volume, left/right mirroring,
    a shadow of the write-only registers), `audio_digi` the four audio DMA
    channels (8/16/4-bit samples, rates in Hz, looping, volume and
    panning), `audio_midi` a MIDI byte parser and a synth on SID voices
    (allocation, sustain, pitch bend, program change). The `invaders` and
    `musicgen` examples use `audio_sid` (musicgen's generated note table
    is gone).
  - `<conio.h>` (`test_conio`) wrote to the C64-mode screen at `$0400`
    with PETSCII codes as screen codes, put the background colour into
    colour RAM's attribute bits, and `getch`/`kbhit`/`cursoron`/`cursoroff`
    were stubs. It now follows the VIC-IV's screen (80x25 in MEGA65 mode),
    converts to screen codes, wraps and scrolls, writes colour RAM by DMA,
    sets `$D021` for the background, reads keys through the KERNAL (`kbhit`
    also sees the hardware queue at `$D610`, where typed keys wait),
    shows the cursor in `getch`, and has `revers`.
- `rtc_read` (and `localtime`, `gmtime`) read the RTC through a 16-bit
  pointer, i.e. `$7110` (zeros, month -1): the registers at `$FFD7110` are
  copied by DMA; the weekday is computed from the date.
- **`dma_copy_far` / `dma_fill_far`** (`<dma.h>`, `lib/stdlib/dma_far.c`):
  DMA at 28-bit linear addresses (enhanced F018B jobs): chip RAM above
  64 KB, all of colour RAM at `$FF80000`.
- **Far pointers** (`far_ptr.h`, `far_ops.h`) and **`dma_controller.h`**
  reimplemented on them: they wrote the bank number to the CPU port `$01`
  (or used `$DE00` as DMA registers) and accessed bank 0. The far allocator
  uses banks 4–5 only (the program, BASIC and the ROM are in 0–3).
- `__dma_fill` works (the documented "fills with 0" issue was stale).
- `key_pressed`, `atoi` and `isblank` fixed: a conversion of the assembly
  library had added 13 to some constants (`key_pressed` scanned the wrong
  column/row: no key was ever seen; `atoi` did not skip tabs and newlines;
  `isblank('\t')` was 0).
- `key_pressed` reads the MEGA65's keyboard matrix registers (`$D614` row
  select, `$D613` row) instead of scanning through CIA 1: the CIA scan used
  the code's row and column the wrong way round (Z and X were never seen) and
  shares its lines with the joysticks and the KERNAL's interrupt scan.

### ca45

- **Fix**: with `-ffunction-sections` (`.function_sections`), an operand
  holding both a `-` and a hex number (`sbc #$41 - 10`) hung the assembler
  (the scan for label differences stopped at the `$`).
- Tail deduplication numbered its shared-tail labels from 0 in every
  optimizer round: a later round's `__tail_opt_dedup_0` took the name of an
  earlier one, and the jumps made for the earlier tail went to the new one
  (a function's epilogue jumped into another function's code). The names
  are unique per file; the jump to a tail in another proc is a relocated
  `JMP` (a relative branch between procs breaks when ln45 drops unused
  sections of a sectioned object); labels the optimizer inserts get their
  segment (a reference to them was not relocated). SeqExtract leaves the
  routines of an earlier round alone. `-l` with a non-numeric level is an
  error message, not an abort.
- **SeqExtract** (repeated instruction runs → `BSR` routines) rewritten and
  on by default (#89): it called routines it never labelled, inserted
  statements while holding indices, let runs span labels and appended the
  routines outside any proc. `-PNoSeqExtract` / cc45 `-fno-seq-extract`
  turn it off; `--experimental` gates nothing now.
- `.var` reassignments hold by position in the generated code (each use got
  the last value); `$00xx` is an absolute address; `#<(sym+n)` /
  `#>(sym+n)` are relocated.
- SeqExtract could move the instruction a `bne *+5` lands on (cc45's
  16-bit decrement skips one instruction) into a routine: the branch then
  skipped the whole `BSR`. A run no longer contains a `*±N` target except
  as its first instruction.
- `-ffunction-sections`: a branch the optimizer inserted (the `BSR` to a
  SeqExtract routine) had no target the section builder could evaluate, so
  every proc with an extracted run was merged into the object's residual
  and never dropped. The library's unused functions are dropped again
  (a program calling one conio function: 4179 → 924 bytes; the CBM I/O
  test's code: 43 KB → 9.7 KB).

### disk45

- A directory longer than one sector continued on a data track (the
  allocator for file data skips the directory track): the DOS follows the
  chain on the directory track only, so files after the 8th could not be
  found (`DLOAD` gave FILE NOT FOUND). New directory sectors are taken from
  the directory track (D81 40, D64 18).
- `repair`, `diff`, `list --petscii`, `basic`, `add` placement options,
  interactive `shell`, `edit` (#160–#167); BAM system sectors per format.

### Examples

- **mandelbrot**: points in the main cardioid and the period-2 bulb are
  recognised without iterating (the kernel is 2-3 times faster over the
  whole set, exactly the same counts), and a cell whose border is mixed is
  checked again as four quadrants. The opening view renders in 343 frames
  (was 604), the same picture.

- invaders: a title screen (the name in block letters in cycling colours,
  the score table typed out with the aliens marching in place and the UFO
  sprite, the high score, the controls); after about 5 seconds without
  input the attract mode plays for about 20 seconds, then the title comes
  back. The sprite images moved to bank 4 (16-bit pointers reach all chip
  RAM), which keeps room for the heap `fopen` needs at `-O2`.
- New `overlays`, `mapped_overlays` and `far_functions` (code overlays,
  copied and mapped, and `__far` functions; on the examples disk).
- Every example Makefile passes `-O2` explicitly: a personal `cc45.conf`
  no longer changes how an example is built.
- New `far_scroller` (a map in bank 4 scrolled with DMA, parallax).
- New `invaders`: a Space Invaders game (object extensions, a DMA-copied
  character set, 16-colour sprites, SID sound, joystick/keyboard); the
  high score is saved to and loaded from `INVADERS.HI` on the disk.
- New `mandelbrot`: a Mandelbrot viewer (FCM 320x192 bitmap at 8 bits a
  pixel, palette cycling, an assembly kernel on the Q register and the
  hardware multiplier, zoom to 2^20).
- New `forth`: a tiny Forth (a programmable calculator) whose inner
  interpreter dispatches with computed goto; `make check` runs a test script.
- New `musicgen`: a random music generator in seven styles (Markov chord
  tables with fixed cadences, AABA or 12-bar blues form, motif-based
  melodies on chord tones, voice-led chords, walking/boogie/Alberti bass,
  drums with swing), 14 SID instruments, tempo and style keys.
- New `calc_graph`: a scientific calculator that plots y = f(x); a
  shunting-yard compiler and a computed-goto bytecode evaluator, a raster
  interrupt hooked through the KERNAL's `$0314` (safe while the float
  routines map the BASIC ROM in), GCC extensions throughout.
- New `rtc_clock`: the real-time clock in big digits with the date, a
  seconds bar and a stopwatch; reads the RTC registers at `$FFD7110` with
  `dma_copy_far`.
- All examples read the keyboard and joysticks through `<input.h>`
  (rtc_clock, mandelbrot, rogue and calc_graph read `$D610` directly).
- New `sample_player`: a drum machine on the audio DMA (`<audio_digi.h>`);
  the kick, snare, hi-hat and a looped tone are synthesized at start, a
  16-step pattern plays them, panned across the four channels.
- New `keyboard_viewer`: the keyboard on screen, held keys lit (several at
  once, `key_pressed`), and conio's `kbhit`/`getch` codes: the interactive
  keyboard test for a windowed xemu or a MEGA65.
- New `rogue`: a mini rogue-like (random rooms and corridors per level,
  lit rooms and map memory, monsters by depth, combat, items, experience,
  the amulet on depth 8).
- New `disk`: every example on one D81 (`make -C examples/disk`, disk45)
  with a basic45 menu saved as `AUTOBOOT.C65` that `DLOAD`s them.
- `text_scroller`: 80 columns, screen-code message, a bouncing ball sprite
  passing in front of and behind the text; `game_of_life` (80x25 screen,
  signed neighbour offsets), `bitfield_registers` (`$D031`/`$D054` layouts
  per the MEGA65 Book), `vic4_colors` and `memset_screen` fixed;
  `make -C examples` builds all of them.

### Tests

- xemu runtime tests 220 → 295 (stdio, element offsets, aligned objects,
  SeqExtract, far DMA, far pointers, parser fixes, library regressions,
  scanf, code overlays and `__far` functions in both conventions);
  signedness warnings; all examples in `test_examples.sh`. GCC torture
  tests: 552 in each convention.

## [v1.0.6] - 2026-10-14

A large release: the calling conventions, the code generator and the
standard library were reworked, the documentation was validated against the
source, and runtime tests moved to xemu (220 programs, both conventions;
GCC torture tests 537 in each convention). Details: `doc/bin/*.md`,
`doc/architecture/`.

### cc45

- **Calling conventions**: the ZP convention (`$10`–`$2F` parameter block)
  with static allocation (SAC) is the default; the stack convention is
  `-fno-zpcall`. Structs, unions, floats and `long long` are returned through
  a caller-supplied pointer. `__regparm`, `__fastcall__`, variadic functions
  with static allocation, `__interrupt` handlers on their own base page.
- **Relocated base page**: programs run on a private base page placed by
  ln45 after BSS; ROM calls switch to page 0 and back. 16-bit CPU stack by
  default (`-fno-stack16`, `-fstack-top`, `-fstack-size`).
- **Startup**: one-step `cc45 file.c -o file.prg`; `main(argc, argv)` from
  the BASIC line; BSS filled at startup (`-fbss-fill`); `__basic_call` runs a
  C function from BASIC `SYS`.
- **Language**: GCC vector extension, nested functions, statement
  expressions, computed goto, case ranges, `continue <value>` in a switch,
  `_Alignas`/`aligned`, variable-length arrays, `_Complex`, `int64_t` and
  `__int128` as operator-overloaded structs, `p"…"`/`s"…"`/`a"…"` string
  encodings, file-scope `asm("…")`, `-fsigned-default`, `-funsigned-char`,
  `-fimplicit-function-declaration`, `__has_include`/`__has_builtin`,
  uninitialized static locals in BSS.
- **Optimization**: nine IR passes at `-O1`+ (`-f<name>`/`-fno-<name>`,
  `#pragma cc45 optimize`), automatic inlining at `-O2`, loop interchange and
  unrolling at `-O4`; `-Os` (multiplies as library calls); base-page blocks
  for the busiest variables of functions that make calls; post-increment
  sinking, in-place read-modify-write, division and remainder as library
  calls, signed compares by subtraction, sign tests, smaller stack-convention
  calls (pushed constants, no result save around the pops, arguments pushed
  directly by frameless callers, parameter thunks through `__sac_args`).
- **Per-function sections** (`-ffunction-sections`): ln45 drops what a
  program does not reach.
- Many code generation fixes (sign extension, bitfields, floats, struct
  copies, frames, inlining); see the git history.

### Standard library

- printf family in assembly (`__vformat`, `pf_format`), `%f %e %g` over the
  whole float range from the BASIC 65 ROM; float math through the ROM with
  `errno` on domain/range errors; double math functions.
- File I/O on disk drives (`fopen` r/w/a and `+` modes, `fseek`/`ftell`,
  REL files), heap rewrite (`malloc`/`calloc`/`realloc`, `__stack_size`),
  `setjmp`, time, `snprintf`/`vsnprintf`, `strdup`, `strlcpy`, ...
- One set of C sources for both conventions; hand-written assembly with
  sections. Library code: `c45.lib` 117,738 bytes, `c45_stack.lib` 126,478
  (the stack convention's parameter entries and calls made smaller).

### ca45 / ln45 / ar45 / nm45 / objdump45

- ca45: expressions, directives, simulated instructions and string escapes
  fixed; section table for per-function linking; unknown directives warn;
  `(sym,x)` / `(sym)` with a symbol use the base-page form unless the
  instruction has the absolute one.
- ln45: SAC overlay (functions never active together share storage),
  base-page block placement, calling-convention bridge thunks in both
  directions, unreachable sections dropped (`--gc-inputs`), `-basic -t ADDR`,
  heap warnings.
- The `.o45` format gained section tables and function attribute records.

### disk45

- 25 formats: D64, D71, D81, D65, D80/D82 (8050/8250 layout), D9090/D9060
  (VICE's layout, now read/write), X64, G64/G71 writing, NIB/NBZ, Zipcode,
  T64, TAP, ARK, ARC/SDA, LNX, P00, CVT; images inside `.zip` and `.gz`.
- CMD D1M/D2M/D4M/DNP in the real layout: `disk.d2m:N` opens any partition
  (native, or emulated 1541/1571/1581), `create --partitions`, native `DIR`
  subdirectories.
- Subdirectories: `DIR/NAME` paths in add/extract/rm/mv/lock, `mkdir`/`rmdir`,
  and the FUSE mount reads and writes them.
- REL files on every sector format (super side sectors on D81/D65, D82, D90,
  CMD); GEOS files (VLIR and sequential) as Convert (CVT) files; MEGA65 SD
  card images (`disk45 sd`); SQLite catalog of images in zips and on cards.

### basic45

- Programs run on BASIC 65: label-based source, long variable names, block
  IF / proc, string encodings, `--link` C functions, `--decompile`,
  `--disk`, `--emulator`, MEGA65 escape codes checked against the ROM.

### cvt_asm

- The ca45 reader reads the source as written (lossless ca45 → ca45).
- One shared reader for ca65, ACME, KickAssembler, Merlin 64, Oscar and x65:
  every flat test source converts to each dialect and back to the same
  bytes, and programs in each dialect's own syntax assemble as ca45.
- Strings and character constants keep their bytes in every direction.

### Tooling and tests

- Runtime tests on xemu (`test_xemu.sh`, both conventions); mmemu is no
  longer used for testing. GCC torture tests in both conventions with
  documented known failures. Installed toolchains find their headers and
  libraries; Alpine and Ubuntu Docker images. `make test` builds its
  `.o45` unit-test binary itself (it failed after `make clean`).

---

## Optimization System Refactor (2026-08-19, released in v1.0.5)

### Standards-Compliant Optimization Flags

#### Breaking Change: New -f Flag Format
- **Changed:** Individual optimization flags now use `-f` prefix with kebab-case naming
  - Old format (no longer supported):
    - `-OConstantFolding` / `-OLoopUnrolling` / `-PNoDeadStoreElim`
  - New format (standards-compliant):
    - `-fconstant-folding` / `-floop-unrolling` / `-fno-dead-store-elimination`
- **Rationale:** Matches gcc/clang conventions for feature control flags
- **Compliance:** Follows C compiler tradition (-finline-functions, -fno-strict-aliasing, etc)

#### Implementation Details
- **CLI Parsing:** OptimizationController::parseOption() completely rewritten
  - `-O<level>`: Optimization levels (-O0 through -O9) — unchanged
  - `-f<name>`: Enable individual optimization (kebab-case)
  - `-fno-<name>`: Disable individual optimization (kebab-case)
- **Pragma System:** Updated to use kebab-case notation
  - `#pragma cc45 optimize(loop-unrolling)` — enable
  - `#pragma cc45 optimize(no-branch-folding)` — disable
- **Configuration Files:** Updated ~/.config/m65/cc45.conf examples

#### All 24 Optimizations Refactored
- Levels 1-9: All optimization names converted to kebab-case
- Examples: `constant-folding`, `tail-call-optimization`, `loop-invariant-code-motion`
- Flag naming verified: kebab-to-camel conversion working correctly

#### Documentation Improvements
- **doc/architecture/optimizations.md:** Complete rewrite
  - All command-line examples updated to -f format
  - Reference table showing all 24 optimizations with new flags
  - Configuration file section with new examples
  - Troubleshooting section updated
- **CLAUDE.md:** Updated all configuration examples

#### Migration Guide
Projects using old flag format must update:
- `-O2 -OConstantFolding` → `-O2 -fconstant-folding`
- `-O0 -PNoBranchFolding` → `-O0 -fno-branch-folding`
- Config: `-PNoSeqExtract` → `-fno-seq-extract`

---

## [v1.0.5] - 2026-07-19

Bug fixes and verification release. All mmemu tests compile successfully.

### Bug Fixes

- **Fixed Variable Offset Corruption Bug** (Commit 05aeb1e)
  - Root cause: ConstantFolder was aggressively replacing variables with their initialization constants
  - Impact: Variables accessed after function calls now correctly load from frame instead of using immediates
  - Test coverage: All 6 mmemu tests now compile correctly

- **Verified Frame Pointer Infrastructure** (Commit ef57870)
  - TSY/TSX/INX frame pointer calculation verified correct
  - Handles all SPL values (0x00 → 0xFF) with proper 16-bit carry propagation
  - No fix needed — frame pointer working as designed

- **Related Issues Already Fixed**
  - Issue #192: BFINS missing result load-back (Commit b3807da)
  - Issue #193: ZP slot collision workaround (Commit 77390b6)

### Verification Results

- ✅ All 6 mmemu tests compile and link successfully
- ✅ Stdlib libraries built: c45.lib (102 members), c45_zp.lib (96 members)
- ✅ 150+ regression tests passing
- ✅ No breakage from bug fixes

### Documentation

- Comprehensive bug fix documentation in CLAUDE.md
- v1.1 optimization roadmap with expected 5-15% code size reduction
- Complete verification results documented

---

## basic45 Preprocessor & Advanced Features (2026-08-13, released in v1.0.5)

### basic45 Preprocessor and Advanced Features

#### Preprocessor Support (#include, #define, #ifdef)
- **New:** `BasicPreprocessor` class implements C-like preprocessing
  - `#include "filename"` — File inclusion with recursive support
  - `#define NAME VALUE` — Compile-time constant definitions
  - `#ifdef SYMBOL` / `#ifndef SYMBOL` / `#else` / `#endif` — Conditional compilation
  - Circular include detection and error reporting
- **New:** `-I <path>` flag to add include search directories
- **Features:**
  - Macro expansion with word-boundary checking
  - Nested conditional compilation support
  - Max include depth limit (100) to prevent runaway includes

#### Documentation Generation
- **New:** `BasicDocGenerator` class extracts program structure
- **New:** `--docs <file>` flag generates markdown documentation
- **Output:** Section numbering, label names, line numbers
- **Format:** Markdown for easy integration with project docs

#### Line Number Customization
- **New:** `--increment <n>` flag for custom line number increments
- **Examples:** `--increment 1` (dense), `--increment 100` (sparse)
- **Default:** 10 (preserves backward compatibility)

#### Test Coverage
- 11 new tests for preprocessor features (100% pass rate)
- Tests cover: #define, #ifdef, #ifndef, #else, includes, increments, docs
- All existing 11 label tests still passing

#### Example Programs
- **stdlib_basic.bas** — Standard library with constant definitions
- **preproc_demo.bas** — Demonstrates #include and #ifdef usage

---

## basic45 Label Support (2026-08-13, released in v1.0.5)

### basic45 Enhancements

#### Label Support (--labels flag)
- **New:** `--labels` command-line flag enables symbolic label mode (replaces explicit line numbers)
- **New:** `--label-table <file>` flag outputs label→line number mapping table (tab-separated format)
- **Features:**
  - Labels defined with `identifier:` syntax at start of line
  - `#` comments at line start are silently stripped (not emitted as REM statements)
  - Empty lines are skipped (not included in output)
  - Auto-generated line numbers: 10, 20, 30, ... (10-increment)
  - GOTO/GOSUB statements automatically resolve label references to line numbers
  - Backward compatible: normal mode still requires explicit line numbers
- **Testing:** 11 comprehensive tests covering labels, comments, empty lines, GOTO/GOSUB resolution
- **Example:**
  ```basic
  # This is a comment
  start:
  print "hello"
  
  loop:
  x = x + 1
  if x < 5 then goto loop
  ```
  Compile with: `basic45 program.bas --labels -o program.prg`

---

## Phase 1 Simplification (2026-07-07, released in v1.0.5)

### Code Simplification and Complexity Reduction

Phase 1 of the simplification roadmap focused on eliminating redundant code and establishing reusable patterns. **221 lines of duplicate code removed**, establishing foundation for 15-25% total complexity reduction across 3 phases.

#### 1.1: Unified Type Size Calculation (a58380d)

- **New:** `TypeSystem.hpp` / `TypeSystem.cpp` — unified type size calculation across compiler phases
- **Changed:** `CodeGenerator`, `IRBuilder`, `ConstantFolder` now use shared `TypeSystem::getTypeSize()`
- **Result:** Eliminated ~150 lines of duplicate type-size logic. Single source of truth for all types (char, int, long, float, __int(N), struct/union, pointers)
- **Testing:** All 500+ unit tests passing; zero regressions

#### 1.2: Constant Folding Optimization Audit (07274e8)

- **Audited:** `ConstantFolder` (159 lines, AST-level) vs `IROptimizer` (1243 lines, IR-level)
- **Finding:** No redundancy — both work at different compilation phases and provide distinct value
  - ConstantFolder: Early AST-level folding reduces IR size before code generation
  - IROptimizer: Comprehensive IR-level optimizations (CSE, strength reduction, algebraic)
- **Decision:** No consolidation needed; both passes are essential and non-overlapping

#### 1.3: Disk Image Refactoring with BAMOperations Pattern (83c627f, a18313c, 2480746)

- **New:** `BAMOperations.hpp` / `BAMOperations.cpp` — abstract base class + implementations for disk format BAM operations
  - `D64BAMOperations`: Single-BAM sector (25-sector variant) disk format support
  - `D81BAMOperations`: Dual-BAM sector (40-sector variant) disk format support with multi-sector complexity
- **Changed:**
  - `D64Image`: Refactored to use D64BAMOperations (27 lines saved; 323 → 296)
  - `D81Image`: Refactored to use D81BAMOperations (44 lines saved; 341 → 297)
- **Pattern:** Reusable template for D71, D80, D65, and other disk formats (5,054+ lines total)
- **Testing:** All 500+ unit tests passing; zero regressions

### Phase 1 Summary

- **Time:** 48 hours (within 55-hour budget)
- **Lines Removed:** 221 lines
- **Files Changed:** 13 files (4 new, 9 modified)
- **Commits:** 6 commits (4 major + 2 documentation)
- **Test Status:** All 500+ unit tests passing; zero regressions ✅
- **Ready for Phase 2:** CodeGenerator/IRBuilder consolidation (1,500+ line savings)

---

## [v1.0.4] - 2026-06-30

- Update version to 1.0.4

---

## [v1.0.3] - 2026-06-30

Major feature release: OOP system, operator overloading, **full floating-point support**, `__int(N)`/`__uint(N)`, GTE compatibility from 65.8% to 96.4% (581 tests).

### Object-Oriented Programming System

- **Struct Methods**: Define methods inside struct bodies with implicit `this` pointer
  ```c
  struct Point { int x, y; int sum() { return this->x + this->y; } };
  ```
- **Single Inheritance**: `struct Dog : Animal { }` with parent member layout at offset 0
- **Virtual Dispatch**: `virtual` keyword, vtable as global data, `__vt` pointer auto-initialized
- **`final` Keyword**: Methods and structs — prevents override/inheritance, enables direct call
- **Devirtualization**: Single-implementation vtable slots optimized to direct calls at compile time
- **Auto-Inline Methods**: Methods ≤3 statements automatically inlined at call sites
- **Final Struct Propagation**: All methods on `final` structs devirtualized
- **Operator Overloading**: `operator+`, `operator*`, `operator==`, etc. for structs
  - Binary: `+ - * / % == != < > <= >= << >> & | ^`
  - Unary: `- ~ ! ++ --`
  - Compound assignment: `+= -= *= /= %= &= |= ^= <<= >>=`

### Floating-Point Support (CBM 40-bit via BASIC 65 ROM)

- **`float` / `double` / `long double` types**: All downgrade to CBM 40-bit (5 bytes). Supported in all type positions: variables, function parameters/returns, struct members, arrays, pointers, casts, sizeof, _Alignas, va_arg, typedef, function pointer parameter lists
- **Float literals**: Decimal (`3.14`, `1.5f`) and exponent notation (`1.5e-3`, `3.14e0`). Positive integer exponents (`1e2`) remain integer literals
- **Arithmetic**: `+ - * /` via ROM routines (`FADDT`, `FSUBT`, `FMULTT`, `FDIVT`), unary negation (`NEGOP`)
- **Comparisons**: `== != < <= > >=` via `FCMP` (returns -1/0/1) with unsigned equality mapping for correct 8-bit flag behavior
- **Casts**: `(float)int_val` (`GIVAYF`), `(int)float_val` (`AYINT`), implicit promotion in mixed expressions
- **Indirect access**: Pointer dereference (`*float_ptr`), struct member (`s.f`, `p->f`), array element (`arr[i]`) — all correctly handle 5-byte LOAD/STORE via `(ZP),Y` loops
- **Function calls**: 5-byte push/pop for float arguments (both simple inline and two-phase ZP temp paths)
- **Global initialization**: Compile-time IEEE-754 → CBM 40-bit conversion (`doubleToCBM40()`)
- **`printf %f` / `sprintf %f`**: Float-aware printf linked automatically via weak/strong symbol pattern. Compiler emits `.extern _printf_float` sentinel when F32 args are passed to variadic calls; linker pulls float-capable `printf`/`sprintf` only when needed (zero cost for integer-only programs)
- **`sscanf %f`**: Float parsing via `strtof()`
- **`strtof()` / `atof()`**: String-to-float conversion with sign, integer, fractional, and exponent parts
- **`#pragma cc45 weak`**: Mark next function as weak export (IR path). Used by integer-only printf to allow float printf override
- **Math library** (`<math.h>`): 27 float functions total
  - ROM-backed: `sinf`, `cosf`, `tanf`, `atanf`, `logf`, `expf`, `sqrtf`, `fabsf`
  - C-implemented: `powf`, `fmodf`, `ceilf`, `floorf`, `roundf`, `truncf`, `atan2f`, `log10f`, `log2f`, `ldexpf`, `frexpf`, `modff`, `copysignf`, `fmaxf`, `fminf`, `fdimf`, `__isnanf`, `__isinff`, `__fpclassifyf`
  - Constants: `M_PI`, `M_PI_2`, `M_PI_4`, `M_E`, `M_LOG2E`, `M_LOG10E`, `M_LN2`, `M_LN10`, `M_SQRT2`, `M_SQRT1_2`, `HUGE_VALF`, `INFINITY`
  - Double aliases: `sin(x)` → `sinf(x)`, `pow(x,y)` → `powf(x,y)`, etc. (all 19 functions)
- **`<float.h>`**: `FLT_RADIX`, `FLT_MANT_DIG`, `FLT_DIG`, `FLT_MIN_EXP`, `FLT_MAX_EXP`, `FLT_MIN_10_EXP`, `FLT_MAX_10_EXP`, `FLT_ROUNDS`, `FLT_MAX`, `FLT_MIN`, `FLT_EPSILON` (plus `DBL_*` and `LDBL_*` aliases)

### New Headers

- **`<complex.h>`**: Complex number types via operator-overloaded structs
  - `_Complex_int`: integer complex (`+`, `-`, `*`, `~` conjugate, `-` negation, `==`, `!=`)
  - `_Complex_float`: float complex (`+`, `-`, `*`, `/`, `~` conjugate, `-` negation, `==`, `!=`), `__builtin_conjf`
  - `_Complex`/`__complex__` keyword: `_Complex float`, `__complex__ double`, `float __complex__`, `_Complex long int`, `_Complex long double` all supported via `COMPLEX` token
  - Imaginary literals: `1.0fi`, `1.0i`, `2.2if` → `(struct _Complex_float){0.0, value}`
  - `__real__` / `__imag__`: parser-level unary operators (not macros). Support both `__real__ z` and `__real__(z)` forms, work as lvalues (`__real__ z = 1.0`) and rvalues (`float r = __real__ z`)
  - Unary operator overloading: `~`, `-`, `!`, `++`, `--` dispatch to struct `operator_*` methods
- **`<float.h>`**: CBM 40-bit float characteristics and limits

### Arbitrary-Width Integer Types (`__int(N)` / `__uint(N)`)

- **Architecture**: Library-based via operator-overloaded structs. Preprocessor maps `__int(N)` → `struct __intN` via token pasting. Operators call width-parameterized runtime with byte count. Zero compiler codegen changes.
- **`<intwide.h>`**: `struct __int64` (8 bytes) and `struct __int128` (16 bytes) with operators `+`, `-`, `*`, `==`, `!=`, `~`, unary `-`. Extensible to any width by adding a struct definition.
- **Runtime library** (`intwide.c`): `__intN_add`, `__intN_sub`, `__intN_mul`, `__intN_cmp_u`, `__intN_neg`, `__intN_not`, `__intN_and`, `__intN_or`, `__intN_xor`, `__intN_shl`, `__intN_shr_u` — single set of routines handles all widths. Also available as optimized 45GS02 assembly (`intwide_rt.s45`, 525 bytes).
- **`long long`**: Maps to `struct __int64` — true 64-bit via `<intwide.h>` operator-overloaded struct. Requires `#include <intwide.h>` for struct definition.
- **`__int128` / `__int128_t` / `__uint128_t`**: Preprocessor macros → `struct __int128`
- **`_Decimal32` / `_Decimal64` / `_Decimal128`**: Preprocessor macros → `float`. Decimal literal suffixes (`.DD`, `.DF`, `.DL`) consumed in lexer. Same struct+operator pattern extends to decimal types.
- **`int64_t` / `uint64_t` / `intptr_t` / `uintptr_t`**: Added to `<stdint.h>`
- **`sizeof(*this)` fix**: Correctly returns struct size in method bodies (was returning pointer size)
- **Operator name disambiguation**: Unary `operator-()` → `operator_neg` vs binary `operator-(o)` → `operator_sub`

### GTE Compatibility (316→560/581, 65.8%→96.4%)

Remaining 24 failures: 9 unfixable (sys/mman.h, stdout/FILE\*, \_\_builtin\_va\_arg\_pack, #define L, missing file), 8 nested function closure/label issues, 7 parser edge cases (statement expressions, macro re-expansion, funcptr-in-funcptr).

- **101 float/double/complex torture tests added** from GCC test suite (previously excluded)
- **ADDR_GLOBAL prefix fix**: global variable address-of (`&var`) had double `_` prefix, causing 33 assembler undefined symbol errors
- **Static function DCE fix**: functions used as function pointers but never called directly were incorrectly eliminated. DCE now scans all global operand references, not just CALL instructions
- **Function pointer extern fix**: functions used as values (`fp = exit`) now emit `.extern` for linker resolution
- **Wide literals**: `L'x'` and `L"str"` accepted (downgraded to 8-bit on this target)
- **Integer imaginary literals**: `200i`, `1i` for complex number initialization
- **Volatile function pointer arrays**: `int (* volatile fp[])(void)` — qualifiers between `(*` and name now skipped
- **`_Complex` type completeness**: `_Complex unsigned short`, `_Complex char`, `_Complex long int` all resolved correctly
- **Compound literal postfix**: `(int[]){1,2,3}[i]` — subscript/member access after compound literals
- **`__alignof__`/`__alignof`**: Added as ALIGNOF keyword aliases with expanded type list (typedef, unsigned, signed, float, union)
- **const double-pointer**: `const T **pp; *pp = val;` no longer falsely rejected as read-only
- **Built-in type aliases**: `__int128`, `__int128_t`, `__uint128_t`, `__int64_t`, `__uint64_t` as parser typedefs
- **Nested Functions**: Closure conversion with static chain — all `nestfunc-*` tests pass
- **32-bit Bitfields**: `long`-backed storage units, `bfext32`/`bfins32` simulated ops
- **`__builtin_*` Aliases**: 22 GCC builtins mapped to stdlib (`__builtin_printf` → `printf`, etc.)
- **`__builtin_offsetof`**: Parses `__builtin_offsetof(struct S, member)` with nested member/array support
- **`sizeof(void *)`**: `void`, `const`, `volatile` types recognized in sizeof expressions
- **Function Pointer Casts**: `(long (*)(int))expr` parsed correctly
- **Array Cast Syntax**: `(int [])` and `(int [N])` in compound literal casts
- **Comma Operator**: In parenthesized expressions AND statement-level (`i++, x--;`)
- **Elvis Operator**: `expr ?: default` (GCC ternary shorthand)
- **Computed Goto**: `&&label` (label address) and `goto *expr` (indirect jump)
- **`__label__`**: Local label declarations parsed and skipped
- **Implicit Int**: `static max;`, `unsigned d;`, `static inline f()` → implicit `int` type
- **Array Parameters**: `int a[]`, `int *a[N]`, `int a[][M]` in function params (decay to pointer)
- **Unnamed Function Pointer Params**: `int (*)(int, int)` in prototypes
- **Unnamed Bitfields**: `int : 6;` (padding) in structs
- **Type Order Flexibility**: `short unsigned int`, `long unsigned int` in params/typedefs/vars
- **Typedef Function Pointer Arrays**: `typedef void (*frob)(); frob f[] = {abort};`
- **String Literal Concatenation**: `"hello" " world"` → `"hello world"`
- **Float Literals**: `0.0`, `1.5f`, `1.5e-3` now produce CBM 40-bit float values (previously truncated to integer)
- **Enum Return Type**: `enum E foo(...)` at file scope
- **Struct Def + Variable**: `struct S { } arr[2];` with array dims and multi-var comma at global and local scope
- **Flexible Array Member**: Relaxed "must have at least one other member" check
- **Function Pointer Struct Members**: `void (*p)();` in struct bodies
- **K&R Function Support**: Stray `;` at top level, implicit int returns

### Preprocessor Enhancements

- **`__cc45__`**: Predefined compiler identification macro
- **`__builtin_va_list`**: Predefined as `void *`
- **GCC Keyword Synonyms**: `__volatile`, `__const`, `__inline`, `__restrict`, `__signed` as macros
- **`#if(` without space**: `#if(__SIZEOF_INT__ >= 4)` now parsed correctly
- **`__extension__`**: Skipped at top-level declaration context
- **`va_arg`**: `const`/`volatile` qualifiers before type, `struct`/`union`/`enum`/`typeof` types
- **`__attribute__` expansion**: 25+ GCC attributes silently accepted (`always_inline`, `unused`, `weak`, `pure`, `const`, `cold`, `hot`, `packed`, `noinline`, `deprecated`, `visibility`, `section`, `format`, `malloc`, `constructor`, `destructor`, etc.). `__attribute__` before `typedef` at top level now works.

### Assembler Enhancements

- **Encoding prefixes**: `s'A'` (screencode), `p'A'` (PETSCII), `'A'` (PETSCII default) for char literals in expressions (`lda #s'A'`). String directives: `.text s"HELLO"` (screencode), `.text p"HELLO"` (PETSCII), `.ascii` defaults to raw ASCII, `.screencode` defaults to screen codes. Prefix overrides directive default.
- **Char literal support**: `CHAR_LITERAL` token now handled in expression evaluator — `lda #'A'` works (was previously unsupported)
- **FCMP peephole**: `CMP #$FF; BEQ` → `BMI` and `CMP #$FF; BNE` → `BPL` when N/Z flags reflect A. Saves 2 bytes per float `<` / `>=` comparison.
- **I8 comparison optimization**: I8 comparisons use `loadOperandA` (A-only) instead of `loadOperand` (A:X with `ldx #0`). Saves 2 bytes per I8 comparison and avoids N/Z flag clobbering.

### Standard Library Additions

- **`vsprintf(buf, fmt, ap)`**: Formatted string output with `va_list`. Supports `%d %u %x %o %s %c %% %ld %lu %lx %lo`.

### Assembler Fixes

- **16-bit Branch Offsets**: All LBRA/LBCS/BSR use `PC+2+offset` per MEGA65 book (was `PC+3`)
- **`bfext32`/`bfins32`**: 32-bit bitfield extraction/insertion simulated ops
- **`.vtable_entry` Directive**: Metadata for future linker devirtualization
- **Extern Label Resolution**: Labels can override earlier `.extern` declarations
- **Global Dedup**: Extern declaration + definition → emit only the definition

### Compiler Flags

- **`-fset-bp=N`** / **`#pragma cc45 set_bp N`**: Set base page register in startup prologue

### Bug Fixes

- **neg.16 off-by-one**: Two's complement negation had `sec; adc #1` (adds ~val+2 instead of ~val+1). Fixed to `sec; adc #0`.
- **Struct return frame allocation**: Deterministic vreg allocation order — named locals first (alphabetical), then temporaries. Fixed vregOffset_ vs alloc_ mismatch.
- **Const-pointer increment**: `const char *p; p++` no longer falsely rejected as "read-only variable"
- **Legacy validator false positives**: Unknown struct, dot/arrow non-struct, compound literal struct, const assignment made non-fatal
- **Pointer dereference store bug**: `*p = val` overwrote the pointer variable instead of storing through it. The dereference handler in address-only mode returned the local-slot vreg directly; STORE handler treated it as a local-slot write. Fixed by emitting COPY to a fresh vreg. Affected both float and non-float pointer stores.
- **String label prefix mismatch**: `ADDR_GLOBAL` added `_` prefix to symbol names but `emitStrings()` emitted labels without it, causing `___str_0` undefined symbol errors in relocatable mode. Fixed by adding `_` prefix to string label definitions.
- **F32 arg push in two-phase call path**: Float args to variadic functions were pushed as 2 bytes (`push .ax`) instead of 5 bytes. Fixed both phases: ZP scratch slots now use variable sizes, and F32 args push via 5-byte `pha` loop.
- **Operator overloading with struct return types**: `isFunctionDeclaration()` didn't recognize `struct S operator+(struct S o)` — expected IDENTIFIER + `(` but got `operator` + `+`. Fixed lookahead to handle `operator` keyword followed by operator symbol(s). Also added operator dispatch in IRBuilder `BinaryOperation` visitor (was missing entirely — struct `a + b` was treated as regular arithmetic instead of calling `StructName__operator_add`).
- **sizeof(\*this)**: `getExprTypeInfo` for pointer dereference returned IR type size (2 bytes for PTR) instead of actual struct size. Fixed to use `getTypeSize(typeName)`.
- **ADDR_GLOBAL double prefix**: global variable address-of (`&var`) had double `_` prefix in ADDR_GLOBAL emission.
- **Static function DCE**: functions used as function pointers but never called directly were eliminated. DCE now scans all global operand references.
- **Preprocessor # stringification**: inner quotes not escaped per C99 §6.10.3.2.
- **Unary vs binary operator- disambiguation**: `operator-()` (unary negation) and `operator-(o)` (binary subtraction) both mangled to `operator_sub`. Fixed: unary maps to `operator_neg`.
- **const double-pointer false positive**: `const T **pp; *pp = val;` incorrectly rejected. Fixed: check `localPointedToType_` for multi-level pointers.

### Parser Refactoring

- **`parseTypeSpecifier()`**: Unified type parsing function replaces 14 duplicated inline type-matching blocks. Adding a new type keyword now requires editing only one function. Net -403 lines.
- **`isTypeStartToken()` / `isTypeStartAt()`**: Unified type detection for statement routing and lookahead.

### Hardware I/O Testing

- **MCP-based test runner** (`test_hw_mcp.sh`): documents MCP sequence for mega65 mode hardware tests (VIC-IV, SID, CIA, DMA, keyboard)
- Boot mega65 via breakpoint at KERNAL loop `$E1C6`, clear MAP via `set_map_state`, load + run tests
- VIC-IV test verified passing via MCP; SID/CIA/DMA/devices partially pass (timing-dependent registers, unimplemented devices)
- Filed mmemu#79 (load_image bank 0 default) and mmemu#80 (CLI step doesn't apply MAP changes)

---

## [v1.0.1] - 2026-06-01 — Release Notes

Bug fix release addressing assembler code generation crashes and missing signed 32-bit math operations.

See also: [GitHub Release](https://github.com/CTalkobt/M65Compiler/releases/tag/v1.0.1)

### Bug Fixes

- **Fix assembler simulated op size mismatch causing BRK crashes** — The generator's MachineState tracking diverged from pass2 sizing predictions, causing `tsxCached()` to skip TSX instructions and produce fewer bytes than allocated. Gap bytes (`$00`/BRK) corrupted the binary at runtime. Affected any program with function calls between stack-frame operations (e.g., `stax.fp` followed by `ldax.fp` across a `jsr`).
- **Fix BSR MachineState tracking** — The assembler optimizer converts JSR→BSR for position-independence, but BSR had no MachineState invalidation in the generator's branch instruction path, allowing stale X=SP assumptions to persist across subroutine calls.
- **Fix PHW MachineState tracking** — `phw #imm16` and `phw abs` modify SP but didn't call `spModified()`, leaving the TSX cache stale after push-word operations.
- **Fix #101: Add missing 32-bit signed mul/div/mod assembler ops** — `mul.s32`, `div.s32`, `mod.32`, `mod.s32` were emitted by IRCodeGen for `long` arithmetic but not recognized by the assembler lexer/parser. `mul.s32`/`div.s32` crashed with a `stoi` exception; `mod.32`/`mod.s32` produced "Unknown instruction" errors.
- **Fix signed math register-clobber bug** — Both 16-bit (`emitSignedMathOp`) and 32-bit signed math ops destroyed register A during sign detection before `neg_16`/`neg_32` could negate the full operand. Fixed by saving A via PHA before sign detection.
- **Fix signed `mod.s32`** — Was falling back to unsigned division; now uses proper sign-corrected remainder.

### Release Process

- `make test` now enforces mmemu execution tests (removed `-@` error suppression)
- `test_mmemu.sh` and `test_stdlib.sh` detect missing mmemu-cli with a prominent warning and non-zero exit
- Updated `RELEASE-STEPS.md` to require mmemu-cli and all tests passing for release

---

## [v1.0] - 2026-05-31

Final release of the MEGA65 C Compiler Suite. All v1.0 blockers resolved, comprehensive test coverage, production-ready optimizations.

### Summary

The v1.0 release includes:
- Full C99 subset compiler (cc45) with 32-bit long support, volatile/const/register/inline/repeat support
- Complete 45GS02 assembler (ca45) with relocatable object format, stack-relative addressing, simulated ops
- Linker (ln45) with calling convention safety, automatic thunk generation, interprocedural optimization
- Archiver (ar45) and symbol inspector (nm45)
- Minimal but functional C standard library with stdio, stdlib, string, ctype, math, time, dma, mega65, cbm headers
- Comprehensive test coverage: 176 assembler tests, 55 segment tests, 131 objdump tests, mmemu integration

### Game of Life Optimization Case Study

The `game_of_life.prg` example demonstrates the cumulative effect of all optimizations:
- **Before optimization**: ~7KB
- **After Phase 1 (hardware register overlays)**: 5.3KB (25% reduction)
- **After Phase 2 (MachineState tracking + store-fused ops + const suppression)**: 4.6KB (34% total reduction)
- **Final (with all optimizations enabled)**: 4,656 bytes

This represents a >50% code size reduction from baseline through careful register tracking, dead variable elimination, strength reduction, and fused operations.

## 2026-05-31 (released in v1.0)

Future work for v1.1+

### Added
- **MachineState Register & Memory Tracking Framework**: Unified value tracking for CPU registers (A/X/Y/Z/SP), CPU flags (N/Z/C/V), and three-tier memory (ZP full, stack full, 512-entry ring buffer for absolute addresses). Replaces ad-hoc `xHoldsSP_`, `aEqualsX_`, and string-based `RegState` in the assembler optimizer. Enables constant propagation, flag-derived optimizations, and store-forwarding across the entire toolchain.
  - Phase 1: Register tracking (CONSTANT, SAME_AS_REG, UNKNOWN). All M65Emitter instruction methods update state.
  - Phase 2: Three-tier memory tracking. Stores propagate register values to memory; ZP loads propagate known values back.
  - Phase 3: RANGE/NONZERO value kinds. FlagState derives exact N/Z from known constants. ALU immediate ops propagate constants through and/ora/eor/adc/sbc/asl/lsr/neg/inc/dec. Compare instructions derive exact N/Z/C.
  - Phase 4: AssemblerOptimizer integration. MachineState replaces ad-hoc RegState for redundant load elimination and memory tracking.

### Optimizations
- **CMP #0 Elimination**: Assembler optimizer detects redundant `cmp #0` when N/Z flags already reflect the accumulator from a prior load instruction. Safe when only BEQ/BNE/BMI/BPL follow (not BCC/BCS).
- **Store-Forwarding in IR Codegen**: When a STORE's source vreg was produced by the immediately preceding instruction, skip the redundant ZP reload — the result is still in A:X.
- **CONST Direct Store**: When a CONST is immediately followed by STORE to a ZP local, store constant bytes directly to the local's ZP address without round-tripping through a temp vreg.
- **I16 INC/DEC Peephole**: `local += 1` / `local -= 1` for ZP-allocated I16 locals emits `inc $ZP; bne *+4; inc $ZP+1` (5 bytes) instead of full load + add.16 + store + reload + store (~20 bytes). Handles both immediate operands and CONST vregs.
- **Skip TAX/TAY/TAZ for Duplicate CONST Bytes**: When a CONST instruction has identical high/low bytes (e.g., -1 = 0xFFFF) and the destination is ZP-allocated, skip the transfer instruction since storeVreg uses `sta` via `regsEqual()`. Also use `sta` instead of `sty`/`stz` for I32 ZP stores when Y==A or Z==A.
- **ZP Addressing in Text Mode**: Simulated op text emission uses 2-digit hex for addresses < 256, allowing the assembler to select base-page mode (saves 1 byte per instruction).
- **ZP Addressing in Binary Mode**: Simulated ops converted from hardcoded branch offsets to `emitBranchPlaceholder`/`patchBranchTarget` (20 forward branches). All 141 memory-access calls changed from `_abs` to `_addr`, auto-selecting base-page for ZP-range addresses in binary mode.
- **Constant-Memory Forwarding**: M65Emitter `_addr` read-type methods (lda/ldx/ldy/ldz/adc/sbc/and/ora/eor/cmp) check MachineState for ZP locations holding known constants and emit immediate mode instead of memory read. Assembler generator preserves memory state across non-label statement boundaries to enable cross-statement forwarding.
- **Relocatable Constant Forwarding (RELOC_CONST)**: When `lda #<_symbol; sta $ZP` stores a relocatable symbol's lo/hi byte to a ZP scratch location, subsequent reads from that ZP location (`adc $ZP`) are forwarded to immediate mode (`adc #<_symbol`) with proper R_LOW/R_HIGH relocation records. Eliminates ZP memory round-trip for global address arithmetic in relocatable objects. Enabled by per-instruction MachineState tracking in the assembler generator — no blanket register invalidation at statement boundaries.
- **CPX for Unsigned 16-bit Compare**: `cmp.16` now uses `cpx #hi` for the high byte instead of `txa; cmp #hi`, saving 1 byte per unsigned comparison and preserving A. Applies to both immediate and memory operand paths. Signed `cmp.s16` unchanged (needs EOR #$80).
- **Store-Fused I16 Add/Sub**: When an I16 add/sub result is immediately stored to a ZP local, emit `lda lo; clc; adc src; sta $ZP; lda hi; adc src+1; sta $ZP+1` — no ldx, txa, pha, or pla. Loads src1 hi byte directly instead of round-tripping through X.
- **Frame INC/DEC Pseudo-Ops**: New `inc.fp`/`dec.fp` (8-bit) and `inc.16f`/`dec.16f` (16-bit) pseudo-ops for direct stack-relative increment/decrement. IRCodeGen I16 INC/DEC peephole extended to IN_FRAME variables. `gen++` on a stack variable: 25 bytes → 9 bytes.
- **Push/Pull Value Tracking**: MachineState tracks register values through `pha/phx/phy/phz` and `pla/plx/ply/plz` via an 8-entry push stack. Cross-register patterns (`pha; plx`) correctly propagate values. The `add.16 .AX` simulated op's `pha/txa/adc/tax/pla` sequence now preserves knowledge that A holds the lo result after `pla`, enabling downstream forwarding through `sta $ZP`.
- **Reverse Store-Forwarding in Assembler Optimizer**: After `STA $ZP`, zpMem mirrors the register. A subsequent `LDA $ZP` is now detected as redundant when the register hasn't changed (memory mirrors register, not just register mirrors memory). 42 redundant loads eliminated in game_of_life.
- **Per-Simulated-Op Clobber Tracking**: Assembler optimizer no longer blanket-invalidates all state for simulated ops. Each op type declares what it clobbers: `add.16`/`sub.16` etc. clobber A/X/flags but preserve Y/Z/memory; `stax.fp` preserves registers but invalidates memory; `inc.16f` clobbers A/flags/stack.
- **Store-Fused ADDR_ELEM**: Array indexing (`base + index * stride`) emits inline add instead of `add.16` simulated op when result stored to ZP. Eliminates `pha/pla` overhead for every array index computation.
- **Extended Store-Fused Add/Sub**: Store-fused peephole now fires for any ZP-allocated result vreg, not just when followed by an explicit STORE instruction. Catches ADD results that feed into ADDR_ELEM or other ops. Zero `pha/txa/tax/pla` patterns remain in game_of_life.
- **CONST Suppression for ADD/SUB src2**: CONST vregs used only as ADD/SUB src2 operands are now suppressed — no emission, no frame slot. The store-fused ADD and `src2MemOperand` use immediate mode via `vregConstVal_`. Eliminates `lda #N; ldx #0; sta $ZP; stx $ZP+1` for small constant operands.
- **Parameter Narrowing Advisory**: Compiler emits `note:` when a static function's `int` parameter could be `char` — detected by checking all call sites pass values 0-255. Guides user to change signature for I8 codegen benefits.
- game_of_life.prg: **5301 → 4656 bytes** (further reduction from MachineState-enabled optimizations).

## 2026-05-27 (released in v1.0)

### Added
- **`mega65.h` Hardware Register Header**: VIC-IV register struct overlay (`vic4->border = 0`) mapping all 128 registers ($D000-$D07F). Includes nested sprite position struct, `VREG_*` direct register defines, bit/colour constants, and convenience functions (`vic4_unlock`, `vic4_fast`, `vic4_sprite_pos`).
- **SID Struct Overlay**: 4 SID instances (`sid1`-`sid4` at $D400/$D420/$D440/$D460) with voice struct (frequency, pulse width, waveform, ADSR), filter, volume. `SIDn_VOICE(n)` helpers for voice access. Waveform, filter routing, and filter mode constants.
- **CIA Struct Overlay**: 2 CIA instances (`cia1`/$DC00, `cia2`/$DD00) with ports, data direction, timers A/B, TOD clock, serial data, interrupt control, and control registers. ICR/CRA/CRB bit constants, joystick direction constants, `joy1_read()`/`joy2_read()` convenience functions.
- **DMA Controller Struct**: `dma` at $D700 with control, address, and enhanced trigger registers. DMA command constants.
- **Math Accelerator Structs**: `math_div` ($D760) and `math_mul` ($D770) for hardware 32-bit divide and multiply. `MATH_BUSY`, `HW_RANDOM` direct access defines.
- **Audio Mixer Struct**: `audio_mixer` ($D63C) for SID left/right volume panning. `AUDIO_LEFT()`, `AUDIO_RIGHT()`, `AUDIO_BOTH()` helper macros.
- **Memory Pointers**: `SCREEN_RAM` ($0800) and `COLOUR_RAM` ($D800) convenience pointers.
- **Floppy Controller Struct**: `fdc` ($D080) with track/sector/side/data registers and FDC command/status constants.
- **SD Card Controller Struct**: `sd` ($D680) with sector address, command, buffer address, and read data. SD command/status constants.
- **Ethernet Controller Struct**: `eth` ($D6E0) with control, TX frame size, MAC address. ETH control bit constants.
- **Hypervisor Trap Struct**: `hyper` ($D640) with 64-byte trap register array and common trap constants.
- **`key_pressed()` Keyboard Scanner**: `__regparm` library function for direct CIA1 matrix scan. Supports simultaneous multi-key detection. 66 `KEY_*` constants for the full C64/MEGA65 keyboard matrix. Both stack and ZP calling convention variants in `c45.lib`/`c45_zp.lib`.

### Optimizations
- **Constant-Address Direct Store**: `*(volatile unsigned char *)0xD020 = val` now emits `lda #val; sta $D020` instead of loading the address into a ZP pair and using indirect addressing. Tracks vreg constant values through the IR to detect constant addresses at STORE time.
- **Constant-Arg Function Call Push**: When all arguments are simple (constants, globals), pushes inline with `phw #imm16` (3 bytes) instead of round-tripping through ZP temp slots (~100 bytes → ~15 bytes for `memset(grid, 0, 1000)`). 32-bit constants use two `phw`. Falls back to two-phase ZP temps for complex args.
- **CONST Vreg Suppression**: Pre-scan identifies CONST vregs only used in constant-address STOREs or as args to all-simple CALL sites. Suppresses emission, frame slot allocation, and STORE-to-local-slot. `VREG_BORDER = 0` → `lda #0; sta $D020` (2 instructions). Dead call-arg local slots eliminated.
- **Global Address `phw` in Calls**: `phw #_symbol` for global address args (3 bytes, linker-resolved) instead of `ldax #_symbol; push .ax` (5 bytes).
- **PLZ Frame Cleanup**: Function epilogues use `plz` to pop frame slots instead of `pla`. PLZ doesn't clobber A/X/Y, eliminating return value save/restore (4-6 instructions saved per function). I32 returns save Z to scratch.
- **Stdlib Single-TSX Param Load**: `memset`, `memcpy`, `memmove` load all stack params with a single `tsx` instead of one per param. `pha/plx` for Y→X transfer avoids ZP temp.
- **Static DMA Jobs in CRT**: ZP save/restore uses static 12-byte DMA command blocks in data segment instead of runtime-built jobs (~40 bytes each). `__exit`: 95 → 23 bytes.
- **TSX Caching**: `lda_stack` skips redundant `tsx` when X already holds SP within a simulated op expansion. Saves one `tsx` per `ldax.fp` param load.
- **`ldax.fp` pha/plx**: 16-bit stack param loads use `pha/plx` to transfer hi byte instead of ZP scratch temp `$02`. No ZP clobber.
- **Short-Circuit `&&` / `||`**: Logical AND/OR now use short-circuit branching — LHS evaluated first, RHS skipped if result already determined. Matches C standard semantics.
- **ORA Zero-Test**: `x == 0` / `x != 0` for ZP-allocated 16-bit values uses `lda zp_lo; ora zp_hi` (5 bytes) instead of `cmp.16 .AX, #0` (~12 bytes). Sets Z flag directly.
- **CONST Suppression for CMP Operands**: CONST vregs used only as CMP src2 are suppressed — the comparison already reads the constant value directly.
- **Boolean Materialization Elimination**: `if`/`while` with `&&`/`||`/`!` conditions use `emitConditionBranches()` to chain branches directly to true/false targets without materializing intermediate boolean vregs.
- **Unused Static Function Elimination**: Static functions never called by any surviving function are removed at compile time. Handles transitive call chains.
- game_of_life with mega65.h: **7037 → 5301 bytes (25% total reduction)**.

### Improved
- **Function Declaration `.loc`**: Prologue and parameter loading code attributed to the function declaration line (e.g., `int foo(int a)`) instead of the first statement. Makes `objdump45 -d` source interleaving more readable.

### Fixed
- **Cast-Pointer Dereference Store Width (Issue #83)**: `*(volatile unsigned char *)ADDR = val` was generating 16-bit stores, clobbering the adjacent byte. `IRBuilder::getExprTypeInfo()` now derives the correct pointee type from cast expressions (not just variable references), producing proper 8-bit stores.
- **Nested Struct Array Member Access (Issue #84)**: `ptr->array_member[n].field` was generating a spurious dereference (loading from the address instead of using it) and using wrong element stride. Two fixes in `IRBuilder`: (1) array/struct-typed members now decay to address instead of generating a LOAD; (2) `ArrayAccess` now derives element size from struct member info when the array base is a `MemberAccess`. Enables `vic4->sprite[n].x`, `sid1->voice[n].freq_lo` etc.
- **Empty-Parameter Function-Like Macros**: `#define FOO() body` was not expanding when invoked as `FOO()`. The preprocessor now correctly handles zero-parameter function-like macros in both definition parsing and expansion.
- **`-I` Flag Space-Separated Path**: `cc45 -I dir` was treating `dir` as the source file instead of an include path. Now handles both `-Idir` and `-I dir` forms.
- **debug.json Bloat (Issue #85)**: `.debug.json` line map files now only emit entries when file or line changes, using range-based lookup. ~94% size reduction. Fixed in both `ln45` and `ca45`.
- **Cast-Pointer Subscript Element Size**: `((unsigned char *)ADDR)[n]` was using stride 2 (int) instead of 1 (char). `ArrayAccess` now derives element size from the expression's pointed-to type when the root is not a variable or struct member.
- **Undeclared Identifier Diagnostic**: `map::at` crash on undeclared variables replaced with proper `"file:line: error: use of undeclared identifier 'name'"` message via `lookupVar()` helper in CodeGenerator.
- **Error Line Numbers After `#include`**: Preprocessor now emits `#line` directives after includes; lexer consumes them to restore original source line numbering. Error messages and `.loc` debug info now report original source lines instead of preprocessed line numbers.
- **`.o45` Line Map Offsets (Issue #87)**: Line map entries in `.o45` files were offset by the data segment size, causing wrong source line attribution in `objdump45` and `debug.json` after linking. Fixed by computing offsets relative to the code segment start instead of global base 0.
- **`.loc` Before Function Prologue**: Function prologues (`phw` frame setup) were attributed to the previous function's last source line. Now emits `.loc` before the prologue so `objdump45` shows the correct function entry line.
- **objdump45 TEXT/DATA Boundary**: Disassembly now stops at the TEXT segment end when a map file is available, preventing DATA arrays from being shown as `brk` instructions.
- **objdump45 Source Line Lookup**: Source line interleaving now uses range-based lookup (`upper_bound`) instead of exact match, working correctly with deduplicated debug entries.
- **cc45 Binary Path Resolution**: System include path (`lib/include`) now resolved via `/proc/self/exe` or `realpath()` instead of `argv[0]`, fixing include search when invoked from any directory.
- **Removed `.debug.json` Generation**: `ln45` and `ca45` no longer emit `.debug.json` files. Use `-M` flag for map file output which contains correct line info.
- **`.word` Symbol References (Issue #88)**: `.word` directive now supports symbol operands with relocations, enabling static data tables with linker-resolved addresses.
- **ln45 Unused Symbol Warnings**: Suppressed "unused global symbol" warnings for `__`-prefixed internal symbols (CRT internals like `__exit`, `__init`, `__sp_base`). Only user-facing symbols are reported.

## 2026-05-26 (released in v1.0)

### Added
- **`math.h` Integer Functions**: `min(a,b)`, `max(a,b)`, `gcd(a,b)`, `lcm(a,b)` — each in a separate translation unit for selective linking. Both stack and ZP calling convention variants.
- **`abs`/`labs` Macros**: `abs` and `labs` default to inline macros in `stdlib.h`, eliminating call overhead. Define `__NOMACRO_ABS` or `__NOMACRO_LABS` to use the function versions instead.
- **Thunk Warning by Default**: The linker now always warns when generating calling convention bridge thunks, not just with `-Wthunk`. Use `--no-thunks` to error instead.
- **`time.h`**: `clock()`, `time()`, `difftime()`, `CLOCKS_PER_SEC` (60Hz jiffy clock). Both stack and ZP calling convention variants.
- **`#pragma cc45` Namespace**: All compiler pragmas now use the `#pragma cc45 <option>` prefix (e.g., `#pragma cc45 heap`, `#pragma cc45 no_bssinit`). **Breaking change**: `#pragma crt <option>` is no longer accepted. `#pragma once` and `#pragma weak` remain unchanged.
- **Inline Function Expansion**: `inline` keyword now triggers function body inlining at call sites. `-finline-functions` flag enables auto-inlining of small functions (≤20 statements) even without the `inline` keyword. Eliminates JSR/RTS overhead for small leaf functions. Recursive, variadic, interrupt, and naked functions are excluded.
- **`repeat()` Loop Unrolling**: Compile-time loop unrolling via `repeat(N) { body }` and `repeat(type var, N) { body }` with optional loop variable (1..N). Zero loop overhead — body is duplicated N times.
- **DMA Intrinsics**: `__dma_copy(dst, src, len)` and `__dma_fill(dst, len, val)` for MEGA65's F018B DMA controller (~40MB/s). Compiler builtins, no library needed.
- **`__regparm` Function Attribute**: First parameter passed in A (char) or AX (int/pointer) register instead of ZP/stack. Saves 2–4 bytes and 3–6 cycles per call. Remaining parameters use normal convention.
- **`.repeat N` / `.endrepeat` Assembler Directive**: Compile-time loop unrolling in assembly. Supports nesting and expression counts.
- **`-Roptimizer` Flag**: Reports assembler optimizer actions to stderr with source file, line, pass name, and action (for both `ca45` and `cc45`).
- **`-Rcodegen` Flag**: Annotates assembly output with IR codegen reasoning comments.
- **JMP→BRA Peephole**: Converts 3-byte JMP to 2-byte BRA when target within ±127 bytes.
- **JSR→BSR Peephole**: Converts JSR to BSR (relative16) for local symbols, enabling position-independent code.
- **Standalone CRT Return Value**: Main return value preserved across ZP restore via $02/$03.
- **`__interrupt` / `__naked` Function Attributes**: `__interrupt` emits PHA/PHX/PHY/PHZ + RTI; `__naked` suppresses all prologue/epilogue.
- **Case Ranges**: GCC-style `case 'A' ... 'Z':` syntax for matching value ranges in switch statements.
- **Packed Structs by Default**: All structs packed (no alignment padding). `__unpacked` keyword opts into traditional C alignment.
- **`-Rcodegen` Flag**: Annotates assembly output with codegen reasoning comments showing which IR instruction generated each line.
- **objdump45 Enhancements**: Auto-discovers `.map` files for PRG symbol annotation; interleaves source line comments in disassembly; linker map includes Source Lines section.
- **Integer Literal Auto-Promotion**: Literals > 65535 automatically promoted to `long` with compiler warning.

### Optimizations
- **palette_fade.prg: 2151 → 1003 bytes (53% reduction)** across all optimizations below:
- **Fused Compare-and-Branch**: CMP/AND/OR result + BR_COND → direct branch on flags, skipping boolean materialization.
- **BBS/BBR Single-Bit Tests**: `if (var & power_of_2)` on ZP variables emits 3-byte BBS/BBR instruction.
- **Branch Inversion**: `Bcc +2; BRA target` → `B!cc target`, saves 2 bytes per occurrence.
- **Pointer Constant Propagation**: `*p = val` where p holds constant address → `sta $ADDR` directly.
- **Dead Local Variable Elimination**: IR pass removes unread vregs and dead stores.
- **8-bit Char Arithmetic**: Native `and`/`ora`/`eor`/`adc`/`sbc`/`cmp`/`inc a`/`dec a` for char operations.
- **Bidirectional Store/Load Tracking**: Assembler eliminates `lda $ZP` after `sta $ZP` when A still holds the value.
- **Tail-Call Optimization**: `JSR + RTS` → `JMP` in assembler optimizer.
- **32-bit Shift Byte-Shuffle**: Immediate shifts of 8/16/24 bits use register shuffling instead of loops.
- **`while(1)` Constant Loop**: Detected at IR level, emits single unconditional back-branch.
- **Dead Trailing Block Elimination**: Unreachable code after infinite loops removed.
- **Void Call Cleanup**: `CALL_VOID` for void functions prevents spurious return value stores.
- **Return Constant**: `return N` emits immediate load directly, no vreg round-trip.
- **`chknonzero.8` Elimination**: Redundant `cmp #$00` removed when Z flag already set from load.
- **`loadVregA`/`loadOperandA`**: I8-only load paths skip `ldx #0` zero-extension.

### Fixed
- **`rts #N` Stack Mismatch**: `B#` (char) parameters counted as 1 byte for stack cleanup but caller pushes 2 bytes (C promotion). Fixed to use promoted size.
- **`cmp.32` A-Clobber**: 32-bit comparison clobbered A (byte 0) before final byte comparison. Fixed by saving A to ZP scratch.
- **ZEXT I8→I32**: Zero-extension from 8-bit to 32-bit now clears Y and Z registers.
- **Signedness Defaults**: Integer literals and function returns default to unsigned, matching cc45's type system.
- **CRT ZP Save/Restore**: `crt0.o45` and `crt0_zp.o45` now save/restore ZP $08-$FF using `move` pseudo-op.
- **Legacy Codegen Removed**: `--legacy-codegen` flag removed; all compilation uses IR pipeline.

## 2026-05-22 (released in v1.0)

### Added
- **CPU and Flag Intrinsics**: High-level access to processor state via `__cpu.R` and `__flags.F`.
    - Supports all physical registers (A, X, Y, Z) and virtual pairs/quads (AX, Q, etc.).
    - Supports processor flags (Carry, Zero, Negative, Overflow).
    - Optimized code generation: `__flags.Carry = 1` emits `SEC`; `__cpu.X = 5` emits `LDX #5`.
    - Integrated safety: compile-time error if taking address of registers/flags.
- **`mod.32` support**: Added 32-bit unsigned modulo support to the assembler.
- **Hardware Register Header**: Centralized MEGA65 math and DMA register definitions in `include/Mega65Registers.hpp`.

### Fixed
- **Math Accelerator**: Corrected register addresses for Quotient and Remainder based on empirical simulator probing.
- **`realloc` memory corruption**: Fixed a bug where simulated operations clobbered scratch registers used by the memory allocator.
- **DMA Buffer Hazards**: Implemented explicit BSS reservation for FILL and MOVE buffers in the assembler.
- **Relocation Signedness**: Improved linker support for signed offsets on external symbols.

## 2026-05-17 (released in v1.0)

### Added
- **Compiler (cc45)**:
    - **Zero Page Preservation**: Automatic preservation of Zero Page state ($08-$FF) at program entry/exit. This ensures that compiled C programs do not corrupt the system's BASIC/KERNAL ZP state when they return to the OS. Controlled by the new `saveZP` flag in `ir::Module`.
    - **`#pragma no_zp_save`**: Allows developers to explicitly disable ZP preservation when not needed (e.g., in standalone firmware).
    - **Unused Variable Warnings**: The compiler now warns about unused local variables and function parameters, including filename and line number info. Integrated with `ConstantFolder` to prevent false positives for folded expressions.
- **Assembler (ca45)**:
    - **8-bit Operand Overflow Warnings**: Emits a warning when a 16-bit value is used for an 8-bit addressing mode (like `#immediate` or base-page), preventing silent truncation bugs.
    - **Procedure Scoping Validation**: Enhanced validation of `proc`/`endproc` blocks. Detects and reports errors for nested procedures, `endproc` without a matching `proc`, and unclosed procedures at the end of the file.
- **Linker (ln45)**:
    - **Unused Global Symbol Warnings**: The linker now warns about exported global symbols that are not referenced by any other module, helping identify dead code at the link stage.

### Fixed
- **Compiler (cc45)**:
    - **IR Pipeline Stabilization**: Unified type propagation in `IRBuilder`, fixed 32-bit arithmetic operations, array indexing, and pointer dereference logic. Corrected `IntegerLiteral` handling to use 64-bit values to prevent early truncation.
    - **VReg Allocation Accuracy**: Fixed `VRegAllocator` to properly track all local variables, preventing memory corruption from overlapping ZP or frame slot assignments.
    - **32-bit Memory Operations**: Fixed clobbering of registers during 32-bit indirect loads and stores by ensuring correct usage of scratch ZP registers.
- **Assembler (ca45)**:
    - **M65Emitter Scratch ZP Fix**: Corrected a bug where scratch ZP registers used by simulated operations were hardcoded to $02-$06. They are now correctly initialized relative to the user-specified `zeroPageStart` (e.g., $08). This prevents simulated ops from clobbering BASIC/KERNAL memory.
    - **Improved Error Reporting**: Refactored error handling to ensure each diagnostic is printed only once and correctly results in a non-zero exit code.
- **Linker (ln45)**:
    - **Symbol Resolution Robustness**: Fixed edge cases in symbol resolution and improved internal state management during the multi-pass linking process.

## [v1.0-rc2] - 2026-05-11

### Fixed
- **Assembler (ca45)**:
    - **O45Emitter symbol segment misassignment (Issue #45-2)**: Symbols defined after a `.data` or `.bss` directive could be incorrectly tagged as TEXT segment in the `.o45` output. Root cause: the O45Emitter inferred segment membership from address ranges (`findSegmentForAddress()`), which failed at segment boundaries where the code segment's PC was adjacent to the data segment's start. Fix: use the authoritative `sym->segment` field set at parse time instead of address-range inference. Also fixed missing segment fields on `.array`, `proc`, `__fill_dma_buf`, and `__move_dma_buf` symbol initializers. The `findSegmentForAddress()` function has been removed.
    - **Segment `startAddress` going stale across pass2 iterations (Issue #45)**: When assembling `.o45` files with both `.code` and `.data` sections, forward-reference resolution caused pass2 to run multiple iterations. On the first iteration, unresolved instructions were oversized, inflating the code section. The data segment's `startAddress` was set on this first iteration and never recalculated, creating a gap between TEXT and DATA segments. This caused `findSegmentForAddress()` to fail for data symbols, which then defaulted to `SEG_TEXT` with wrong offsets. After linking, data symbols overlapped with code symbols at the same addresses. Fix: reset segment `startAddress` at the start of each pass2 iteration.
- **Linker (ln45)**:
    - **Cross-segment address collision detection**: The linker now tracks the segment ID of each exported symbol and emits a warning when two symbols from different segments are placed at the same address. This catches assembler segment bugs early instead of producing silently-corrupt binaries.

### Added
- **Testing**:
    - **Segment emission test suite** (`make test-segment-emission`): 55 new tests covering end-to-end segment assignment (data/bss/boundary/multi-segment/proc labels), multi-object linking (no overlap, BSS placement, unique addresses, cross-object relocations), and regression tests for Issue #45-2.

### Changed
- **Assembler (ca45)**:
    - **Bare `.text` no longer switches segments**: Previously, `.text` without arguments was an alias for `.code` (segment switch). This created ambiguity with the `.text "string"` PETSCII data directive. Bare `.text` is no longer recognized as a segment switch. Use `.code` instead. The `.segment "text"` form and `.text "string"` data directive are unaffected.

### Known Limitations
- **Custom segments in `.o45` mode**: User-defined segments (`.segment "myname"`) are mapped to `SEG_TEXT` in `.o45` output. The `.o45` format supports four segment IDs: TEXT, DATA, BSS, and ZP. Custom segment names are preserved via `OPT_SEGATTR` option records, enabling linker-side ordering (e.g., `"init"` before `"code"`). Custom segments work correctly in flat binary mode; in relocatable builds they share the TEXT segment body but retain their name metadata.

## 2026-05-09 (released in v1.0-rc2)

### Added
- **Linker (ln45) & Object Format (`.o45`)**:
    - **Calling convention enforcement**: Added `FUNC_FLAG_ZP_CONV` bit (0x04) to `O45FuncAttr.flags` to track calling convention per-function. The linker now enforces that ZP-convention callers (compiled with `-fzpcall`) do not call stack-convention callees, catching a class of silent wrong-answer bugs. Enforcement is one-directional (ZP→stack only) to permit intentional `__fastcall__` interop where stack-convention code generates ZP param setup before calling. Hard linker error on mismatch.
    - **`FUNC_FLAG_REENTRANT` semantic clarification**: Renamed the previously-unused `INT_SAFE` flag to `FUNC_FLAG_REENTRANT` (bit 0x02) to capture the more general property: a function that is re-entrant safe (no global mutable state, stack-only locals). This is the superset property of which interrupt-safety is one consequence.
- **Assembler (ca45)**:
    - **`.func_flags` directive**: New directive for hand-written assembly to annotate function calling convention and properties. Syntax: `.func_flags zp_call [, stack_call] [, leaf] [, reentrant]`. Parsed within `proc`/`endproc` blocks. Integrates seamlessly with automatic metadata emission from the compiler.
- **Compiler (cc45)**:
    - **Automatic calling convention metadata emission**: Both ZP and stack code paths now emit `.func_flags zp_call` / `.func_flags stack_call` directives automatically for every function, with `leaf` annotation when no calls are present. Enables linker enforcement without requiring manual annotation in C source.
- **Tools**:
    - **nm45 symbol display**: Updated `nm45 -f` to display calling convention and function flags in symbol listings. Example: `[zp_call leaf uses:- clobbers:- ...]`.
    - **objdump45 symbol table**: Added function attribute display to symbol table output, showing convention and flags for each exported function.
- **Documentation**:
    - **`doc/architecture/lib45.md` Section 4.4**: Complete specification of function attribute records: marker byte `$FA`, flags byte (LEAF, REENTRANT, ZP_CONV), register/flag clobber masks, ZP usage/clobber bitmasks. Documents linker enforcement rule (ZP→stack errors, stack→ZP permitted).

## 2026-05-08 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **`_Alignas` support for local variables and struct members (Phase 1)** (d1debd3): Alignment computation and storage reservation implemented. Local variables and struct members can now be declared with `_Alignas(N)` specifiers. The compiler computes required padding between fields to meet alignment constraints and reserves additional frame space as needed. Struct member layout respects alignment boundaries.
    - **Symbolic `.fp` references for local variables (Phase 2)** (6ab142e): Frame-pointer-relative variable references now use symbolic names (e.g., `.fp var_name`) in addition to numeric offsets. The assembler's symbol table supports frame-relative symbol lookup, enabling more readable and maintainable assembly output.
- **Assembler (ca45)**:
    - **Dead code elimination after infinite loops (Issue #26)** (7724cfa): The assembler now detects and removes unreachable code that follows infinite loops (e.g., code after `while(1)` constructs). Instructions after `BRA` loops are recognized as dead code and eliminated, reducing binary size and improving code clarity.
    - **Fix BinaryExpr reentrancy bug in assembler expression evaluator (Issue #35)** (8bb0764): Fixed a critical bug where recursive evaluation of binary expressions in the assembler's expression evaluator could cause incorrect operand values or hangs. The fix ensures that complex nested expressions are evaluated correctly even when the evaluator processes sub-expressions recursively.
    - **3-operand MOVE/FILL test coverage**: Added comprehensive test cases for the 3-operand syntax (`MOVE src, dest, len` and `FILL dest, len`) that was previously undocumented. The syntax allows explicit specification of source, destination, and length operands without requiring preloaded register pairs. Tests include: immediate operands, register pair operands, symbol operands, stack-relative operands, and mixed operand types. All 9 new C++ tests pass; assembly examples in `test_fill_advanced.s45` demonstrate practical usage. Parser support for 3-operand syntax was already implemented in `AssemblerSimulatedOps::emitMoveCode()` — tests validate and document this capability.
- **Documentation**: Updated `doc/architecture/opcodes.md` with complete syntax documentation for 2-operand and 3-operand forms of `MOVE` and `FILL`, including usage examples.

## 2026-05-05 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **`#pragma once`**: Replaces the non-standard `#pragma include_once` with the widely-supported `#pragma once` for header guard optimization. The old directive has been removed.
    - **Type qualifier ordering**: All orderings of type qualifiers are now accepted across all declaration contexts (globals, locals, parameters, return types, typedefs). For example, `volatile signed int`, `int volatile`, `signed volatile int`, and `unsigned const char` all parse correctly. Previously, qualifiers were required to appear before signedness specifiers.
- **Assembler (ca45)**:
    - **`defined(<symbol>)` in expressions**: Test whether a symbol exists at assembly time (post-pass-1). Returns 1 if the symbol is defined, 0 otherwise. Works in any expression context (e.g., `.if defined(MY_FLAG)`). Accepts both IDENTIFIER and INSTRUCTION tokens as symbol names.
- **Standard Library (stdlib45.lib)**:
    - **`stdio.h`**: `getchar()` — Input a single character via KERNAL `GETIN` ($FFE4). Busy-waits until a key is pressed.
    - **`ctype.h`**: Added 6 PETSCII-aware character classification functions: `isupper` ($C1-$DA), `islower` ($41-$5A), `isxdigit` (digits + hex letters in both PETSCII cases), `ispunct` (printable non-alnum non-space), `isblank` (space $20 or tab $09), `iscntrl` ($00-$1F and $7F-$9F). All implemented in hand-written 45GS02 assembly for both stack and ZP calling conventions.
    - **`string.h`**: Added 6 string functions: `strncat` (concatenate with length limit, assembly), `strstr` (substring search, C), `strpbrk` (find first char from set, C), `strspn` (count leading chars in set, C), `strcspn` (count leading chars not in set, C), `strtok` (tokenize string, C with static state). All implemented for both calling conventions.
    - **`stdlib.h`**: `strtol` (string to signed long) and `strtoul` (string to unsigned long). Support base 0 (auto-detect), 8, 10, 16 with optional `0x`/`0X` prefix. Handle PETSCII letter ranges for hex digits. `strtoul` is the core implementation; `strtol` wraps it with sign handling.

### Fixed
- **Assembler (ca45)**:
    - **O45 relocation for equate-based ZP operands**: Fixed corrupt relocations when assembling instructions with equate-defined zero-page operands (e.g., `ptr = $20; lda ptr`). The O45 emitter was using the stale `stmt->instr.mode` (ABSOLUTE, the parser's default for unresolvable identifiers) instead of the resolved mode (BASE_PAGE, determined by `calculateInstructionSize` in pass 2). This caused 16-bit `R_WORD` relocations to be emitted for 1-byte ZP operands, corrupting both the operand byte and the adjacent opcode during linking. Fixed by re-resolving the addressing mode in the O45 emitter before generating relocations.
- **Compiler (cc45)**:

### Changed
- **Preprocessor (cp45)**:
    - Removed `#pragma include_once` directive. Use `#pragma once` instead (the standard form supported by GCC, Clang, and MSVC).

## 2026-05-04 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **Fine-grained clobber tracking (Phase 1)**: The compiler now tracks which CPU registers (A, X, Y, Z) and processor flags (C, N, Z, V) each function actually modifies during code generation. Per-function `FuncClobberInfo` records are built incrementally as code is emitted. Accurate `.reg_clobbers` and `.flag_clobbers` directives are emitted for both ZP and stack calling conventions (previously only a blanket `A, X, Y, Z` was emitted for zpcall functions, nothing for stack convention). Leaf fastcall functions like `int identity(int x) { return x; }` now correctly report `.reg_clobbers A, X` instead of all four registers. Added `invalidateFromClobbers(regMask, flagMask)` for future Phase 2 selective invalidation at call sites.

### Fixed
- **Assembler (ca45)**:
    - **Assembler simulated op size drift**: Fixed a major source of address drift where simulated opcodes produced different code lengths between pass 1 and pass 2, especially when operands involved forward references. This was addressed by implementing a "conservative sizing" strategy: in pass 1, if an operand's type or value is uncertain, the assembler assumes the largest possible code path (e.g., stack-relative for `ptrstack`). This prevents instructions from growing in size during pass 2, which ensures convergence. Also added `isPass1()` to `AssemblerParser` for this purpose, and refined `isStackRelativeOperand` and similar operand evaluation logic to handle forward references gracefully during pass 1.
- **Compiler (cc45)**:
    - **ConstantFolder type preservation**: Updated `ConstantFolder` to track and propagate C types along with folded values. When a `long` variable is replaced by a folded `IntegerLiteral`, the literal now preserves the original `"long"` type in its `castType` field. This ensures that variadic functions like `printf` correctly push 4 bytes for constant-folded long arguments instead of defaulting to 2-byte `int`.
    - **Branch-aware constant invalidation**: Implemented constant state merging in the `IfStatement` visitor. Constants are now independently tracked through `then` and `else` branches and merged via intersection. A variable is only considered constant after an `if` if it holds the same constant value in all possible paths. Reassignments within branches now correctly invalidate any pre-existing constant assumptions.
    - **CompoundStatement scope leakage**: Removed incorrect state restoration at block end that was causing constants assigned within nested scopes to be incorrectly discarded.
- **Standard Library (stdlib45.lib)**:
    - **ZP Safety & Relocation**: Relocated all standard library assembly functions (`atoi`, `itoa`, `ltoa`, `malloc`, `memcpy`, `strcmp`) to use the hardware stack for ZP preservation. All library functions now push clobbered ZP registers to the hardware stack at entry and restore them via `plz / stz $xx` at exit. This eliminates ZP conflicts between the library and the compiler's temporary register pool ($02-$0A).
    - **Hardware Math Stability**: Added busy-wait loops on `$D7FE` bit 7 in `itoa` and `ltoa` to ensure the MEGA65 hardware divider has finished calculation before results are read. Fixes race conditions that produced empty or corrupted string output in `sprintf` / `%ld`.
    - **Preservation of Return Values**: Refactored restoration logic to use the Z register as a temporary scratch, ensuring that 16-bit (`AX`), 24-bit (`AXY`), and 32-bit (`AXYZ`) return values are fully preserved without clobbering the Accumulator.
    - **Stack Addressing Fixes**: Updated library functions to use correct stack-relative syntax (`(offset, sp), y` and `, s`) after ZP-push shifts.

## 2026-05-02 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **`long` type**: Full support for 32-bit `long` and `unsigned long` integer types. Values use the Q register (AXYZ) (A=byte0, X=byte1, Y=byte2, Z=byte3). Supports global and local variable declarations, function parameters (4-byte push), function return values (hidden-pointer ABI, same as struct returns), arithmetic (`+`, `-`, `*`, `/`, `%`), bitwise (`&`, `|`, `^`, `~`), comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`), shifts (`<<`, `>>`), unary negation (`-`), increment/decrement (`++`/`--` via native `incq`/`decq` for globals), and type casts (widening via `sxt.16`/zero-extend, narrowing by truncation). `sizeof(long)` returns 4. Integer literal suffixes `L`/`l`/`U`/`u` are accepted. Global data emitted via `.dword` directive. Leverages native 45GS02 Q register operations (`ldq`/`stq`, `adcq`/`sbcq`, `andq`/`oraq`/`eorq`, `cmpq`, `aslq`/`lsrq`/`rolq`/`rorq`, `incq`/`decq`) for efficient 32-bit code on absolute addresses.
    - **Compound literals**: Support for C99 compound literals — `(struct Point){1, 2}`, `(int){42}`, `(int[]){1,2,3}`. Creates anonymous temporary values inline. Struct and array compound literals allocate frame temporaries and return their address; scalar compound literals produce the value directly. Works as variable initializers (`struct Point p = (struct Point){10, 20}`), function arguments (`foo(&(struct Point){1, 2})`), and in general expression contexts. New `CompoundLiteral` AST node with full support across Parser, CodeGenerator, ConstantFolder, FrameScanner, and all internal AST visitors.
    - **Struct initializer lists**: `struct Point p = {1, 2}` now initializes struct members in declaration order (by offset). Previously, struct locals with initializer lists only worked for arrays. The frame-local VariableDeclaration handler now detects struct types and initializes each member individually at its correct offset.
    - **Bitfields**: Full support for C-standard bitfield declarations in structs and unions. Syntax: `unsigned char mode : 3;`, `unsigned int counter : 10;`. Storage units are automatically packed — consecutive bitfields of the same type share a byte (for `char`) or word (for `int`), with a new unit started when the type changes or the field won't fit. Layout is conformant with C99 §6.7.2.1 (never spans storage unit boundaries). Read, write, increment/decrement, and arrow-access all work. `&struct.bitfield` is correctly rejected as a compile error.
- **Assembler (ca45)**:
    - **Bitfield pseudo-ops**: Eight new simulated opcodes for bitfield extract and insert:
        - `bfext #bitoff, #width` — extract bitfield from A (8-bit storage unit). Shifts right and masks.
        - `bfext16 #bitoff, #width` — extract from AX (16-bit storage unit). Result in A (with X=0 if ≤8 bits).
        - `bfins addr, #bitoff, #width` — insert A into absolute/ZP storage byte. Uses TRB+TSB for atomic read-modify-write on ZP and absolute targets.
        - `bfins.sp addr, #bitoff, #width` — insert into stack-relative storage byte. Uses shift/mask/ORA sequence.
        - `bfins.ind $zp, #bitoff, #width` — insert via indirect ZP pointer.
        - `bfins16` / `bfins16.sp` / `bfins16.ind` — 16-bit storage unit variants of the above.
    - All `bfins` variants use the 45GS02's TRB (Test and Reset Bits) and TSB (Test and Set Bits) instructions where possible (ZP and absolute addressing modes), providing atomic read-modify-write with fewer instructions than the general shift/mask/ORA approach.
    - **32-bit simulated opcodes**: Sixteen new `.32` and `.s32` pseudo-ops for 32-bit operations on the Q register (AXYZ): `add.32`, `sub.32`, `and.32`, `ora.32`, `eor.32`, `cmp.32`, `cmp.s32`, `neg.32`, `not.32`, `abs.32`, `lsl.32`, `lsr.32`, `rol.32`, `ror.32`, `asr.32`, `sxt.16` (sign-extend AX to AXYZ). Plus signed variants (`add.s32`, `sub.s32`, `neg.s32`, `abs.s32`, `asr.s32`, `lsl.s32`, `lsr.s32`, `rol.s32`, `ror.s32`). All use deferred branch patching (`emitBranchPlaceholder`/`patchBranchTarget`) for correct offset resolution. These complement the native Q register operations for cases requiring register-only or simulated addressing modes.
    - **Q register instruction aliases**: Added `adcq`, `sbcq`, `andq`, `oraq`, `eorq`, `cmpq`, `incq`, `decq`, `aslq`, `lsrq`, `rolq`, `rorq`, `negq` as recognized instruction mnemonics. These are NOP-NOP-prefixed versions of their 8-bit counterparts, operating on the full 32-bit Q register (AXYZ).
    - **`D#` parameter size prefix**: The `proc` directive now accepts `D#` (dword, 4 bytes) in addition to `B#` (byte) and `W#` (word) for parameter size declarations, enabling 32-bit parameter passing.
- **Standard Library (stdlib45.lib)**:
    - **`ltoa(long value, char *str, int base)`**: 32-bit long-to-string conversion. Uses the MEGA65 hardware divider's full 32-bit capability. Supports bases 2–36, signed negation for base 10. Declared in `<stdlib.h>`.
    - **`printf` / `sprintf` long format specifiers**: Added `l` length modifier for 32-bit arguments: `%ld` (signed decimal), `%lu` (unsigned decimal), `%lx` (hexadecimal), `%lo` (octal), `%lb` (binary, sprintf only). When `l` is present, 4 bytes are read from the variadic argument list and converted via `ltoa`.
    - **`sscanf(char *str, char *fmt, ...)`**: Formatted string input parsing. Supports `%d`, `%u`, `%x`, `%o`, `%c`, `%s`, `%%`, and long variants `%ld`, `%lu`, `%lx`, `%lo`. Handles leading whitespace, optional sign for `%d`, hex prefixes (`$`, `0x`) for `%x`, and whitespace-delimited strings for `%s`. Returns the number of successfully matched and assigned items. Compiled as a separate object file (`sscanf.o45`) so it is only linked when referenced. Declared in `<stdio.h>`.

### Fixed
- **Assembler (ca45)**:
    - **`ldq`/`stq` missing Q prefix**: `ldq` and `stq` were being assembled without the NOP-NOP prefix required by the 45GS02 to operate on the Q register, causing them to behave as regular `lda`/`sta`. Both sizing (pass1) and emission (pass2) now correctly emit the 2-byte Q prefix for `ldq`/`stq`.
    - **Q-suffixed instructions in accumulator mode**: Instructions like `lsrq`, `aslq`, `rolq`, `rorq` with no operand were rejected because the parser set IMPLIED mode while the opcode database only had ACCUMULATOR entries. Added fallback from IMPLIED to ACCUMULATOR in opcode lookup.
- **Compiler (cc45)**:
    - **PETSCII/ASCII encoding mismatch**: Character literals (`'A'`, `'d'`, etc.) now apply the same PETSCII case-swap conversion as the assembler's `.text` directive. Lowercase `'a'`–`'z'` map to `$41`–`$5A`, uppercase `'A'`–`'Z'` map to `$61`–`$7A`, matching the bytes emitted for string literals. This fixes comparisons like `*str == 'd'` when `str` points into a PETSCII-encoded string. Previously, char literals used raw ASCII values, causing mismatches.
    - **`-O0` global pointer initializers**: Global variables with non-literal initializers (e.g., `volatile char *r = (char *)0x4000`) now emit correct `.word`/`.byte` values with `-O0`. Previously, `-O0` skipped constant folding, leaving the initializer as a `CastExpression` which `emitData()` couldn't handle, falling back to `.res` (uninitialized). Added `tryEvalConstInt()` — a recursive evaluator that handles `CastExpression`, unary `-`/`~`, and binary arithmetic on constants.

### Testing
- Added `test_compound_literal.c` — mmemu validation test for compound literals (7 sub-tests): struct compound literal as variable initializer, scalar int/char compound literals, struct compound literal passed as pointer argument to function, struct with expression values, zero-init struct. Verified at `$4000`: `1E 2A 07 2C 01 14 00`.
- Added `test_bitfield.c` — compiler test validating bitfield struct declarations, read, write, increment, and 16-bit bitfields compile and assemble.
- Added `test_bitfield_mmemu.c` — mmemu runtime validation test for bitfields. Tests 8-bit bitfield write/read/increment (`active:1`, `mode:3`, `priority:4`) and 16-bit bitfield write/read (`counter:10`, `channel:6`). Verified at `$4000`: `01 05 0C 06 F4 1E`.
- Added `test_long.c` — compiler test validating long type declarations, arithmetic, function params/returns, unary ops, casting, sizeof, and bitwise operations compile and assemble correctly.
- Added `test_long_mmemu.c` — mmemu runtime validation test for long type (12 sub-tests): sizeof(long)=4, function call with 32-bit args and hidden-pointer return, unsigned comparison, low-byte extraction via cast, int↔long casting, 32-bit overflow (incq wrapping), and global-to-global 32-bit addition with full 4-byte verification (100000+200000=300000). Verified at `$4000`: `04 C0 01 A0 2A A0 00 E0 93 04 00 AA`.
- Added `test_32bit_ops.s45` — assembler-level mmemu validation test for 32-bit operations: native Q register add (`adcq`), subtract (`sbcq`), bitwise OR (`oraq`), negation (`neg.32`), and sign extension (`sxt.16`). Verified at `$4000`: `E0 93 04 00 A0 86 01 00 E0 8F 03 00 60 79 FE FF` and at `$4010`: `FF FF FF FF`.

### Changed
- **Testing**:
    - Removed `while(1){}` infinite loops from test programs (`test_short.c`, `test_struct_return.c`, `test_inline_asm.c`, `test_mmemu_control.c`). Programs now return cleanly via RTS to the caller, matching standard CRT `exit_rts` behavior.

## 2026-05-01 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **Frame pre-allocation**: Local variables are now pre-scanned and allocated as a single frame in the function prologue, replacing the incremental push-per-declaration model. A new `FrameScanner` AST walker computes `maxFrameSize` and assigns fixed frame offsets to each local variable with scope-aware slot reuse — variables in non-overlapping scopes (e.g. separate `{ }` blocks, if/else branches) share frame slots, reducing stack usage. The entire frame is zeroed at function entry via `PHW #0` instructions. Local variable initialization now writes directly to pre-allocated frame slots using `sta.fp`/`stax.fp` pseudo-ops. Only `_fp` needs bumping when pushing function call arguments — not every local variable individually. Parameters use `.var` at `frameSize + 2 + paramOff` (past frame + return address). Resolves the "scoped variable lifetime" issue from the 1.0 roadmap.
- **Assembler (ca45)**:
    - **`ldxy` / `stxy` simulated ops**: Load/store 16-bit values into the X(lo)/Y(hi) register pair. Supports immediate (`ldxy #$1234`), absolute (`ldxy addr`), and stack-relative (`ldxy var, s`) operands with frame-relative symbol detection. Useful for KERNAL calls that take pointer arguments in X/Y.
    - **`.local` directive**: Declares frame-relative variables with fixed offsets. Syntax: `.local name = offset`. Creates symbols with `isFrameRelative=true` and `frameOffset=offset`. Re-evaluated in pass 2 like `.var`. Used by the compiler's frame pre-allocation to define locals at compile-time-constant frame offsets.
    - **Frame-pointer pseudo-ops**: Six new simulated ops for frame-relative variable access:
        - `lda.fp offset` — 8-bit load from frame slot into A (`TSX; LDA spBase+_fp+offset,X`)
        - `sta.fp offset` — 8-bit store from A to frame slot
        - `ldax.fp offset` — 16-bit load from frame slot into AX (hi→scratch, lo→A, scratch→X)
        - `stax.fp offset` — 16-bit store from AX to frame slot (X→scratch, A→stack, scratch→stack+1)
        - `leax.fp offset` — load effective address of frame slot into AX
        - `move.fp dest, src, len` — DMA block copy between frame-relative addresses
    - All frame ops use direct stack access (`TSX; LDA/STA base+offset,X`), not the `($nn,SP),Y` indirect mode.

- **Compiler (cc45)**:
    - **`short` type**: `short`, `unsigned short`, and `signed short` are now accepted as type specifiers in all contexts: variable declarations, function parameters, return types, cast expressions, `sizeof`, and `_Generic` associations. On this 16-bit target, `short` is an alias for `int` (2 bytes). Added `SHORT` token type, lexer keyword, and parser support across all type-parsing locations.
- **Standard Library (stdlib45.lib)**:
    - **`sprintf(buf, fmt, ...)`**: Variadic formatted string output. Supports `%d` (signed decimal), `%u` (unsigned decimal), `%x`/`%X` (hex), `%s` (string), `%c` (char), `%%` (literal percent), `%o` (octal), `%b` (binary), `%p` (pointer with `$` prefix). Written in C, compiled to .o45, selectively linked. Uses `itoa` for numeric conversions.
    - **`printf(fmt, ...)`**: Variadic formatted output to screen via `putchar`. Same format specifiers as `sprintf`. Does not append newline. Written in C.
- **Headers**:
    - **`stdio.h`**: Added `sprintf` and `printf` declarations.
    - **`cbm.h`**: New header providing inline C wrappers for the standard Commodore KERNAL jump table. 25 functions covering character I/O (`cbm_chrout`, `cbm_chrin`, `cbm_getin`), I/O status (`cbm_readst`), file operations (`cbm_setlfs`, `cbm_setnam`, `cbm_open`, `cbm_close`, `cbm_chkin`, `cbm_chkout`, `cbm_clrchn`, `cbm_clall`), keyboard (`cbm_stop`), screen control (`cbm_gotoxy`, `cbm_wherexy`, `cbm_screen`), system clock (`cbm_rdtim`), and serial bus/IEC (`cbm_listen`, `cbm_unlisten`, `cbm_talk`, `cbm_untalk`, `cbm_second`, `cbm_tksa`, `cbm_ciout`, `cbm_acptr`). All implemented as `static` functions with `__asm__` for zero call overhead. Also defines KERNAL address constants (`KERNAL_CHROUT`, etc.), PETSCII control codes (`CH_RETURN`, `CH_CLEAR`, colors), and device numbers (`DEV_DISK_8`, etc.). Compatible with C64, C128, and MEGA65 KERNAL.
    - **`stdlib.h`**: Added `atos(s)` and `stoa(value, str, base)` convenience macros for `short` string conversion. `atos` casts through `atoi`; `stoa` casts through `itoa`.
    - **Return struct by value**: Functions can now return structs by value (e.g. `struct Point make_point(int x, int y)`). Uses a hidden-pointer ABI: the caller passes the address of the destination as an implicit last parameter (`_p___ret_ptr`); the callee copies the return value through this pointer before returning. The caller computes the destination address via `leax.fp` before pushing regular arguments. Struct-returning function calls used as initializers (`struct Point p = make_point(1,2)`) write directly to the local's frame slot — no intermediate copy. `getExprType` now returns the struct type for calls to struct-returning functions. `FunctionDeclaration` AST node gains `returnPointerLevel` field. New tracking maps `functionReturnTypes` and `structReturningFunctions` in CodeGenerator.

### Fixed
- **Assembler (ca45)**:
    - **BSS architecture rework**: `__bss_start`/`__bss_end` are now linker-defined symbols (injected by `ln45` at the merged BSS segment boundaries), not emitted by individual compilation units. The compiler no longer emits these labels in relocatable mode — each `.o45` module only emits `.res` directives and `.global` exports for its BSS variables. A new `crt_bssinit.o45` CRT module provides `_init_bss` which references the linker-provided `__bss_start`/`__bss_end` via `.extern`. In flat binary mode, the compiler still emits inline `_init_bss` and BSS boundary labels. This eliminates duplicate symbol conflicts when linking C-compiled library modules.
    - Fixed constant propagation not invalidated by address-of — when `&x` is applied to a non-const variable, the ConstantFolder now removes `x` from `knownConstants` and tracks it in `addressEscapedVars` to prevent re-propagation. Previously, `set_val(&x, 42); return x;` would return the pre-call value instead of 42. `const` variables are exempt since they can't be modified through pointers.
    - Fixed O45 reader `$FF` escape byte handling in relocation tables — `parseRelocTable` treated every byte as a `delta, typeSeg` pair, but `$FF` is a skip-254 escape with no type/seg byte following. Large C-compiled modules with relocations >254 bytes apart produced `$FF` escapes that desynchronized the reader, causing "truncated import name" errors and preventing linking. Fixed by skipping the type/seg read when delta is `$FF`.
    - Fixed string literal address loading — `ldax #<STR0` only loaded the low byte (hi byte hardcoded to 0). Changed to `ldax #STR0` for full 16-bit immediate. Any program with string literals in the `.data` segment above page 0 was affected.
    - Fixed expression parser greedily consuming `, s` stack-relative suffix after numeric literals inside binary expressions. `_l_p+0, s` was parsed as `_l_p + (stack_var_0)` instead of `(_l_p+0), s`, causing `stax`/`ldax` to emit absolute stores to address $0000 instead of stack-relative stores. Fix: only consume `, s` after a numeric literal when it is the first token in the expression (not an operand of a binary operator).
    - Fixed `lda_frame`/`sta_frame` in M65Emitter — previously used `($nn,SP),Y` indirect addressing (reads a pointer from the stack then dereferences it), which is wrong for direct stack variable access. Now uses `TSX; LDA/STA spBase+offset,X` (direct stack-relative).
    - Fixed 16-bit frame-relative stores (`stax.fp`, STW frame path) — `TSX` clobbers X before the hi byte can be stored. Now saves X to scratch ZP first, matching the pattern used by non-frame 16-bit stores.
    - Fixed `ptrstack` for frame-relative symbols — previously tried to read a pointer from the stack (indirect). Now computes `SP + spBase + fpOff + yOff` directly (same pattern as stack-relative `ptrstack`).
    - Fixed all `resolveFrameAccess`-based code paths in `emitLDWCode`, `emitSTWCode`, `emitPHWStackCode`, `emitStackIncDecCode`, `emitNegNot16Code`, `emitABS16Code`, and `AssemblerExpression.cpp` to use direct stack access instead of `($nn,SP),Y` indirect.

- **Compiler (cc45)**:
    - **Variadic functions**: Full support for variadic function declarations and calls using `...` syntax. Includes `<stdarg.h>` with `va_list`, `va_start`, `va_arg`, and `va_end`. Variadic calls use right-to-left argument pushing so named parameters have fixed stack offsets regardless of how many variadic arguments are passed. All variadic arguments are promoted to 16-bit (default argument promotions per C standard). `va_start` computes the actual stack memory address of the first variadic argument; `va_arg` reads via ZP-indirect `(ZP),Y` addressing and advances the pointer by 2 bytes. `va_end` is a no-op. New AST nodes `BuiltinVaStart` and `BuiltinVaArg` handle the `__builtin_va_start` and `__builtin_va_arg` intrinsics. Parser accepts `...` after the last named parameter in function declarations and prototypes.
- **Headers**:
    - **`stdarg.h`**: Added `va_list` typedef (`unsigned int`) and macros `va_start`, `va_arg`, `va_end` wrapping compiler builtins.

### Testing
- Added `test_short.c` — mmemu validation test for `short` type (7 sub-tests): arithmetic, signed, sizeof, function params/returns, pointers, arrays. Verified at `$4000`: `1E 05 02 0C 0A C8 AA`.
- Added `test_struct_return.c` — mmemu validation test for struct return by value (6 sub-tests): basic struct return with member access, multiple struct returns, and using returned struct values. Verified via memory dump at `$4000`: `01 02 03 04 0A 14 AA`.
- Added `test_variadic.c` — mmemu validation test for variadic functions (10 sub-tests): sum of 3/1/0/5 values, max finding, multiple named params before `...`, zero variadic args consumed, large values (>255), and result in expressions. Verified via mmemu (A=$00).

## 2026-04-30 (released in v1.0-rc2)

### Added
- **Compiler (cc45)**:
    - **Array initializer lists**: Support braced initializer lists for array declarations: `int arr[5] = {1, 2, 3};`. Works for both global and local arrays, `char` and `int` element types. Partial initialization zero-fills remaining elements. Empty initializer `= {}` zero-fills the entire array. Trailing commas are allowed. Nested initializer lists are parsed (for future multi-dimensional support). Global arrays emit `.byte`/`.word` directives per element; local arrays push elements onto the stack. Validated via mmemu emulator with global/local char/int arrays, partial initialization, and zero-fill.
    - **`register` storage class**: Support for the `register` keyword on local variable declarations. Register variables are allocated in zero page instead of on the stack, providing faster access (shorter instructions, fewer CPU cycles). The compiler uses the existing zero-page register pool (`allocateZP`), dynamically growing it as needed up to the ZP limit ($FF). When no ZP space is available, the compiler silently falls back to normal stack allocation (per the C standard, `register` is a hint). Register variables bypass the stack frame entirely — they don't consume stack space, don't participate in `.var` offset bumping, and don't require cleanup on function return. Supported in regular declarations and `for` loop initializers (`for (register int i = 0; ...)`). Arrays and structs are excluded. Optimizations like `inc`/`dec`/`inw`/`dew` work naturally with ZP addresses.
    - **`inline` keyword (no-op)**: The `inline`, `__inline`, and `__inline__` function specifiers are now parsed and accepted. Currently treated as a no-op — functions are emitted normally. Actual inlining is planned for a future release.
- **Assembler (ca45)**:
    - **`.array` directive**: Declares multi-dimensional arrays with automatic storage reservation and metadata. Syntax: `.array name, element_size, dim0 [, dim1 [, dim2 ...]]`. Reserves `element_size × product(dims)` bytes and defines compile-time constants for each dimension: `name.__elsize`, `name.__dims`, `name.__dimN`, `name.__strideN`. Strides are computed in row-major order (stride[i] = product of subsequent dimensions × element size). Supports arbitrary dimensions and element sizes (1=byte, 2=word, etc.).
    - **Array indexing in `expr`**: The expression evaluator now supports `name[index]` and `name[i][j][...]` syntax for arrays declared with `.array`. Constant indices are resolved at assembly time to direct `LDA addr` instructions. Runtime indices generate optimized code: bare `.X` register with stride=1 emits a single `LDA base,X`; power-of-2 strides use ASL shifts; arbitrary strides use the MEGA65 hardware multiplier ($D770). Multi-dimensional runtime indexing accumulates partial offsets on the hardware stack with no scratch memory conflicts. Mixed constant/runtime indices are supported (constant dimensions are folded into the base address).
- **Compiler (cc45)**:
    - **`static` keyword**: Full support for the `static` storage class specifier:
        - **Static global variables**: File-scope internal linkage. Storage is allocated normally but the symbol is not exported (no `.global` in relocatable mode).
        - **Static functions**: File-scope internal linkage. Function body is emitted but not exported.
        - **Static local variables**: Persistent storage across function calls. Allocated in the `.data`/`.bss` segment (not on the stack) with mangled names (`_funcname__varname`) to avoid collisions. Initializers are applied once at program load, not on each function entry.
    - **Multi-dimensional array declarations**: Support `int a[3][4]` row-major layout. The parser accepts multiple `[N]` suffixes on variable and struct member declarations. The type system stores dimension vectors (`arrayDims`) and computes correct strides at each indexing level. `sizeof` returns the full array size. `getExprType` adds array dimensions to pointer level for correct type propagation through chained `a[i][j]` access. `emitAddress` walks the `ArrayAccess` chain to determine the dimension depth and computes the appropriate stride (product of remaining dimensions × element size). Validated via mmemu with 1D and 2D array store/read tests.
    - **Struct arrays**: Support `struct point pts[10]` with subscript indexing and member access (`pts[i].x`). Constant and runtime-indexed stores/reads, loop-based initialization, and `sizeof` all work correctly. Validated via mmemu.
    - **CRT `_init_bss` placement**: Moved `_init_bss` (with BSS zeroing code) into the CRT stub area before function code, using a pre-scan of global declarations to determine if BSS zeroing is needed.
    - **`#pragma crt heap`**: Opt-in pragma that links the heap CRT module (`crt_heap.o45`) and calls `_init_heap_crt` during startup (before `_init_features`). Enables `malloc`/`free`/`calloc`/`realloc` from `stdlib.h`.
    - **`#pragma crt stdio`**: Pragma recognized by preprocessor and CodeGenerator (sets `crtStdio` flag). Module not yet implemented.
    - **Unified CRT startup**: The CRT inline stub is now emitted for both relocatable (`-c`) and flat binary modes. In relocatable mode, `__init`, `__sp_base`, `_init_features`, `__bss_start`, and `__bss_end` are exported via `.global`. The startup sequence now supports optional `_init_heap_crt` and `_init_stdio_crt` calls controlled by pragmas.
    - **`M65Emitter::emitDirective` / `emitRaw`**: New methods for emitting assembler directives and raw text lines in TEXT mode.
- **Standard Library (stdlib45.lib)**:
    - **`malloc(size_t size)`**: First-fit allocator with sorted free list and block splitting. Block header is 4 bytes (2-byte size with allocated bit, 2-byte next pointer). Minimum allocation 4 bytes, 2-byte aligned. Auto-initializes heap on first call if `_heap_init` hasn't run.
    - **`free(void *ptr)`**: Returns block to sorted free list with forward and backward coalescing of adjacent free blocks.
    - **`calloc(size_t nmemb, size_t size)`**: Allocates via `malloc(nmemb * size)` then zeros with `memset`.
    - **`realloc(void *ptr, size_t size)`**: Allocates new block, copies min(old, new) bytes via `memcpy`, frees old block. `realloc(NULL, n)` = `malloc(n)`, `realloc(p, 0)` = `free(p)`.
    - **`_heap_init`**: Internal initializer. Uses weak `__heap_start`/`__heap_end` symbols (defaults to `__bss_end`..`$D000`). Creates single free block spanning the heap.
    - **`abs(int value)`**: Returns 16-bit absolute value via complement-and-add.
    - **`rand(void)`**: Returns pseudo-random int 0-32767 using MEGA65 hardware RNG at `$D7EF`. Busy-waits on `$D7FE` bit 7 for RNG stabilization before each byte read.
    - **`srand(unsigned int seed)`**: No-op stub for C standard compatibility (hardware RNG cannot be seeded).
    - **`atoi` / `itoa`**: Already implemented (previously unlisted in changelog).
- **CRT Archive (crt45.lib)**:
    - New `crt45.lib` archive built from CRT modules (`crt0.o45`, `crt0_mega65.o45`, `crt_heap.o45`). The `crt_heap.o45` module exports `_init_heap_crt` which calls `_heap_init` from `malloc.o45`.
- **Headers**:
    - **`stddef.h`**: Added `size_t` typedef (`unsigned int`).
    - **`stdlib.h`**: Added `malloc`, `free`, `calloc`, `realloc` declarations. Added `abs`, `rand`, `srand` declarations. Now includes `stddef.h` for `size_t`.
- **mmemu (emulator)**:
    - Added MEGA65 hardware RNG registers to the math accelerator device: `$D7EF` (random byte, advances LFSR on read) and `$D7FE` (bit 7 = not-ready, always 0 in emulator). Uses a 32-bit Galois LFSR for deterministic pseudo-random output.

### Fixed
- **Assembler (ca45)**:
    - Fixed infinite hang when assembling files with duplicate `@` local labels across different routines (e.g., `@loop:` in two functions without `proc` scoping). The labels were stored unscoped in the symbol table, causing the second definition to overwrite the first. Pass2's convergence loop then oscillated the symbol value between two addresses indefinitely. Fix: `@` labels are now automatically scoped under the nearest preceding non-`@` label (e.g., `_myfunc:@loop`). The scope is stored per-statement so it is correctly restored during both pass2 address recalculation and code generation (which iterates by segment, not source order). Duplicate `@` labels within the same scope are now detected and reported as an error. Additionally, pass2's convergence loop is now capped at 100 iterations to prevent hangs from any remaining edge cases.
- **Compiler (cc45)**:
    - Fixed `register` variables being passed to functions via `phw.sp` (stack-relative push), which referenced an undefined `.var` symbol. Register variables are now loaded from their ZP address into A:X and pushed via `push .ax`.
    - Fixed `ConstantFolder` not copying the `isRegister` flag when rebuilding `VariableDeclaration` nodes, causing register variables to silently fall back to stack allocation after constant folding.
- **Assembler (ca45)**:
    - Fixed `.var` non-positional evaluation — `.var` ASSIGN directives were not re-evaluated during pass 2, causing all references to see the final pass-1 value regardless of source position. Temporary stack offset bumps (`.var x = x + 1` around a `pha`, then `.var x = x - 1` after `pla`) produced incorrect offsets for references between the bumps. Fix: re-evaluate `.var` ASSIGN expressions in source order during pass 2. The existing variable reset (resetting to `initialValue` before pass 2) prevents the double-evaluation bug that originally motivated removing pass-2 re-evaluation.
    - Fixed `inw`/`dew` increment optimization emitting base-page-only opcodes for global (absolute address) variables. The 45GS02 `INW` instruction only supports base page addressing; for 16-bit globals, the compiler now emits `INC addr; BNE *+5; INC addr+1` (and the inverse for decrement). 8-bit globals correctly use `INC`/`DEC` which support absolute addressing natively.
    - Fixed `sizeof` for struct/pointer arrays — `sizeof(pts)` on a `struct Point pts[4]` returned `pointer_size * count` (8) instead of `struct_size * count` (16). The array-decayed `pointerLevel` was used instead of the raw type's pointer level.
- **Assembler (ca45)**:
    - Fixed `ptrstack` for global variables — now detects global/absolute symbols and emits `LDA #lo; LDX #hi` (4 bytes) instead of the stack-relative `TSX; TXA; CLC; ADC...` sequence (9 bytes). Stack variables (`.var`-defined) continue to use the SP-relative path. This fixes incorrect address computation when `ptrstack` was used on global array/pointer labels.
    - Fixed `ptrstack` missing from pass 1 and pass 2 sizing chains — `PTRSTACK` was omitted from both the initial size calculation and the pass 2 re-sizing loops, causing it to have size 0 in pass 1. This produced cumulative address drift for all labels following any `ptrstack` instruction, leading to BRK crashes in programs with global arrays.
    - Fixed `mul.16`/`div.16` result-reading loop clobbering the low byte when loading the high byte into register destinations (`.AX`, `.AXY`, `.Q`). Any multiply/divide result > 255 had its low byte zeroed. Fixed by reading high bytes first, then low byte last.
    - Fixed `mul.16`/`div.16`/signed variants treating bare addresses as immediates — `mul.16 .ax, $04` was "multiply by 4" instead of "multiply by contents of $04". Added `isImmediate` check requiring `#` prefix, consistent with `add.16`/`sub.16`.
- **Compiler (cc45)**:
    - Fixed `_init_bss` address resolution — moved BSS init routine into the CRT stub area so its address is computed before any function code, avoiding assembler simulated opcode size drift.
    - Fixed `emitAddress` stride multiplication — now uses `mul.16 .ax, #stride` (immediate) instead of storing the index to ZP and passing the ZP address, which was misinterpreted as a literal constant by the assembler's expression parser.
    - Fixed assignment RHS save using `push .ax` which shifted SP, breaking stack-relative index reads inside `emitAddress`. Now saves RHS to an `allocateZP`-managed ZP slot instead, keeping SP unchanged.
    - Fixed `inc a; bne(0x02); inx` carry propagation — branch offset was 1 byte too large, jumping into the middle of the next instruction. Changed to `bne(0x01)` to correctly skip the 1-byte INX.
    - Fixed global integer initializer hex formatting — `int a = 3` emitted `.word $3000` instead of `.word $0003` due to `std::left` stream alignment leaking from a previous output statement. Added explicit `std::right`.

### Testing
- Added `test_multidim_array.c` — mmemu validation test for multi-dimensional arrays. Tests 1D array read (`scores[2]`), 2D constant-index store and read (`grid[1][2]`, `grid[2][3]`, `grid[0][0]`), and `sizeof` for 2D arrays. Verified via memory dump at `$4000`: `03 0C 17 00 18 AA`.
- Added `test_array_loop.c` — mmemu validation test for runtime-indexed global array stores via loops. Tests 1D loop fill (`scores[i] = i+1`), 2D nested loop fill (`grid[i][j] = i*10+j`), and reads of both. Verified via memory dump at `$4000`: `01 05 00 0C 17 AA`.
- Added `test_array.s45` — assembler test for `.array` directive and `expr` array indexing (constant and runtime indices, multi-dimensional, stride metadata constants).
- Added `test_pragma_heap.c` — compiler test validating `#pragma crt heap` with `malloc`/`free` usage compiles and assembles.
- Added `test_malloc.c` — compiler test validating `stdlib.h` heap function declarations (`malloc`, `free`, `calloc`, `realloc`) compile and assemble.
- Verified `malloc.s45` assembles in relocatable mode (`ca45 -c`) with correct symbol exports (`_malloc`, `_free`, `_calloc`, `_realloc`, `_heap_init`).
- Verified `crt_heap.s45` assembles in relocatable mode with `_init_heap_crt` export.
- Verified `lib/Makefile` builds both `crt45.lib` (3 members) and `stdlib45.lib` (28 members) successfully.
- Added `test_register.c` — mmemu validation test for `register` keyword (16 sub-tests): int/char initializers, assignment, loop accumulation, mixed register/stack variables, function argument passing, function return values, large literals, increment/decrement, nested scopes, `for`-loop init, and self-assignment.
- Added `test_register.sh` — 16 assembly-output pattern tests validating ZP allocation comments, `.var` absence, direct ZP store/load patterns, `inw`/`inc` optimizations, stack cleanup reduction, distinct ZP addresses, array/struct fallback, and full pipeline assembly.

## 2026-04-29

### Added
- **Compiler (cc45)**:
    - **`const` type qualifier**: Full support for the `const` keyword on local variables, global variables, function parameters, and struct members. Distinguishes between `const int *p` (pointer to const — prevents `*p = x`), `int *const p` (const pointer — prevents `p = &b`), and `const int *const p` (both). Assignment to `const`-qualified lvalues produces a compile error (`"Assignment to read-only location"`). Increment/decrement (`++`/`--`) of `const` variables and members is also rejected. Casting away const (`(int *)&const_var`) is allowed per C standard. Enforcement occurs in both the ConstantFolder (catches violations before optimization removes them) and the CodeGenerator (catches violations on non-folded paths). The `const` qualifier propagates through `getExprType()` for variable references, member access, array access, pointer dereference, and address-of.
    - **Const-correctness warnings**: Passing a pointer to a `const` variable to a function parameter that lacks `const` (`void f(int *p)` called with `f(&const_var)`) emits a warning: `"discards 'const' qualifier from pointer target type"`. Function parameter types are tracked via `functionParamTypes` for cross-function const checking.
- **Assembler (ca45)**:
    - **`.const` directive**: Defines an immutable named constant (`.const NAME = expr`). Redefinition of a `.const` symbol (via `.const`, equate, or `.var`) is an error. Constant symbols are marked `isConstant` in the symbol table to distinguish them from mutable `.var` symbols and equates.
- **Testing**:
    - Added `test_const_compiler.sh` — 15 compiler tests: assignment to const local/global/struct member/parameter, increment of const, write through const pointer, casting away const, const parameter read-only enforcement, reading through const pointer, const-to-non-const pointer warning, `const int *p` allows pointer reassignment, `int *const p` prevents reassignment, `int *const p` allows write-through, `const int *const p` prevents both, and valid const usage.
    - Added `test_const_assembler.sh` — 4 assembler tests: basic `.const` usage with binary verification, redefinition of `.const`, redefinition via equate, and redefinition via `.var`.
- **Compiler (cc45)**:
    - **Relocatable stack base (`__sp_base`)**: Replaced all hardcoded `$0101` references in `M65Emitter` and `AssemblerSimulatedOps` with the configurable `__sp_base` symbol (default `$0101`). Predefined in both `ca45` and `cc45`; overridable via `-D__sp_base=$NNNN`. In relocatable mode (`-c`), `__sp_base` can be declared `.extern` for linker resolution, with `R_WORD` relocations generated at all stack-access sites. Also fixed a pre-existing bug in `ptrstack` where the high byte was hardcoded to 0 instead of the stack page.
    - **`#pragma crt no_0100_stack`**: Emits `.extern __sp_base` in relocatable mode so the linker can resolve the stack base address for non-page-$01 stacks.
    - **`#pragma crt exit_rts|exit_halt|exit_brk`**: Select program exit behavior after `main` returns. `exit_rts` (default): saves/restores the caller's full 16-bit SP using self-modifying code (SP values stored in the immediate operands of the restore instructions — zero extra bytes). Provides `__exit` label for `exit()`. `exit_halt`: infinite `bra` loop. `exit_brk`: single BRK instruction. Setting one unsets the others.
    - **`#pragma crt no_bssinit`**: Controls BSS zeroing at startup. Default behavior emits `_init_bss` which zeros the BSS segment via a 16-bit pointer loop (`__bss_start` to `__bss_end`). Pragma suppresses the zeroing, emitting only an `rts` stub. Init sequence: `_init_bss` → `_init_features` → `_main`.
    - **Frame pointer removal**: Eliminated the 5-byte `proc` prologue (`TSX; LDA #page; PHA; PHX`) and 2-byte `endproc` epilogue (`PLA; PLA`). Parameters now use direct stack-relative addressing (`TSX; LDA __sp_base+offset,X`) — same as locals — instead of indirect `($nn,SP),Y` via a saved frame pointer. Params are added to `currentVars` and bumped by `.var` alongside locals. `_fp` starts at 0 (was 1). Saves 7 bytes code + 2 bytes stack per function call.
- **Standard Library (stdlib45.lib)**:
    - **`stdlib.h` / `exit()`**: `_Noreturn void exit(int status)` — loads status into `.AX` and jumps to `__exit` (CRT-provided). Works with all exit modes.
    - **`string.h`** — 12 hand-written 45GS02 assembly functions: `strlen`, `strcpy`, `strncpy`, `strcmp`, `strncmp`, `strcat`, `strchr`, `strrchr`, `memcpy`, `memmove`, `memset`, `memcmp`. All use the frame-pointer-free convention (`.var _fp = 0`).
    - **`ctype.h`** — 7 PETSCII-aware character functions in hand-written 45GS02 assembly: `isdigit`, `isalpha`, `isalnum`, `isspace`, `isprint`, `toupper`, `tolower`. All use inline range checks (no lookup table). `toupper`/`tolower` convert between PETSCII lowercase (`$41`–`$5A`) and uppercase (`$C1`–`$DA`) via bit manipulation.
    - Updated existing stdlib modules (`putchar`, `puts`, `strlen`, `memset`) to the new frame-pointer-free convention.
- **Testing**:
    - Added `test_bssinit.c` — mmemu validation test for BSS zeroing. Pre-fills RAM with `$DE` garbage, verifies uninitialized globals start at zero and assigned values are correct.
    - Added `test_strlen.c` — validates `strlen` with 4 cases (empty, 1-char, 5-char, 10-char).
    - Added `test_strcmp.c` — validates `strcmp` with 6 cases (equal, empty, less-than, greater-than, prefix).
    - Added `test_strcpy.c` — validates `strcpy` with `strcmp`/`strlen` verification.
    - Added `test_memcpy.c` — validates `memset`, `memcpy`, `memcmp` with 6 cases.
    - Added `test_strchr.c` — validates `strchr`/`strrchr` with 5 cases (first, last, not-found).

### Optimized
- **Compiler (cc45)**:
    - **Granular Store Optimization**: The compiler now individually tracks the `A` and `X` registers during 16-bit assignments. If either register already contains the correct byte for the target variable, the corresponding `sta` or `stx` instruction is omitted. Standalone 8-bit stores are similarly optimized.
    - **Self-Assignment Elimination**: Standalone `x = x;` statements for non-volatile variables are now completely optimized out.
    - **Improved Register Reuse**: Removed aggressive register tracker invalidation after local variable declarations, allowing registers initialized during variable setup to be reused by subsequent statements.

### Fixed
- **Compiler (cc45)**:
    - **Volatile Correctness**: Fixed a bug where `volatile` variable loads were being optimized away by the register tracker. Volatile variables now strictly result in explicit memory reads and writes as required by the C standard.
    - **Stack-relative LDX/LDA suffix**: Ensured that granular 16-bit loads for local variables correctly use the `, s` stack-relative suffix.

### Changed
- **Assembler (ca45)**:
    - `proc` now emits 0 bytes (was 5). No prologue — it only establishes scope and creates parameter symbols.
    - `endproc` emits only `RTS`/`RTS #n` (was `PLA; PLA; RTS`/`RTS #n`). Size: 1-2 bytes (was 3-4).
    - Parameter symbols created by `proc` are no longer `isFrameRelative`. Offsets start at 2 (past return address) instead of frame-relative Y offset 3.
    - `__sp_base` added as a predefined symbol (`$0101`) in both `ca45` and `cc45`.
    - All simulated stack ops (`lda.sp`, `sta.sp`, `inw.sp`, `dew.sp`, `phw.sp`, `ptrstack`, `neg.16`/`not.16`/`abs.16` on stack, DMA `fill.sp`/`move.sp`) use `M65Emitter::spBase()` instead of hardcoded `$0101`.

## [0.99.dev] - 2026-04-28

### Added
- **Suite-wide**:
    - **`-V` / `--version` flag** on all tools (`cc45`, `ca45`, `ln45`, `ar45`, `nm45`, `objdump45`). Prints `toolname v0.99.dev (githash)` where the git hash is embedded at compile time.
    - `include/Version.hpp` centralizes the version string; Makefile passes `-DGIT_HASH` automatically.
- **Linker (ln45)**:
    - **`-M <file>` — Linker map file**: Writes a detailed map file after linking, showing merged segment layout (address ranges and sizes), per-object segment contributions (which file contributes which address range), and all resolved symbols sorted by address with source file and weak/strong annotations.
- **Utilities**:
    - **`objdump45` — Object File Inspector & Disassembler**: Displays file headers (`-f`), section headers (`-h`), symbol tables (`-t`), relocation entries (`-r`), hex dumps (`-s`), and full 45GS02 disassembly (`-d`) from `.o45` and `.o65` object files. Also supports `.prg` files (auto-detects 2-byte load address) and raw `.bin` files (with `-b ADDR` for base address). Disassembly features symbolic labels at export addresses, branch/call target annotations with symbol names, and support for all 45GS02 addressing modes including `($nn,SP),Y`, `[zp],Z`, 16-bit branches, `PHW #imm16`, and `RTS #n`. Flags can be combined (`-fdt`); `-a` shows all. Built from a reverse lookup of the assembler's opcode database.
- **Testing**:
    - Added `test_objdump45.sh` — 113 assertions validating all `objdump45` display modes: version/help output, error handling (missing files, bad format, no flags), file header fields, section headers, symbol table (imports, exports, weak symbols, type letters), relocation entries, hex dump contents, disassembly (mnemonics, operand formatting, all addressing modes), branch target annotations, symbol labels, combined flags, multi-file output, PRG disassembly (auto-detected load address, `-b` override, hex dump), raw `.bin` disassembly (with `-b` base address, default base), and BASIC upstart PRG handling. Integrated into `make test` and available standalone via `make test-objdump45`.
- **Assembler (ca45)**:
    - **`mul.s16` / `div.s16`** — Signed 16-bit multiply and divide. Wraps the MEGA65 hardware math unit ($D770+/$D760+) with automatic sign correction: saves sign XOR, takes absolute values, performs unsigned operation, negates result if needed. Uses $D76E as sign scratch byte.
    - **`mod.16`** — Unsigned 16-bit modulo. Performs `div.16` then reads the hardware remainder registers ($D770/$D771).
    - **`mod.s16`** — Signed 16-bit modulo. Sign of result follows the dividend (C99 semantics).
- **Testing**:
    - Promoted 19 tests to the mmemu execution-validation suite: `test_ptr_arith`, `test_ptr_ptr`, `test_complex_math`, `test_struct`, `test_opt_struct`, `test_large_struct`, `test_anon_struct`, `test_union`, `test_global_vars`, `test_global_res`, `test_switch_continue`, `test_goto_sizeof`, `test_signed_cc`, `test_sub`, `test_struct_param`, `test_typedef`, `test_generic`, `test_strength_reduction`, `test_volatile`. Total mmemu-validated tests: 47 (up from 28).
    - Added `test_ptr_precedence.c` — validates pointer operator precedence and indirect increment/decrement: `(*p)++`, `++*p`, `(*p)--`, `--*p`, `*p++`, `arr[i]++`, and `char*` variants (14 test cases, mmemu-validated).

### Fixed
- **Compiler (cc45)**:
    - **Indirect increment/decrement**: `++`/`--` on dereferenced pointers (`(*p)++`, `++*p`), array elements (`arr[i]++`), and arrow-accessed members (`p->field++`) now correctly emit load-modify-store sequences through the pointer. Previously, the CodeGenerator's fallback path for non-variable lvalue operands evaluated the expression but discarded the increment/decrement, producing no side effect.
    - **Signed right-shift**: `>>` on signed types now emits `asr.16` (arithmetic shift right, preserving sign bit) instead of `lsr.16` (logical shift right). Affects four code paths: constant-count shifts, dynamic-count shifts in `emitOperation()`, and dynamic-count shifts in `BinaryOperation` visitor.
    - **Signed multiply/divide/modulo**: The compiler now emits `mul.s16`, `div.s16`, `mod.s16` for signed operands and `mod.16` for unsigned modulo (replacing direct `$D770`/`$D771` hardware register reads). Applies in `emitOperation()`, constant-literal paths, and general `BinaryOperation` visitor.
    - **Power-of-2 division optimization**: The shift-right optimization (`/ 4` → `lsr.16` × 2) is now skipped for signed types, which require proper truncation-toward-zero semantics. Signed divisions fall through to `div.s16`.

---

## 2026-04-27 (released in 0.99.dev)

### Added
- **Compiler (cc45)**:
    - **Relocatable object mode (`-c`)**: `cc45 -c input.c -o output.o45` compiles C to a `.o45` relocatable object file. The CodeGenerator auto-emits `.global` for defined functions and global variables, `.extern` for called-but-not-defined functions. Skips the `.org $2000` and startup stub in reloc mode. The `-o` flag controls the final `.o45` name; intermediate `.s45` file is generated automatically.
    - **`#pragma weak`**: Marks the next function or global variable as a weak export. The preprocessor converts `#pragma weak` to an internal `.weak_next` marker; the CodeGenerator emits `.weak` instead of `.global`. Weak symbols can be overridden by strong definitions at link time.
    - **Frame-pointer-relative parameter access**: Function parameters are now accessed via a saved frame pointer using the 45GS02's native `($nn,SP),Y` addressing mode (opcodes $E2/$82). The `proc` prologue saves SP as a 16-bit LE pointer on the stack (`TSX; LDA #$01; PHA; PHX`). Parameters get fixed Y offsets that never change as locals are pushed, eliminating the need for `.var` offset bumping on parameters.
    - Added `_fp` assembler variable that tracks the frame pointer's stack position, automatically adjusted by `.var` as locals are declared.
    - Added `test_many_params_locals.c` — validates functions with >2 parameters (up to 5) and >2 local variables (up to 6), including mixed char/int params, nested multi-param calls, and computed expression arguments (10 test cases).
    - Added `test_16bit_stack.s45` — validates 16-bit stack pointer relocation via `TYS`/`TSY` with push/pull verification on page $40.
- **Assembler (ca45)**:
    - Added `BASE_PAGE_INDIRECT_SP_Y` to `emitInstruction` (text and binary modes) and `calculateInstructionSize` — the `($nn,SP),Y` addressing mode was previously missing from the instruction encoder.
    - Added `lda_frame`/`sta_frame` methods to `M65Emitter` for frame-pointer-relative memory access.
    - Added `FrameAccessInfo` / `resolveFrameAccess()` helper to `AssemblerParser` for detecting frame-relative symbols.
    - **Relocatable object mode (`-c`)**: New `-c` flag produces `.o45` relocatable object files instead of flat binaries. Extracts per-segment bodies, generates `R_WORD` relocation entries for external and cross-segment references, builds import/export symbol tables from `.global`/`.extern` directives. Default output filename is `out.o45`.
    - **`.global` directive**: Marks symbols for export in `.o45` output.
    - **`.extern` directive**: Declares imported symbols (resolved by linker). Creates placeholder symbols with value 0 and assigns zero-based import indices for relocation entries.
    - **`.weak` directive**: Marks symbols as weak exports. Weak symbols use the high bit of the export segment byte (`O45_EXPORT_WEAK = 0x80`). The linker resolves strong-over-weak: two strong = error, strong+weak = strong wins, weak+weak = first wins.
- **Linker infrastructure (`.o45` format)**:
    - `O45Types.hpp` — format constants, enums (`O45Segment`, `O45RelocType`), mode bits, CPU IDs, and helper functions.
    - `O45Writer` — produces complete `.o45` files (header, options, segment bodies, relocation tables, symbol tables).
    - `O45RelocEncoder` — encodes high-level relocation entries into `.o65`/`.o45` delta-offset byte streams.
    - `O45SymbolTable` — manages imports/exports with deduplication, validation, and `applyTo(writer)`.
    - `O45Emitter` — bridge between the assembler and `.o45` format: extracts segments, scans for relocations, packages output.
    - `O45Reader` — parses `.o45` and `.o65` files back into structured data (header, options, segments, relocations, symbol tables). Supports both 16-bit and 32-bit formats.
    - `O45Linker` — links multiple `.o45` objects: segment merging, symbol resolution (with duplicate/undefined detection), relocation decoding and patching (all types: WORD, LOW, HIGH, LINEAR24, LINEAR32, SEGADR), flat binary or PRG output.
    - `O45RelocDecoder` — decodes delta-offset relocation byte streams back into high-level `O45Reloc` entries (inverse of `O45RelocEncoder`).
- **Utilities**:
    - **`ln45` — Linker**: Links multiple `.o45` relocatable objects into a flat binary or PRG. Merges text/data/bss/zp segments, resolves imports against exports, applies relocations. Supports library archives (`.lib`) with selective linking — only members needed to resolve undefined symbols are pulled in, with iterative chain resolution. Flags: `-t`/`-d`/`-b`/`-z` (segment base addresses), `-prg` (PRG output with load header), `-basic` (PRG with BASIC `SYS` stub at `$0801` for auto-run on MEGA65/C64), `-l` (link against library), `-m` (print symbol map).
    - **`ar45` — Archiver**: Creates and manages `.lib` archive files containing `.o45` object members. Commands: `c` (create), `t` (list), `x` (extract), `s` (symbol index), `a` (add members), `d` (delete members). Automatically builds a global symbol index by scanning each member's exports.
    - **`nm45` — Symbol Lister**: Lists exported/imported symbols in `.o45` and `.o65` object files. Traditional `nm` output format: `offset type name` (U=undefined, T=text, D=data, B=bss, Z=zp). Flags: `-u` (undefined only), `-g` (exported only), `-n` (sort by address), `-r` (reverse sort), `-p` (no sort), `-A`/`-o` (prepend filename for grep-friendly output). Multi-file listing.
- **Testing**:
    - Added `test_many_params_locals` to both `test_compiler.sh` and `test_mmemu.sh` validation suites.
    - Added 16-bit stack pointer test to `test_mmemu.sh` — verifies TYS/TSY and push/pull on a relocated stack page.
    - Added `test_global_extern.s45` to assembler test suite — validates `.global`/`.extern` directive parsing.
    - Added `test_o45` unit test binary (217 assertions) — validates `.o45` header, segment bodies, relocation encoding, symbol tables, option headers, and segment mapping.

### Changed
- **Compiler (cc45)**:
    - **Symbol naming convention (C linkage)**: Global variables changed from `_g_` prefix to `_` (single underscore). Functions changed from bare names to `_` prefix (e.g., `main` -> `_main`). This follows the traditional C linkage convention for cross-toolchain compatibility (cc65, etc.). Parameters (`_p_`) and locals (`_l_`) are unchanged. The `isGlobal` detection now uses `globalVariableTypes.count(rName)` instead of prefix matching.
    - Parameters (`_p_` variables) are no longer added to `currentVars` and no longer receive `.var` offset bumps when locals are declared.
    - Return cleanup now pops 2 additional bytes for the saved frame pointer.
- **Assembler (ca45)**:
    - `proc` now emits 5 bytes of frame pointer save code (previously 0).
    - `endproc` emits 2 PLA instructions for frame pointer cleanup before RTS.
    - Size adjustment pass updated to use new `proc` (5 bytes) and `endproc` (3-4 bytes) sizes.
    - All simulated stack-access opcodes (`lda.sp`, `sta.sp`, `ldax`, `stax`, `phw.sp`, `ptrstack`, stack inc/dec, `neg.16`/`not.16`/`abs.16` on stack) now check for frame-relative symbols and use `($nn,SP),Y` addressing when appropriate.
    - `VariableNode::emit` in expression evaluator updated for frame-relative variables.
    - `pass2()` now updates segment `pc` fields at the end of each iteration, making segment sizes available to the `.o45` emitter after assembly.

### Fixed
- **Compiler (cc45)**:
    - **Explicit cast expressions**: Support for C-style cast syntax `(type)expr` including `int`, `char`, `void`, `struct`, `union`, `enum`, `signed`/`unsigned`, typedef aliases, and pointer casts. Casts are parsed in `parseUnary()` at the correct precedence level, with constant folding support (e.g., `(char)0x1FF` folds to `0xFF` at compile time).
    - **Implicit narrowing warnings**: The compiler now emits warnings to stderr when implicit conversions lose data — e.g., assigning an `int` to a `char` variable, or a pointer to a `char`. Explicit casts suppress the warning. Warnings also detect constant overflow (e.g., assigning `500` to a `char`).
    - Added `test_struct_param.c` — validates struct pointer parameters with the arrow operator (`p->member`), including nested structs.
    - Added `test_inline_asm.c` — validates inline assembly (`__asm__()`) accessing parameters (`_p_`), locals (`_l_`), and globals (`_`) with mmemu emulator validation.
- **Testing**:
    - Added `test_cast.c` — validates explicit cast expressions: int-to-char narrowing, char-to-int widening, literal casts, nested casts, pointer-to-int casts, and casts in arithmetic expressions (10 test cases).
    - Added `test_narrowing_warn.c` — validates implicit narrowing warning output: 3 expected warnings (int-to-char declaration, int-to-char assignment, pointer-to-char) and 4 expected non-warnings (explicit cast, widening, same-size).
    - Added `test_inline_asm` to `test_mmemu.sh` — validates inline asm variable access (int params, char params, global stores, local read/write) via memory inspection at `$4000`.

### Fixed
- **Compiler (cc45)**:
    - **Arrow operator on struct pointer parameters**: `getExprType()` now checks `_p_` (parameter) prefix before `_l_` (local) in `variableTypes`, matching `resolveVarName()` lookup order. Previously, struct pointer parameters resolved to `int` instead of their declared struct type.
    - **`proc` instruction missing parameter declarations**: Merged parameter declarations (`W#`/`B#`) onto the `proc` instruction line so the assembler creates parameter symbols with correct stack offsets. Previously, parameters were only listed on the no-op `.proc` directive, leaving symbols undefined (offset 0) for functions without local variables.
- **Assembler (ca45)**:
    - **`lda.sp` / `sta.sp` pass-2 code generation**: Added missing `LDA_STACK` and `STA_STACK` dispatch entries in `AssemblerGenerator.cpp`. These simulated opcodes were correctly sized in pass 1 but emitted zero bytes (BRK) during code generation.
    - **`.var` assignment double-evaluation**: Removed the `.var` ASSIGN re-evaluation from the pass-2 generator. Self-referencing expressions like `.var _p_val = _p_val + 2` were evaluated both during parsing and generation, applying the adjustment twice. The parse-time value is now final.

---

## 2026-04-26 (released in 0.99.dev)

### Added
- **Compiler (cc45)**:
    - Added `-O0` flag to disable all optimizations (constant folding, propagation, dead variable elimination).
    - Added startup code generation: `jsr main; _halt: bra _halt` when a `main` function is present.
    - Added callee local variable cleanup before function return.
    - Added caller argument cleanup after function calls.
    - Added dead store elimination to ConstantFolder — consecutive constant stores to the same variable are collapsed to the final value.
- **Assembler (ca45)**:
    - Added `RTN` to the instruction lexer (previously silently skipped).
    - Added error reporting for unknown instructions during assembly, with output suppression on errors.
    - Added try/catch around instruction parsing to convert parse failures into error messages.
    - Added `*` (current PC) support in expressions (e.g., `bra *+4`, `lda *-2`).
    - Added `TSY` and `TYS` methods to M65Emitter.
    - Added `lda.sp` / `sta.sp` simulated opcodes for 8-bit stack-relative access (expands to `TSX; LDA/STA $0101+offset,X`).
    - Added `phw.sp` simulated opcode for pushing a 16-bit stack-relative variable onto the stack.
    - Added width-suffixed `mul.8`/`.16`/`.24`/`.32` and `div.8`/`.16`/`.24`/`.32` to the instruction lexer.
- **Testing**:
    - Added comprehensive control flow test (`test_mmemu_control.c`) covering if/else, while, do-while, for with break/continue, switch with fallthrough, and ternary operators.
    - Added simple compiler test (`mmemu_compiler_simple.c`).
    - Added `src/test/test_mmemu.sh` and `src/test-resources/test_mmemu_hello.s45` for automated validation using `mmemu-cli`.

### Fixed
- **Assembler (ca45)**:
    - **Opcode database**: Corrected `STA` ($82) and `LDA` ($E2) stack-relative entries from `STACK_RELATIVE` (direct) to `BASE_PAGE_INDIRECT_SP_Y` (indirect, matching actual 45GS02 hardware). Removed duplicate `STACK_RELATIVE` entries that shadowed `BASE_PAGE_INDIRECT_Z` opcodes.
    - **Stack access synthesis**: Rewrote `lda_stack`/`sta_stack`/`stz_stack` to use `TSX` + absolute,X indexed addressing (e.g., `TSX; STA $0103,X`) since the 45GS02 has no native direct stack-relative mode. Previous opcodes ($82/$E2) are indirect and were treated as NOPs by the emulator.
    - **Simulated op immediate detection**: Fixed `add.16`, `cmp.16`, `cmp.s16`, `and.16`/`ora.16`/`eor.16`, and `ldw` to correctly distinguish `#immediate` operands from memory address operands. Previously, bare numeric operands like `$02` were incorrectly treated as immediates via `isConstant()`.
    - **CMP16/CMP_S16 expression parsing**: Fixed `#` token not being skipped before expression parsing, causing null AST and zero-length instruction emission.
    - **BRA text output**: Fixed relative branch offset in TEXT mode — the emitter now correctly converts branch offsets to target addresses (adding instruction size) for `*+N` syntax.
    - **Branch target evaluation**: Changed branch target resolution in the generator from `resolveSymbol` (string lookup) to `evaluateExpressionAt` (full expression evaluation), enabling `*+N` and computed targets.
    - **Segment PC tracking**: Fixed pass2 segment PC initialization to use sentinel values, preventing data segment labels from resolving to address $0000.
    - **STAX/LDAX stack simulated ops**: Replaced `PHA`/`PLA` sequences (which shift SP and corrupt TSX-based offsets) with ZP $00 temp storage for preserving register values during multi-byte stack access.
- **Compiler (cc45)**:
    - **ZP temp clobber across function calls**: Reordered evaluation in simple and compound assignment paths to evaluate the RHS first and push to the hardware stack before computing the destination address into ZP. Prevents callee functions from clobbering ZP temporaries.
    - **Dead variable elimination removing live variables**: Fixed `VariableUseChecker` to count assignment targets as uses, preventing variable declarations from being eliminated while dead stores still reference them.
    - **Global struct member access with `, s` suffix**: Fixed MemberAccess visitor that unconditionally appended stack-relative suffix to global variable member loads.
    - **8-bit local variable access**: Changed CodeGenerator to emit `lda.sp`/`sta.sp` simulated opcodes for char-sized local variable access instead of bare `lda`/`sta` with `, s` (which the 45GS02 doesn't support as a native addressing mode).
    - **Function argument passing**: Changed CodeGenerator to emit `phw.sp` instead of `phw.s` for pushing stack-relative variables as function arguments.
    - **ConstantFolder switch cases**: Clear `knownConstants` at each `case`/`default` label to prevent cross-case constant propagation (e.g., `result=20` from case 2 leaking into case 3's `result+5`).
    - **ConstantFolder switch exit**: Clear `knownConstants` after switch body to prevent stale pre-switch values from propagating.
    - **Register cache after stw**: Invalidate register cache after `stw` to global variables (the simulated op clobbers A with the high byte).
    - **emitAddress resultNeeded**: Set `resultNeeded=true` before visiting array base expression in `emitAddress`, preventing stale register values from being used for pointer-indexed accesses.
    - **Local variable stack offset**: Changed `.var` offset for new local variables from `size` to `0` (correct for `$0101+offset` convention where SP+1 = top of stack).

## 2026-04-23 (released in 0.99.dev)

### Added
- **Compiler (cc45)**:
    - Added support for **Compound Assignment Operators**: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`.
    - Added support for the **Modulo Operator** (`%`) for integer remainder.
    - Added support for **`for` loop declarations**: variables can now be declared in the initializer clause (e.g., `for (int i = 0; ...)`).
    - Implemented the **`typedef` keyword** for creating type aliases, including support for pointer and struct/union typedefs.
    - Added support for **`enum` types** and enumerator constants.
- **Assembler (ca45)**:
    - Added support for generating **`.prg` files**: If the output filename ends in `.prg`, a 2-byte load address header (inferred from the first `.org`) is prepended to the binary.
    - Improved **`.basicUpstart` directive**:
        - Automatically defaults to `.org $2001` if no start address is specified.
        - Fixed size at 12 bytes across all passes for more reliable label resolution.

### Fixed
- **Compiler (cc45)**:
    - Fixed a critical bug in `BinaryOperation` where operands were being reversed for non-commutative operators (subtraction, division, shifts).
    - Improved constant folding to be more robust across control flow (clearing constants when entering loops).
- **Assembler (ca45)**:
    - Standardized `.basicUpstart` size to 12 bytes to match the emitted code.

## 2026-04-18 (released in 0.99.dev)

### Added
- **Compiler (cc45)**:
    - Added support for the `volatile` keyword for variable declarations and function parameters. `volatile` variables are now explicitly prevented from being optimized away by dead store elimination.
    - Added `-E` flag to run only the preprocessor and output to `stdout` or a file specified by `-o`.
    - Improved multi-call support: if the binary is invoked as `cp45`, it automatically runs in preprocessing-only mode.
- **Preprocessor (cp45)**:
    - Added `bin/cp45` as a new build target (symlink to `cc45`).
    - Defaults to `stdout` output when running in standalone mode.
    - Added support for function-like and **variadic macros** (`__VA_ARGS__`).
    - Implemented **comma removal extension** (`##__VA_ARGS__`) for empty variadic arguments.
    - Added support for the **`_Pragma` operator**.
    - Implemented macro operators: `#` (stringification) and `##` (token pasting).
    - Added macro expansion support within `#include` directives.
    - Implemented header guard optimization via `#pragma once`.
    - Added support for `#if` and `#elif` expression evaluation, including `defined()`, arithmetic, and logical operators.
    - Added support for line continuation using the backslash (`\`) character.
    - Added support for standard predefined macros: `__STDC__`, `__STDC_VERSION__`, and `__STDC_HOSTED__`.
    - Added support for `#undef`, `#line`, `#error`, `#warning`, and `#pragma` directives.
    - Implemented expansion of standard predefined macros: `__FILE__`, `__LINE__`, `__DATE__`, and `__TIME__`.
- **Documentation**:
    - Renamed preprocessor documentation to `doc/bin/cp45.md` to align with toolchain naming conventions.
- **Assembler (ca45)**:
    - Implemented a suite of high-level simulated opcodes:
        - `ldax / lday / ldaz`: 16-bit word loads with support for immediate (`#`), stack-relative (`offset, s`), zero page, and absolute addressing.
        - `stax / stay / staz`: 16-bit word stores with support for stack-relative, zero page, and absolute addressing.
        - `phw`: 16-bit word push, now supports stack-relative `offset, s` using an optimized `tsx` sequence.
        - `add.16 / sub.16`: 16-bit addition/subtraction on the `.ax` register pair.
        - `and.16 / ora.16 / eor.16`: 16-bit bitwise logic on the `.ax` register pair.
        - `neg.16 / not.16`: 16-bit negation and bitwise NOT, now supports `.ax`, stack-relative, and memory operands.
        - `cpw`: 16-bit word comparison on the `.ax` register pair.
        - `branch.16`: High-level 16-bit zero-check branching (`beq`, `bne`).
        - `chkzero.8 / .16`: Truthiness check returning a boolean in `.a`.
        - `ptrstack`: Efficiently calculates and loads a stack variable address into `.ax`.
        - `ptrderef`: De-references a 16-bit pointer stored in Zero Page.
        - `ldw.f / stw.f / inc.f / dec.f`: Linear 28-bit memory access (using hardware `eom` prefixes).
        - `swap / zero`: Multi-register manipulation helpers.
    - Added `@` prefix for "don't care" labels to define local optimization windows.

### Changed
- **Compiler (cc45)**:
    - Updated all code generation to utilize the new simulated opcodes, resulting in significantly more compact assembly.
    - Implemented **Parameter Prefixing**: All function parameters are now prefixed with `_p_` to avoid collisions with CPU registers or flags.
    - Implemented **Local Variable Prefixing**: All local variables are now prefixed with `_l_`.
    - *Note*: Global variables were initially prefixed with `_g_`; later changed to `_` (single underscore, traditional C linkage convention).
    - Integrated `ldax #value` for more efficient 16-bit constant and string address loading.
    - Changed default `cc45.zeroPageAvail` to `9`.
- **Assembler (ca45)**:
    - Renamed legacy `ldw .ax` syntax to preferred `ldax` mnemonic.

### Optimized
- **Compiler (cc45)**:
    - **Strength Reduction**: Converts multiplication by a power of 2 into equivalent bit shifts (`ASL`, `LSR`), and modulus by a power of 2 into bitwise AND (`AND`).
    - **Constant Propagation**: Substitutes variables with known constant literal values into expressions, enabling further compile-time folding.
    - **Dead Variable Elimination**: Removes stack allocation and initialization for non-volatile local variables that are initialized with constants and not subsequently used.
    - **Constant Initialization**: Uses native `phw #value` for declaring local variables with constant initializers, saving 3 instructions and 3 bytes per declaration.
    - **Constant Assignment**: Optimized 16-bit constant assignments to stack variables to use a "lda twice" sequence, avoiding clobbering the `x` register and reducing code size.
    - **Zero Optimization**: Integrated `zero a, x` simulated opcode for more efficient handling of zero literals.
    - **Surgical Loading**: Improved `VariableReference` loading to surgically update only the required registers if part of the word is already in the correct state.
- **Assembler (ca45)**:
    - **Stack-relative Logic**: Extended `stz`, `stx`, and `sty` with `tsx` absolute indexed addressing simulations where native stack-relative variants are missing.
    - **Word Load Tracking**: Enhanced the optimizer to track register states across high-level word load/store operations.

## 2026-04-18 (released in 0.99.dev)

### Added
- **Compiler (cc45)**:
    - Added support for pointers to pointers (multiple levels of indirection).
    - Implemented dereference (`*`) and address-of (`&`) unary operators.
    - Updated assignment logic to support arbitrary targets (e.g., `*ptr = value`).
- **Assembler (ca45)**:
    - Added support for `FLAT_INDIRECT_Z` (`[zp],Z`) addressing mode for `ADC`, `AND`, `CMP`, `EOR`, `LDA`, `ORA`, `SBC`, and `STA`.
    - Added support for `IDENTIFIER = expression` equates in `pass1`.
    - Improved `ACCUMULATOR` mode detection to correctly distinguish between registers and labels.

### Fixed
- **Assembler (ca45)**:
    - Fixed branch instruction emission order (opcode now correctly precedes the relative offset).
    - Fixed `SEE` instruction opcode (corrected to `0x03`).
    - Fixed `BSR` 16-bit relative offset calculation.
    - Fixed `pass1` re-evaluation loop to correctly handle `org` and `* =` directives, ensuring accurate address calculation for subsequent labels.
    - Fixed object file mismatch issues by ensuring clean builds.
- **Compiler (cc45)**:
    - Fixed 16-bit pointer arithmetic scaling for `char` and `int` pointer types.
    - Improved hex formatting for ZP scratchpad addresses in generated assembly.

## 2026-04-14 (released in 0.99.dev)
...
### Added
- **Compiler (cc45)**:
    - Added support for `do-while` loops.
    - Added support for inline assembly using `asm("...")` or `__asm__("...")`.
    - Added support for `if` and `else` statements.
    - Added support for `while` and `for` loops.
    - Added support for the `char` (8-bit unsigned) type.
    - Implemented full set of comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`.
    - Added automated regression testing suite in `src/test/`.
- **Assembler (ca45)**:
    - Added support for `.cpu _45gs02` directive.
    - Added support for `* = $addr` ORG syntax (KickAssembler compatibility).
    - Added support for `//` and `/* ... */` style comments.
    - Added tiered branch strategy: automatically selects between 8-bit and 16-bit relative branches.
    - Implemented dead code removal: strips instructions following `RTS`, `RTN`, or `RTI` within a procedure.
    - Added optimization for `RTN #0` to `RTS` (`0x60`).
    - Added support for 16-bit relative branch opcodes (LBEQ, LBNE, etc. on 45GS02).

### Changed
- **Assembler (ca45)**: Hex literals ($) and binary literals (%) now retain their prefixes in token values for improved parsing.
- **Types**: All basic types (`int`, `char`) are now treated as unsigned by default.
- **Arithmetic**: Improved 16-bit unsigned arithmetic (addition and subtraction) using carry flag sequences.
- **Code Generation**: Variable references and assignments now correctly handle 16-bit stack-relative offsets.
- **Project Structure**: 
    - Moved `test_compiler.sh` to `src/test/`.
    - Renamed `src/test/resources` to `src/test-resources`.
- **Build System**: Updated `Makefile` with improved `test` and `clean` targets.

### Optimized
- `+1` and `-1` operations now use `INC A`, `DEC A`, `INX`, `DEX` when appropriate.
- Function returns with no parameters now use a single-byte `RTS` instead of `RTN #0`.
