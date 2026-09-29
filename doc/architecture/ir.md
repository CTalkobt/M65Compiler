# cc45 Intermediate Representation (IR)

## Overview

The cc45 IR is a **linear three-address code** with basic blocks and virtual registers,
designed for the 45GS02 target. It sits between the AST and assembly emission:

```
C Source → Lexer → Parser → AST → [ConstantFolder] → IRBuilder → IR → IRCodeGen → Assembly
```

The `IRBuilder` (AST → IR) and `IRCodeGen` (IR → assembly) are fully implemented and integrated into `cc45`. The IR pipeline handles all C language constructs, including floating-point operations. Use `cc45 --emit-ir` to generate a `.ir` dump alongside normal assembly output.

## Type System

| IR Type | C Type(s)          | Width | Register Mapping / Storage |
|---------|-------------------|-------|---------------------------|
| `i8`    | `char`            | 1     | A                         |
| `i16`   | `int`, `short`, pointers | 2 | A:X                    |
| `i32`   | `long`            | 4     | A:X:Y:Z (Q)              |
| `F32`   | `float`, `double` | 5     | ZP / MFLUT 40-bit format  |
| `void`  | `void`            | 0     | —                         |

Pointers are `i16` (45GS02 has a 16-bit address space).

## Text Format

### Module

```
; source: main.c

global @_count : i16
global @_buffer : i8[256]

func @_main() -> i16 { ... }
func @_add(i16, i16) -> i16 { ... }
```

### Functions

```
func @_add(i16 %x, i16 %y) -> i16 conv=stack {
entry:
  %2 = add i16 %0, %1
  ret %2
}
```

- `@_name`: Global symbol (mangled with underscore prefix)
- `%N`: Virtual register
- `conv=stack` or `conv=zp`: Calling convention (default: stack)
- Variadic functions: `func @_printf(i16 %fmt, ...) -> i16`

### Instructions

#### Constants

```
%0 = const i16 42
%1 = const i8 65
%2 = const i32 100000
```

#### Arithmetic

```
%3 = add i16 %0, %1
%4 = sub i16 %0, %1
%5 = mul i16 %0, %1
%6 = div i16 %0, %1
%7 = mod i16 %0, %1
%8 = neg i16 %0
```

#### Bitwise

```
%3 = and i16 %0, %1
%4 = or  i16 %0, %1
%5 = xor i16 %0, %1
%6 = not i16 %0
%7 = shl i16 %0, %1
%8 = shr i16 %0, %1       ; logical (unsigned)
%9 = asr i16 %0, %1       ; arithmetic (signed)
```

#### Comparison

Returns `i8` (0 or 1):

```
%2 = cmp_eq  i16 %0, %1
%3 = cmp_lt  i16 %0, %1   ; signed
%4 = cmp_ltu i16 %0, %1   ; unsigned
```

Full set: `cmp_eq`, `cmp_ne`, `cmp_lt`, `cmp_le`, `cmp_gt`, `cmp_ge`,
`cmp_ltu`, `cmp_leu`, `cmp_gtu`, `cmp_geu`.

#### Memory

```
%1 = load i16 %0             ; load 2 bytes from address in %0
store i16 %1, %0              ; store 2 bytes to address in %0

%2 = load_zp i8 $03           ; load from zero-page address
store_zp i8 %1, $03           ; store to zero-page address
```

#### Address Computation

```
%0 = addr_global @_count          ; address of global variable
%1 = addr_local 4                  ; address of local at frame offset 4
%2 = addr_elem %base, %index, 2   ; base + index * elemSize
```

#### Type Conversion

```
%1 = sext i8 %0 to i16        ; sign-extend char → int
%2 = zext i8 %0 to i16        ; zero-extend unsigned char → int
%3 = trunc i16 %0 to i8       ; truncate int → char
%4 = sext i16 %0 to i32       ; sign-extend int → long
```

#### Bitfield Operations

```
%2 = bfext i16 %1, 4, 4       ; extract 4 bits at bit offset 4
bfins i16 %0, %addr, 0, 4     ; insert %0 into *%addr at bit offset 0, width 4
```

Bitfield layout follows C packing rules: consecutive same-type bitfields share
a storage unit. A new unit starts on type change or overflow.

#### Control Flow

```
  br .loop_top                           ; unconditional jump
  br_cond %flag, .then, .else           ; conditional branch
  switch %val, .default, [0: .case0, 1: .case1, 5: .case5]
  ret %0                                ; return value
  ret void                              ; void return
```

Break and continue within loops emit `br` to the appropriate loop-end or
loop-continue label. Switch/case emits inline `cmp_eq` + `br_cond` per case.

#### Function Calls

```
%2 = call @_add(%0, %1) -> i16          ; direct call with return
call_void @_puts(%0)                     ; void call
%3 = call_indirect %fptr(%0) -> i16      ; indirect call via function pointer
```

#### Variadic Functions

```
; va_start: compute address past last named param, store to ap
%1 = add i16 %last_param, 2
store i16 %1, %ap

; va_arg: load from *ap, advance ap
%2 = load i16 %ap           ; current ap pointer
%3 = load i16 %2            ; argument value
%4 = add i16 %2, 2          ; advance past argument
store i16 %4, %ap           ; update ap
```

#### Ternary (Conditional Expression)

```
  br_cond %cond, .tern_then, .tern_else
tern_then:
  ...                         ; evaluate then-expression
  br .tern_end
tern_else:
  ...                         ; evaluate else-expression
  br .tern_end
tern_end:
  %r = phi [%thenVal, .tern_then], [%elseVal, .tern_else]
```

#### Special

```
asm "sei"                                ; inline assembly
%3 = phi [%1, .then], [%2, .else]       ; SSA phi node
nop                                      ; no operation
```

### Source Locations

Instructions carry source location annotations as comments:

```
  %0 = const i16 42                     ; main.c:10
  %1 = call @_foo(%0) -> i16            ; main.c:11
```

## IRBuilder Coverage

The `IRBuilder` (`include/IRBuilder.hpp`, `src/main/IRBuilder.cpp`) implements
all 42 `ASTVisitor` methods:

| Category | Nodes | Notes |
|----------|-------|-------|
| **Expressions** | IntegerLiteral, StringLiteral, VariableReference, BinaryOperation, UnaryOperation, Assignment, CastExpression, FunctionCall, ConditionalExpression, SizeofExpression, AlignofExpression | Full arithmetic, comparison, bitwise, shift, logical ops |
| **Struct/Union** | StructDefinition, MemberAccess | Layout with alignment, bitfield packing, `.` and `->` access, BFEXT/BFINS |
| **Arrays** | ArrayAccess | ADDR_ELEM with stride computation + LOAD |
| **Control flow** | IfStatement, WhileStatement, DoWhileStatement, ForStatement, SwitchStatement, CaseStatement, DefaultStatement, BreakStatement, ContinueStatement, GotoStatement, LabelledStatement | Proper basic blocks, loop label stack for break/continue |
| **Functions** | FunctionDeclaration, ReturnStatement, FunctionCall (direct + indirect) | Params, return types, calling convention, CALL_INDIRECT for function pointers |
| **Variadic** | BuiltinVaStart, BuiltinVaArg | Address computation + load/advance pattern |
| **Compound** | CompoundLiteral, CompoundStatement, InitializerList | Scalar passthrough or frame-allocated aggregate |
| **Type-level** | EnumDefinition, StructDefinition, StaticAssert | Enum/static_assert are compile-time only; struct builds IRStructInfo |
| **Misc** | ExpressionStatement, AsmStatement, GenericSelection, SwitchContinueStatement, TranslationUnit | ASM_INLINE for inline asm |

## Design Rationale

### Why Virtual Registers?

The 45GS02 has only 4 asymmetric hardware registers (A, X, Y, Z). Virtual registers
decouple the IR from physical register constraints, enabling:

- Register allocation as a separate pass
- Liveness analysis across basic blocks
- Treating ZP slots as register-class candidates

### Why Linear (not SSA)?

SSA form is powerful but complex. For a 16-bit retro target, a linear three-address
code provides most benefits (separate optimization passes, testability, debug info
propagation) without the complexity of phi-node insertion and dominance frontiers.
PHI nodes are included in the opcode set for future use if SSA is adopted.

### Relationship to Assembly

Each IR instruction maps roughly to 1-5 assembly instructions depending on
addressing mode and operand types. The IR abstracts away:

- Stack frame management (TSX/spBase offsets)
- Register spilling to ZP scratch
- Simulated ops (add.16, ldax.fp, etc.)
- Calling convention mechanics (push/pop vs ZP store)

## Future Work

1. **IR Optimization Passes**: Advanced CSE, DCE, and peephole optimizations operating directly on IR
2. **Register Allocator Enhancements**: Advanced vReg → {A,X,Y,Z} + ZP slot assignment
3. **Debug Info**: Extended IR source locations → line map emission
