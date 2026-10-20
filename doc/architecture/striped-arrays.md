# Striped Arrays (`__striped`)

**Status: a reserved keyword for possible future expansion — accepted by the parser, ignored by code generation.**
A `__striped` array has exactly the layout, initializer data, `sizeof` and
indexing code of the same array declared without the keyword. There is no
striped layout, no reordered initializer and no indexing optimization in cc45
today.

## What the compiler does

| Part | Behaviour |
|------|-----------|
| Lexer (`src/main/Lexer.cpp`) | `__striped` and `__striped__` → `TokenType::STRIPED` |
| Parser (`src/main/Parser.cpp`) | accepted as a declaration qualifier on global, `static` and local variables (any type, any number of dimensions, combinable with `volatile`/`const`); it must come before the type name (`int __striped a[2];` is a syntax error); sets `VariableDeclaration::isStriped` |
| IRBuilder / IRCodeGen (the active code path) | never read `isStriped` — the flag has no effect |
| `#pragma cc45 no_striped`, `#pragma cc45 force_striped` | not implemented; silently ignored like other unknown `cc45` pragmas |
| ca45 `fieldStripedOpt` (`AssemblerOptimizer.cpp`, "Phase 95.5") | stub: detection always returns false, nothing is changed |

No implementation remains in code cc45 runs. The legacy AST → asm
generator (`CodeGenerator.cpp`) had `emitStripedArrayAccess` for 2D `int`
arrays and struct/union striping hooks (`FieldStripedOffsetCalc`); both were
removed with it (#92). `StructFieldStriper.cpp` and
`ExtendedStripedArrayValidator.cpp`, in `lib45-phases.a`, are not referenced by
anything.

## Verification

Done with the current `bin/` toolchain (scratch files under `build/tmp/`):

1. **Data layout.** A global with an initializer:

   ```c
   __striped int sprite[4][4] = {
       {1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}
   };
   ```

   `cc45 -S` emits the data in plain row-major order (`int` is 2 bytes):

   ```
   _sprite:
       .byte 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0
       .byte 9, 0, 10, 0, 11, 0, 12, 0, 13, 0, 14, 0, 15, 0, 16, 0
   ```

2. **Access code.** `int get(unsigned char r, unsigned char c) { return sprite[r][c]; }`
   produces the same assembly as the identical function on a non-striped array
   (only the symbol name and `.loc` line differ): `row * 8 + col * 2` via
   `mul.16`.

3. **Whole files.** A file using `__striped` on a 3D `int` array, a 2D struct
   array, a 2D union array, a `volatile` array, a 1D `char` array, a `static`
   array and a local array, plus both pragmas above, assembles to the same
   output (ignoring `.loc`) as the file with every `__striped` removed.

4. **Runtime (xemu).** A program built with `cc45 s.c -o s.prg` that checks
   `sprite[r][c] == r*4 + c + 1` for all elements, `sizeof sprite == 32` and
   `&sprite[1][0] == &sprite[0][0] + 4` exits with status 0; returning
   `sprite[1][2] + 100 * sizeof(sprite[0])` gives 807. Run with
   `timeout 15 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem s.mem -prg s.prg`,
   status read from `$FD/$FE` of the dump.

## Using the keyword

Code using `__striped` compiles and behaves like standard C arrays, so it is
safe to write, but brings no size or speed benefit. No code-size or cycle
figures apply; earlier versions of this document quoted 35–50 % size
reductions and 1.6× speedups that are not reproducible with this toolchain.

## Design intent (not implemented)

The planned layout groups the columns of the last two dimensions into
power-of-two "stripes" so that an element offset is computed with shifts and
8-bit adds instead of a 16-bit `row * width` multiply, with the compiler
re-ordering constant initializers into that layout. Implementing it would need
IRBuilder to carry `isStriped` into the global's initializer image
(`Module::GlobalVar::initBytes`) and into the `addr_elem` lowering of
`ArrayAccess`, and every way of taking an element's address (pointer
arithmetic, `memcpy`, passing a row to a function) to agree with the new
layout.

## See also

- `doc/architecture/ir.md` — the IR path that `__striped` currently passes through unchanged
- The older design notes for this unimplemented feature (phases 93-96) were
  removed; they are in the git history
