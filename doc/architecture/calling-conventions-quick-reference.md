# Calling Conventions — Quick Reference

Short summary of [calling-conventions.md](calling-conventions.md). See that
document for details and examples.

## Modes

| Flag | Parameters | Library | Notes |
|---|---|---|---|
| (default) / `-fzpcall` | ZP block `$10-$2F` | `c45.lib` | variadic or >32 bytes of params → stack convention |
| `-fno-zpcall` | pushed right to left, caller pops | `c45_stack.lib` | |
| (default) SAC | locals/params in static BSS | — | `-fno-staticalloc` disables |

`__fastcall__ int f(...)` forces the ZP convention in a `-fno-zpcall` module.
`__regparm` passes the first argument in A or A:X.

## Parameter slots

- `char` takes 2 bytes; `int` and pointers take 2; `long` takes 4; `float`
  takes 5.
- A struct, union or `long long` passed by value is copied by the caller,
  and the copy's address is passed (2 bytes).
- Slots are laid out left to right: from `$10` in the ZP convention, or from
  `SP+3` at entry in the stack convention.

## Return values

```
char / _Bool          A
int / short / ptr     A (lo), X (hi)
long                  A X Y Z (Q)
float, struct, union, long long
                      the caller passes a hidden __sret pointer as the
                      first parameter; the callee writes through it and
                      returns it in A:X
```

## Frames (non-SAC functions)

- A frame exists only if the function has stack temporaries. The frame
  pointer `$FD/$FE` is saved by the callee and set to SP+1 with
  `tsy/tsx/inx/bne/iny`.
- From FP the layout is:
  `[frame bytes][saved FP (2)][return address (2)][stack params]`.
- The frame pointer is never recalculated after calls.
- Locals are accessed with `lda ($FD),y`; the pseudo-ops are
  `ldax.local`, `ldax.param` and `stax.local`.

## SAC

SAC is not used, and the function gets a stack frame instead, for:

- direct recursion (warning; silence it with `#pragma cc45 recurse`);
- recursion through other functions in the same file;
- `__interrupt`, `__naked` and variadic functions;
- a runtime `alloca`.

Each SAC function has two entries:

- `<fn>` copies the parameters into `<fn>__param_<name>`.
- `<fn>__sac` skips that copy. Stack-mode SAC callers in the same file use it.

ln45 reports an error for a SAC function on a call cycle, or one reachable
from an `__interrupt` handler.

## Zero page (program base page `__zp_page`)

```
$06-$07 static chain    $08-$0F scratch (__zp_scratch..4)
$10-$2F ZP parameters   $30-$E9 temporaries    $FD-$FE frame pointer
```

- ROM calls are wrapped as `jsr __bp_rom` … `jsr __bp_prog`.
- An `__interrupt` handler runs on `__zp_page_isr`.

## Stack

The stack is 16-bit, with its top at `$BFFF`. Change it with
`-fstack-top=ADDR` or `#pragma cc45 stack_top(ADDR)`; keep the ROM's page-1
stack with `-fno-stack16` or `#pragma cc45 no_stack16`.

## Mixing modes (ln45)

- A cross-module mismatch, in either direction, gets a bridge thunk:
  `__thunk_s2z_<fn>` or `__thunk_z2s_<fn>`.
- `-Wthunk` warns about each thunk; `--no-thunks` makes a mismatch an error.
- Calls through function pointers are not bridged.
