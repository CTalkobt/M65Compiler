# MEGA65 Calling Conventions — Quick Reference

**v1.0.4** | For detailed information, see `doc/calling-conventions-v1.0.4-actual.md`

---

## Stack Calling Convention (Default)

### Parameter Passing
```c
// Example function
int add(int a, short b, char c);

// Caller pushes: right-to-left (c, then b, then a)
// Stack at JSR: [SP+0=ret_lo, SP+1=ret_hi, SP+2=c, SP+3=b_lo, SP+4=b_hi, SP+5=a_lo, SP+6=a_hi]
```

**Cost:** 8 cycles per parameter (push overhead)

### Return Values
```
8-bit (char):          A
16-bit (short/int):    A (lo), X (hi)
32-bit (long):         A (byte0), X (byte1), Y (byte2), Z (byte3)
struct:                Static temp location (hidden pointer mechanism)
```

### Frame Layout
```
[Return address: 2 bytes]
FP → [Frame space: N bytes]  ← FP set here (SP+1)
     [Locals/vregs: M bytes]
SP → [Next free byte]
```

**Frame setup (unconditional in v1.0.4):**
```asm
tsx; txa; clc; adc #1; sta $FD; lda #$01; adc #0; sta $FE
.frameptr_zp $FD
```

### Key Rules
- ✅ Parameters accessed via frame pointer: `lda.fp @_p_param_name`
- ✅ Locals accessed via frame pointer: `lda.fp @_l_local_name`
- ✅ Frame pointer is **stable** throughout function (never recalculated)
- ✅ Use `plz` to pop frame space before RTS
- ❌ Don't modify frame pointer during execution
- ❌ Don't assume any registers preserved except S (stack pointer)

### Struct Returns
```c
struct Point { int x, y; };
struct Point make_point(int x, int y) { ... }

// Compiled to:
// 1. Caller allocates static __return_struct_temp
// 2. Caller pushes temp address as hidden first parameter
// 3. Caller pushes regular parameters (2, then 1)
// 4. Function writes to temp address
// 5. Caller copies temp to destination
```

### Varargs Support
```c
int printf(const char *fmt, ...);    // ✅ Supported
int custom(int count, ...) {         // ✅ Supported
    va_list args;
    va_start(args, count);
    while (count--) {
        int val = va_arg(args, int); // Read 2 bytes, advance
    }
}
```

---

## ZP Calling Convention (`-fzpcall`)

### Parameter Passing
```asm
; Fixed locations in zero-page:
; $20-$21:  Param 1 (16-bit)
; $22-$23:  Param 2 (16-bit)
; $24-$25:  Param 3 (16-bit)
; $26-$27:  Param 4 (16-bit)

lda #<a_val
sta $20              ; Param 1 lo
lda #>a_val
sta $21              ; Param 1 hi
jsr func_zp          ; Call
```

**Cost:** 3 cycles per parameter (direct ZP access)

### Return Values
Same as stack convention: AXYZ registers

### Key Rules
- ✅ No stack operations
- ✅ No frame pointer
- ✅ 4 parameters maximum
- ✅ Faster than stack convention
- ❌ No varargs support
- ❌ ZP parameters clobbered by callee (caller-saved)

### Calling Convention Safety
```
ZP caller → Stack callee:  ❌ ERROR (params in wrong place)
Stack caller → ZP callee:  ✅ OK (caller doesn't assume layout)
ZP caller → ZP callee:     ✅ OK
Stack caller → Stack:      ✅ OK
```

---

## Frame Variables: Offset Calculation

### Layout Order (From Frame Start)
```
1. Hidden struct return pointer (if returning struct) — 2 bytes
2. Spilled parameters (if any)                        — variable
3. Local variables                                    — variable
4. Vregs (temporary variables)                        — variable
```

### Example
```c
int func(int x, short y) {
    int a = 10;
    short b = 20;
    int *p;
    // Frame layout:
    // offset 0-1: x (spilled if in ZP, otherwise not allocated)
    // offset 0-1: y (spilled if in ZP, otherwise not allocated)
    // offset N-N+1: a
    // offset N+2-N+3: b
    // offset M: p
    // offset M+?: vregs
}
```

### Accessing Frame Variables in Assembly
```asm
; Load 16-bit local at frame offset 4
ldy #4
lda ($FD),y
ldx ($FE),y          ; High byte uses ($FE),Y

; Store 16-bit value to frame offset 6
ldy #6
sta ($FD),y
stx ($FE),y

; Get address of frame local (for pointer)
leax.fp @_l_var_name ; Result in A:X
```

---

## Register Clobbering

### Stack Convention
```
Caller Must Preserve:  S (stack pointer, implicit)
Caller Can't Assume:   A, X, Y, Z (all clobbered)
Always Reserved:       $FD/$FE (frame pointer)
```

### ZP Convention
```
Caller Must Preserve:  S (stack pointer, implicit)
Caller Can't Assume:   A, X, Y, Z (all clobbered)
ZP Params Clobbered:   $20-$2A (caller-saved)
Callee-Declared:       .reg_clobbers, .flag_clobbers in .o45
```

---

## Page Boundary Crossing

Frame pointer handles automatic page crossing:

```asm
tsx                  ; X ← SPL
txa                  ; A ← SPL
clc
adc #1               ; A ← SPL + 1 (wraps to $00 if SPL=$FF)
sta $FD              ; FP_lo ← SPL + 1
lda #$01             ; Page $01 for stack
adc #0               ; Add carry if wrapped
sta $FE              ; FP_hi ← $01 + carry
```

Example: SPL=$FF → FP=$0200 (correct boundary handling)

---

## Stack Space Calculation

### Frame Size
```
Frame = phw_count * 2 + locals_size + vregs_size

// Example: 10 phw instructions = 20 bytes
// Plus 4 bytes locals + 6 bytes vregs = 30 bytes frame
```

### Cleanup Before Return
```asm
; For each 2-byte word allocated (phw):
plz
plz
plz
...
rts
```

---

## Common Patterns

### Getting Address of Stack Parameter
```c
int *ptr_to_param = &param;
```

Compiles to:
```asm
leax.fp @_p_param    ; A:X ← address of parameter
stax @_l_ptr_to_param
```

### Calling Function with Struct Result
```c
struct Point p = make_point(1, 2);
```

Compiles to:
```asm
leax.fp @__return_struct_temp    ; Temp address
phx                              ; Push hi-byte (hidden)
pha                              ; Push lo-byte (hidden)
lda #2                           ; Regular param y
ldx #0
phx
pha
lda #1                           ; Regular param x
ldx #0
phx
pha
jsr make_point
; Copy temp to p (if not already there)
```

### Zero-Argument Function Call
```c
int value = get_value();
```

Compiles to:
```asm
jsr get_value        ; Returns in A:X
stax @_l_value
```

---

## Compilation Flags

| Flag | Convention | Parameters | Varargs | Speed |
|------|-----------|-----------|---------|-------|
| (default) | Stack | On stack | Yes | Medium |
| `-fzpcall` | ZP | Fixed ZP | No | Fast |

### Per-File Selection
```bash
cc45 -fzpcall file.c      # Entire file uses ZP convention
cc45 file.c               # Entire file uses stack (default)
```

### Per-Function Selection (v1.1+, Planned)
```c
int __attribute__((zpcall)) fast_func(int a, int b) {
    return a + b;
}
```

---

## Debugging Tips

### Check Parameter Access
```asm
; If parameter access is wrong, verify:
; 1. Frame pointer set correctly (check FP in $FD/$FE)
; 2. Parameter offset correct (match .local directives)
; 3. Frame pointer not modified (should be stable)
```

### Disassemble for Verification
```bash
cc45 -S input.c -o input.s45     # Generate assembly
ca45 input.s45 -o output.prg     # Assemble
objdump45 -d output.prg          # Disassemble to verify
```

### Enable Debug Output
```bash
cc45 -g input.c                  # Includes .loc directives
ca45 -c input.s45 -o output.o45  # Generate object file
nm45 output.o45                  # Inspect symbols
```

---

## Performance Notes

### Stack Convention Overhead
- Frame pointer setup: 11 bytes, ~20 cycles
- Parameter push: 4 cycles per parameter
- Return value handling: 1-3 cycles
- **Total for 3-param function:** ~35-45 cycles + parameter size

### ZP Convention Overhead
- Parameter load to ZP: 3 cycles per parameter
- No stack operations
- **Total for 3-param function:** ~9 cycles

### Optimization Opportunities (v1.1+)
- ✨ Lazy FP init for leaf functions: ~20 cycles saved
- ✨ Skip FP recalc for 0-arg calls: ~10 cycles saved per call
- ✨ Small struct optimization (≤4 bytes): Return in AXYZ directly

---

## See Also

- **Full Reference:** `doc/calling-conventions-v1.0.4-actual.md`
- **Corrections:** `doc/v1.0.5-calling-conventions-corrections.md`
- **Optimizer Guide:** `doc/optimizer.md`
- **Object Format:** `doc/lib45.md`
- **Standard Library:** `doc/stdlib.md`

