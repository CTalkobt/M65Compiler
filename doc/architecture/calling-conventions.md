# MEGA65 C Compiler Calling Conventions

This is the reference for how cc45-generated code passes parameters, returns
values, lays out stack frames and static activation records, uses the zero
page, and how ln45 bridges convention mismatches. It describes the toolchain
as it is now. Every example was compiled with `./bin/cc45 -S`, and every
runtime claim was checked on xemu.

Sources: `src/main/IRBuilder.cpp` (which convention a call uses),
`src/main/IRCodeGen.cpp` (prologue/epilogue, call sites, SAC),
`src/main/O45Linker.cpp` (thunks, SAC checks, `__zp_page`), `lib/crt0.s45`,
`lib/stdlib_asm/__stack_top.s45`.

---

## 1. Summary

| | ZP convention (default) | Stack convention |
|---|---|---|
| Select | `-fzpcall` (default) | `-fno-zpcall` |
| Standard library linked by one-step builds | `c45.lib` | `c45_stack.lib` |
| Parameters | ZP block `$10-$2F` | pushed on the CPU stack, right to left |
| Who removes parameters | nothing to remove | the caller (`plz` per byte) |
| Object marker | `.func_flags zp_call`, `cc45.mode_zp` | `.func_flags stack_call`, `cc45.mode_stack` |

Local storage is chosen separately from the parameter convention:

- **SAC** (Static Allocation Convention, default; `-fno-staticalloc` turns it
  off): parameters and locals live in static BSS storage.
- **Stack frame**: used by every function when `-fno-staticalloc` is given,
  and otherwise only by functions that cannot use SAC (see §5).

Return values work the same way in every mode (§2).

## 2. Return values

| Return type | Where the value is |
|---|---|
| `char`, `_Bool` (8-bit) | A |
| `short`, `int`, pointers (16-bit) | A = low byte, X = high byte |
| `long` (32-bit) | Q register: A = byte 0, X = byte 1, Y = byte 2, Z = byte 3 |
| `float` / `double` / `long double` (5-byte CBM float), `struct`, `union`, `long long` (which is `struct __int64`) | memory, through a hidden result pointer (sret) |

**Hidden result pointer (sret).** For a float or aggregate return, the caller
reserves a buffer in its own locals (frame or SAC storage). It passes the
buffer's address as an extra first parameter named `__sret`: at `$10` under
the ZP convention, or as the last word pushed under the stack convention. The
callee writes the value through that pointer and returns the pointer in A:X.

No struct is returned in registers, whatever its size. There is no shared
static return temporary.

## 3. Parameters

### 3.1 Slot sizes (both conventions)

Each argument takes one slot of `max(2, sizeof)` bytes:

- `char` and `_Bool` take 2 bytes; the callee reads only the low byte.
- `int` and pointers take 2 bytes.
- `long` takes 4 bytes.
- `float` takes 5 bytes.
- A struct or union passed by value is copied by the caller into a temporary.
  Only the 2-byte address of that copy is passed.
- `long long` is passed the same way as a struct.

The slot layout is identical in the ZP block and on the stack. That is why
ln45's thunks can copy the bytes across one for one.

### 3.2 ZP convention

Arguments are stored left to right into the 32-byte block `$10-$2F`. The first
argument (or `__sret`) goes at `$10`. Example from `int add3(int a, unsigned
char b, long c)`:

```
.var @_p_a = $10        ; 2 bytes
.var @_p_b = $12        ; char, 2-byte slot
.var @_p_c = $14        ; 4 bytes
```

A function uses the **stack convention instead, even in a ZP module**, when
either of these holds:

- it is variadic, or
- its slots (including `__sret`) total more than 32 bytes. For example, nine
  `long` parameters do not fit, but eight do.

Callers apply the same rule to the prototype they see.

Calls through a function pointer follow the same rule, applied to the
pointer's type in the caller's mode. There is no per-call thunk for indirect
calls (see §7).

The parameter block is caller-owned scratch. A callee may overwrite it, and it
is not preserved across calls.

### 3.3 Stack convention

The caller pushes the slots right to left, so the first parameter ends up at
the lowest address. On entry the stack looks like this:

```
SP+1, SP+2   return address
SP+3 ...     first parameter slot, then the following ones
```

After the call returns, the caller pops the arguments with one `plz` per
byte. `plz` changes only Z, so a result in A or A/X needs no saving; a `long`
result (A/X/Y/Z) is saved with `stq __zp_scratch4` and reloaded with `ldq`. Z
is not preserved across a stack call.

Pushing: a caller without a stack frame (a SAC function) pushes each
argument as it loads it; one with a frame first loads every argument into the
parameter block, then pushes them. A constant or a symbol's address is pushed
with `phw #`, which leaves its operand's **high** byte at SP+1, so cc45 writes
the bytes swapped (`phw #$3412` pushes `$1234` little-endian) and a symbol as
`.byte $F4, >sym, <sym` (ca45 relocates `.byte <sym` / `>sym`). `phw abs`
reverses a memory word the same way and is not used.

The stack is reached through a pointer (`tsx`/`tsy` → `$08/$09`, then
`lda ($08),y`). It is never read as `$0100,x`, because the stack is 16-bit by
default (§8).

### 3.4 Function attributes

These are keywords written before the return type, for example
`__fastcall__ int f(int a, int b)`:

- `__fastcall__`: this function uses the ZP convention even in a
  `-fno-zpcall` module. Callers that see the declaration call it the ZP way;
  in a stack-mode object those calls are marked `cc45.zpcall.<callee>` (a
  weak absolute symbol) so ln45 does not bridge them. The library's headers
  declare its assembly functions `__fastcall__`, so `c45_stack.lib` and
  stack-mode programs call them without thunks. A call to one of those
  assembly routines (`lib/stdlib_asm/*.s45`: `alloca`, `setjmp`, `longjmp`,
  `abs`, `labs`, `mem*`, `str*`, `malloc`..., and the library's C
  `__fastcall__` functions) that sees no declaration at all — an
  implicit declaration, or `__builtin_alloca`, `__builtin_setjmp`,
  `__builtin_longjmp`, `__builtin_abs`..., which name the library function —
  also uses the ZP convention, unless the file declares or defines a function
  of that name itself (then the file's declaration decides).
  cc45 reads the list from the `c45_stack.lib` it would link (`lib/build/`
  or `lib/cc45/` beside its `bin/`, then `CC45_LIB`): its text exports
  marked ZP-convention in their function attributes, except `__` names and
  `__regparm` `key_pressed` (`-vv` logs the count, `-vvvv` the names; #374),
  so a new ZP routine needs no change to cc45. Without the library the list
  is empty and such calls use the stack convention, which ln45 bridges.
  `src/test/test_libcall_conv.sh` (`make test`) checks it has every ZP
  routine of `lib/stdlib_asm`. A `__fastcall__` declaration that follows a stack-convention call
  to the same function in a `-fno-zpcall` file is an error: ln45 learns per
  object, not per call, that the object calls it the ZP way, so it could not
  bridge the earlier call.
- `__regparm`: the first argument is passed in A (8-bit) or A:X (16-bit), and
  the rest use the module's convention (its ZP slot stays free; under the
  stack convention it is not pushed). The callee stores it where the normal
  convention would have it before any other code runs.
- `__interrupt`: see §6.
- `__naked`: no prologue or epilogue at all.

### 3.5 Variadic functions

Variadic functions always use the stack convention. They use SAC like other
functions (unless recursive): the named parameters are copied from the stack
by the entry thunk, which also keeps the address of the first `...` argument
in `<f>__va` for `va_start`; the `...` arguments stay on the stack (the
caller removes them). Same-file callers push their arguments and call the
normal entry (never `<f>__sac`).

- Every argument, fixed and variable, is pushed with the slot sizes in §3.1.
- `va_list` is an `unsigned int` pointer into the pushed arguments.
- `va_arg(ap, T)` reads `sizeof(T)` bytes (at least 2) and advances past
  them.
- `printf`, `sprintf` and the other variadic library functions work in both
  modes.

## 4. Stack frames

A function that is not SAC gets a stack frame only if it has stack-allocated
temporaries. The size is the bytes the register allocator spilled to the
frame. A function with nothing in its frame has no prologue at all: no frame
pointer and no saved `$FD/$FE`.

Prologue (from `int id(int a)` with `-fno-zpcall -fno-staticalloc`):

```asm
    lda $FD          ; save caller's frame pointer (callee-saved)
    pha              ; (through A: "phw $00FD" would be absolute and read
    lda $FE          ;  the ROM's page 0, not the relocated base page)
    pha
    phw #0           ; frame bytes, 2 at a time (odd size: lda #0 / pha)
    tsy              ; FP = SP + 1, 16-bit
    tsx
    inx
    bne @__fp_no_carry_0
    iny
@__fp_no_carry_0:
    stx $FD
    sty $FE
    .frameptr_zp $FD
```

Frame-pointer facts:

- **Callee-saved.** `$FD/$FE` is restored on exit, so the caller's frame
  pointer stays valid after every call. It is set once per function and
  never recalculated.
- **Offsets from FP:** the frame bytes come first (offset 0 up). Then come
  2 bytes of saved FP, 2 bytes of return address, and then the stack
  parameters. In `id` above that puts `@_p_a` at offset 6.
- **Access** is `lda ($FD),y` with the offset in Y; the assembler pseudo-ops
  are `ldax.local`, `stax.local` and `ldax.param`; `leax.local N` is the
  address FP + N. A frame can be larger than 255 bytes (no warning is
  given): scalars get the lowest offsets and arrays follow, reached through
  their addresses. Such a frame is pushed in a loop (or made by moving SP)
  and dropped by setting SP from FP.
- **ZP parameters** are copied into the frame at entry when a ZP-convention
  function has a frame.

Epilogue: the return value is saved in `__zp_scratch`. Then the frame and the
saved FP are pulled with `pla` (restoring `$FE`, then `$FD`), A is reloaded,
and the function returns with `rts`.

A runtime `alloca`/`__builtin_alloca` forces a frame, of at least 1 byte, so
that SP can be reset on return.

## 5. Static Allocation Convention (SAC)

SAC is on by default and disabled with `-fno-staticalloc`. The predefined
macro `__CC45_STATIC_ALLOC__` is 1 when SAC is on.

A function uses SAC unless one of these applies:

- it calls itself directly. It falls back to a stack frame with a warning
  `recursive function uses a stack frame instead of static allocation`. Put
  `#pragma cc45 recurse` before the function to choose the stack frame
  silently.
- it is on a call cycle with other functions in the same file (mutual
  recursion). This falls back silently.
- it is `__interrupt` or `__naked`.
- it calls a runtime `alloca`.
- it follows `#pragma cc45 recurse`, which applies to the next function.

What a SAC function looks like:

- **Storage.** Each SAC function gets one BSS block:
  - `<fn>__ar` (global) marks its start, and the absolute export
    `cc45.sacsize.<fn>` gives its size;
  - `<fn>__param_<name>` for each parameter (global symbols);
  - `<fn>__local_<n>` for each local or temporary.

  The block ends with a pad byte (counted in the size), and a pad byte
  precedes it unless another block's pad does: ln45 sees only offsets, and
  a pointer just past an array must not land in a different piece of BSS
  when the block moves. A block holding an aligned local (`_Alignas`) has no
  size marker. Constant
  parameters (below) are in the data segment, outside the block. crt0 fills
  BSS with `__bss_fill` (default 0) at startup.
- **Overlay.** ln45 lets SAC functions that are never active at the same time
  share storage (see [ln45](../bin/ln45.md#sac-overlay)): the values of a SAC
  function's locals do not survive from one call to the next.
- **Two entry points.**
  - `<fn>` is the entry for ordinary callers. Its thunk copies the parameters
    from the ZP block (ZP mode) or from the stack (stack mode) into
    `__param_` storage. In stack mode (in a `.o45`) the thunk is
    `jsr __sac_args` followed by each parameter's size and storage address
    (`.byte n`, `.word <fn>__param_<p>`, ..., `.byte 0`; 3 bytes a parameter
    instead of 7 a byte); `lib/stdlib_asm/sac_args.s45` reads the arguments
    through SP and returns past the list. A variadic function, and `-S`
    output, copy them inline. In ZP mode the thunk is `jsr __zp_args` with
    the same list when that is shorter than copying inline (4-5 bytes a
    byte: a single `char` stays inline): the address is `<fn>__param_<p>` or,
    for a parameter in the function's base-page block, `<fn>__zar+n`; a
    parameter not copied (kept in the ZP block, below) is a size byte with
    bit 7 set and no address, which only moves past its slot
    (`lib/stdlib_asm/zp_args.s45`, `test_zp_args_xemu.c`). The lists are
    smaller but about 300 cycles a call slower: `-fzp-args` (#359) uses them
    at `-O1`, `-O2` and `-Os`; from `-O3` / `-Ospeed`, with `-fno-zp-args` or
    `#pragma cc45 optimize(no-zp-args)` before a function, both conventions
    copy inline (`test_zp_args_speed_xemu.c`).
  - `<fn>__sac` is the entry past that thunk.
- **Leaf functions in ZP mode keep their parameters in the ZP block.** A
  ZP-convention function that makes no calls and has no inline asm or float
  code (`ir::paramsStayInZp`) uses its arguments where the caller put them,
  `$10`…, which nothing it runs can overwrite: no copy at the entry, and a
  pointer parameter is dereferenced in place (`lda ($10),y`). The same holds
  with a stack frame. A parameter whose address is used (`&x`, a vector
  passed by value) is still copied to its storage. In a function that does make
  calls, a parameter used only where no call (or asm) can have run yet stays
  there too: the block is untouched until a call stores its own arguments. A
  pointer computed from the parameter (`p + 2`, a field's address, which the
  code generator fuses into `lda ($10),y`) counts as a use of it where it is
  used (#470). `CC45_NO_ZP_PARAMS=1`
  turns this off (`test_zp_params_xemu.c`, `test_zp_param_derived_xemu.c`).
- **SAC-to-SAC calls in stack mode.** A SAC caller in the same file stores
  the arguments straight into `<callee>__param_<name>` and does
  `jsr <callee>__sac`. Nothing is pushed or popped. In ZP mode, callers
  always use the ZP block and the normal entry.
- **Constant parameters.** For a `static` function whose address is never
  taken (so every call is in the file), a parameter that gets the same
  constant from every call has its storage initialised in the data segment
  (for example `_add3__param_a: .word 1`), and SAC callers in stack mode skip
  the store. An exported, weak or address-taken function is never treated
  this way: another module or an indirect call enters through the normal
  entry, which overwrites the storage (`test_constparam_xemu.c`).
- **Object metadata.** The function carries
  `.func_flags zp_call|stack_call, static_alloc`, plus `.param_sizes` and
  `.frame_size`.

ln45 checks over the whole program:

- A SAC function that can reach itself through the call graph (a
  cross-module cycle) is an error: `'f' uses static allocation but is
  recursive`. The graph has an edge for each `JSR` and for each `JMP` to a
  function's entry (a tail call, which the assembler makes of a call
  followed by a return; #615). A function is recursive when something it
  calls by `JSR` reaches it again, by calls or tail calls; what it only
  tail-calls runs after it has finished, so a cycle of tail calls alone is
  a loop and links.
- A SAC function reachable from an `__interrupt` handler is an error:
  `'f' uses static allocation but is reachable from ISR handler`. Mark such
  functions `#pragma cc45 recurse`.
- SAC storage of functions that cannot be active together is overlaid
  (`test_sac_overlay_xemu.c`).

## 6. Zero page, base page and interrupts

### 6.1 Zero-page map

All of these addresses are offsets in the program's base page:

| Address | Use |
|---|---|
| `$06-$07` | `__static_chain` (nested functions) |
| `$08-$0F` | `__zp_scratch`, `__zp_scratch2`, `__zp_scratch3`, `__zp_scratch4` (2 bytes each) |
| `$10-$2F` | ZP parameter block |
| `$30-$7F` | register-allocator pool (temporaries / virtual registers) |
| `$80-$F1` | functions' own base-page blocks (the most used variables of static-allocation functions that make calls, up to 16 bytes a function: `-fzp-locals-max=N`, `-fno-zp-locals`), placed by ln45 (`.zar`; ln45 "base-page blocks"), and `__zp` variables at the top; the area starts at the end of the pool the program uses (ca45's `cc45.bptop`), which can be below `$80` |
| `$F2-$F5` | 32-bit temporary (cc45) |
| `$F6-$FC` | library temporaries (float code `$F6-$FA`, `puts` and large frames `$FB-$FC`) |
| `$FD-$FE` | frame pointer |

Which variables go into the base-page blocks is chosen per file by cc45, or
for the whole program by a two-pass build: `ln45 --zp-plan-out=FILE` writes
the plan, `cc45 -fzp-plan=FILE` compiles with it (#304; see
[ln45](../bin/ln45.md#the-program-wide-base-page-plan) and
[optimizations.md](optimizations.md)).

### 6.2 Relocated base page

crt0 moves the base page register (`lda #>__zp_page / tab`). `__zp_page` is a
page-aligned 256-byte block that ln45 allocates at the end of BSS, and only
when something references it. Program ZP accesses therefore never touch the
ROM's page 0.

KERNAL and BASIC routines expect B=0, so calls to them are bracketed by:

- `jsr __bp_rom`: sets B=0 and preserves A/X/Y/Z.
- `jsr __bp_prog`: restores the program's page and preserves A/X/Y/Z and
  the flags.

Both are in crt0 (`lib/crt0.s45`):

```
__bp_rom:   pha / lda #0 / tab / pla / rts
__bp_prog:  php / pha
            lda __isr_depth         ; in an __interrupt handler: its own page
            bne @isr
            lda #>__zp_page
            bra @set
@isr:       lda #>__zp_page_isr
@set:       tab / pla / plp
            jmp __ovl_rom_leave     ; weak rts; mapped overlays: map the overlay again
```

The library's `putchar` (`lib/stdlib_asm/putchar.s45`) is the pattern:

```
    lda $10                 ; the character (ZP convention)
    phz
    jsr __bp_rom
    jsr $FFD2               ; CHROUT
    jsr __bp_prog
    plz
```

The bracket switches only B: the routine runs on the program's stack (the
16-bit one at `__stack_top` by default). That is enough for CHROUT to the
screen and for GETIN, not for the KERNAL's disk code, which needs the page-1
stack. `__rom_call` (`lib/stdlib_asm/rom_call.s45`, linked only when used)
calls the routine whose address is in `___cbm_target` with B=0 *and* the
caller's 8-bit page-1 stack (the one saved at startup), with interrupts off
while SP changes: A, X, Y, Z and the flags go in and come back as for a
direct `JSR` (C for PLOT), and it returns through `__bp_prog`. With
`-fno-stack16` it is the bracket above. `__rom_calli` is the same call with
the address in the word after the `JSR` (`jsr __rom_calli` / `.word $FFD2`:
5 bytes instead of 13 for storing `___cbm_target` and `jsr __rom_call`);
it returns past the word. al45's `mega65-rom-call-bp` reports a `jsr` to
`$E000` and up outside such a bracket (`doc/bin/al45.md`).

`__jsr` (cc45) is the way to call an address from C: a ROM address
(`$C000` and up) goes through `__rom_calli`, `__jsr_raw` and a program's own
routine through a plain `jsr`. The values go in registers and the results
come back from them (through the library's `___jsr_x` / `___jsr_a` … bytes of
`stdlib_asm/cbm_args.s45` when a value has no address of its own or must be
kept after the call). From `cc45 -O2 -S`:

```c
extern void my_routine(void);
unsigned char row;
void show(unsigned char c) {
    __jsr(0xFFD2, .A = c);                      /* ROM: through __rom_calli */
    __jsr(0xFFF0, .Carry = 1, .X -> row);       /* PLOT: X into row */
    __jsr_raw(my_routine, .A = 1);              /* direct jsr */
}
```

```
    lda $10
     jsr __rom_calli
 .word $FFD2
     sec
 jsr __rom_calli
 .word $FFF0
 stx ___jsr_x
    lda ___jsr_x
    sta _row
 lda #$01
 jsr _my_routine
@__return:
    rts
```

(in an object the last `jsr` / `rts` become a `jmp _my_routine`). The
`<cbm.h>` wrappers are such calls; the syntax is in
[cc45.md](../bin/cc45.md#calling-an-address-__jsr-__jsr_raw-__jmp).

The float runtime (`lib/src/float_rom.s45`) brackets its ROM calls the same
way, but reaches the BASIC 65 ROM's float routines through the library's own
map switch, `__float_romcall` (`lib/stdlib_asm/float_map.s45`, #614), not the
KERNAL's JSRFAR: it maps `$4000-$7FFF` to the ROM for the call and puts the
map found at `$011C` back afterwards. While such a routine runs the program's
memory at `$4000-$7FFF` is not visible, so the CPU stack must not be there.
With `-ffast-float=1` (or `-Ospeed`; `-ffast-float=0` / `-fno-fast-float` is
the default) float `+ - * /` and comparisons call RAM routines instead
(`__ffast_add` … in `lib/stdlib_asm/float_fast.s45`) and make no ROM call.

An instruction that has no base-page addressing mode is assembled absolute and
ignores B. Examples: `ldz $xx`, `lda $xx,y`, `jmp ($xx)`. Generated code
therefore uses `ldq`, and saves `$FD/$FE` through A.

Standalone (`-S` + ca45, not linked) programs place `__zp_page` after the
program. `#pragma cc45 no_zp_save` keeps page 0, and
`#pragma cc45 set_bp <page>` uses a given page.

### 6.3 `__interrupt`

The prologue is:

```
pha / phx / phy / phz      save registers
tba / pha                  save the interrupted base page
inc __isr_depth            make __bp_prog return to the handler's page
lda #>__zp_page_isr        switch to the handler base page
tab
```

`__zp_page_isr` is the page after `__zp_page`. ln45 allocates it only when
some object has an `__interrupt` function. The epilogue undoes all of this
and ends in `rti`. The handler never uses SAC.

## 7. Mixing conventions: ln45 thunks

cc45 marks each object with `cc45.mode_zp` or `cc45.mode_stack`, and marks each
function with `.func_flags zp_call` or `.func_flags stack_call`.

- **Same file.** Every call already matches the callee's convention.
- **Across modules.** ln45 compares the caller's mode with the callee's
  convention, and bridges a mismatch **in either direction**. It redirects
  the caller's `JSR` to a generated thunk:
  - `__thunk_s2z_<fn>`: a stack caller calling a ZP callee. The thunk copies
    the stacked slots into `$10...` and `JMP`s to the callee.
  - `__thunk_z2s_<fn>`: a ZP caller calling a stack callee. The thunk pushes
    `$10...` (highest first), `JSR`s the callee, pulls the bytes off again
    (A is kept in `$0E`), and returns with `RTS`.

Both directions were verified on xemu (a ZP `main` calling a stack-built
`int`/`long` callee and the reverse; both exit 0).

Linker options:

| ln45 option | Behaviour |
|---|---|
| (default) | generate thunks and warn: `calling convention mismatch: '_main' calls '_sub2' (zp_call -> stack_call) (generating thunk)` |
| `-Wthunk` | the same (accepted for compatibility) |
| `--no-thunks` | link error for each mismatch |

A program built entirely in one convention with its own library needs no
thunks: every GCC torture test links without a mismatch in both conventions.
(The 18 stack-mode mismatches the GTE runs used to report were calls to
`alloca`, `setjmp`, `longjmp` and `abs` through `__builtin_*` names or
implicit declarations, which cc45 made with the stack convention; the
`s2z` thunks were correct — the thunk `JMP`s to the callee, so `setjmp`
still records the caller's return address and stack pointer — but needless.)

Limitations:

- Only direct calls are redirected: a `JSR`, and in an object cc45 made
  (one with a `cc45.mode_zp` / `cc45.mode_stack` marker) also a `JMP` to a
  function's entry, which is a tail call (#615): the bridge is entered by
  the jump and returns to the jumping function's caller. A `JMP` from one
  routine of a hand-written object to another is a continuation (the
  registers and the stack go on as they are; the library's `__dma_mem` ends
  in `jmp __dma_start`) and is never bridged, whatever conventions the two
  declare. Test: `src/test/test_ln45_tailcalls.sh`.
- ln45 decides per object, not per call site, whether a stack-mode object
  calls a function the ZP way (`cc45.zpcall.<callee>`); cc45 therefore rejects
  a `__fastcall__` declaration after a stack-convention call to it (§3.4). A function pointer taken in
  one mode and called from code built in the other mode is **not** bridged.
  Build all such code in one mode.
- Hand-written `.s45` functions get checked only if they carry a
  `.func_flags` record inside a named `proc <name>`. In an unnamed `proc`,
  ca45 warns that `.func_flags` is ignored. The parameter count is taken
  from `.param_sizes`, or from `.zp_uses` for ZP routines.

## 8. Program startup and the stack

crt0 (`lib/crt0.s45`) runs these steps:

1. Saves the caller's SP (the flags are saved in step 5).
2. Sends the `GS` knock to `$D02F`, with interrupts masked (`php`, `sei`, the two writes,
   `plp`): an IRQ between the writes found `$D611` reading `$FF`, and the KERNAL's keyboard
   scan turned its Ctrl-S pause (`$1119`) on, so the next PRINT waited for ever (#667).
3. Sets the exit status at ROM `$FD/$FE` to `$FFFF`.
4. Relocates the base page.
5. Saves the caller's flags and switches to a **16-bit stack** whose top
   byte is `__stack_top`:
   `sei`, `cle`, then `tys`/`txs` with `__stack_top`, then restores the
   caller's I flag. `__stack_top` defaults to `$BFFF` (from the library).
   - `-fstack-top=ADDR` or `#pragma cc45 stack_top(ADDR)` moves the top.
   - `-fno-stack16` or `#pragma cc45 no_stack16` keeps the ROM's 8-bit
     page-1 stack: the main module then defines `__stack_top = 0`.
6. Fills BSS (`__bss_start` to `__bss_end`, the private base page with it)
   with `__bss_fill` (0; cc45 `-fbss-fill=V`, ln45 `--bss-fill=V`) by a DMA
   fill job, then calls `__zp_init` (the initial values of `__zp`
   variables; weak, from ln45).
7. Calls `_init_features` (weak `rts`), then `__ovl_init` (loads the code
   overlays; weak).
8. Calls `___mainargs`, which returns a pointer to argc and argv, and then
   `main`: the two words are written to `$10-$13` and also pushed, so both
   conventions see them. crt0's weak default gives argc = 0 and argv =
   NULL; when `main` is defined with parameters the library's `mainargs`
   member replaces it and parses the BASIC line (see
   [stdlib.md](stdlib.md)).
9. After `main` returns, runs the `atexit` handlers (`___atexit_run`) and
   falls into `__exit` with `main`'s result in A:X.

`crt0.s45` and `crt0_zp.s45` are the same code (the library member of each
convention).

`__exit` then:

- sets B back to 0;
- stores A:X as the exit status at `$FD/$FE` (`abort()` stores `$FFFE`);
- restores the caller's 8-bit stack and flags;
- returns with `rts`.

`__basic_call` (the library's `basic_call.s45`, with crt0's start-up; linked only when a program has such functions, #322) runs a single C function for a BASIC `SYS` (the thunks
`basic45 --link` makes store the function's address in `___bc_target` and
jump to it). It does steps 1-6 like `__init` (the BSS fill only on the first
call, so globals keep their values between calls; `_init_features` then
too), puts the SYS's A, X and Y in the first three parameter slots (`$10`,
`$12`, `$14`, high bytes 0; they also stay in A, X, Y), calls the function,
and returns to BASIC like `__exit` (B = 0, BASIC's stack and flags) with the
function's A, X, Y, which BASIC reads with `RREG`. The function uses the ZP
convention (`__fastcall__` in a stack-mode object).

## 9. Registers and clobbers

Registers:

- A, X, Y and Z are never preserved across a call.
- `$FD/$FE` is preserved by every function that changes it.
- The ZP parameter block and the scratch slots are not preserved.

Clobber metadata:

- cc45 writes per-function `.reg_clobbers` and `.flag_clobbers`, plus a
  `leaf` flag, into the object.
- ca45's optimizer uses these at each `JSR` so it forgets only the registers
  the callee changes.
- `nm45 -f file.o45` shows them, for example
  `_sub2 [stack_call leaf ... regs:AX flags:CNZV]`.

## 10. Calls through pointers: the call-graph markers

ln45's call graph (the SAC overlay, the base-page blocks, the
`__interrupt` checks, the overlay checks) has an edge for each direct
`JSR` / tail `JMP`. A call through a pointer is not one, so the objects say
what they may call that way, as absolute symbols of value 1 (no bytes):

| Symbol | Exported by | Means |
|---|---|---|
| `cc45.icallinfo` (weak) | every object cc45 makes (`-c`) | This object marks its functions: one that does not export `cc45.icall.<f>` / `cc45.isig.<f>.*` makes no call through a pointer, and its labels not saying `cc45.fsig` are not called through pointers |
| `cc45.icall.<f>` (global) | `<f>` calls through a pointer of unknown type (unprototyped, `void (*)()`), or has inline asm | `<f>` may call every function whose address is used |
| `cc45.isig.<f>.<sig>` (weak) | `<f>` calls through a prototyped pointer | `<f>` may call the address-taken functions of signature `<sig>` |
| `cc45.fsig.<f>.<sig>` (weak) | every function `<f>` | `<f>`'s own signature |

`<sig>` is the parameter slots: `s`, then each slot's size joined by `_`
(at least 2: a `char` is `2`), `v` at the end for `...`. `int f(int)` is `s2`, `int f(int, long)`
`s2_4`, `void f(void)` `s`.

ln45 (`O45Linker::buildCallGraph`):

- A function with `cc45.icall.<f>` gets an edge to every function whose
  address is used (a relocation outside a `JSR`/`JMP` operand: a table, an
  `lda #<f`), but itself.
- One with only `cc45.isig.<f>.<sig>` gets an edge to each address-taken
  function whose `cc45.fsig` is `<sig>` or `any`, and to those that say no
  signature, unless their object has `cc45.icallinfo`.
- Every text label of an object without `cc45.icallinfo` (hand-written
  asm, an old object) counts as `cc45.icall.<label>`. Library asm that makes
  no such calls exports `cc45.icallinfo` itself, and marks the labels that
  do: `rom_call.s45` exports `cc45.icallinfo` and `cc45.icall.__rom_call`.
  A label it exports that is called through pointers says
  `cc45.fsig.<label>.any`.

So an asm object without the marker makes all its functions "call
anything": correct, but every SAC function whose address is used then
shares no storage with them. ln45 `-M` lists the result ("Calls Through
Pointers"; also `-vv`). From

```c
typedef int (*op_t)(int);
static int twice(int x) { return x * 2; }
static int neg(int x) { return -x; }
static void hello(void) {}
op_t ops[2] = { twice, neg };
void (*hook)() = hello;
int run(int i, int v) { op_t f = ops[i]; return f(v); }
void fire(void) { hook(); }
int main(void) { fire(); return run(0, 1) == 2 ? 0 : 1; }
```

```text
$ cc45 -O2 -c ptr.c -o ptr.o45 && nm45 ptr.o45 | grep 'cc45\.[if]'
00000001 W cc45.fsig._fire.s
00000001 W cc45.fsig._hello__ptr.s
00000001 W cc45.fsig._main.s
00000001 W cc45.fsig._neg__ptr.s2
00000001 W cc45.fsig._run.s2_2
00000001 W cc45.fsig._twice__ptr.s2
00000001 A cc45.icall._fire
00000001 A cc45.icall._main
00000001 W cc45.icallinfo
00000001 W cc45.isig._main.s2
00000001 W cc45.isig._run.s2
$ ln45 -basic -M ptr.map -o ptr.prg ptr.o45 lib/build/c45.lib
$ grep -A5 '^Calls Through' ptr.map
Calls Through Pointers
----------------------
  call graph: _fire calls through any pointer
  call graph: _main calls through any pointer
  call graph: _main calls through pointers s2
  call graph: _run calls through pointers s2
```

(further lines: `call graph: address of _twice__ptr used, parameters s2`,
… and the library's address-taken symbols). `_main` has both because
`fire` and `run` are inlined into it. `_run` reaches `_twice__ptr` and
`_neg__ptr`, not `_hello__ptr` (`s`).

## 11. `__far` and `__bank` functions: the overlay call path

A `__far` function is code in a code overlay (`far1`, `far2`, …: ln45 packs
the objects' `far` text sub-segments into 8 KB mapped overlays); a
`__bank(n)` function's overlay is kept in bank n (`far.n`). Its caller does
not know: it calls `_f` as any function, in either convention. ln45 defines
`_f` as a resident 6-byte stub and links the function itself at the overlay
region:

```
_f:     jsr __ovl_calli
        .byte id            ; the overlay's number, 1-255
        .word f             ; the function in the region
```

(21 bytes, `sta __ovl_sa` / `lda #id` / … / `jmp __ovl_call`, with ln45
`--overlay-fast-stubs`). `__ovl_calli` reads the id and the address after
its return address and pulls that address off, so the caller's own return
address is on top again; `__ovl_call` (`lib/stdlib_asm/ovl.s45`) moves it
to the overlay return stack with the overlay to bring back, maps the
overlay in (or copies it by DMA), and jumps to the function with its
return address replaced by `__ovl_ret`'s. The stack is otherwise as the
caller left it, so both conventions find their arguments (the ZP block or
the stack) where they expect them; A, X, Y, Z go in unchanged and come back
as the function left them, the flags too. A call from inside the same
overlay is a direct `JSR`. A function pointer to `_f` (taken anywhere) is
the stub's address, so calls through pointers take the same path. A SAC
`__far` function's `_f__sac` entry is in the overlay (`$6000` below).

```c
__far int triple(int x) { return x * 3; }
int (*fp)(int) = triple;
int main(void) { return triple(2) + fp(1) == 9 ? 0 : 1; }
```

```text
$ cc45 -O2 -c farfn.c -o farfn.o45
$ ln45 -basic -M farfn.map -o farfn.prg farfn.o45 lib/build/c45.lib
Code overlays: farfn.ov1 (11 bytes; loaded as FARFN.OV1), described in farfn.ovd
Linked 1 object to farfn.prg (1590 bytes)
$ grep '_triple ' farfn.map
  $0003  cc45.sacsize._triple           farfn.o45
  $2587  _triple                        farfn.o45  [uses:- clob:- rel:- regs:AXY]  [stub: overlay far1, $6000]
    $6000  _triple                        bank 5 $0000   farfn.o45  (stub $2587)
  _triple -> __mul16y
  call graph: address of _triple used, parameters s2
$ objdump45 -d farfn.prg
    ...
    212f:	20 87 25    	jsr	$2587	; <_triple>
    ...
00002587 <_triple>:
    2587:	20 d4 21    	jsr	$21d4	; <__ovl_calli>
    258a:	01 00       	ora	($00,x)
    258c:	60          	rts
```

The stub's last three bytes are its data, disassembled as code: id `01`,
the address `$6000`. `fp`'s initial value is `$2587`, the stub. The program
exits 0 in xemu with `FARFN.OV1` on its `-8` disk. Limits: no
`__interrupt` handler may be, or reach, an overlay function (the runtime is
not reentrant: a link error); calls through overlays nest 32 deep
(`--overlay-depth`). The rest is in [ln45](../bin/ln45.md#code-overlays).

## 12. Inspecting

```bash
mkdir -p build/tmp && cd build/tmp
../../bin/cc45 -S x.c -o x.s                 # prologues, .func_flags, SAC storage
../../bin/cc45 -c -fno-zpcall x.c -o x.o45
../../bin/nm45 -f x.o45                      # convention / leaf / clobbers per function
../../bin/ln45 -basic -Wthunk -o x.prg main.o45 x.o45 ../../lib/build/c45.lib
```
