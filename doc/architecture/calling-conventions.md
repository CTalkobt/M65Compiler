# MEGA65 C Compiler Calling Conventions

**Version:** 1.1  
**Last Updated:** 2026-07-17  
**Status:** Documented current implementation + optimization roadmap for v1.1+

## Overview

The MEGA65 C Compiler (cc45) supports two calling conventions:

1. **Stack Calling Convention** (default) — Parameters on stack, suitable for variable-argument and complex calling patterns. Supports varargs (`printf`, `va_args`).
2. **ZP Calling Convention** (`-fzpcall`) — Parameters in fixed zero-page region, faster, suitable for performance-critical code. Fixed parameter count only; no varargs support.

Both conventions use the same register return values (AXYZ for multi-byte results).

---

## Stack Calling Convention

### Register Usage

| Register | Purpose | Preserved? |
|----------|---------|-----------|
| A | Accumulator, lo-byte of return value or byte 0 (32-bit) | No |
| X | Index, hi-byte of return value (16-bit) or byte 1 (32-bit) | No |
| Y | Index, byte 2 of 32-bit return | No |
| Z | Index, byte 3 of 32-bit return (hi-byte of 32-bit value) | No |
| S | Stack pointer (8-bit, page $01 or above on MEGA65) | Preserved by convention |
| $FD/$FE | Frame pointer (dynamic, set up at function entry) | Caller-saved; reserved even if not used* |

*Note: Frame pointer setup is currently unconditional (planned optimization: lazy initialization only when needed)

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

Stack frames are allocated, and the frame pointer ($FD/$FE) is set up at function entry.

**Current Implementation:** Frame pointer is always set up for all stack-convention functions.  
**Planned Optimization (v1.1):** Lazy initialization — only set up when:
1. A function uses frame-relative variable access, OR
2. A function calls another function that requires a frame pointer

The frame layout is:

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

### Frame Pointer Recalculation After Function Calls

After each function call (JSR), the frame pointer must be recalculated because:
- Stack pointer may have changed if arguments were pushed and cleaned up
- FP must continue pointing to the frame start for subsequent frame-relative access

**Current Implementation:**
```asm
jsr _called_function
; Caller cleans up pushed arguments (if any)
; Then recalculate FP:
taz            ; Save A (return value) to Z
tsx
txa
clc
adc #1
sta $FD
lda #$01       ; High byte
adc #0
sta $FE
tza            ; Restore A
```

**Planned Optimization (v1.1):** Skip FP recalculation if no arguments were pushed (zero-argument calls, common in recursion and callbacks). Saves ~11 bytes and ~20 cycles per call.

**Future Enhancement (v1.2):** Leaf function detection — skip FP recalculation entirely for functions that make no outgoing calls.

### Variadic Functions (Varargs)

**Stack convention supports variadic functions.** Varargs are passed on the stack after fixed parameters.

#### Declaration and Usage

```c
#include <stdarg.h>

int printf(const char *fmt, ...);  // fmt is fixed parameter, rest are varargs

int custom_sum(int count, ...) {
    va_list args;
    va_start(args, count);     // Start after count
    int sum = 0;
    for (int i = 0; i < count; i++) {
        sum += va_arg(args, int);  // Read next int from stack
    }
    va_end(args);
    return sum;
}

// Caller:
int result = custom_sum(3, 10, 20, 30);  // 3 fixed args, then varargs
```

#### Stack Layout for Varargs

For `custom_sum(3, 10, 20, 30)`:

```
Stack (from caller's perspective):
SP+0:  30 (lo-byte, last vararg)
SP+1:  00 (hi-byte)
SP+2:  20 (lo-byte, 2nd vararg)
SP+3:  00 (hi-byte)
SP+4:  10 (lo-byte, 1st vararg)
SP+5:  00 (hi-byte)
SP+6:  03 (lo-byte, count parameter)
SP+7:  00 (hi-byte)
```

The fixed parameter (`count`) is at the lowest stack offset. Variadic arguments follow above it (higher stack addresses).

#### How va_start/va_arg Work

```c
va_list args;           // Type defined in <stdarg.h>, typically uint16_t (pointer to stack)
va_start(args, count);  // Initialize args to point to first vararg (above count parameter)
int val = va_arg(args, int);  // Read 16-bit int, advance args by 2
va_end(args);           // Cleanup (typically no-op for stack varargs)
```

**Implementation Details:**
- `va_start(args, last_fixed)` computes the address of the first vararg (stack offset of `last_fixed` + size of `last_fixed`)
- `va_arg(args, type)` reads the value at the current args pointer and advances it by `sizeof(type)`
- Works with frame-relative addressing: args pointer computed relative to frame or stack

#### Varargs in Variadic Functions

When a variadic function is called, the compiler:
1. Pushes all arguments (fixed + varargs) onto stack in the usual way
2. Callee uses `va_start` to initialize the varargs list to point past the last fixed parameter
3. Callee uses `va_arg` to read each vararg in sequence

Example assembly for `custom_sum(3, 10, 20, 30)`:

```asm
; Caller pushes: 30, 20, 10, 3 (right-to-left)
lda #30
ldx #0
phx
pha
lda #20
ldx #0
phx
pha
lda #10
ldx #0
phx
pha
lda #3
ldx #0
phx
pha
jsr custom_sum

; Callee: va_start(&args, count)
; Compute: args = SP + offset_of_count + sizeof(count)
; So: args points to first vararg (the 10)
ldax.fp @_p_count     ; Load count parameter address
leax                  ; Get address after count
stax @_l_args         ; Save to args variable
```

#### Limitations

- **Stack convention only** — Varargs require stack to work (parameter list on stack)
- **ZP convention incompatible** — ZP uses fixed parameter locations; no varargs support
- **Type information** — Caller responsible for correct types; `va_arg(args, int)` vs `va_arg(args, short)` changes how many bytes are read
- **Format string parsing** — Functions like `printf` infer types from format string; custom varargs functions must track arity explicitly

#### Common Varargs Functions

Stack convention stdlib functions supporting varargs:

- `printf(const char *fmt, ...)` — formatted output
- `sprintf(char *str, const char *fmt, ...)` — format to string
- `snprintf(char *str, size_t n, const char *fmt, ...)` — bounded format to string
- `sscanf(const char *str, const char *fmt, ...)` — formatted input from string
- Custom functions using `<stdarg.h>` macros

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
2. **No variadic functions** — Fixed parameter count only (cannot use `...`); see [Variadic Functions](#variadic-functions-varargs) section for stack-convention varargs support
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

## Implementation Roadmap

### v1.1 — Calling Convention Optimizations (Planned)

**Priority 1: Lazy Frame Pointer Initialization**
- Only set up FP if function uses frame variables or calls frame-using functions
- Benefits: Eliminates 11 bytes + ~20 cycles overhead per leaf function
- Detection: Scan function body for frame-relative access during FrameScanner pass
- Implementation: Conditional FP setup at function entry based on analysis

**Priority 2: Smart Frame Pointer Recalculation**
- Skip FP recalculation after calls with zero arguments
- Benefits: Saves ~10 cycles per zero-argument call (significant in recursion, callbacks)
- Implementation: Check `argBytes > 0` before emitting setupFramePointer after JSR
- Common case: Recursive functions, array iteration callbacks

**Priority 3: Leaf Function Detection**
- Detect functions that make no outgoing calls
- Skip FP recalculation entirely for leaf functions
- Benefits: Zero FP-recalc overhead in leaf functions
- Implementation: Build call graph, mark leaf functions during analysis

### v1.2 — Advanced Calling Convention Features (Planned)

**Priority 4: Hybrid Stack-Register Convention**
- Use A/X/Y/Z/Q registers for first N parameters, stack for remainder
- Benefits: Faster parameter passing, smaller code, fewer stack operations
- Example: `int add(int a, short b, char c, int d)` → first 3 params in regs/ZP, `d` on stack
- Requirements:
  - New calling convention marker for linker
  - Per-function parameter layout specification
  - Linker enforcement to prevent mismatched calls

**Priority 5: Per-Function Frame Pointer Control**
- `__attribute__((no_frame_pointer))` — opt out of FP setup entirely
- `__attribute__((zpcall))` — use ZP convention for single function
- `__attribute__((regparm(N)))` — hint first N params go in registers
- Benefits: Fine-grained control for performance-critical functions

**Priority 6: Tail Call Optimization**
- Detect tail calls (function call as final statement)
- Reuse frame space instead of allocating new frame
- Benefits: Eliminates frame setup/teardown for tail-recursive functions
- Example: `return factorial_helper(n-1, acc * n)` → no new frame

### Future Research

**Variadic ZP Functions**
- Extend ZP convention to support varargs (e.g., `printf` in ZP mode)
- Challenge: Fixed ZP layout incompatible with variable-length argument lists
- Possible solution: Use stack fallback for varargs portion, ZP for fixed params
- Status: Research required; lower priority than stack convention optimizations

**Struct Return Optimization**
- Small structs (≤4 bytes) returned in AXYZ instead of static allocation
- Benefits: Fewer memory accesses, better for inline functions
- Requirements: Track struct sizes, handle unaligned field packing
- Current implementation uses static allocation (safe, predictable)

**Call Frame Reuse**
- Optimize sequences of function calls with different frame sizes
- Reuse frame space across multiple calls instead of repeated setup/teardown
- Benefits: Reduced stack pressure, better cache locality
- Challenge: Complex analysis to ensure correctness

### Optimization Impact Summary

| Optimization | Savings | Complexity | Priority |
|--------------|---------|-----------|----------|
| Lazy FP init | 11 bytes/20 cycles per leaf | Low | P1 |
| Skip FP recalc (0-arg) | 10 cycles per call | Low | P2 |
| Leaf detection | 20+ cycles per call | Medium | P3 |
| Hybrid convention | 4-8 bytes per call, 5-10 cycles | High | P4 |
| Per-function attrs | Variable | Low | P5 |
| Tail call opt | 30+ bytes per tail call | High | P6 |

## Known Issues / Limitations

1. **Frame-Relative Pointer Dereferencing** — Currently failing mmemu tests (test_short.c)
   - Pointer to frame-relative variable may compute wrong address
   - Needs investigation and fix before optimizations proceed

2. **Struct Return Values** — test_struct_return.c returns all zeros
   - Hidden pointer mechanism may have address computation issue
   - Verify frame offset calculation in leax.fp and struct assignment

3. **Page Boundary Crossing** — Potential issue in frame pointer arithmetic
   - TSX/INX/BNE/INY sequence may not correctly handle page wrapping
   - Needs verification with edge case testing (frame crossing page boundary)

4. **ZP Scratch Space Limitations** — $FD/$FE always reserved
   - Limits available ZP for high-speed code
   - Lazy FP init will partially address this

---

## References

- **CLAUDE.md** — Compiler feature matrix and architecture overview
- **lib45.md** — Object file format and relocation details
- **stdlib.md** — Standard library conventions and signatures
- **Opcodes** — 45GS02 instruction reference for address modes and stack operations
- **.plan/todo.md** — Development roadmap and optimization tracking

## Implementation Status

**Current Version (v1.0.4):**
- ✅ Stack calling convention (non-lazy FP, recalc after every call)
- ✅ ZP calling convention (fixed parameters)
- ✅ Struct returns via hidden pointer
- ✅ Variadic function support
- ❌ Lazy FP initialization (planned v1.1)
- ❌ Smart FP recalculation optimization (planned v1.1)
- ❌ Hybrid register-stack convention (planned v1.2)

**Known Issues:**
- Frame-relative pointer dereferencing failing in mmemu tests
- Struct return value corruption in some scenarios
- Page boundary crossing in FP arithmetic (needs verification)

