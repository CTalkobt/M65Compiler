# Phase 28.1: Assembly Overload Mangling Scheme Documentation

**Status**: Documentation  
**Target Version**: v1.0.18+  
**Builds On**: Phase 28 Standard Library Overloads  
**Audience**: Assembly language programmers using overloaded standard library functions

## Overview

Phase 28 introduces parameter-based function overloading to the standard library. Assembly programmers calling these overloaded functions must use **mangled names** that encode the parameter types. This document provides the complete reference for name mangling and calling conventions.

---

## Type Code Reference

### Fundamental Types

| Type | Code | Size | Notes |
|------|------|------|-------|
| `void` | `v` | — | Used only in pointer context (pv) |
| `_Bool` / `int` | `i` | 2 bytes | Default integer type |
| `char` | `c` | 1 byte | Character type |
| `short` | `s` | 2 bytes | Short integer |
| `long` | `l` | 4 bytes | Long integer |
| `long long` | `j` | 8 bytes | Extended integer |
| `float` | `f` | 5 bytes | CBM 40-bit float |
| `double` | `d` | 5 bytes | CBM 40-bit double |
| `size_t` | `z` | 2 bytes | Unsigned integer for sizes |
| `ptrdiff_t` | `t` | 2 bytes | Signed integer for differences |

### Pointer Types

| Type | Code | Size | Notes |
|------|------|------|-------|
| `void*` | `pv` | 2 bytes | Local void pointer (BANK0) |
| `char*` | `pc` | 2 bytes | Local char pointer (BANK0) |
| `int*` | `pi` | 2 bytes | Local int pointer (BANK0) |
| `const void*` | `rpv` | 2 bytes | Const local void pointer |
| `const char*` | `rpc` | 2 bytes | Const local char pointer |
| `far_ptr_t` | `fp` | 3 bytes | Far pointer (24-bit: bank + offset) |
| `const far_ptr_t` | `gp` | 3 bytes | Const far pointer |

### Special Cases

| Type | Code | Context |
|------|------|---------|
| Return type only | Prefix function name | `strlen` vs `strlen_` |
| No parameters | Empty suffix | `strlen__fp` |
| Variadic | Not overloaded | Use base name only |

---

## Mangling Algorithm

### Format

```
<base_name> '__' <param1_code> <param2_code> ... <paramN_code>
```

### Rules

1. **Base name**: Function name as written (e.g., `memcpy`, `strlen`)
2. **Separator**: Double underscore `__`
3. **Parameters**: Type codes concatenated in order
4. **Return type**: NOT encoded (not needed for linker resolution)

### Examples

```
memcpy(void*, void*, size_t)
  → memcpy__pvpvz

memcpy(far_ptr_t, void*, size_t)
  → memcpy__fpvz

strlen(const char*)
  → strlen__rpc

strlen(far_ptr_t)
  → strlen__fp

strcpy(far_ptr_t, const char*)
  → strcpy__fprpc
```

---

## Standard Library Functions (Overload Reference)

### memcpy() — Copy Memory Block

```
memcpy(void* dst, const void* src, size_t len)
  → memcpy__pvpvz

memcpy(void* dst, far_ptr_t src, size_t len)
  → memcpy__fpvz

memcpy(far_ptr_t dst, const void* src, size_t len)
  → memcpy__pvfpz

memcpy(far_ptr_t dst, far_ptr_t src, size_t len)
  → memcpy__fpfpz
```

| Variant | Source | Dest | Notes |
|---------|--------|------|-------|
| `__pvpvz` | Local | Local | Standard C memcpy |
| `__fpvz` | Far | Local | Load from far memory |
| `__pvfpz` | Local | Far | Store to far memory |
| `__fpfpz` | Far | Far | Cross-bank or same-bank transfer (uses DMA if available) |

### memset() — Fill Memory Block

```
memset(void* ptr, int val, size_t len)
  → memset__pvist

memset(far_ptr_t ptr, int val, size_t len)
  → memset__fpist
```

| Variant | Target | Notes |
|---------|--------|-------|
| `__pvist` | Local | Standard C memset |
| `__fpist` | Far | Fill far memory block |

### strlen() — String Length

```
strlen(const char* str)
  → strlen__rpc

strlen(far_ptr_t str)
  → strlen__fp
```

| Variant | Source | Notes |
|---------|--------|-------|
| `__rpc` | Local | Standard C strlen |
| `__fp` | Far | Length of null-terminated far string |

### strcpy() — String Copy

```
strcpy(char* dst, const char* src)
  → strcpy__pcrpc

strcpy(char* dst, far_ptr_t src)
  → strcpy__pcfp

strcpy(far_ptr_t dst, const char* src)
  → strcpy__fprpc

strcpy(far_ptr_t dst, far_ptr_t src)
  → strcpy__fpfp
```

| Variant | Source | Dest | Notes |
|---------|--------|------|-------|
| `__pcrpc` | Local | Local | Standard C strcpy |
| `__pcfp` | Far | Local | Load from far memory |
| `__fprpc` | Local | Far | Store to far memory |
| `__fpfp` | Far | Far | Copy between far pointers |

### strcmp() — String Comparison

```
strcmp(const char* str1, const char* str2)
  → strcmp__rpcrpc

strcmp(far_ptr_t str1, const char* str2)
  → strcmp__fprpc

strcmp(const char* str1, far_ptr_t str2)
  → strcmp__rpcfp
```

| Variant | Str1 | Str2 | Notes |
|---------|------|------|-------|
| `__rpcrpc` | Local | Local | Standard C strcmp |
| `__fprpc` | Far | Local | Far string vs local |
| `__rpcfp` | Local | Far | Local vs far string |

### strcat() — String Concatenation

```
strcat(char* dst, const char* src)
  → strcat__pcrpc

strcat(char* dst, far_ptr_t src)
  → strcat__pcfp
```

| Variant | Source | Dest | Notes |
|---------|--------|------|-------|
| `__pcrpc` | Local | Local | Standard C strcat |
| `__pcfp` | Far | Local | Concatenate far string to local |

### memcmp() — Memory Comparison

```
memcmp(const void* ptr1, const void* ptr2, size_t len)
  → memcmp__pvpvz

memcmp(far_ptr_t ptr1, const void* ptr2, size_t len)
  → memcmp__fpvz

memcmp(const void* ptr1, far_ptr_t ptr2, size_t len)
  → memcmp__pvfpz
```

| Variant | Ptr1 | Ptr2 | Notes |
|---------|------|------|-------|
| `__pvpvz` | Local | Local | Standard C memcmp |
| `__fpvz` | Far | Local | Far memory vs local |
| `__pvfpz` | Local | Far | Local vs far memory |

---

## Assembly Calling Conventions

### Far Pointer Layout (24-bit: bank:8 | offset:16)

Far pointers are stored/passed as 3 bytes:

```
Byte 0: BANK (0-7)
Byte 1: OFFSET_LOW (bits 0-7)
Byte 2: OFFSET_HIGH (bits 8-15)
```

### Parameter Passing (Stack Convention)

Standard stack calling convention applies. Parameters pushed right-to-left:

```
; Call memcpy(far_ptr_t dst, far_ptr_t src, size_t len)
; Mnemonic: memcpy__fpfpz

; Push length (2 bytes, size_t)
lda #<len
ldx #>len
pha
phx

; Push src (3 bytes, far_ptr_t)
lda #src_bank
pha
lda #<src_offset
ldx #>src_offset
pha
phx

; Push dst (3 bytes, far_ptr_t)
lda #dst_bank
pha
lda #<dst_offset
ldx #>dst_offset
pha
phx

; Call
call memcpy__fpfpz

; Stack cleanup (if needed, depends on calling convention)
; (Stack convention typically caller-cleanup)
```

### Example: memcpy(far_ptr_t, far_ptr_t, size_t)

**Calling from Assembly:**

```asm
; Setup destination (BANK1:0x1000)
lda #BANK1      ; dst bank
sta setup_bank_temp

lda #<0x1000    ; dst offset low
ldx #>0x1000    ; dst offset high
sta setup_low_temp
stx setup_high_temp

; Setup source (BANK2:0x2000)
lda #BANK2      ; src bank
pha
lda #<0x2000    ; src offset low
ldx #>0x2000    ; src offset high
pha
phx

; Push destination (3 bytes)
lda setup_bank_temp
pha
lda setup_low_temp
ldx setup_high_temp
pha
phx

; Push length (256 bytes)
lda #<256
ldx #>256
pha
phx

; Call far-to-far memcpy
call memcpy__fpfpz

; Restore stack (caller-cleanup)
; 3 (dst) + 3 (src) + 2 (len) = 8 bytes to clean
pla
pla
pla
pla
pla
pla
pla
pla
```

**Simpler with Register Variables:**

```asm
; Assume we've loaded pointers into memory/registers
; dst in $FA/$FB/$FC (24-bit)
; src in $FD/$FE/$FF (24-bit)
; len in A:X (16-bit)

push_memcpy_fpfpz:
    ; Push length
    pha
    phx
    
    ; Push src (3 bytes from $FD/$FE/$FF)
    lda $FF
    pha
    lda $FE
    ldx $FD
    pha
    phx
    
    ; Push dst (3 bytes from $FA/$FB/$FC)
    lda $FC
    pha
    lda $FB
    ldx $FA
    pha
    phx
    
    call memcpy__fpfpz
    
    ; Cleanup (8 bytes)
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    rts
```

---

## Assembly Code Examples

### Example 1: Copy from Far to Far (Audio Buffer)

**Scenario**: Copy 256-byte audio buffer from BANK1 to BANK2

```asm
copy_audio_frame:
    ; Destination: BANK2, offset 0x1000
    lda #BANK2
    pha
    lda #<0x1000
    ldx #>0x1000
    pha
    phx
    
    ; Source: BANK1, offset 0x0000
    lda #BANK1
    pha
    lda #<0x0000
    ldx #>0x0000
    pha
    phx
    
    ; Length: 256 bytes
    lda #<256
    ldx #>256
    pha
    phx
    
    ; Call memcpy(far_ptr_t, far_ptr_t, size_t)
    call memcpy__fpfpz
    
    ; Cleanup: 8 bytes
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    rts
```

### Example 2: Copy from Local to Far (Load Sprite)

**Scenario**: Load sprite pattern from local RAM into BANK1

```asm
load_sprite:
    ; Destination: BANK1, offset 0x0000 (far)
    lda #BANK1
    pha
    lda #<0x0000
    ldx #>0x0000
    pha
    phx
    
    ; Source: local at $8000 (local)
    lda #<0x8000
    ldx #>0x8000
    pha
    phx
    
    ; Length: 2048 bytes (sprite data)
    lda #<2048
    ldx #>2048
    pha
    phx
    
    ; Call memcpy(far_ptr_t, void*, size_t)
    call memcpy__pvfpz
    
    ; Cleanup: 8 bytes
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    rts
```

### Example 3: Fill Far Memory with Pattern

**Scenario**: Clear graphics buffer in BANK3

```asm
clear_graphics:
    ; Destination: BANK3, offset 0x0000 (far)
    lda #BANK3
    pha
    lda #<0x0000
    ldx #>0x0000
    pha
    phx
    
    ; Fill value: 0 (blank)
    lda #0
    pha
    
    ; Length: 4096 bytes
    lda #<4096
    ldx #>4096
    pha
    phx
    
    ; Call memset(far_ptr_t, int, size_t)
    call memset__fpist
    
    ; Cleanup: 7 bytes (1 value + 2 offset + 2 length)
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    rts
```

### Example 4: String Length of Far String

**Scenario**: Get length of null-terminated string in far memory

```asm
get_string_length:
    ; Pointer: BANK3, offset 0x2000 (far)
    lda #BANK3
    pha
    lda #<0x2000
    ldx #>0x2000
    pha
    phx
    
    ; Call strlen(far_ptr_t) → returns length in A:X
    call strlen__fp
    
    ; Result in A:X (length)
    ; A = length low byte
    ; X = length high byte
    
    ; Cleanup: 3 bytes
    pla
    pla
    pla
    rts
```

---

## Quick Reference Table

### Common Function Overloads

| Function | Local-Local | Local-Far | Far-Local | Far-Far |
|----------|------------|-----------|-----------|---------|
| **memcpy** | `__pvpvz` | `__pvfpz` | `__fpvz` | `__fpfpz` |
| **strcpy** | `__pcrpc` | `__fprpc` | `__pcfp` | `__fpfp` |
| **strcmp** | `__rpcrpc` | `__fprpc` | `__rpcfp` | — |
| **memset** | `__pvist` | — | — | `__fpist` |
| **strlen** | `__rpc` | — | — | `__fp` |
| **strcat** | `__pcrpc` | `__pcfp` | — | — |

---

## Error Cases and Disambiguation

### Ambiguous Call (Both Overloads Valid)

**Scenario**: Calling with `NULL` pointer

```asm
; NULL is both a valid local pointer (void*)
; and a valid far pointer (far_ptr_t with bank 0)

lda #0
ldx #0
; Call memcpy?
call memcpy  ; ERROR: Ambiguous — which variant?
```

**Solution**: Use explicit mangled name

```asm
call memcpy__pvpvz  ; Explicitly: local-to-local
```

Or use the explicit form:

```asm
; Push NULL as local pointer (2 bytes)
lda #0
ldx #0
pha
phx
call memcpy__pvpvz
```

### Type Mismatch (No Valid Overload)

**Scenario**: Trying to use `strcpy` with integers

```asm
; strcpy expects character pointers or far pointers
; NOT integer pointers

lda #<int_array
ldx #>int_array
pha
phx

call strcpy__pipi  ; ERROR: No such overload
```

**Solution**: Use `memcpy` instead for raw memory

```asm
call memcpy__pvpvz  ; Raw memory copy
```

---

## Testing Your Assembly Calls

### Verification Checklist

- [ ] Parameter count matches function signature
- [ ] Parameter sizes match (2 bytes for local, 3 for far)
- [ ] Parameters pushed in reverse order (right-to-left)
- [ ] Mangled name is spelled correctly
- [ ] Stack cleaned up (pushed bytes = popped bytes)
- [ ] Return values handled correctly (if any)

### Debug: Print Mangled Name

Enable debug info in linker to see resolved names:

```asm
; Assembler/linker should print resolution:
; Resolving: memcpy__fpfpz
; Resolved to: $1234 (in code section)
```

---

## Common Mistakes

### Mistake 1: Wrong Parameter Order

```asm
; WRONG: Parameters in forward order
call memcpy__fpfpz  ; Should be pushed right-to-left
lda #BANK1
pha
; ... etc

; RIGHT: Parameters in reverse order (stack grows down)
lda #len_high
ldx #len_low
pha
phx
lda #src_bank
pha
; ... etc
```

### Mistake 2: Forgetting Bank Byte for Far Pointers

```asm
; WRONG: Only 2 bytes for far pointer
lda #<offset
ldx #>offset
pha
phx

; RIGHT: 3 bytes (bank + 2-byte offset)
lda #bank
pha
lda #<offset
ldx #>offset
pha
phx
```

### Mistake 3: Using Wrong Size Constants

```asm
; WRONG: Using size_t code 'z' for non-size_t
lda #BANK1      ; Not size_t!
call strlen__fz  ; ERROR

; RIGHT: Use 'i' for int, 'z' only for size_t
call strlen__fp
```

---

## Further Resources

- **Phase 28**: Standard Library Overloads design
- **Phase 27**: Far pointer operations (reference)
- **Phase 26**: Far pointer infrastructure
- **ca45 Documentation**: Assembly syntax and conventions
- **MEGA65 Hardware**: 45GS02 CPU and calling conventions

---

**Phase 28.1 provides assembly programmers with complete documentation to use overloaded functions correctly!** 📚

With clear type codes, parameter passing conventions, and real-world examples, assembly code can seamlessly integrate with Phase 28's overloaded standard library functions.
