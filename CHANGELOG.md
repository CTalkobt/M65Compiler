# Changelog

All notable changes to the cc45 / ca45 suite will be documented in this file.

## [Unreleased] - 2026-05-01

### Added
- **Compiler (cc45)**:
    - **Frame pre-allocation**: Local variables are now pre-scanned and allocated as a single frame in the function prologue, replacing the incremental push-per-declaration model. A new `FrameScanner` AST walker computes `maxFrameSize` and assigns fixed frame offsets to each local variable with scope-aware slot reuse — variables in non-overlapping scopes (e.g. separate `{ }` blocks, if/else branches) share frame slots, reducing stack usage. The entire frame is zeroed at function entry via `PHW #0` instructions. Local variable initialization now writes directly to pre-allocated frame slots using `sta.fp`/`stax.fp` pseudo-ops. Only `_fp` needs bumping when pushing function call arguments — not every local variable individually. Parameters use `.var` at `frameSize + 2 + paramOff` (past frame + return address). Resolves the "scoped variable lifetime" issue from the 1.0 roadmap.
- **Assembler (ca45)**:
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
- **Headers**:
    - **`stdlib.h`**: Added `atos(s)` and `stoa(value, str, base)` convenience macros for `short` string conversion. `atos` casts through `atoi`; `stoa` casts through `itoa`.
    - **Return struct by value**: Functions can now return structs by value (e.g. `struct Point make_point(int x, int y)`). Uses a hidden-pointer ABI: the caller passes the address of the destination as an implicit last parameter (`_p___ret_ptr`); the callee copies the return value through this pointer before returning. The caller computes the destination address via `leax.fp` before pushing regular arguments. Struct-returning function calls used as initializers (`struct Point p = make_point(1,2)`) write directly to the local's frame slot — no intermediate copy. `getExprType` now returns the struct type for calls to struct-returning functions. `FunctionDeclaration` AST node gains `returnPointerLevel` field. New tracking maps `functionReturnTypes` and `structReturningFunctions` in CodeGenerator.

### Fixed
- **Assembler (ca45)**:
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

## [Unreleased] - 2026-04-30

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
- Added `test_array.s` — assembler test for `.array` directive and `expr` array indexing (constant and runtime indices, multi-dimensional, stride metadata constants).
- Added `test_pragma_heap.c` — compiler test validating `#pragma crt heap` with `malloc`/`free` usage compiles and assembles.
- Added `test_malloc.c` — compiler test validating `stdlib.h` heap function declarations (`malloc`, `free`, `calloc`, `realloc`) compile and assemble.
- Verified `malloc.s` assembles in relocatable mode (`ca45 -c`) with correct symbol exports (`_malloc`, `_free`, `_calloc`, `_realloc`, `_heap_init`).
- Verified `crt_heap.s` assembles in relocatable mode with `_init_heap_crt` export.
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

## [Unreleased] - 2026-04-27

### Added
- **Compiler (cc45)**:
    - **Relocatable object mode (`-c`)**: `cc45 -c input.c -o output.o45` compiles C to a `.o45` relocatable object file. The CodeGenerator auto-emits `.global` for defined functions and global variables, `.extern` for called-but-not-defined functions. Skips the `.org $2000` and startup stub in reloc mode. The `-o` flag controls the final `.o45` name; intermediate `.s` file is generated automatically.
    - **`#pragma weak`**: Marks the next function or global variable as a weak export. The preprocessor converts `#pragma weak` to an internal `.weak_next` marker; the CodeGenerator emits `.weak` instead of `.global`. Weak symbols can be overridden by strong definitions at link time.
    - **Frame-pointer-relative parameter access**: Function parameters are now accessed via a saved frame pointer using the 45GS02's native `($nn,SP),Y` addressing mode (opcodes $E2/$82). The `proc` prologue saves SP as a 16-bit LE pointer on the stack (`TSX; LDA #$01; PHA; PHX`). Parameters get fixed Y offsets that never change as locals are pushed, eliminating the need for `.var` offset bumping on parameters.
    - Added `_fp` assembler variable that tracks the frame pointer's stack position, automatically adjusted by `.var` as locals are declared.
    - Added `test_many_params_locals.c` — validates functions with >2 parameters (up to 5) and >2 local variables (up to 6), including mixed char/int params, nested multi-param calls, and computed expression arguments (10 test cases).
    - Added `test_16bit_stack.s` — validates 16-bit stack pointer relocation via `TYS`/`TSY` with push/pull verification on page $40.
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
    - Added `test_global_extern.s` to assembler test suite — validates `.global`/`.extern` directive parsing.
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

## [Unreleased] - 2026-04-26

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
    - Added `src/test/test_mmemu.sh` and `src/test-resources/test_mmemu_hello.s` for automated validation using `mmemu-cli`.

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

## [Unreleased] - 2026-04-23

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

## [Unreleased] - 2026-04-18

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
    - Implemented header guard optimization via `#pragma include_once`.
    - Added support for `#if` and `#elif` expression evaluation, including `defined()`, arithmetic, and logical operators.
    - Added support for line continuation using the backslash (`\`) character.
    - Added support for standard predefined macros: `__STDC__`, `__STDC_VERSION__`, and `__STDC_HOSTED__`.
    - Added support for `#undef`, `#line`, `#error`, `#warning`, and `#pragma` directives.
    - Implemented expansion of standard predefined macros: `__FILE__`, `__LINE__`, `__DATE__`, and `__TIME__`.
- **Documentation**:
    - Renamed preprocessor documentation to `doc/cp45.md` to align with toolchain naming conventions.
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

## [Unreleased] - 2026-04-18

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

## [Unreleased] - 2026-04-14
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
