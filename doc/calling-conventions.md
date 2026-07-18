# MEGA65 C Compiler Calling Conventions

**Version:** 1.0  
**Last Updated:** 2026-07-17

## Overview

The MEGA65 C Compiler (cc45) supports two calling conventions:

1. **Stack Calling Convention** (default) — Parameters on stack, suitable for variable-argument and complex calling patterns
2. **ZP Calling Convention** (`-fzpcall`) — Parameters in fixed zero-page region, faster, suitable for performance-critical code

Both conventions use the same register return values (AXYZ for multi-byte results).

---

## Stack Calling Convention

### Register Usage

| Register | Purpose | Preserved? |
|----------|---------|-----------|
| A | Accumulator, lo-byte of return value | No |
| X | Index, hi-byte of return value (16-bit) or byte 1 (32-bit) | No |
| Y | Index, byte 2 of 32-bit return | No |
| Z | Index, byte 3 of 32-bit return (hi-byte of 32-bit value) | No |
| S | Stack pointer (8-bit, page $01 or above on MEGA65) | Preserved by convention |
| $FD/$FE | Frame pointer (dynamic, set up lazily) | Caller-saved if used |

### Parameter Passing

Parameters are pushed **right-to-left** (rightmost parameter pushed first) onto the stack, 8 bits at a time:

```c
void func(int a, short b, char c) {
    // Stack layout (before JSR):
    // SP+4: a (16-bit, lo-byte at SP+4, hi-byte at SP+5)
    // SP+2: b (16-bit, lo-byte at SP+2, hi-byte at SP+3)
    // SP+0: c (8-bit)
}
```

Caller sequence:
```asm
lda #c_val
pha
lda #b_val_lo
ldx #b_val_hi
phx
pha
lda #a_val_lo
ldx #a_val_hi
phx
pha
jsr func
```

### Return Values

Return values are placed in hardware registers (no stack return):

- **8-bit (`char`)**: A register
- **16-bit (`short`, `int`, pointers)**: A (lo-byte), X (hi-byte)
- **32-bit (`long`)**: A (byte 0), X (byte 1), Y (byte 2), Z (byte 3)

### Stack Frame Layout

Stack frames are allocated **lazily** — the frame pointer ($FD/$FE) is only set up when:

1. A function uses frame-relative variable access, OR
2. A function calls another function that requires a frame pointer

When set up, the frame layout is:

```
[Caller's return address (2 bytes)]
[Stack parameters (N bytes)]
← SP before JSR
← FP after setup (SP + 1, handling page crossing)
[Frame variables (M bytes)]
[Vregs (K bytes)]
← SP during function execution
```

**Frame Setup Sequence:**

```asm
; Push frame space (10 words = 20 bytes in this example)
phw #0
phw #0
phw #0
phw #0
phw #0

; Set up frame pointer with page-boundary handling
tsy           ; Load Y from stack pointer (high byte)
tsx           ; Load X from stack pointer (low byte)
inx           ; Increment X to point to frame start
bne @no_carry ; If X didn't wrap (no carry), skip INY
iny           ; If X wrapped, increment Y (page boundary)
@no_carry:
stx $FD       ; Store FP low-byte
sty $FE       ; Store FP high-byte
.frameptr_zp $FD
```

The frame pointer is now at $FD/$FE and points to the start of the allocated frame.

### Frame-Relative Variable Access

Once the frame pointer is set up, variables are accessed using indirect indexed addressing:

```asm
; Load 16-bit variable at frame offset 4
ldy #4
lda ($FD),y   ; Load low-byte
ldx ($FE),y   ; Load high-byte (note: must handle page crossing)
```

For write operations:

```asm
; Store 16-bit value to frame offset 4
ldy #4
sta ($FD),y   ; Store low-byte
ldx #hi_val
sty ($FE),y   ; Store high-byte
```

### Struct Returns (Stack Convention)

**Important:** Structs are NOT returned in registers. Instead:

1. Caller allocates static storage for the return struct
2. Caller passes the address of this storage as a **hidden implicit parameter**
3. Callee writes the struct to this location
4. Caller copies from static storage to the actual destination if needed

Example:

```c
struct Point { int x, y; };

struct Point make_point(int x, int y) {
    struct Point p;
    p.x = x;
    p.y = y;
    return p;  // Compiled as write to __return_struct_temp, return temp address
}

void caller() {
    struct Point p = make_point(1, 2);
    // Internally:
    // - Static temp location allocated: __return_struct_temp
    // - JSR make_point (with temp address passed)
    // - make_point writes to temp
    // - Caller copies temp to p
}
```

**Rationale:** This avoids corrupting return values during frame pointer setup/manipulation in the caller.

### Caller/Callee Responsibilities

**Caller Must:**
- Push parameters right-to-left
- Preserve S (stack pointer is caller's responsibility to restore)
- Not assume any registers are preserved (except S)
- Handle struct return values (allocate and pass temporary location)

**Callee Must:**
- Set up frame pointer only if needed
- Restore/preserve the stack upon return (via `plz` instructions to pop frame space)
- Return values in A/X/Y/Z as specified
- Clean up any temporary storage

### Return Sequence

```asm
; Pop frame space (opposite of setup)
plz
plz
plz
plz
plz
; ... (one plz per word of frame space)
rts
```

---

## ZP Calling Convention (`-fzpcall`)

### Overview

The ZP calling convention is an alternative for performance-critical code. It uses fixed zero-page locations for parameter passing and return values, avoiding stack manipulation entirely.

**Compiler Flag:** `-fzpcall`

### Register/ZP Locations

Fixed parameter locations in zero-page:

| ZP Address | Purpose | Notes |
|-----------|---------|-------|
| $20-$21 | Parameter 1 (16-bit) | Caller sets; callee reads |
| $22-$23 | Parameter 2 (16-bit) | Caller sets; callee reads |
| $24-$25 | Parameter 3 (16-bit) | Caller sets; callee reads |
| $26-$27 | Parameter 4 (16-bit) | Caller sets; callee reads |
| $28-$2A | Additional parameters | Variable length |

Return values (same as stack convention):
- A, X for 16-bit
- A, X, Y, Z for 32-bit

### Parameter Passing

No stack operations. Caller directly loads parameters into ZP:

```c
int func(short a, short b);
```

Caller sequence:
```asm
lda #a_val_lo
sta $20
lda #a_val_hi
sta $21
lda #b_val_lo
sta $22
lda #b_val_hi
sta $23
jsr func_zpcall
```

### Advantages

1. **No stack overhead** — Parameters never touch the stack
2. **Faster** — Direct ZP addressing (3 cycles) vs. stack push/pop (4+ cycles each)
3. **Smaller code** — No `phx/pha/plx/pla` sequences
4. **Predictable** — No frame pointer setup needed

### Disadvantages

1. **Limited parameters** — Maximum 4 parameters (8 bytes in ZP)
2. **No variadic functions** — Fixed parameter count only
3. **Clobber tracking** — Caller must know which registers are clobbered
4. **Calling convention mixing** — Requires linker support to prevent mismatched calls

### Clobber Tracking

Each ZP-convention function tracks which registers it clobbers:

```asm
.func_flags zp_call
.reg_clobbers A, X, Y
.flag_clobbers C, N, Z
```

The linker verifies that ZP callees are only called from ZP callers (one-directional safety):
- **ZP caller → Stack callee**: ERROR (unsafe; parameters in wrong place)
- **Stack caller → ZP callee**: OK (caller doesn't assume stack layout; adjusts its call)
- **ZP caller → ZP callee**: OK (both use ZP convention)
- **Stack caller → Stack callee**: OK (both use stack convention)

### Struct Returns (ZP Convention)

Same as stack convention: structs use static return location.

---

## Calling Convention Selection

### When to Use Stack Convention

- Default for all code
- Variable-argument functions (`printf`, `va_args`)
- Functions with more than 4 parameters
- Complex calling patterns
- When frame-relative storage is needed

### When to Use ZP Convention

- Performance-critical loops
- Simple functions with 1-4 parameters
- Fixed argument count
- When inlining is not possible but speed is critical

**Compilation:**

```bash
cc45 file.c                  # Stack convention (default)
cc45 -fzpcall file.c         # ZP convention for all functions
```

Per-function control via function attributes (future work):

```c
int __attribute__((zpcall)) fast_add(short a, short b) {
    return a + b;
}
```

---

## Implementation Notes

### Frame Pointer Mechanics

The frame pointer ($FD/$FE) is necessary because the MEGA65 stack is not limited to page $01. It can extend into higher memory, requiring a 16-bit address to track the frame base.

The lazy initialization approach ensures:
- Zero overhead for leaf functions (no frame setup)
- Minimal overhead for simple functions
- Correct behavior for complex stack layouts

### Struct Return Storage

Static allocation for struct returns (`__return_struct_temp`) avoids:
- Return value corruption from frame pointer recalculation
- Complex register juggling for multi-byte structs
- Ambiguity about when/where the struct is written

The cost is one extra memory copy (temp → destination), which is negligible compared to the complexity avoided.

### Zero-Page Preservation

In stack convention, $FD/$FE (frame pointer) is caller-saved:
- Caller must preserve if it needs the frame pointer across nested calls
- Typically handled automatically by the compiler

In ZP convention, $20-$2A are caller-saved (callee clobbers freely).

---

## Future Enhancements

1. **Per-function convention selection** via `__attribute__`
2. **Parameter register convention** — use A/X for first param instead of stack
3. **Tail call optimization** — reuse frame space across tail calls
4. **Variadic ZP functions** — multiple calling points with different arities
5. **Struct optimization** — small structs (≤4 bytes) returned in AXYZ

---

## References

- **CLAUDE.md** — Compiler feature matrix and architecture overview
- **lib45.md** — Object file format and relocation details
- **stdlib.md** — Standard library conventions and signatures
- **Opcodes** — 45GS02 instruction reference for address modes and stack operations

