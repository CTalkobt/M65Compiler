# TODO: cc45 Suite Roadmap
Legend:
[ ] - Todo
[d] - Deferred long term, possible future
[W] - Won't do
[I] - In progress
[X] - Done.

## Known Bugs

(none)

---

## Current Optimizations
- [X] **Dead Code Removal**: Detect and remove code following a `return` statement.
- [X] **Redundant RTN Removal**: Optimize `RTN #0` to `RTS`.
- [X] **Constant Folding**: Evaluate `1 + 2` at compile time.
- [X] **Strength Reduction**: Convert `x * 2` to `x << 1`, etc.
- [X] **Register Allocation**: Better usage of A, X, Y, Z to reduce stack traffic. (Tracking implemented).
- [X] **Increment Optimization**: Use `INC A`, `INX`, `INW`, etc. for `+ 1`.
- [X] **Tiered Branching**: Automatic selection of short/long branches.
- [X] **Logical Short-circuiting**: Implement `&&` and `||` short-circuiting in control flow and expressions.
- [X] **Parameter/Local Mangling**: Prefix variables with `_p_` and `_l_` to avoid CPU register collisions.
- [X] **16 bit operators**: Add 16 bit simulated opcodes (or stack modes) for ASW (asl), ROW (ror), and AND. 

- [X] **Extended Register Tracking**: Track register contents more closely (e.g. variable affinity beyond just value).
- [X] **Constant Propagation**: Substitute variables with known constant values into expressions.
- [X] **Volatile Keyword Support**: Correctly parse 'volatile' and prevent dead store elimination for volatile variables.

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

## Roadmap - Linker & Libraries (ln45)
- [ ] **Object Format**: Define the `.o45` relocatable object format as an extension of the `.o65` specification.
    - [ ] **Extended Header**: Support 45GS02 architecture bits and 32-bit width flags.
    - [ ] **Linear Relocations**: Support for patching linear 28-bit addresses and 32-bit data types.
- [X] **Sections**: Update `ca45` to support named segments (`.text`, `.data`, `.bss`) and user defined segments (.segment "name" { ... }).
- [X] **External Symbols**: Implement `.global` and `.extern` in the assembler.
- [ ] **Relocations**: Support standard (`WORD`, `LOW`, `HIGH`) and linear (`LINEAR24`, `LINEAR32`) relocations.
- [ ] **Linker Tool**: Build the `ln45` binary to aggregate objects and resolve symbols.
- [ ] **Archiver**: Build `ar45` to create static `.lib` archives.

### 2. Type System Enhancements
- [X] **Structures**: Support `struct` definitions, members, and dot/arrow operators.
- [X] **Pointers**: Multi-level indirection, dereferencing (`*`), and address-of (`&`).
- [X] **Anonymous Structures & Unions**: Support nested structs/unions without names.
- [d] **Atomic Types**: Support `_Atomic` type qualifier (requires assembler primitives for locking/atomic ops).
- [d] **Variable Length Arrays**: Support C99/C11 VLAs (requires dynamic stack allocation logic).
- [X] **Global Variables**: Support top-level variable declarations.
- [X] **Type Definitions**: Implement `typedef`.
- [X] **Signed Integers**: Support `signed` types and signed comparisons.
- [X] **`unsigned` Keyword**: Support the explicit `unsigned` type specifier for variables and return types.
- [d] **`short` Type**: Support 16-bit `short` integer type.
- [d] **`long` Type**: Support 32-bit `long` integer type.
- [ ] **`long long` Type**: Support 64-bit `long long` integer type (C99).
- [ ] **`_Bool` Type**: Support C99 `_Bool` boolean type and `<stdbool.h>` header.
- [ ] **`float` / `double` Types**: Support floating-point types (may leverage Commodore 40-bit float format via assembler).
- [X] **`enum` Types**: Implement `enum` for defining named integer constant sets.
- [ ] **`const` Qualifier**: Support `const` type qualifier and associated read-only variable semantics.
- [ ] **`restrict` Qualifier**: Support C99 `restrict` pointer qualifier to declare non-aliasing pointer arguments.
- [ ] **`inline` Specifier**: Support the `inline` function specifier for inlining hints (C99).
- [X] **Storage Classes**: Implement `auto` (default local) and `volatile` (qualifier).
- [d] **Modern Type Inference**: Implement `auto` as C23/C++ style type inference for declarations with initializers.
- [ ] **Storage Classes (remaining)**: Implement `static` (local persistence and file-scope linkage), `extern` (external linkage), `register` (hint).
- [X] **Arrays**: Implement native array declarations (`type name[size]`), subscript indexing (`a[i]`), and pointer decay.
- [ ] **Array Initializers**: Support initialized array declarations including partial initialization and `= {0}` zero-fill.
- [ ] **Designated Initializers**: Support C99 designated initializers for structs (`{.x=1}`) and arrays (`{[2]=3}`).
- [ ] **Compound Literals**: Support C99 compound literals for creating unnamed temporary objects inline (e.g., `(struct Point){.x=1, .y=2}`).
- [ ] **Flexible Array Members**: Support C99 flexible array members as the last member of a struct (`int data[]`).
- [ ] **Function Pointers**: Support declaration, assignment, and call-through of function pointer types.
- [d] **Lambda Functions**: Support anonymous functions and closures (capturing local scope) leveraging stack-relative context.
- [d] **Generator Functions**: Support stateful functions that can yield values (iterators).
- [d] **Generator Loop Syntax**: Implement `for (var : generator)` syntax for idiomatic iteration over generators.
- [ ] **Variadic Functions**: Support defining variadic functions and the `va_list`, `va_start`, `va_arg`, `va_end` macros.
- [ ] **Local `_Alignas`**: Extend `_Alignas` support to local (stack-allocated) variables.

---

## Roadmap - Assembler (ca45)

### 1. Registers
- [X] **Mega65 Multiplication**: Simulated `mul.<width> <dest>, <src>` opcode leveraging hardware multiplier.
- [X] **Mega65 Division**: Simulated `div.<width> <dest>, <src>` opcode leveraging hardware divider.
- [X] **Stack-Relative Word Ops**: Simulated `INW/DEW offset, s` leveraging `TSX`.
- [X] **Other 16 bit registers**: Full support for `.AX`, `.AY`, `.AZ`, `.XY` in simulated high-level opcodes.
- [X] **Mega65 Memory**: High-speed memory FILL and MOVE (copy) leveraging the Mega65 DMA controller.
- [X] **PC Register**: Treat current program counter as a register named .PC similar to how .A, .X etc are defined. 

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


### 4. Expanded Literals
- [X] **Dword/Long**: Support `.dword` and `.long` for 32-bit unsigned data.
- [X] **Float Support**: Implement `.float` for Commodore 40-bit floating point format.
- [W] **Unicode Support**: Support UTF-8/UTF-16 character and string constants if emitted by the compiler.

### 5. Atomic Primitives
- [ ] **Synchronization Macros**: Provide built-in macros for atomic exchange or compare-and-swap if targeting multi-core or interrupt-safe code.

### 6. KickAssembler Compatibility
- [X] **CPU Selection**: Support `.cpu _45gs02` directive.
- [X] **Comments**: Support `//` and `/* ... */` style comments.
- [X] **ORG Syntax**: Support `* = $addr` for compatibility.
- [X] **BasicUpstart**: Implement `.basicUpstart <addr>` for C64 BASIC stubs.
- [ ] **Binary Import**: Implement `.import binary "file.bin"`.

### 7. Missing Syntax & Instructions
- [W] **Addressing Modes**: Support Absolute Indirect Indexed `($1234),y` (Not supported by 45GS02 hardware).
- [ ] **Native Quad Mode**: Add full native support for `adcq`, `sbcq` etc. (currently prefixed).
- [ ] **Macros**: Implement `.macro` system.
- [X] **Preprocessor**: Implement `#include`, `#define`, `#undef`, `#if`, `#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `#line`, `#error`, `#warning`, `#pragma`. Support function-like macros and operators.
- [ ] **Standard Library**: Add built-in functions like `sin()`, `cos()`, `round()`.

---

## Roadmap - Companion Utilities

### High Priority (directly support the linker workflow)

- [ ] **`dis45` — Disassembler**: Disassemble `.bin`/`.prg` output or `.o45` sections back to 45GS02 assembly. Critical for verifying compiler and linker output; no existing disassembler fully covers the 45GS02 instruction set (quad-mode, linear addressing, etc.).
- [ ] **`nm45` — Symbol Lister**: Lists exported/imported symbols in `.o45` object files and `.lib` archives. Shows symbol name, segment, offset, and type (export/import/undefined). Essential for diagnosing linker errors before invoking `ln45`.
- [ ] **`readobj45` — Object File Inspector**: Human-readable dump of `.o45` file internals: header fields, segment sizes, relocation tables, symbol table. Analogous to `readelf`; primary debugging tool during `ln45` development.

### Medium Priority

- [ ] **`size45` — Segment Size Reporter**: Reports `.text`, `.data`, `.bss`, `.zp` sizes for one or more `.o45` files or the final binary. Critical on the MEGA65's constrained memory map.
- [ ] **`strip45` — Symbol Stripper**: Removes symbol/debug information from a final linked binary to reduce size. Particularly relevant since `.zp` space is precious.
- [ ] **`prg45` — PRG File Utility**: Inspect, split, or re-header `.prg` files (2-byte load-address format). Could also handle multi-part PRG loads or inject a BASIC stub.
- [ ] **Linker Map File**: `-M` flag on `ln45` to emit a map file showing final address assignment for every symbol and segment.

### Required Deliverable (not a standalone tool)

- [ ] **`crt45.s` — C Runtime Startup**: Assembly source linked into every C program. Responsibilities: copy initialized `.data` from ROM to RAM, zero `.bss`, set up the zero-page register pool (the `B` register for Direct Page), then `JSR main` and handle return.

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
