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
`src/main/IROptimizer.cpp` after IR generation (`-O1` and up), in this order:
`inferPurity` (which functions are `const` / pure), `optimizeLoopUnroll`
(from `-O4`), `optimizeParamNarrowing`, `optimizeStrengthReduction`,
`optimizeAlgebraic`, `optimizeTypeNarrowing`, `optimizeBranchFold`, `optimizeCSE`,
`optimizeShareDivMod`, `optimizeLICM`, `optimizeLoopStrengthReduction`,
`optimizeCopyChains`, then `optimizeTypeNarrowing`, `optimizeCopyChains`,
`optimizeAddrElemFusion`, `optimizeStrengthReduction` and `optimizeAlgebraic`
again; `removeDeadTemps`; the leaf-function pointer allocation
(`leafPointerParamsToZp`); `optimizePostIncrementSink`; the late folds
(`foldElemIndexOffset`, `foldGlobalOffsetAccess`, `foldFarOffset`,
`foldNegOffset`, `orderCommutativeOperands`) and last `optimizeBlockReorder`.
Loop rotation follows in `CodegenStage::generateAssembly`. The catalog's
passes are switchable with `-f[no-]<name>` and per function with
`#pragma cc45 optimize(...)` (`ir::passEnabled`, `Function::optOverrides`); see
[optimizations.md](optimizations.md#ir-passes). `-Roptir` traces them.
`CodegenStage::applyInlining()` and `eliminateDeadCode()` only count
candidates or log (`applyIPOSpecialization()` is no longer called; it is in
`lib45-phases.a`). Other transformations happen inside `IRBuilder` itself:

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

The dump (`ir::Printer`, `src/main/IRPrinter.cpp`) is printed at the start
of `CodegenStage::generateAssembly`: the IR after IRBuilder and the IR passes
of the `-O` level, i.e. exactly what IRCodeGen receives (with `-O0`, the IR
as IRBuilder made it). Instructions a pass removed show as `nop` lines
(`nop void _`, `nop ptr _`). It is a debugging aid only — there is no IR
parser.

## Types

| IR type (`ir::Type`) | Printed | C types (`IRBuilder::mapType`) | Size |
|----------|---------|-------------------------------|------|
| `VOID` | `void` | `void` | 0 |
| `I8`   | `i8`   | `char`, `unsigned char`, `_Bool`, near pointers (`T __near *`); also the result of comparisons | 1 |
| `I16`  | `i16`  | `int`, `short`, enums, 16-bit `__fixed` and everything not listed elsewhere | 2 |
| `I32`  | `i32`  | `long`, far pointers (`T __far *`), 32-bit `__fixed`, 4-byte structs/unions, `__int(17..32)` | 4 |
| `PTR`  | `ptr`  | any (ordinary, 16-bit) pointer | 2 |
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
| Memory | `load`, `store`, `load_zp`, `store_zp`, `deref`, `far_load`, `far_store` (a 32-bit address: far pointers, #68), `near_load`, `near_store`, `near_addr` (an 8-bit base-page offset: near pointers) |
| Address | `addr_global`, `addr_local`, `addr_label` (`&&label`), `addr_uplevel` (nested-function parent frame), `addr_elem` |
| Conversion | `sext`, `zext`, `trunc` |
| Bitfields | `bfext`, `bfins` |
| Moves | `copy` |
| Control flow | `br`, `br_cond`, `br_indirect` (computed goto), `switch`, `ret` (`RET` and `RET_VOID`, the latter printed `ret void`) |
| Calls | `call`, `call_void`, `call_indirect` |
| Float (CBM 40-bit) | `FADD`, `FSUB`, `FMUL`, `FDIV`, `FCMP`, `FNEG`, `FTOI`, `ITOF`, `FCONST` — all printed as `unknown` (missing from `opName`). IRCodeGen lowers the arithmetic and comparisons to runtime calls: `__float_add` … (the BASIC 65 ROM's routines, `lib/src/float_rom.s45`, reached through `__float_romcall`, #614), or `__ffast_add` … (RAM routines, `stdlib_asm/float_fast.s45`) with `-ffast-float=1` / `-Ospeed` (`Module::fastFloat`) |
| Special | `asm` (inline asm), `va_start`, `trampoline`, `vec_bin` (GCC vector ops), `cpu_reg_read`/`cpu_reg_write`, `cpu_flag_read`/`cpu_flag_write` (`__cpu`/`__flags`), `get_fp`, `phi`, `nop` |

Defined but never produced by IRBuilder: `switch` (switch statements are lowered
to `cmp_eq` + `br_cond` chains), `phi`, `lsl`, `lsr`. IRCodeGen has a `PHI` case;
`SWITCH`, `LSL` and `LSR` are handled by IROptimizer passes but never produced.

## Operands and instructions

An `ir::Inst` has `op`, `dest`, `src1`, `src2`, `resultType`, an `args` vector
(call arguments, `addr_elem` element size, bitfield offset/width, ...), plus
`callConv`, `isRegparm`, `isTailCall`, `callSig` (an indirect call's
parameter slots, for ln45's call graph), `elemOffset` (bytes added to a
global base: `a[i + c]`, or a far access's Z offset), `switchCases`,
`phiIncoming`, `asmText`, a `SourceLoc` and, for an inlined instruction,
`callLoc` (the call it was inlined at). A `Block` has a label, its
instructions and a `cold` flag (`__unlikely`).

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

This is the verbatim output of `CC45_DUMP_IR=1 ./bin/cc45 -O0 -S t.c`.
stderr also carries the compiler's normal notes (here the parameter-narrowing
note for `add`). At the default `-O2` the same file dumps one function:
`add` is inlined into `main` (its `add` followed by `@inline_end4:`) and
removed, the constants are immediates (`%3 = cmp_ltu i8 %0, 8`), the
`addr_elem` is fused into the store, and the removed instructions are `nop`
lines.

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

### Far and near pointers, vectors, trampolines

All from `CC45_DUMP_IR=1 ./bin/cc45 -O2 -S x.c`.

**`far_load` / `far_store`, `near_load` / `near_store`.** A far pointer is an
`i32` (a 28-bit address), a near pointer an `i8` (a base-page offset):

```c
unsigned char __far *fp;
unsigned char __near *np;
unsigned char get(void) {
    *fp = 1;
    *np = 2;
    return fp[3] + *np;
}
```

```
fn _get() -> i8 [fastcall] {
@entry:
  %0 = load i32 @_fp ; farnear.c:5
  nop void _
  %2 = const i8 1
  far_store i8 %2, %0 ; farnear.c:5
  %3 = load i8 @_np ; farnear.c:6
  nop void _
  %5 = const i8 2
  near_store i8 %5, %3 ; farnear.c:6
  %6 = load i32 @_fp ; farnear.c:7
  nop void _
  nop void _
  nop void _
  %10 = far_load i8 %6 +3 ; farnear.c:7
  %11 = load i8 @_np ; farnear.c:7
  %12 = near_load i8 %11 ; farnear.c:7
  nop void _
  nop void _
  %15 = add i8 %12, %10 ; farnear.c:7
  nop i8 _
  ret %15 ; farnear.c:7
}
```

`far_store <type> <value>, <address>` / `%d = far_load <type> <address>` take
the 32-bit address in a vreg; ` +3` before the location is a constant byte
offset folded into the access (`fp[3]`: the `nop`s are the address
arithmetic it replaced; IRCodeGen reads it with `[bp],z`, `ldz #3`). The
near forms are the same with the 8-bit offset (`$00,x`). `near_addr`
(`&` of a base-page variable as a near pointer) prints in the generic form.

**`vec_bin`.** A GCC vector `+ - & | ^` of `char` or `int` elements is one
instruction (comparisons, `*`, `/` and other element types are lowered to a
statement per element):

```c
typedef int v4 __attribute__((vector_size(8)));
v4 a, b, c;
void add(void) { c = a + b; }
```

```
fn _add() -> void [fastcall] {
@entry:
  %1 = addr_local %0 ; vec.c:5
  vec_bin @_a, @_b, %1, 0, 2, 4, 0, 1 ; vec.c:5
  %2 = addr_local %0 ; vec.c:5
  call_void @_memcpy(@_c, %2, 8) ; vec.c:5
  ret void
}
```

`vec_bin <A>, <B>, <R>, <op>, <element size>, <count>, <B is a scalar>, <unrolled>`:
the addresses of the operands and of the result (here a temporary, `%1`,
then copied to `c`), the operator (`0` `+`, `1` `-`, `2` `&`, `3` `|`,
`4` `^`), the element size (1 or 2), the number of elements, `1` when `B`
is a scalar value (`a + 1`: `B` is then the value, not an address), and `1`
when the bytes are written out instead of a loop (when `count × size × 8`
fits `-fmax-unroll-bytes`, default 128).

**`trampoline`.** A nested function whose address is taken (GCC nested
functions):

```c
int apply(int (*f)(int), int v) { return f(v); }
int outer(int k) {
    int add(int x) { return x + k; }
    return apply(add, 1);
}
```

```
fn _apply(ptr f, i16 v) -> i16 [fastcall] {
@entry:
  %2 = call_indirect %0(%1) -> i16 ; tramp.c:1
  ret %2 ; tramp.c:1
}

fn _outer(i16 k) -> i16 [fastcall] {
@entry:
  %2 = get_fp ; tramp.c:5
  %3 = trampoline target=@_outer_add, link=%2, buf=%1 ; tramp.c:5
  nop void _
  nop ptr _ ; tramp.c:5
  %6 = const i16 1 ; tramp.c:5
  %8 = call_indirect %3(%6) -> i16 ; tramp.c:1
  nop i16 _ ; tramp.c:1
@inline_end0:
  ret %8 ; tramp.c:5
}

fn _outer_add(i16 x) -> i16 [fastcall] {
@entry:
  %1 = load_zp @__static_chain ; tramp.c:4
  %2 = addr_uplevel levels=1, target=_outer(v0) ; tramp.c:4
  %3 = load i16 %2 ; tramp.c:4
  %4 = add i16 %3, %0 ; tramp.c:4
  ret %4 ; tramp.c:4
}
```

`%d = trampoline target=<function>, link=<static link>, buf=<buffer>`
writes, into the buffer (a local of `outer`, `%1`), 11 bytes of code that
set the static chain (`$06`, `__static_chain`) to the link (`get_fp`:
`outer`'s frame) and jump to the nested function (`lda #<link` / `sta $06` /
`lda #>link` / `sta $07` / `jmp target`); `%d` is the buffer's address, the
pointer passed on. `apply` is inlined (`@inline_end0`), so its call through
the pointer is in `outer`. The nested function finds `k` through the chain:
`addr_uplevel levels=1, target=_outer(v0)` is the address of `outer`'s
vreg 0.

## Function attributes (`ir::Function`)

| Field | Set from | Effect in IRCodeGen |
|-------|----------|---------------------|
| `conv` (`CallConv::STACK` / `ZP`) | `IRBuilder::usesZpConv`: ZP when `-fzpcall` (default) or `__fastcall__`, not variadic, params fit the 32-byte ZP block | parameter passing; printed `[fastcall]` |
| `CallConv::STATIC` | — | declared but never assigned |
| `isVariadic` | `...` | forces stack params; with SAC the entry thunk keeps the address of the `...` arguments in `<f>__va` |
| `isStatic`, `isWeak` | `static`, `#pragma cc45 weak` | symbol export |
| `isInterrupt`, `isNaked`, `isRegparm` | `__interrupt`, `__naked`, `__regparm` | prologue/epilogue, no SAC for interrupt/naked |
| `isRecurse` | `#pragma cc45 recurse` | opts the function out of SAC |
| `isFar`, `bank`, `section` | `__far`, `__bank(n)`, `__section("NAME")` | the text sub-segment the code is emitted in (`far`, `far.n`, `sect.NAME`) |
| `noOptimize`, `optOverrides`, `argLists` | `#pragma cc45 optimize(off)` / `optimize(...)`, `-fzp-args` | no IR pass and `.noopt_start`/`.noopt_end`; per-function pass switches; the SAC entry copies parameters through `__zp_args` / `__sac_args` lists |
| `restrictVregs`, `memoryVregs`, `volatileVregs`, `registerVregs` | `restrict` parameters; volatile / address-taken variables (`volatileVregs`: those declared `volatile`, every read a read, #659); `register` | what the passes may assume, and where IRCodeGen places the vreg |
| `ptrSig`, `isMethod` | the parameter slots (`IRBuilder::ptrSigKey`); a struct method | ln45's call-graph markers (`cc45.fsig.<f>.<sig>`); emitted weak |
| `isNested`, `staticLinkVreg` | GCC nested functions | static chain |
| `align`, `retAddrSlot`, `addressTakenLabels` | `aligned(N)`, `__builtin_return_address`, `&&label` | — |

SAC is not an IR calling convention: IRCodeGen decides it per function
(`staticAllocMode` on, not `isRecurse`/`isInterrupt`/`isNaked`,
no runtime `alloca`, not on a call cycle in the module, no direct self-call).
Variadic functions use SAC too (see
[calling-conventions.md](calling-conventions.md#35-variadic-functions)).

`ir::Module` also carries `globals` (with constant-initializer images,
address space, alignment, `section`, `bank`), `sections` (`#pragma
section`), `strings`, `externs` (with `zpExterns`, `externSizes`),
`pureFunctions`, `volatileGlobals`, `fileAsm` (file-scope `asm`),
`unpackSites` (the pack45 formats the file unpacks, for ln45's
`cc45.unpackfmt.*`, #666), the float `printf` / `scanf` needs
(`floatVarArg`, `floatSprintfArg`, `floatScanfArg`, `callsVscanf`), debug
information (`debugGlobals`, `debugTypes`) and the file's settings: `saveZP`, `setBP`, `stackTop`, `stackSize`, `bssFill`,
`fastFloat`, `overlay`, `maxUnrollBytes`.

## IRBuilder coverage

`IRBuilder` (`include/IRBuilder.hpp`, `src/main/IRBuilder.cpp`; far and near
pointers in `IRBuilderFar.cpp`) overrides all 47 `ASTVisitor::visit` methods: IntegerLiteral, FloatLiteral, StringLiteral,
VariableReference, Assignment, BinaryOperation, UnaryOperation,
ConditionalExpression, GenericSelection, InitializerList, ArrayAccess,
FunctionCall, MemberAccess, CastExpression, CompoundLiteral, AlignofExpression,
SizeofExpression, VariableDeclaration, ReturnStatement, BreakStatement,
ContinueStatement, SwitchContinueStatement, GotoStatement, LabelledStatement,
ExpressionStatement, IfStatement, WhileStatement, DoWhileStatement, ForStatement,
RepeatStatement, SwitchStatement, CaseStatement, DefaultStatement, AsmStatement,
StaticAssert, EnumDefinition, StructDefinition, CompoundStatement,
StatementExpression, FunctionDeclaration, BuiltinVaStart, BuiltinVaArg,
CpuRegisterAccess, CpuFlagAccess, BitSelect, LabelAddressExpression,
TranslationUnit.

## Design notes

### Linear, not SSA

The IR is not SSA. A vreg can be assigned in several blocks — the ternary
above assigns `%11` with `copy` in both arms — and local-variable slots are
vregs that are stored to repeatedly. `phi` exists in the opcode set (and
IRCodeGen can lower it) but IRBuilder never emits it.

### Relationship to assembly

Each IR instruction becomes a short sequence of ca45 instructions and simulated
ops. The IR leaves to IRCodeGen: vreg placement (registers, ZP slots, frame or
SAC storage, base-page blocks), frame setup, calling-convention mechanics (ZP
parameter block vs. stack pushes, SAC thunks), and the float runtime calls.

## Known gaps

1. `--emit-ir` is a no-op; use `CC45_DUMP_IR=1`.
2. The printer has no names for `F32`/`I_N` (`?`) or the float ops (`unknown`),
   and does not print bitfield offset/width or inline-asm text.
3. Vregs are not SSA: a local's slot vreg is assigned by `store`, and a temp
   can be defined on several paths (`a && b`). Passes treat a vreg as a
   constant or fixed value only when it has exactly one definition.
