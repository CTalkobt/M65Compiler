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

After the call returns, the caller saves the result in `__zp_scratch4`, pops
the arguments with one `plz` per byte, and reloads the result. Z is therefore
not preserved across a stack call; a `long` result is restored with
`ldq __zp_scratch4`.

The stack is reached through a pointer (`tsx`/`tsy` → `$08/$09`, then
`lda ($08),y`). It is never read as `$0100,x`, because the stack is 16-bit by
default (§8).

### 3.4 Function attributes

These are keywords written before the return type, for example
`__fastcall__ int f(int a, int b)`:

- `__fastcall__`: this function uses the ZP convention even in a
  `-fno-zpcall` module. Callers in the same file call it the ZP way.
- `__regparm`: the first argument is passed in A (8-bit) or A:X (16-bit), and
  the rest use the module's convention. **Known issue:** a `__regparm`
  function that needs a stack frame (always the case with `-fno-staticalloc`)
  loses X. Its frame-pointer setup (`tsy`/`tsx`) runs before A:X is saved. In
  stack mode the remaining parameter offsets are also 2 bytes too high.
  `__regparm int rp(int a,int b)` returns wrong values under
  `-fno-staticalloc`. It works with the default SAC.
- `__interrupt`: see §6.
- `__naked`: no prologue or epilogue at all.

### 3.5 Variadic functions

Variadic functions always use the stack convention, and they never use SAC.

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
  are `ldax.local`, `stax.local` and `ldax.param`. Offsets must fit in 8
  bits, and cc45 warns when a frame is larger than 128 bytes.
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
- it is `__interrupt`, `__naked`, or variadic.
- it calls a runtime `alloca`.
- it follows `#pragma cc45 recurse`, which applies to the next function.

What a SAC function looks like:

- **Storage.** Each SAC function gets BSS labels:
  - `<fn>__param_<name>` for each parameter (global symbols);
  - `<fn>__local_<n>` for each local or temporary.

  The BSS is zeroed by crt0 unless `#pragma cc45 no_bssinit` is given. Each
  function has its own storage: storage is **not** overlaid between
  functions (ln45's call-graph colouring pass does not take effect; see the
  link note).
- **Two entry points.**
  - `<fn>` is the entry for ordinary callers. Its thunk copies the parameters
    from the ZP block (ZP mode) or from the stack (stack mode) into
    `__param_` storage.
  - `<fn>__sac` is the entry past that thunk.
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
  recursive`.
- A SAC function reachable from an `__interrupt` handler is an error:
  `'f' uses static allocation but is reachable from ISR handler`. Mark such
  functions `#pragma cc45 recurse`.

## 6. Zero page, base page and interrupts

### 6.1 Zero-page map

All of these addresses are offsets in the program's base page:

| Address | Use |
|---|---|
| `$06-$07` | `__static_chain` (nested functions) |
| `$08-$0F` | `__zp_scratch`, `__zp_scratch2`, `__zp_scratch3`, `__zp_scratch4` (2 bytes each) |
| `$10-$2F` | ZP parameter block |
| `$30-$E9` | register-allocator pool (temporaries / virtual registers) |
| `$FD-$FE` | frame pointer |

### 6.2 Relocated base page

crt0 moves the base page register (`lda #>__zp_page / tab`). `__zp_page` is a
page-aligned 256-byte block that ln45 allocates at the end of BSS, and only
when something references it. Program ZP accesses therefore never touch the
ROM's page 0.

KERNAL and BASIC routines expect B=0, so calls to them are bracketed by:

- `jsr __bp_rom`: sets B=0 and preserves A/X/Y/Z.
- `jsr __bp_prog`: restores the program's page and preserves A/X/Y/Z and
  the flags.

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
| (default) | generate thunks silently |
| `-Wthunk` | generate thunks and warn: `calling convention mismatch: '_main' calls '_sub2' (zp_call -> stack_call) (generating thunk)` |
| `--no-thunks` | link error for each mismatch |

Limitations:

- Only direct `JSR` relocations are redirected. A function pointer taken in
  one mode and called from code built in the other mode is **not** bridged.
  Build all such code in one mode.
- Hand-written `.s45` functions get checked only if they carry a
  `.func_flags` record inside a named `proc <name>`. In an unnamed `proc`,
  ca45 warns that `.func_flags` is ignored. The parameter count is taken
  from `.param_sizes`, or from `.zp_uses` for ZP routines.

## 8. Program startup and the stack

crt0 (`lib/crt0.s45`) runs these steps:

1. Saves the caller's SP and flags.
2. Sends the `GS` knock to `$D02F`.
3. Sets the exit status at ROM `$FD/$FE` to `$FFFF`.
4. Relocates the base page.
5. Switches to a **16-bit stack** whose top byte is `__stack_top`:
   `sei`, `cle`, then `tys`/`txs` with `__stack_top`, then restores the
   caller's I flag. `__stack_top` defaults to `$BFFF` (from the library).
   - `-fstack-top=ADDR` or `#pragma cc45 stack_top(ADDR)` moves the top.
   - `-fno-stack16` or `#pragma cc45 no_stack16` keeps the ROM's 8-bit
     page-1 stack: the main module then defines `__stack_top = 0`.
6. Calls `_init_features`.
7. Calls `main` with argc = 0 and argv = NULL. These are written to
   `$10-$13` and also pushed as two words, so both conventions see them.

`__exit` then:

- sets B back to 0;
- stores A:X as the exit status at `$FD/$FE` (`abort()` stores `$FFFE`);
- restores the caller's 8-bit stack and flags;
- returns with `rts`.

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

## 10. Inspecting

```bash
mkdir -p build/tmp && cd build/tmp
../../bin/cc45 -S x.c -o x.s                 # prologues, .func_flags, SAC storage
../../bin/cc45 -c -fno-zpcall x.c -o x.o45
../../bin/nm45 -f x.o45                      # convention / leaf / clobbers per function
../../bin/ln45 -basic -Wthunk -o x.prg main.o45 x.o45 ../../lib/build/c45.lib
```
