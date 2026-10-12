# Link-Time ZP Calling Convention Promotion

**Status:** Design proposal. **Not implemented.** No ln45 promotion pass
exists, and no `-flink-zp-promote` or `--zp-promote-budget` options exist.
**Author:** Craig Taylor (CTalkobt)
**Date:** 2026-08-27, revised to match the current toolchain

---

## Current state (what the toolchain does today)

See [calling-conventions.md](calling-conventions.md). In short:

- **ZP is the default convention.** `cc45` uses it by default (`-fzpcall`):
  parameters go in the 32-byte block `$10-$2F`. `-fno-zpcall` selects stack
  parameters. A variadic function, or one with more than 32 bytes of
  parameters, uses the stack convention even in a ZP module.
- **One library source, built twice.** `lib/Makefile` compiles the same
  sources twice:
  - `c45.lib` uses the ZP convention. `c45_zp.lib` is the same archive
    under its earlier name.
  - `c45_stack.lib` uses the stack convention.

  No per-convention library sources are maintained.
- **Cross-module mismatches are bridged.** ln45 handles both directions with
  `__thunk_s2z_*` / `__thunk_z2s_*`. `-Wthunk` warns about each one, and
  `--no-thunks` makes them errors.
- **Most locals are static.** Most functions use SAC (static activation
  records in BSS), so they have no stack frame to read parameters from.

## Idea

The ZP parameter block is a fixed 32-byte window that every ZP function
shares. The proposal is to let ln45, which sees the whole program, assign
parameter locations instead:

1. **Call graph and scores.** ln45 already builds the call graph (it is used
   for thunks and SAC checks). Score each function by call sites,
   parameter bytes and loop depth.
2. **Exclusions.** Exclude recursive, variadic and address-taken functions.
   Indirect calls cannot be rewritten.
3. **Slot allocation.** Give promoted functions dedicated ZP slots, either
   exclusive or shared through interference-graph colouring of
   non-overlapping call chains.
4. **Rewriting.** Rewrite call sites and SAC entry thunks to use the
   assigned slots. This needs relocations or metadata that identify
   argument stores. Pattern-matching linked machine code is fragile, so the
   compiler should emit explicit markers instead.

Open questions:

- **Is this still worth it?** The default ZP convention plus SAC already
  removes stack traffic. The remaining win is mainly the per-call copy from
  `$10...` into the callee's `__param_` storage. The SAC-to-SAC direct store
  path, which today is used only in `-fno-zpcall` mode, may be a simpler way
  to get it.
- **Where would the pool come from?** It would compete with the allocator
  pool `$30-$E9`.

No size or speed figures are given, because nothing has been implemented or
measured.

## References

- [calling-conventions.md](calling-conventions.md)
- `src/main/IRCodeGen.cpp`: the `CALL` lowering (ZP block stores, stack
  pushes, SAC direct stores)
- `src/main/O45Linker.cpp`: `callNeedsThunk`, `generateThunks` and
  `verifyStaticAllocSafety`
