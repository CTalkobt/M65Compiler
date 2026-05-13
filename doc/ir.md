# cc45 Intermediate Representation (IR)

## Overview

The cc45 IR is a **linear three-address code** with basic blocks and virtual registers,
designed for the 45GS02 target. It sits between the AST and assembly emission:

```
C Source → Lexer → Parser → AST → [ConstantFolder] → IRBuilder → IR → IRCodeGen → Assembly
```

Currently only the IR data structures are defined (`include/IR.hpp`). The `IRBuilder`
(AST → IR) and `IRCodeGen` (IR → assembly) are future work.

## Type System

| IR Type | C Type(s)          | Width | Register Mapping |
|---------|-------------------|-------|-----------------|
| `i8`    | `char`            | 1     | A               |
| `i16`   | `int`, `short`, pointers | 2 | A:X          |
| `i32`   | `long`            | 4     | A:X:Y:Z (Q)    |
| `void`  | `void`            | 0     | —               |

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
func @_add(i16 %0, i16 %1) -> i16 conv=stack {
entry:
  %2 = add i16 %0, %1
  ret %2
}
```

- `@_name`: Global symbol (mangled with underscore prefix)
- `%N`: Virtual register
- `conv=stack` or `conv=zp`: Calling convention (default: stack)

### Instructions

#### Constants

```
%0 = const i16 42
%1 = const i8 'A'
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

#### Control Flow

```
  br .loop_top                           ; unconditional jump
  br_cond %flag, .then, .else           ; conditional branch
  switch %val, .default, [0: .case0, 1: .case1, 5: .case5]
  ret %0                                ; return value
  ret void                              ; void return
```

#### Function Calls

```
%2 = call @_add(%0, %1) -> i16          ; direct call with return
call_void @_puts(%0)                     ; void call
%3 = call_indirect %fptr(%0) -> i16      ; indirect call via function pointer
```

#### Special

```
asm "sei"                                ; inline assembly
%3 = phi [%1, .then], [%2, .else]       ; SSA phi node (future use)
```

### Source Locations

Instructions may carry source location annotations:

```
  %0 = const i16 42                     ; main.c:10
  %1 = call @_foo(%0) -> i16            ; main.c:11
```

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

1. **IRBuilder**: AST → IR visitor (shadow mode alongside CodeGenerator)
2. **IRCodeGen**: IR → assembly emitter (replaces CodeGenerator)
3. **IR Optimizer**: Passes operating on IR (CSE, DCE, constant propagation)
4. **Register Allocator**: vReg → {A,X,Y,Z} + ZP slot assignment
5. **Debug Info**: IR source locations → line map emission
