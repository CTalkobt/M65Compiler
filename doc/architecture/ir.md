# cc45 Intermediate Representation (IR)

## Overview

The cc45 IR is a **linear three-address code** with basic blocks and virtual
registers (vregs), defined in `include/IR.hpp` (namespace `ir`). It sits between
the AST and the assembly text that cc45 hands to ca45:

```
C source → cp45 preprocessor → Lexer/Parser → AST
  → OptimizeStage   (ConstantFolder on the AST, FunctionAnalyzer)
  → CodegenStage    IRBuilder (AST → ir::Module) → IRCodeGen (IR → ca45 assembly text)
  → AssemblyStage   runs ca45 (incl. its assembler optimizer) → .o45
  → LinkingStage    runs ln45 → .prg
```

The stages live in `src/main/{Optimize,Codegen,Assembly,Linking}Stage.cpp`,
driven by `src/main/CompilationPipeline.cpp`. IRCodeGen writes ca45 source text
(simulated ops such as `mul.16`, `ldax`); M65Emitter/O45Emitter belong to the
assembler, not to this path.

**IR-level optimization.** `CodegenStage::optimizeIR()` runs the passes of
`src/main/IROptimizer.cpp` after IR generation (`-O1` and up): `optimizeStrengthReduction`,
`optimizeAlgebraic`, `optimizeTypeNarrowing`, `optimizeBranchFold`, `optimizeCSE`,
`optimizeLICM`, `optimizeCopyChains`, `optimizeAddrElemFusion`, each switchable
with `-f[no-]<name>` and per function with `#pragma cc45 optimize(...)`
(`ir::passEnabled`, `Function::optOverrides`); see
[optimizations.md](optimizations.md#ir-passes). `-Roptir` traces them.
`applyIPOSpecialization()` and `applyInlining()` only count candidates. Other
transformations happen inside `IRBuilder` itself:

- function inlining (`-O2`+ or `-finline-functions`, limited by
  `-fmax-inline-ir-size`), producing `copy` + `br @inline_endN` sequences;
- removal of unreferenced `static` functions;
- devirtualization of single-implementation vtable slots (`CALL_INDIRECT` → `CALL`);
- the parameter-narrowing advisory note.

Constant expressions are folded earlier, on the AST, by `ConstantFolder`.
Most low-level optimization happens later, in IRCodeGen (register/ZP allocation
via `VRegAllocator`, SAC, store fusion) and in the ca45 optimizer.

## Dumping the IR

`--emit-ir` is accepted by cc45 (it sets `config.emitIR`) but **does nothing**;
no `.ir` file is written. The working dump is an environment variable:

```bash
CC45_DUMP_IR=1 ./bin/cc45 -S file.c -o file.s     # IR printed to stderr
```

The dump (`ir::Printer`, `src/main/IRPrinter.cpp`) is the IR after IRBuilder,
i.e. exactly what IRCodeGen receives. It is a debugging aid only — there is no
IR parser.

## Types

| IR type (`ir::Type`) | Printed | C types (`IRBuilder::mapType`) | Size |
|----------|---------|-------------------------------|------|
| `VOID` | `void` | `void` | 0 |
| `I8`   | `i8`   | `char`, `unsigned char`; also the result of comparisons | 1 |
| `I16`  | `i16`  | `int`, `short`, enums and everything not listed elsewhere | 2 |
| `I32`  | `i32`  | `long`, 4-byte structs/unions, `__int(17..32)` | 4 |
| `PTR`  | `ptr`  | any pointer | 2 |
| `F32`  | `?` (printer gap) | `float`, `double` (CBM 40-bit) | 5 |
| `I_N`  | `?` (printer gap) | `__int(N)` wider than 4 bytes; size in `vregSizes` / `Operand::byteWidth` | N |

Aggregates are handled through their addresses (`addr_local`, `addr_global`,
`addr_elem` + `load`/`store`). Because cc45 integers are unsigned unless declared
`signed`, plain `int` comparisons/multiplies appear as the `_u`/`u` forms
(`cmp_gtu`, `mul_u`).

## Opcodes (`ir::Op`)

Printed names are those of `ir::opName()`.

| Group | Printed names |
|-------|---------------|
| Constant | `const` |
| Arithmetic | `add`, `sub`, `mul`, `div`, `mod`, `mul_u`, `div_u`, `mod_u`, `neg` |
| Bitwise / shifts | `and`, `or`, `xor`, `not`, `shl`, `shr` (logical), `asr` (arithmetic), `lsl`, `lsr` |
| Compare (signed) | `cmp_eq`, `cmp_ne`, `cmp_lt`, `cmp_le`, `cmp_gt`, `cmp_ge` |
| Compare (unsigned) | `cmp_ltu`, `cmp_leu`, `cmp_gtu`, `cmp_geu` |
| Memory | `load`, `store`, `load_zp`, `store_zp`, `deref` |
| Address | `addr_global`, `addr_local`, `addr_label` (`&&label`), `addr_uplevel` (nested-function parent frame), `addr_elem` |
| Conversion | `sext`, `zext`, `trunc` |
| Bitfields | `bfext`, `bfins` |
| Moves | `copy` |
| Control flow | `br`, `br_cond`, `br_indirect` (computed goto), `switch`, `ret` (`RET` and `RET_VOID`, the latter printed `ret void`) |
| Calls | `call`, `call_void`, `call_indirect` |
| Float (CBM ROM) | `FADD`, `FSUB`, `FMUL`, `FDIV`, `FCMP`, `FNEG`, `FTOI`, `ITOF`, `FCONST` — all printed as `unknown` (missing from `opName`) |
| Special | `asm` (inline asm), `va_start`, `trampoline`, `vec_bin` (GCC vector ops), `cpu_reg_read`/`cpu_reg_write`, `cpu_flag_read`/`cpu_flag_write` (`__cpu`/`__flags`), `get_fp`, `phi`, `nop` |

Defined but never produced by IRBuilder: `switch` (switch statements are lowered
to `cmp_eq` + `br_cond` chains), `phi`, `lsl`, `lsr`. IRCodeGen has a `PHI` case;
`SWITCH`, `LSL` and `LSR` are handled by IROptimizer passes but never produced.

## Operands and instructions

An `ir::Inst` has `op`, `dest`, `src1`, `src2`, `resultType`, an `args` vector
(call arguments, `addr_elem` element size, bitfield offset/width, ...), plus
`callConv`, `isRegparm`, `isTailCall`, `switchCases`, `phiIncoming`, `asmText`
and a `SourceLoc`.

Operand kinds (`ir::OperandKind`) and their printed form:

| Kind | Printed | Meaning |
|------|---------|---------|
| `VREG` | `%N` | virtual register |
| `IMM` | `42` | immediate |
| `GLOBAL` | `@_name` | global symbol (C names get a `_` prefix; file-`static` functions become `_name__<file>`) |
| `LABEL` | `@label` | block label |
| `FRAME_RELATIVE` | `fp+N` | frame offset |
| `NONE` | `_` | unused operand |

Local variables and parameters are vregs too (`Function::localSlotVregs`);
they are written with `store <type> <val>, %slot`, used directly as operands, and
read with `deref` where the old value must be captured (post-increment).

## Text format (real `CC45_DUMP_IR` output)

```c
int count;
unsigned char buf[8];
static int add(int x, int y) { return x + y; }
int main(void) {
    int i;
    for (i = 0; i < 8; i++) buf[i] = (unsigned char)i;
    count = add(i, 2);
    return count ? 1 : 2;
}
```

```
; Module: t.c

fn _add__t(i16 x, i16 y) -> i16 [fastcall] {
@entry:
  %2 = add i16 %0, %1 ; t.c:3
  ret %2 ; t.c:3
}

fn _main() -> i16 [fastcall] {
@entry:
  %1 = const i16 0 ; t.c:6
  store i16 %1, %0 ; t.c:6
  br @for_cond0
@for_cond0:
  %2 = const i16 8 ; t.c:6
  %3 = cmp_ltu i8 %0, %2 ; t.c:6
  br_cond %3, @for_body1, @for_end3
@for_body1:
  %4 = trunc i8 %0
  %5 = addr_elem @_buf, %0, 1 ; t.c:6
  store i8 %4, %5 ; t.c:6
@for_inc2:
  %6 = deref i16 %0 ; t.c:6
  %7 = add i16 %6, 1 ; t.c:6
  store i16 %7, %0
  br @for_cond0
@for_end3:
  %8 = const i16 2 ; t.c:7
  %9 = call @_add__t(%0, %8) -> i16 ; t.c:7
  store i16 %9, @_count ; t.c:7
  %10 = load i16 @_count ; t.c:8
  br_cond %10, @tern_then4, @tern_else5 ; t.c:8
@tern_then4:
  %12 = const i16 1 ; t.c:8
  %11 = copy i16 %12
  br @tern_end6
@tern_else5:
  %13 = const i16 2 ; t.c:8
  %11 = copy i16 %13
  br @tern_end6
@tern_end6:
  ret %11 ; t.c:8
}
```

This is the verbatim output of `CC45_DUMP_IR=1 ./bin/cc45 -S t.c` (default
options). stderr also carries the compiler's normal notes, and — for modules with
three or fewer functions — a debug line `[DCE] Functions: ... | used: ...` from
`IRBuilder::generate`.

Format rules, from `IRPrinter.cpp`:

- Module: `; Module: <file>` then each function. Globals, externs and string
  literals are **not** printed.
- Function: `fn <name>(<type> <param>, ...) -> <type>`, followed by
  `[fastcall]` when the function uses the ZP convention, then `{`. Blocks print as
  `@label:`. A function's parameters are vregs `%0..%n-1`. No other attribute is shown (variadic `...`, `static`, interrupt,
  SAC are invisible in the dump).
- Generic instruction: `[%d = ]<op> <resultType> <src1>[, <src2>]`.
  Special forms: `%d = const <type> <value>`; `store <type> <val>, <addr>`;
  `%d = call @f(<args>) -> <type>`; `call_void @f(<args>)`;
  `%d = call_indirect %p(<args>) -> <type>`; `br @l`; `br_cond %c, @then, @else`;
  `ret <val>` / `ret void`; `%d = addr_elem <base>, <index>, <elemSize>`;
  `%d = addr_uplevel levels=N, target=<name>(vK)`;
  `%d = trampoline target=..., link=..., buf=...`; `%d = phi [v, label], ...`.
- `bfext`/`bfins` print through the generic form, so the bit offset and width
  (held in `args`) are not shown. `asm` prints as `asm void _` (the text is not
  shown).
- `; file:line` is appended when the instruction has a source location.

## Function attributes (`ir::Function`)

| Field | Set from | Effect in IRCodeGen |
|-------|----------|---------------------|
| `conv` (`CallConv::STACK` / `ZP`) | `IRBuilder::usesZpConv`: ZP when `-fzpcall` (default) or `__fastcall__`, not variadic, params fit the 32-byte ZP block | parameter passing; printed `[fastcall]` |
| `CallConv::STATIC` | — | declared but never assigned |
| `isVariadic` | `...` | forces stack params, no SAC |
| `isStatic`, `isWeak` | `static`, `#pragma cc45 weak` | symbol export |
| `isInterrupt`, `isNaked`, `isRegparm` | `__interrupt`, `__naked`, `__regparm` | prologue/epilogue, no SAC for interrupt/naked |
| `isRecurse` | `#pragma cc45 recurse` | opts the function out of SAC |
| `isNested`, `staticLinkVreg` | GCC nested functions | static chain |
| `align`, `retAddrSlot`, `addressTakenLabels` | `aligned(N)`, `__builtin_return_address`, `&&label` | — |

SAC is not an IR calling convention: IRCodeGen decides it per function
(`staticAllocMode` on, not `isRecurse`/`isInterrupt`/`isNaked`/`isVariadic`,
no runtime `alloca`, not on a call cycle in the module, no direct self-call).

`ir::Module` also carries `globals` (with constant-initializer images),
`strings`, `externs`, `saveZP`, `setBP` and `stackTop`.

## IRBuilder coverage

`IRBuilder` (`include/IRBuilder.hpp`, `src/main/IRBuilder.cpp`) overrides all
46 `ASTVisitor::visit` methods: IntegerLiteral, FloatLiteral, StringLiteral,
VariableReference, Assignment, BinaryOperation, UnaryOperation,
ConditionalExpression, GenericSelection, InitializerList, ArrayAccess,
FunctionCall, MemberAccess, CastExpression, CompoundLiteral, AlignofExpression,
SizeofExpression, VariableDeclaration, ReturnStatement, BreakStatement,
ContinueStatement, SwitchContinueStatement, GotoStatement, LabelledStatement,
ExpressionStatement, IfStatement, WhileStatement, DoWhileStatement, ForStatement,
RepeatStatement, SwitchStatement, CaseStatement, DefaultStatement, AsmStatement,
StaticAssert, EnumDefinition, StructDefinition, CompoundStatement,
StatementExpression, FunctionDeclaration, BuiltinVaStart, BuiltinVaArg,
CpuRegisterAccess, CpuFlagAccess, LabelAddressExpression, TranslationUnit.

## Design notes

### Linear, not SSA

The IR is not SSA. A vreg can be assigned in several blocks — the ternary
above assigns `%11` with `copy` in both arms — and local-variable slots are
vregs that are stored to repeatedly. `phi` exists in the opcode set (and
IRCodeGen can lower it) but IRBuilder never emits it.

### Relationship to assembly

Each IR instruction becomes a short sequence of ca45 instructions and simulated
ops. The IR leaves to IRCodeGen: vreg placement (registers, ZP slots, frame or
SAC storage), frame setup, calling-convention mechanics (ZP parameter block vs.
stack pushes, SAC thunks), and float calls into the ROM.

## Known gaps

1. `--emit-ir` is a no-op; use `CC45_DUMP_IR=1`.
2. The printer has no names for `F32`/`I_N` (`?`) or the float ops (`unknown`),
   and does not print bitfield offset/width or inline-asm text.
3. Vregs are not SSA: a local's slot vreg is assigned by `store`, and a temp
   can be defined on several paths (`a && b`). Passes treat a vreg as a
   constant or fixed value only when it has exactly one definition.
