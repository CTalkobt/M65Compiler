# asmCvt45 Conversion Guide

What [`asmCvt45`](../bin/asmCvt45.md) keeps, translates and loses for each assembler format (#223), and three worked conversions (#224). The tool's options are in its reference page; this page is about the result.

**How it was checked.** For ca65, ACME and KickAssembler every statement below was checked against the real assembler, byte for byte:

- **ca45 → dialect:** sample programs (data directives, expressions, macros, local labels, 65CE02 and 45GS02 instructions) assembled by ca45 and, converted, by ca65 2.18 (+ ld65), ACME 0.97 and KickAssembler (the 65CE02/45GS02 build, `KickAss65CE02.jar`).
- **dialect → ca45:** a program in each of those dialects' own idioms assembled natively and, converted, by ca45.
- **dialect → dialect:** each of those native programs converted to the other two and assembled there.

The native programs and their bytes are test fixtures (`src/test-resources/asmCvt45/`), checked by `test_asmCvt45.sh` in `make test`. Merlin 64, Oscar and x65 could not be checked against their assemblers: what is written for them follows their documented syntax, and anything not certain is written with a warning.

## How a conversion works

A reader turns the source into a common representation, and a writer prints it in the target syntax. Three consequences matter for every conversion:

- **Macros are expanded.** Each macro call becomes the instructions it stands for; the definition itself is not written. The result assembles to the same bytes, but it is longer and has no macros left.
- **Conditional assembly is resolved.** ca45 has none, so the readers keep the branch taken and drop the others: ca65 `.if`/`.ifdef`/`.ifndef`/`.elseif`/`.else`/`.endif`, ACME `!if`/`!ifdef`/`!ifndef { … } else { … }`, KickAssembler `.if (…) { … } else { … }`. A condition is evaluated from the constants defined above it (numbers, `+ - * / % & | ^ << >>`, comparisons, `&& ||`, `defined()`); one that depends on something else (a label's address) is kept as written, with a warning.
- **Strings become bytes.** Each assembler encodes strings its own way (ASCII, PETSCII, screen codes, and KickAssembler's `.encoding`), so a string is written as the `.byte` list of the bytes it stood for, with the original string in a comment. The bytes are exact; the readable string is only in the comment.

Comments and blank lines are kept. Warnings go to stderr and name the line; a conversion without warnings assembles to the same bytes.

## Per format

### ca45 (the input most conversions start from)

| | |
|---|---|
| **Kept** | Instructions and addressing modes, labels (`@local` labels too, see each target), constants, `.org`, data, `.global`/`.extern`/`.weak` (as each target writes them), comments. |
| **Translated** | `.text`/`.ascii`/`.screencode` → byte lists; `'c'` → its PETSCII value; `.long`/`.dword`, `.res`/`.fill`/`.space`, `.align`, `.fillto`, `.incbin` → each target's spelling (below); a negative data literal (`.byte -1`) → its stored value (`$FF`); `.macro` calls expanded. |
| **Lost** | Macro definitions (expanded), `#define` (expanded by the preprocessor). |
| **Kept as written, warned** | What only ca45 understands: simulated instructions (`add.16`, `push a`, `FILL`), `proc`/`endproc`, stack-relative and frame operands, `.var`, `.local`, `.func_flags` and the other attribute directives. The target will not accept them; they must be rewritten by hand. |
| **Note** | ca45 has **no conditional assembly**: `.if` is ignored with a warning (both branches are assembled). |

### ca65 (cc65)

**Reading ca65:**

| | |
|---|---|
| **Kept** | Labels, `@cheap` locals, constants (`=`, `:=`), `.byte`/`.byt`, `.word`/`.addr`, `.dword`, `.res`, `.align`, `.org`, `.incbin`, segments (`.code`, `.data`, `.rodata` → `.data`, `.bss`, `.segment`), `.import`/`.export` (also `zp`). |
| **Translated** | `.proc name` → the label `name` (each procedure's `@locals` are its own); unnamed labels `:` with `:-`/`:+`/`:--`/`:++` → generated names; `.define NAME value` → a constant; `.asciiz` → the string and a 0; `.lobytes`/`.hibytes` → `.byte <…`/`>…`; `.macro` calls expanded; conditionals resolved; the file starts in `.code`, as ca65's does, when code comes before the first segment switch. |
| **Lost** | `.endproc`; `.scope`/`.endscope` (their labels become global: `scope::name` references do not convert, warned); `.setcpu`, `.p02`, `.p4510` (ca45 is a 45GS02 assembler). |
| **Note** | ca65 code is relocatable: its address comes from ld65. Assemble the result with `ca45 -c` and link with `ln45` (see [the cc65 example](#1-a-cc65-module-into-a-mega65-program)). |

**Writing ca65:**

| | |
|---|---|
| **Written** | `.setcpu "4510"` first (ca45 is a 45GS02 assembler; ca65's 4510 is the 65CE02 plus MAP); `.dword`, `.res n, v` (for `.fill`/`.res`/`.space`), `.align`, `.res (addr)-*` (for `.fillto`), `.incbin`; `.import`/`.export`. ca45's code before any segment switch goes in a segment `DEFAULT` when the program also switches segments (it is ca45's own first segment, which ca45 writes after code and data). |
| **Translated** | `*` in a data list (`.word *, * - 4`): ca65's `*` there is the address of the item, ca45's the address of the line, so the line gets a label (`__cvt_pc_N`) that is used instead. |
| **Not available, warned** | The 45GS02's 32-bit instructions (`ldq`, `adcq`, …) and flat-memory addressing (`lda [zp],z`): ca65's 4510 has neither. `.float`. |
| **Note** | A label spelled like a 4510 instruction (`tab:`, `taz:`, `map:`) is an error in ca65; ca45 accepts it. |

### ACME

**Reading ACME:**

| | |
|---|---|
| **Kept** | Labels (with `:` or in column 1), constants, `*=`, `!byte`/`!by`/`!8`, `!word`/`!wo`/`!16`, `!32`, `!fill`, `!binary`, `!cpu`. |
| **Translated** | Zone-local labels (`.loop`, local to the `!zone`) → `<zone>__loop`; anonymous labels (`+`, `-`, `++`, `--`) → generated names; `@cheap` locals kept; `!macro name .a, .b { … }` with `+name` calls expanded (the `.a` parameters substituted); `!if`/`!ifdef`/`!ifndef` resolved; `!align mask, 0[, fill]` → `.align mask+1[, fill]`; `!text`/`!tx` (ASCII), `!pet` (PETSCII), `!scr` (screen codes) → byte lists. |
| **Lost** | `!zone` itself (its labels are renamed instead), `!to`, `!sl`, `!initmem`, `!warn`. |
| **Not translated** | `!for`, `!do`, `!set` loops and other script-like pseudo-ops: an unknown one reaches ca45 under its own name, where ca45 reports it. An `!align` with a value other than 0 (a warning). |

**Writing ACME:**

| | |
|---|---|
| **Written** | `!cpu m65` first; `!32` (for `.dword`), `!fill`, `!align mask, 0, fill` (always with a fill byte: ACME's default is NOP, `$EA`, ca45's 0), `!fill (addr)-*` (for `.fillto`), `!binary`. |
| **Changed** | ACME assembles one program without a linker: imports and exports become comments (`; extern: putc (define it before assembling)`), and segment switches too (`; segment: data`): the code is in source order. |
| **Needs a hand** | An origin. Relocatable source (ca65's) has none, and ACME needs one: the output has a comment `; *= <address>` where it goes, and a warning. |
| **Not available, warned** | `ldq [zp],z` and the other 32-bit flat-memory forms: ACME 0.97 has the 32-bit instructions and `[zp],z`, but not both together. `.float`. |

### KickAssembler

**Reading KickAssembler:**

| | |
|---|---|
| **Kept** | Labels, `.const`/`.var`/`.label`, `*=`/`.pc =`, `.byte`, `.word`, `.dword`, `.fill`, `.align`, `.cpu`; `//` and `/* */` comments. |
| **Translated** | Multi-labels (`!:`, `!loop:`, referenced as `!-`, `!+`, `!loop--`, …) → generated names; `.macro name(a, b) { … }` with `name(x, y)` calls expanded; `.if (…) { } else { }` resolved; `((zp)),z` → ca45's `[zp],z`; `.import binary "f"` → `.incbin "f"`; `.text` in the current `.encoding` (screencode_mixed by default) → byte lists. |
| **Lost** | `.filenamespace`, `.print`, `.file`, `.disk`, `.segmentdef`, `.memblock`. |
| **Not translated** | KickAssembler's script language (`.for`, `.function`, `.eval`, lists and hashtables), `!namespace` scoping (flattened to dotted names), `.fill` with an expression of the index. |

**Writing KickAssembler:**

| | |
|---|---|
| **Written** | `.cpu _45gs02` first; `.dword`, `.fill n, v` (for `.fill`/`.res`/`.space`: KickAssembler's needs the value), `.align n`; `.align n, fill` as a `.fill` of the padding; `.fill (addr)-*, v` (for `.fillto`), `.import binary`; `[zp],z` as `((zp)),z`; `.const`. |
| **Changed** | `@local` labels → `<label>__name` (KickAssembler's `@` names are global: two `@loop`s in a file are an error); imports, exports and segment switches become comments, as for ACME. |
| **Note** | KickAssembler's `bsr` (16-bit relative `jsr`) is missing from the 65CE02 build used here; the rest of the 65CE02 and 45GS02 instruction set assembled. |

### Merlin 64, Oscar, x65 (not checked against their assemblers)

| | Merlin 64 | Oscar | x65 |
|---|---|---|---|
| **Reading** | Labels in column 1, `EQU`/`=`, `ORG`, `DFB`/`DB`, `HEX`, `DW`/`DA`, `ASC` (as ASCII), `DS`, `ENT`/`EXT`, `*` comment lines | Labels in column 1, `=`, `ORG`, `BYTE`/`DB`, `WORD`/`DW`, `GLOBAL`/`EXTERN` | Labels, `=`, `.org`, `.byte`, `.word`, `.long`, `dc.b`/`dc.w`/`dc.l`, `.text`, `.import`/`.export`, `xref`/`xdef` |
| **Writing** | `DFB`, `DW`, `ASC`, `DS`, `ENT`, `EXT`; `@locals` as `<label>__name` | `BYTE`, `WORD`, `GLOBAL`, `EXTERN`; `@locals` as `<label>__name` | `.byte`, `.word`, `.long`, `.export`/`.import` |
| **Numeric `.dword` / `.fill`** | as `DFB` byte lists | as `BYTE` byte lists | `.fill` as a `.byte` list |
| **Warned** (written as ca45 has them) | `.align`, `.fillto`, `.incbin`, a `.dword`/`.fill` of symbols | the same | the same, `.align` |

## Instruction sets by target

| | 6502 | 65CE02 (`ldz`, `(zp),z`, `taz`, `inw`, `bra`, `map`, …) | 45GS02 32-bit (`ldq`, `adcq`, …) | 45GS02 flat (`lda [zp],z`) | `ldq [zp],z` |
|---|---|---|---|---|---|
| ca45 | yes | yes | yes | yes | yes |
| ca65 2.18 (`.setcpu "4510"`) | yes | yes | no (warned) | no (warned) | no (warned) |
| ACME 0.97 (`!cpu m65`) | yes | yes | yes | yes | no (warned) |
| KickAssembler (`.cpu _45gs02`) | yes | yes (`bsr` not in the build tested) | yes | yes, as `((zp)),z` | yes, as `((zp)),z` |

## Worked examples

### 1. A cc65 module into a MEGA65 program

A ca65 source in cc65's style: procedures with their own `@loop`s, a macro, `.define`, an `.if`, unnamed labels, read-only data. (It is the fixture `src/test-resources/asmCvt45/native_ca65.s65`.)

```asm
.define COLOR 2

.macro  border col
        lda #col
        sta $d020
.endmacro

.proc   main
        border COLOR
        ldx #0
@loop:  lda msg,x
        beq @done
        sta SCREEN,x
        inx
        bne @loop
@done:  jsr helper
        rts
.endproc

.proc   helper
@loop:  dey
        bne @loop
        rts
.endproc

inc16:  inc count
        bne :+
        inc count+1
:       rts

.rodata
msg:    .byte "hello", 0
        .asciiz "world"
        .lobytes main, helper
```

```bash
asmCvt45 -f ca65 -t ca45 game.s65 -o game.s45
ca45 -c game.s45 -o game.o45          # relocatable, as the ca65 object was
ln45 -basic -o game.prg game.o45 c45.lib    # or with cc45-compiled C objects
```

The result (abridged):

```asm
.code
COLOR = 2
main:
  lda #COLOR
  sta $d020
  ldx #0
@loop:
  lda msg, X
  beq @done
  ...
helper:
@loop:
  dey
  bne @loop
  rts
inc16:
  inc count
  bne __ca65_anon_1
  inc count+1
__ca65_anon_1:
  rts
.data
msg:
.ascii "hello"
.byte 0
.ascii "world"
.byte 0
.byte <(main), <(helper)
```

Each `.proc` is a label, so its `@loop` is its own in ca45 too; the macro is expanded; `.define` is a constant; the unnamed label has a name. Linked at `$2000`, this is byte for byte what ca65 and ld65 made of the original. What a cc65 project also needs:

- **Calls between C and assembly:** cc65 passes arguments on its software stack (`sp`, `ptr1`, …, `__fastcall__` in A/X); cc45 passes them in base-page slots or on the CPU stack (see [calling conventions](calling-conventions.md)). Routines called from C need their entry rewritten; routines called only from assembly convert as they are.
- **cc65's runtime and zero page:** imports of `sp`, `ptr1`, `tmp1`, `popax`, … are cc65's runtime and have no ca45 counterpart.
- **Scopes:** `.scope` blocks and `scope::name` references are not kept (warned).

### 2. Between ACME and KickAssembler

A converted program can go straight from one dialect to the other: `asmCvt45 -f acme -t kickassembler`. From the ACME fixture (`native_acme.a65`):

```asm
!zone main
main:   +border 2
        ldx #0
.loop:  lda msg,x
        beq .done
        ...
!zone helper
helper:
.loop:  dey
        bne .loop
@cheap: dex
        bne @cheap
        rts
wait:   ldx #10
-       dex
        bne -
        rts
!if DEBUG = 1 {
        lda #1
} else {
        lda #2
}
msg:    !text "hello"
        !align 15, 0, $ff
```

becomes, for KickAssembler:

```asm
.cpu _6502
*= $2000
main:
    lda #2
    sta $d020
    ldx #0
main__loop:
    lda msg,x
    beq main__done
    ...
helper:
helper__loop:
    dey
    bne helper__loop
helper__loop__cheap:
    dex
    bne helper__loop__cheap
    rts
wait:
    ldx #10
__acme_b1_1:
    dex
    bne __acme_b1_1
    rts
    lda #2
msg:
.byte $68, $65, $6C, $6C, $6F // .ascii "hello"
.fill (16-((*)&15))&15, $ff
```

KickAssembler assembles it to ACME's bytes, and the KickAssembler fixture converts to ACME the same way. Converting this way gives one **working, expanded copy** of the code: zone and anonymous labels have generated names, macros and `!if` are resolved, strings are byte lists. For a codebase that stays in two dialects, keep one as the source and convert into the other as a build step, rather than editing both.

The other directions: ACME → ca65 and KickAssembler → ACME also assemble to the same bytes. ca65 → ACME needs an origin added by hand (ca65 code has none: the output says where), and KickAssembler → ca65 stops at 45GS02 flat-memory instructions, which ca65 does not have (warned).

### 3. Porting 6502 code to the 45GS02

A 6502 program from any of these dialects converts to ca45 unchanged in meaning (a 6502 program is a 45GS02 program). Then the 45GS02's instructions can replace common 6502 idioms. Two routines, after `asmCvt45 -t ca45`:

```asm
count   = $40           ; a 16-bit counter in the base page
inc16:  inc count       ; count = count + 1
        bne @done
        inc count+1
@done:  rts
copy4:  ldx #3          ; 4 bytes from src to dst
@loop:  lda src,x
        sta dst,x
        dex
        bpl @loop
        rts
```

and rewritten for the 45GS02:

```asm
inc16:  inw count       ; a 16-bit increment of a base-page word
        rts
copy4:  ldq src         ; 4 bytes at once (A, X, Y, Z: the Q register)
        stq dst
        rts
```

19 bytes become 14, and the copy no longer loops. Others worth looking for: `dew` for 16-bit decrements, `asw`/`row` for 16-bit shifts, `phw #`/`phw` to push words, `bra` and the 16-bit branches (`lbne`, …) for `jmp`s and branch-over-`jmp` sequences, `(zp),z` instead of `(zp),y` when Y is needed for something else, `[zp],z` to reach memory beyond 64 KB without banking, and ca45's simulated 16-bit instructions (`add.16`, `cmp.16`, … in [opcodes](opcodes.md)), which ca45 expands into the shortest sequence. Note the base page: on the MEGA65 it can be moved (`tab`), and a cc45 program has its own (see [CLAUDE.md](../../CLAUDE.md) and [calling conventions](calling-conventions.md)): `inw` and the `[zp],z` pointers need their variables in it.

Keep a converted file in ca45 once it uses 45GS02 instructions: of the other targets only ACME and KickAssembler have them (see the table above).

## Checking a conversion

- **Warnings first.** Every construct that is not converted is named with its line. A file that converts without warnings assembles to the same bytes in the targets checked here.
- **Assemble both and compare.** For a flat program, assemble the original with its own assembler and the converted file with the target's, and compare the binaries (`cmp a.bin b.bin`). ca45: `CA45_OPTIMIZE=0 ca45 x.s45 -o x.bin` assembles without its optimizer, so the bytes are the program as written. Relocatable code (ca65's): `ca45 -c` and `ln45 -prg -t 2000`, then drop the PRG's two-byte header (`tail -c +3`).
- **`--validate` and `--stats`** report how much of a file survives a conversion to a target and back.
