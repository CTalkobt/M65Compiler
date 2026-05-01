# TODO: cc45 Suite Roadmap
Legend:
[ ] - Todo
[d] - Deferred long term, possible future
[W] - Won't do
[I] - In progress
[X] - Done.

## Known Bugs

- [d] **BinaryExpr reentrancy (assembler expr only)**: `BinaryExpr::emit()` for `+`, `-`, `*`, `/` stores the left operand into MEGA65 hardware multiplier registers ($D770+), then evaluates `right->emit()`. If the right sub-expression itself contains `+`/`-`/`*`/`/`, it clobbers $D770 before the outer operation reads it. The compiler's `visit(BinaryOperation)` codegen is NOT affected — it evaluates left to ZP first, then right, so the hardware registers are used sequentially. This bug only affects hand-written assembler `expr` with nested runtime arithmetic sub-expressions, which is very hard to construct (constant operands are folded, register operands get clobbered). Deferred until an actual use case emerges.
- [X] **Global array store SP offset**: Fixed. The assignment handler now saves the RHS to an `allocateZP`-managed ZP slot instead of using `push .ax`/`pop .ax`, so SP remains unchanged during LHS address computation. Additionally fixed `bne(0x02)` → `bne(0x01)` for the `inc a; bne; inx` carry propagation pattern — the offset was 1 byte too large, causing a branch into the middle of the next instruction.
- [ ] **Assembler simulated op size drift**: Some simulated opcodes may produce different sizes in pass 1 vs pass 2 when operand symbols are forward-referenced. Accumulates over large code bodies causing label address drift. `ptrstack` was fixed (was missing from pass 1 and pass 2 size chains). Other ops may have similar issues with large programs containing many function calls.
- [X] **Compiler `emitAddress` ZP conflict**: Resolved — `emitAddress` already used `allocateZP` (not hardcoded addresses as initially suspected). The assignment handler was the actual conflict source, using `push/pop` instead of `allocateZP`. Fixed by switching to ZP-based RHS save.
- [ ] **Assembler `.var` non-positional evaluation**: `.var` assignments are evaluated during pass 1 parsing and the FINAL value is used for ALL references in pass 2. Temporary bumps (`.var x = x + 2` then `.var x = x - 2`) don't work positionally — all references see the last-set value. This prevents SP offset compensation via `.var` bumps around `push`/`pop` sequences.

---

## Current Optimizations
- [X] **Dead Code Removal**: Detect and remove code following a `return` statement.
- [X] **Redundant RTN Removal**: Optimize `RTN #0` to `RTS`.
- [X] **Constant Folding**: Evaluate `1 + 2` at compile time.
- [X] **Strength Reduction**: Convert `x * 2` to `x << 1`, etc.
- [X] **Register Allocation**: Better usage of A, X, Y, Z to reduce stack traffic. (Tracking implemented).
- [X] **Granular Load/Store**: Individually track low and high bytes of 16-bit variables in AX registers to eliminate redundant half-word memory operations.
- [X] **Self-Assignment Elimination**: Completely optimize out standalone `x = x` for non-volatile variables.
- [X] **Increment Optimization**: Use `INC A`, `INX`, `INW`, etc. for `+ 1`.
- [X] **Tiered Branching**: Automatic selection of short/long branches.
- [X] **Logical Short-circuiting**: Implement `&&` and `||` short-circuiting in control flow and expressions.
- [X] **Parameter/Local Mangling**: Prefix variables with `_p_` and `_l_` to avoid CPU register collisions.
- [X] **16 bit operators**: Add 16 bit simulated opcodes (or stack modes) for ASW (asl), ROW (ror), and AND. 

- [X] **Extended Register Tracking**: Track register contents more closely (e.g. variable affinity beyond just value).
- [X] **Constant Propagation**: Substitute variables with known constant values into expressions.
- [X] **Volatile Keyword Support**: Correctly parse 'volatile' and prevent dead store elimination for volatile variables.
- [X] **Frame Pointer Removal**: Eliminated 5-byte `proc` prologue and 2-byte `endproc` epilogue. Parameters now use direct stack-relative addressing (same as locals) instead of indirect `($nn,SP),Y` via a saved frame pointer. Saves 7 bytes + 2 stack bytes per function call.
- [X] **Relocatable Stack Base (`__sp_base`)**: Replaced hardcoded `$0101` with the linker-resolvable `__sp_base` symbol. Enables stack relocation to any page via `#pragma crt no_0100_stack` or `-D__sp_base=$NNNN`.

---

## Roadmap - Compiler (cc45)
Steps required to bring the C compiler closer to C11 standards.

### 1. Keyword & Syntax Support
- [X] **Control Flow**: Implement `if`, `else`, `while`, `do-while`, and `for` statements.
- [X] **Logical Operators**: Full support for `&&`, `||`, `!` with short-circuiting.
- [X] **Comparison Operators**: Support `==`, `!=`, `<`, `>`, `<=`, `>=` (signed/unsigned aware).
- [X] **Regression Testing**: Implement automated build and test runner (`test_compiler.sh`).
- [X] **Static Assertions**: Implement `_Static_assert(const-expr, string)` parsing and validation.
- [X] **Generic Selections**: Implement `_Generic` expressions for type-based dispatch.
- [X] **Function Specifiers**: Support `_Noreturn` (enabling optimization to skip return opcodes).
- [X] **Alignment**: Implement `_Alignas` and `_Alignof` to manage data alignment.
- [X] **Inline Assembly**: Support `asm("...")` or `__asm__("...")` for direct assembly insertion.
- [X] **Preprocessor**: Support `#include`, `#define`, `#undef`, `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `#line`, `#error`, `#warning`, `#pragma`.
- [X] **Macro Expansion**: Support object-like, function-like, and variadic macros, rescanning, and `#` / `##` operators.
- [X] **Variadic Macros**: Support `__VA_ARGS__` and `##__VA_ARGS__` comma removal.
- [X] **Pragma Operator**: Support `_Pragma("...")`.
- [X] **Header Guards**: Support `#pragma include_once` for file inclusion optimization.
- [X] **Expression Evaluation**: Support `defined()`, arithmetic, and logic in `#if` / `#elif`.
- [X] **Line Continuation**: Support `\` at end of line.
- [X] **Predefined Macros**: Support `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`, `__STDC__`, `__STDC_VERSION__`, `__STDC_HOSTED__`.
- [X] **Break/Continue**: Implement `break` and `continue` for loop control.
- [X] **Switch/Case**: Implement `switch`, `case`, and `default`.
- [X] **`goto` Statement**: Implement `goto` and labeled statements for unconditional jumps within a function.
- [X] **Ternary Operator**: Implement the `? :` conditional expression operator.
- [X] **`sizeof` Operator**: Implement `sizeof(type)` and `sizeof expr` to return the byte size of a type or expression at compile time.
- [X] **Compound Assignment Operators**: Support `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`.
- [X] **Bitwise Binary Operators**: Support binary `&`, `|`, `^`, `<<`, `>>` as user-level expression operators.
- [X] **Modulo Operator**: Support `%` for integer remainder in expressions.
- [X] **`for` Loop Declarations**: Support variable declarations in the `for` loop initializer clause (C99: `for (int i = 0; ...)`).
- [X] **Mixed Declarations and Code**: Allow variable declarations to appear anywhere within a block, not just at the start (C99).
- [X] **Explicit Cast Expressions**: Support C-style cast syntax `(type)expr` for explicit type conversions (narrowing, widening, pointer casts).
- [X] **Implicit Narrowing Warnings**: Emit compile-time warnings when implicit conversions lose data (e.g., `int` to `char`, pointer to `char`).
- [X] **`__func__` Identifier**: Support the predefined `__func__` string literal containing the enclosing function name (C99).
- [X] **Nested switch**: Support switch statements nested inside other switch statements.
- [X] **Deeply nested if/else**: Support 10+ levels of conditional nesting.
- [X] **Empty loop bodies**: Support `while(--n);` and similar constructs.
- [X] **Operator precedence (extended)**: Support complex pointer precedence like `*p++` vs `(*p)++`.

## Roadmap - Linker & Libraries (ln45)
- [X] **Object Format**: Define the `.o45` relocatable object format as an extension of the `.o65` specification. See [doc/lib45.md](doc/lib45.md).
    - [X] **Extended Header**: 41-byte header with 32-bit segment fields, mode word ($8800), CPU ID ($45).
    - [X] **Linear Relocations**: `R_LINEAR24` ($60) and `R_LINEAR32` ($A0) relocation types defined.
    - [X] **C++ Constants**: `O45Types.hpp` with all format enums, constants, and helpers.
    - [X] **Object Writer**: `O45Writer` class producing complete `.o45` files (header, options, segments, relocations, symbols).
    - [X] **Relocation Encoder**: `O45RelocEncoder` producing delta-offset byte streams from high-level reloc entries.
    - [X] **Symbol Table**: `O45SymbolTable` managing imports/exports with deduplication and validation.
    - [X] **Option Headers**: String and raw-byte options, `addDefaultOptions()` for OPT_OS (MEGA65) + OPT_ASM.
- [X] **Sections**: Update `ca45` to support named segments (`.text`, `.data`, `.bss`) and user defined segments (.segment "name" { ... }).
- [X] **External Symbols**: Implement `.global`, `.extern`, and `.weak` in the assembler. Weak exports use high bit of segment byte (`O45_EXPORT_WEAK`). Linker resolves strong-over-weak. C compiler supports `#pragma weak`.
- [X] **Relocatable Mode (`-c`)**: `ca45 -c` produces `.o45` object files. Extracts per-segment bodies, generates relocation tables for external and cross-segment references, builds import/export symbol tables.
- [X] **Relocations**: Support standard (`WORD`, `LOW`, `HIGH`) and linear (`LINEAR24`, `LINEAR32`) relocations in the format. `WORD` relocations emitted by `-c` mode for absolute address operands.
- [X] **Linker Tool**: `ln45` links multiple `.o45` objects: segment merging, symbol resolution, relocation patching, PRG/binary output. Supports `-t`/`-d`/`-b`/`-z` for memory layout, `-prg` for PRG output, `-m` for symbol map.
- [X] **Archiver**: `ar45` creates and manages `.lib` archives. Commands: `c` (create), `t` (list), `x` (extract), `s` (symbol index), `a` (add), `d` (delete). Global symbol index built by scanning member .o45 exports. `ln45` supports `-l` for selective linking from archives (iterative resolution handles dependency chains).

### 2. Type System Enhancements
- [X] **Structures**: Support `struct` definitions, members, and dot/arrow operators.
- [X] **Pointers**: Multi-level indirection, dereferencing (`*`), and address-of (`&`).
- [X] **`void *` Type**: Support `void` as a type specifier in parameters, return types, locals, and casts. `void *` is a generic 16-bit pointer.
- [X] **Anonymous Structures & Unions**: Support nested structs/unions without names.
- [d] **Atomic Types**: Support `_Atomic` type qualifier (requires assembler primitives for locking/atomic ops).
- [d] **Variable Length Arrays**: Support C99/C11 VLAs (requires dynamic stack allocation logic).
- [X] **Global Variables**: Support top-level variable declarations.
- [X] **Type Definitions**: Implement `typedef`.
- [X] **Signed Integers**: Support `signed` types and signed comparisons.
- [X] **`unsigned` Keyword**: Support the explicit `unsigned` type specifier for variables and return types.
- [d] **`short` Type**: Support 16-bit `short` integer type.
- [d] **`long` Type**: Support 32-bit `long` integer type.
- [d] **`long long` Type**: Support 64-bit `long long` integer type (C99).
- [X] **`_Bool` Type**: Support C99 `_Bool` boolean type and `<stdbool.h>` header.
- [d] **`float` / `double` Types**: Support floating-point types (may leverage Commodore 40-bit float format via assembler).
- [X] **`enum` Types**: Implement `enum` for defining named integer constant sets.
- [X] **`const` Qualifier**: Support `const` type qualifier and associated read-only variable semantics.
- [X] **`restrict` Qualifier**: Support C99 `restrict` pointer qualifier to declare non-aliasing pointer arguments.
- [ ] **`inline` Specifier**: The `inline` keyword is parsed and accepted (no-op). Actual function body inlining at call sites is not yet implemented — currently emits a normal `jsr` call. Future work: inline small functions (leaf, no loops, below a size threshold) by substituting the function body at the call site, mapping parameters to the caller's argument expressions.
- [X] **Storage Classes**: Implement `auto` (default local), `volatile` (qualifier), and `register` (ZP allocation hint).
- [d] **Modern Type Inference**: Implement `auto` as C23/C++ style type inference for declarations with initializers.
- [X] **`static` Storage Class**: Implement `static` for file-scope internal linkage (globals, functions) and persistent local variables.
- [X] **`register` Storage Class**: Implement `register` keyword. Register variables are allocated in zero page for faster access; falls back to stack if ZP is exhausted.
- [X] **Arrays**: Implement native array declarations (`type name[size]`), subscript indexing (`a[i]`), and pointer decay.
- [X] **Multi-dimensional arrays**: Support `int a[3][4]` row-major layout. Parser, type system, stride computation, constant-index and runtime-index codegen implemented. Validated via mmemu with loop-based stores and reads.
- [ ] **Array Initializers**: Support initialized array declarations including partial initialization and `= {0}` zero-fill.
- [X] **Struct arrays**: Support `struct point pts[10];`. Constant and runtime-indexed access with member access (`pts[i].x`), loop-based stores, and `sizeof` all work. Validated via mmemu.
- [ ] **Designated Initializers**: Support C99 designated initializers for structs (`{.x=1}`) and arrays (`{[2]=3}`).
- [ ] **Compound Literals**: Support C99 compound literals for creating unnamed temporary objects inline (e.g., `(struct Point){.x=1, .y=2}`).
- [ ] **Flexible Array Members**: Support C99 flexible array members as the last member of a struct (`int data[]`).
- [ ] **Return struct**: Support returning structs from functions.
- [ ] **Function Pointers**: Support declaration, assignment, and call-through of function pointer types.
- [d] **Lambda Functions**: Support anonymous functions and closures (capturing local scope) leveraging stack-relative context.
- [d] **Generator Functions**: Support stateful functions that can yield values (iterators).
- [d] **Generator Loop Syntax**: Implement `for (var : generator)` syntax for idiomatic iteration over generators.
- [ ] **Variadic Functions**: Support defining variadic functions and the `va_list`, `va_start`, `va_arg`, `va_end` macros.
- [ ] **Local `_Alignas`**: Extend `_Alignas` support to local (stack-allocated) variables.
- [ ] **32-bit flat addressing**: Support pointer access above $FFFF (requires flat memory codegen).

---

## Roadmap - Assembler (ca45)

### 1. Registers & Simulated Opcodes
- [X] **Mega65 Multiplication**: Simulated `mul.<width> <dest>, <src>` opcode leveraging hardware multiplier.
- [X] **Mega65 Division**: Simulated `div.<width> <dest>, <src>` opcode leveraging hardware divider.
- [X] **Stack-Relative Word Ops**: Simulated `INW/DEW offset, s` leveraging `TSX`.
- [X] **Other 16 bit registers**: Full support for `.AX`, `.AY`, `.AZ`, `.XY` in simulated high-level opcodes.
- [X] **Mega65 Memory**: High-speed memory FILL and MOVE (copy) leveraging the Mega65 DMA controller.
- [X] **PC Register**: Treat current program counter as a register named .PC similar to how .A, .X etc are defined.
- [ ] **16-bit Loads/Stores**: Support `ldax`, `stax`, `ldaz`, `staz` simulated opcodes.
- [ ] **16-bit Math**: Support `add.16`, `sub.16`, `neg.16` simulated opcodes.
- [ ] **Stack Operations**: Support `phw`, `ptrstack` simulated opcodes.
- [ ] **Control Flow**: Support `branch.16` (16-bit relative jumps).
- [ ] **Indirect Call**: Support `call (.ax)` — Indirect call through a 16-bit pointer held in `.AX`.
- [ ] **Dynamic Shifts**: Support `shl.16 .ax, src` / `shr.16 .ax, src` — Dynamic 16-bit shift with count from memory/ZP.
- [ ] **AX Increment/Decrement**: Support `inax` / `deax` — Increment/decrement `.AX` as a 16-bit pair.

### 2. Segments
- [X] **Local Optimization Windows**: Implemented `@` labels to define boundaries for register/flag tracking.
- [X] **Segment handling**: Ability to define segments to enforce local scope. 
      Also, allow anonymous segments where scope is merely defined. 
- [X] **Segment Address**: For named segments, allow mapping to various regions of memory. (eg: .segment "READONLY", .segment "CODE", etc. ). Have certain built-in segments pre-defined. Allow usage of other custom segments however. 
- [ ] **Banked Segments**: Ability to define segments as being in different banks and have an associated loader which shifts the segments upon load to the appropriate place. 

### 3. Memory & Alignment
- [X] **Stack-relative Simulation**: Extended `STX/STY/STZ offset, s` with `TSX` sequences for thread-safety.
- [X] **Alignment Directive**: Implement `.align <n>` or `.balign <n>` to support C11 `_Alignas`.
- [X] **Segment Management**: Implement `.section` or `.segment` to support `_Thread_local` storage and separate data/text areas.
- [ ] **Struct Layout Helper**: Implement `.struct` / `.endstruct` to define named field offsets as equates.
- [X] **Array Directive**: Implement `.array name, element_size, dim0 [, dim1 ...]` for multi-dimensional array storage with auto-generated stride constants.

### 4. Expanded Literals & Data
- [X] **Dword/Long**: Support `.dword` and `.long` for 32-bit unsigned data.
- [X] **Float Support**: Implement `.float` for Commodore 40-bit floating point format.
- [W] **Unicode Support**: Support UTF-8/UTF-16 character and string constants if emitted by the compiler.
- [ ] **Null-terminated Strings**: Implement `.asciiz "<str>"` — Automatically appends $00.

### 5. Atomic Primitives
- [ ] **Synchronization Macros**: Provide built-in macros for atomic exchange or compare-and-swap if targeting multi-core or interrupt-safe code.

### 6. KickAssembler Compatibility
- [X] **CPU Selection**: Support `.cpu _45gs02` directive.
- [X] **Comments**: Support `//` and `/* ... */` style comments.
- [X] **ORG Syntax**: Support `* = $addr` for compatibility.
- [X] **BasicUpstart**: Implement `.basicUpstart <addr>` for C64 BASIC stubs.
- [ ] **Binary Import**: Implement `.import binary "file.bin"`.

### 7. Expressions & Logic
- [X] **Array Indexing in Expressions**: Support `name[expr]` and `name[i][j]` syntax in `expr` for `.array`-declared arrays. Constant indices resolve at assembly time; runtime indices generate optimized indexed loads.
- [ ] **Symbol Existence**: Implement `defined(<symbol>)` — Test existence at assembler level (post-pass-1).
- [ ] **Extended Operators**: Support bitwise shift (`<<`, `>>`) and modulo (`%`) in constant expressions.
- [ ] **Conditional Assembly**: Implement `.if <expr>` / `.else` / `.endif` (distinct from preprocessor `#if`).

### 8. Missing Syntax & Instructions
- [W] **Addressing Modes**: Support Absolute Indirect Indexed `($1234),y` (Not supported by 45GS02 hardware).
- [ ] **Native Quad Mode**: Add full native support for `adcq`, `sbcq` etc. (currently prefixed).
- [ ] **Macros**: Implement `.macro` system. Support local labels (`\@`), arg count checking, and `.exitm`.
- [ ] **Repeat Block**: Implement `.repeat <n>` / `.endrep` for repeated code/data generation.
- [ ] **User Warning**: Implement `.warn "<msg>"` — User-defined assembly-time warning.
- [X] **Preprocessor**: Implement `#include`, `#define`, `#undef`, `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `#line`, `#error`, `#warning`, `#pragma`. Support function-like macros and operators.

---

## Roadmap - Companion Utilities

### High Priority (directly support the linker workflow)

- [X] **`objdump45` — Object File Inspector & Disassembler**: Disassembles `.o45`/`.o65` sections back to 45GS02 assembly with symbolic labels and branch annotations. Also displays file headers, section headers, symbol tables, relocation entries, and hex dumps. Covers all 45GS02 addressing modes. 85-assertion test suite (`make test-objdump45`).
- [X] **`nm45` — Symbol Lister & Object Inspector**: Lists symbols and inspects `.o45`/`.o65` object files. Symbol flags: `-u`, `-g`, `-n`, `-R`, `-p`, `-A`. Inspection flags: `-h` (header/mode/CPU/options), `-r` (decoded relocation tables), `-s` (segment sizes), `-a` (all). Replaces the need for separate `readobj45` and `size45` utilities.
- [X] **`readobj45`**: Covered by `nm45 -h` (header) and `nm45 -r` (relocations).

### Medium Priority

- [X] **Linker Map File**: `-M <file>` flag on `ln45` emits a detailed map file showing merged segment layout (address ranges, sizes), per-object contributions (which file contributes which address range to each segment), and all resolved symbols sorted by address with source file and weak annotations.

### Required Deliverable (not a standalone tool)

- [ ] **`crt45.s` — C Runtime Startup**: Modular CRT archive (`crt45.lib`) linked into every C program. Infrastructure for `#pragma crt` feature flags is implemented. Individual CRT modules:
    - [X] **`#pragma crt no_0100_stack`**: Relocate stack to a non-$01 page. Emits `.extern __sp_base` for linker resolution; all stack-relative ops use the relocatable `__sp_base` symbol.
    - [X] **`#pragma crt no_bssinit`**: Skip BSS zeroing at startup. Default behavior zeros BSS via a 16-bit pointer loop in `_init_bss` (called from `__init` before `_main`). Pragma suppresses the zeroing routine, emitting only an `rts` stub.
    - [ ] **`#pragma crt no_datainit`**: Skip `.data` ROM→RAM copy at startup. Opt-out flag excludes `crt_datainit.o45`.
    - [ ] **`#pragma crt no_dpsetup`**: Skip Direct Page / B register setup. Opt-out flag excludes `crt_dpsetup.o45`.
    - [X] **`#pragma crt exit_halt|exit_rts|exit_brk`**: Select program exit behavior after `main` returns. `exit_rts` (default): saves/restores caller's full 16-bit SP, provides `__exit` label for future `exit()` support. `exit_halt`: infinite `bra` loop. `exit_brk`: break to debugger. Setting one unsets the others.
    - [X] **`#pragma crt heap`**: Pull in heap initialization and malloc/free support. Opt-in flag links `crt_heap.o45` from the CRT archive. CRT startup calls `_init_heap_crt` before `_init_features`.
    - [ ] **`#pragma crt stdio`**: Pull in stdio initialization (screen, I/O). Opt-in flag forces `crt_stdio.o45` from the CRT archive. Pragma recognized but module not yet implemented.

## Roadmap - Standard Library (stdlib45.lib)
All modules are hand-written 45GS02 assembly in `lib/stdlib/`, archived into `stdlib45.lib` by `lib/Makefile`.

- [X] **`string.h`**: `strlen`, `strcpy`, `strncpy`, `strcmp`, `strncmp`, `strcat`, `strchr`, `strrchr`, `memcpy`, `memmove`, `memset`, `memcmp` (12 functions). Validated via mmemu emulator tests (`test_strlen`, `test_strcmp`, `test_strcpy`, `test_memcpy`, `test_strchr`).
- [X] **`stdio.h`**: `putchar`, `puts` (KERNAL CHROUT wrappers).
- [X] **`stdlib.h`**: `exit` (jumps to CRT `__exit` label with status in `.AX`).
- [X] **`ctype.h`**: `isdigit`, `isalpha`, `isalnum`, `isspace`, `isprint`, `toupper`, `tolower` (PETSCII-aware). Validated via build + link.
- [X] **stdlib.h** (remaining): `abs`, `atoi`, `itoa`, `rand`, `srand`. `rand` uses MEGA65 hardware RNG at `$D7EF` with busy-wait on `$D7FE` bit 7. `srand` is a no-op (hardware RNG).
- [ ] **`math.h`**: `sin`, `cos`, `tan`, `sqrt`, `log`, `exp`, `pow`, `round`, `floor`, `ceil` (may leverage Commodore 40-bit float format or hardware).
- [X] **Heap**: `malloc`, `free`, `calloc`, `realloc` (requires `#pragma crt heap`).
- [ ] **o45 Support in Mcp Server:** Add o45 support for mmemu.  This could allow various workflows such as dynamic reloading, etc. 

---

## Fixed Bugs

- [X] **mmemu MCP: `create_machine` / `reset_machine` do not clear CPU state** —
  After `create_machine` or `reset_machine`, registers retain values from the
  previous run (A, X, Y, Z, SP, PC, cycle counter all stale). The CLI creates a
  fresh machine correctly on each invocation, so this only affects the MCP server's
  persistent machine instance. Workaround: use CLI for validation testing.
  Fix: `create_machine` now erases the cached machine instance before re-creating.
  `reset_machine` now falls back to directly calling `cpu->reset()`, `bus->reset()`,
  and `ioRegistry->resetAll()` for raw machines that lack an `onReset` callback.

- [X] **Assembler: Remove debug logging for TOKEN: emission during parsing** —
  Token dumps and AST printing moved to `-vv` (verbose level 2). Phase status
  messages (preprocessing, lexing, parsing, codegen) remain at `-v` (level 1).
  No output when neither flag is used.

- [X] **Assembler: C function names collide with simulated opcode keywords** —
  The assembler lexer tokenizes identifiers like `mul` and `div` as INSTRUCTION
  tokens rather than IDENTIFIER tokens, because they match simulated opcode
  mnemonics. This caused `jsr mul` to fail because the parser's label check
  (IDENTIFIER + COLON) rejected INSTRUCTION tokens as labels. Fix: the parser's
  label recognition now accepts both IDENTIFIER and INSTRUCTION tokens followed
  by `:`. The expression evaluator already handled INSTRUCTION tokens as symbol
  references, so no other changes were needed.

- [X] **ConstantFolder / CodeGenerator: Loop results eliminated** —
  The ConstantFolder propagates constants through sequential assignments, replacing
  all reads with literal values. The CodeGenerator's `isVariableUsed` check then
  incorrectly classifies the variable as unused (only dead writes remain, which
  were explicitly excluded), eliminating the variable declaration while leaving
  the dead stores referencing an unallocated stack variable. Fix: (1) Fixed
  `VariableUseChecker::visit(Assignment&)` to count assignment targets as uses,
  ensuring the variable is always allocated when written to. (2) Added dead store
  elimination to `ConstantFolder::visit(CompoundStatement&)` — consecutive constant
  stores to the same non-volatile variable are detected and only the last surviving
  store is kept. Conservatively clears tracking on any non-assignment statement.

- [X] **CodeGenerator: ZP temp registers clobbered by function calls** —
  When generating `results[N] = func(arg)`, the CodeGenerator computes the
  destination address and stores it in ZP $02/$03, then calls the function. The
  callee clobbers ZP $02/$03 with its own temporaries. After return, the store
  via `sta ($02),y` uses the wrong address. Fix: reordered evaluation in both
  simple and compound assignment paths to evaluate the RHS first, push the
  result to the hardware stack, then compute the destination address into ZP.
  ZP is now only live during the final store sequence with no intervening calls.

- [X] **Assembler: `sta`/`lda` stack-relative addressing mode rejected** —
  The 45GS02 has no native `sta offset,s` or `lda offset,s` instructions.
  Fix: added `sta.sp` and `lda.sp` simulated opcodes that expand to
  `TSX; STA/LDA $0101+offset,X`. Updated CodeGenerator to emit these for
  8-bit local variable access. Also fixed MemberAccess visitor which hardcoded
  `, s` suffix on global struct member access.

- [X] **Assembler: `mul.16` and `div.16` simulated opcodes not registered** —
  The assembler lexer recognized `MUL` and `DIV` but not the width-suffixed
  variants `MUL.16`, `DIV.16` etc. Fix: added `MUL.8`/`.16`/`.24`/`.32` and
  `DIV.8`/`.16`/`.24`/`.32` to the lexer instruction set.

- [X] **Assembler: `phw.sp` simulated opcode not registered** —
  Fix: added `PHW.SP` to the lexer instruction set, added parser dispatch to
  `PHW_STACK` statement type, and updated CodeGenerator to emit `phw.sp`
  (consistent with `stw.sp`/`ldw.sp` naming convention).

- [X] **Assembler: `push`/`pop` simulated opcodes emit null bytes** —
  `push .ax` emitted `$00 $00` (BRK; BRK) instead of `PHA ($48); PHX ($DA)`.
  Root cause: a duplicate PUSH/POP handler in `AssemblerGenerator.cpp` was
  unreachable (the first handler at the correct position had `continue`), and
  the first handler was never properly wired into the generator's emit path.
  Fix: removed the duplicate handler, confirmed the first handler at line 251
  correctly calls `emitPushPopCode` and emits proper bytes. This fixed the
  `mmemu_compiler_simple` emulator validation test.

- [X] **CodeGenerator: Switch case comparisons fail at runtime** —
  The `test_mmemu_control` emulator test shows switch cases 1 and 2 falling
  through to the default case instead of matching. Root cause: `push .ax`
  emitted PHA then PHX, placing bytes in big-endian order on the stack
  (A/low byte at higher address, X/high byte at lower address). But
  `ldax ..., s` reads in little-endian order (offset = low byte,
  offset+1 = high byte), swapping the bytes. Fix: reversed the push order
  to PHX then PHA (high byte first) so low byte (A) ends up at the lower
  stack address, matching the little-endian convention used by `ldax`,
  `stax`, and the native `phw` instruction. Pop order adjusted accordingly.

- [X] **CodeGenerator: Arrow operator fails on struct pointer parameters** —
  When a function receives a `struct *` parameter and uses `p->member`, the
  CodeGenerator throws "Dot/Arrow operator on non-struct type: int". The
  parameter's type is recorded with `pointerLevel > 0` and `type = "struct Foo"`,
  but `getExprType()` resolves the parameter to its base type without preserving
  the struct name, so `isStruct()` returns false. Fix: `getExprType()` now checks
  the `_p_` (parameter) prefix in `variableTypes` before checking `_l_` (local),
  matching the lookup order used by `resolveVarName()`. Parameters with struct
  types are now correctly resolved.

- [X] **CodeGenerator: `proc` instruction emitted without parameter declarations** —
  The CodeGenerator emitted `proc funcname` (no params) on one line and
  `.proc funcname, W#_p_val` (with params) on the next. Only the `proc`
  instruction creates assembler scope and parameter symbols; the `.proc`
  directive is a no-op. Functions without local variables had undefined
  parameter symbols (offset 0), causing stack-relative reads to access the
  return address instead of the parameter. Functions with locals appeared to
  work because `.var _p_x = _p_x + 2` happened to create the symbol with a
  usable (but accidentally correct) value via double-evaluation. Fix: merged
  parameter declarations onto the `proc` instruction line and removed the
  redundant `.proc` directive.

- [X] **Assembler: `lda.sp` / `sta.sp` missing from pass-2 code generator** —
  The `LDA_STACK` and `STA_STACK` statement types were handled during pass-1
  size calculation but had no corresponding emit handlers in the pass-2
  generator (`AssemblerGenerator.cpp`). The generator fell through to the
  default case and emitted zero bytes, producing BRK instructions in place
  of stack-relative loads/stores. Fix: added `LDA_STACK` and `STA_STACK`
  dispatch entries alongside the existing `LDX_STACK`/`STX_STACK` handlers.

- [X] **Assembler: `.var` assignment re-evaluated during pass-2 generation** —
  The `.var` directive was evaluated both during initial parsing (pass 1) and
  again during code generation (pass 2) in `AssemblerGenerator.cpp`. For
  self-referencing expressions like `.var _p_val = _p_val + 2`, this caused
  the adjustment to be applied twice (e.g., 2→4 in pass 1, then 4→6 in
  pass 2). Fix: removed the ASSIGN re-evaluation from the generator; the
  value set during parsing is now final. INC/DEC variants are unaffected.

---

- [X] **Assembler: `ptrstack` broken for global variables** —
  `ptrstack _globalVar` always emitted stack-relative code (`TSX; TXA; CLC; ADC #(spBase + addr)...`),
  treating the global label's absolute address as a stack offset. This produced
  nonsensical addresses for global arrays and pointers. Fix: `emitPtrStackCode`
  now checks `resolveSymbol` — if the symbol is not stack-relative or `.var`-defined,
  it emits `LDA #lo; LDX #hi` (absolute address load) instead.

- [X] **Assembler: `ptrstack` missing from pass 1 and pass 2 sizing chains** —
  `PTRSTACK` was not listed in the simulated opcode sizing blocks in either
  `pass1()` or the pass 2 recalculation loop. This caused `stmt->size = 0` for
  all `ptrstack` instructions, producing cumulative address drift for every label
  after a `ptrstack`. Programs with multiple `ptrstack` calls (e.g., global array
  access) would crash with BRK due to label addresses being off by the total
  missing bytes. Fix: added `PTRSTACK` to both sizing chains alongside `PTRDEREF`.

- [X] **Compiler: `emitAddress` stride multiply treated ZP address as constant** —
  For array element size > 2, `emitAddress` stored the index value to a ZP
  scratch address (e.g., `stax $04`), then emitted `mul.16 .ax, $04`. The
  assembler's `mul.16` expression parser interpreted `$04` as the literal value 4,
  not as a memory reference to ZP $04. All stride multiplications produced
  `index * 4` instead of `index * stride`. Fix: emit `mul.16 .ax, #$stride`
  using an immediate operand, keeping the index in AX.

- [X] **Assembler: `mul.16`/`div.16` result clobbered low byte** —
  The result-reading loop for register destinations (`.AX`, `.AXY`, `.Q`) loaded
  bytes sequentially: `LDA $D778` (low into A), then `LDA $D779` (high into A,
  clobbering low), then `TAX` (high into X). Result: A=high, X=high, low byte
  lost. For any multiply/divide producing a result > 255, the low byte was
  always 0. Fix: read high bytes first into their target registers (X/Y/Z),
  then load the low byte into A last. Same fix applied to `emitDivCode`.

- [X] **Assembler: `mul.16`/`div.16` treated bare address as immediate** —
  `mul.16 .ax, $04` parsed `$04` as constant value 4 (immediate) instead of
  reading from memory address $04. The `isConstant()` check was used as a proxy
  for immediate mode without verifying the `#` prefix. Fix: added `isImmediate`
  check (looks for `HASH` token) to `emitMulCode`, `emitDivCode`, and
  `emitSignedMathOp`. Only uses the immediate path when `#` is present.

- [X] **Compiler: `sizeof` wrong for struct/pointer arrays** —
  `sizeof(pts)` on `struct Point pts[4]` returned 8 instead of 16. The
  `getExprType` array decay added `arrayDims.size()` to `pointerLevel`, making
  `getTypeSize` treat the element as a 2-byte pointer instead of a 4-byte struct.
  Fix: `sizeof` now uses the raw `vi->pointerLevel` from VarInfo, bypassing decay.

- [X] **Compiler: global integer initializers emitted with wrong hex padding** —
  `int a = 3` emitted `.word $3000` instead of `.word $0003`. The `std::setw(4)`
  hex formatting was right-padded because a previous output statement had set
  `std::left` on the stream. Fix: add `std::right` before `setw`/`setfill`.

- [X] **Compiler: assignment RHS saved via `push .ax` shifted SP** —
  The generic assignment fallback evaluated the RHS, pushed it to the hardware
  stack (`push .ax`), then called `emitAddress` to compute the LHS address.
  `emitAddress` re-evaluated index sub-expressions via `ldax _l_i, s` which read
  the wrong stack offset (SP shifted by 2). Fix: save RHS to an `allocateZP`-managed
  ZP slot instead of the hardware stack. SP remains unchanged during LHS computation.

- [X] **Compiler: `inc a; bne *+4; inx` branch offset off by 1** —
  The 16-bit increment pattern emitted `bne(0x02)` to skip the 1-byte `INX`.
  Relative offset 2 skips 2 bytes past the BNE, but INX is only 1 byte — the
  branch landed 1 byte into the following instruction. This caused BRK crashes
  when the next instruction wasn't a 2-byte opcode (previously masked because
  `push .ax` = PHX+PHA happened to have a valid opcode at byte 2).
  Fix: `bne(0x01)` to correctly skip the single-byte INX.

- [X] **Compiler: `_init_bss` placed after function code** —
  The BSS init routine was emitted in `emitData()` after all function bodies,
  requiring a `.code` segment resume. The assembler's simulated opcode size
  drift caused the `_init_bss` label to resolve to an incorrect address,
  jumping into the middle of a preceding instruction (BRK). Fix: moved
  `_init_bss` into the CRT stub area (before functions) with a pre-scan
  of global declarations to determine if BSS zeroing is needed.

## Validation & Stress Testing
- [ ] **Duff's device**: Switch interleaved with loop (stress test).
- [ ] **Stack depth stress**: Deeply nested calls exhausting 8-bit stack.
- [ ] **Optimizer differential**: Same test at -O0 vs -O1, compare results.
- [ ] **Register allocation conflicts**: Expressions using A, X, Y, Z simultaneously.
- [ ] **Constant propagation safety**: Volatile prevents folding across branches.
