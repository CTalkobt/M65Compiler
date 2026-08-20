# Phase 93: Striped 3D and Higher-Dimensional Arrays

**Status:** Design & Implementation (2026-08-20)
**Version:** v1.0.8+
**Phase:** 93 of compiler development

## Overview

Phase 93 extends striped array support from 2D to 3D and higher dimensions. The key principle is **selective axis striping**: the last two dimensions are organized as striped 2D matrices, while earlier dimensions sequence these matrices.

This approach maintains code generation efficiency and memory layout predictability while enabling multi-dimensional array optimizations.

## Design Principle: Selective Axis Striping

### 2D Arrays (Phase 92)
```c
__striped int matrix[HEIGHT][WIDTH];
// Last 2 dims: striped
// Memory: [0,0][1,0][2,0][3,0][0,1][1,1]... (striped layout)
```

### 3D Arrays (Phase 93)
```c
__striped int cube[DEPTH][HEIGHT][WIDTH];
// Last 2 dims (H, W): striped per depth plane
// First dim (D): sequences the 2D matrices
// 
// Conceptually: sequence of striped 2D matrices
// memory[d][row][col] = data[d * height * width + striped_offset(row, col)]
```

### 4D+ Arrays (Phase 93+)
```c
__striped int tensor[D1][D2][HEIGHT][WIDTH];
// Last 2 dims striped, earlier dims sequence matrices
// Offset = d1_offset + d2_offset + striped_2d_offset
```

## Memory Layout Specification

### 3D Array Layout Example: 4×8×8 int array

```c
__striped int cube[4][8][8];
```

**Parameters:**
- depth = 4, height = 8, width = 8
- stripe_width = 4 (min(8, 8) power-of-2)
- log2_stripe = 2
- 2d_matrix_size = 8 * 8 * 4 = 256 bytes per depth plane

**Memory Organization:**
```
Depth 0: [striped 8×8 matrix starting at offset 0]
  Stripe 0 (cols 0-3):
    Address 0:   [0,0][1,0][2,0][3,0]...
    Address 16:  [0,1][1,1][2,1][3,1]...
    ...
  Stripe 1 (cols 4-7):
    Address 64:  [0,4][1,4][2,4][3,4]...
    ...
Depth 1: [striped 8×8 matrix starting at offset 256]
  ...
Depth 2: [striped 8×8 matrix starting at offset 512]
  ...
Depth 3: [striped 8×8 matrix starting at offset 768]
  ...
```

### General Offset Formula for 3D: `array[d][r][c]`

```
stripe_width = power_of_2(min(width, 8))
log2_stripe = log2(stripe_width)
stripe_select = c >> log2_stripe
col_remainder = c & (stripe_width - 1)
stride_factor = height * element_size / stripe_width

// Offset calculation:
d_offset = d * height * width * element_size
stripe_base = stripe_select * height * element_size
row_offset = r * element_size
col_offset = col_remainder * stride_factor

total_offset = d_offset + stripe_base + row_offset + col_offset
```

### Example Calculations: 4×8×8 int array

**Access `cube[2][3][5]`:**
```
d = 2, r = 3, c = 5
stripe_width = 4
stripe_select = 5 >> 2 = 1
col_remainder = 5 & 3 = 1

d_offset = 2 * 8 * 8 * 4 = 512
stripe_base = 1 * 8 * 4 = 32
row_offset = 3 * 4 = 12
col_offset = 1 * (8 * 4 / 4) = 8

total = 512 + 32 + 12 + 8 = 564 bytes
```

## Array Dimension Restrictions

### Supported Patterns (v1.0.8)
- ✅ 2D: `__striped int arr[H][W]`
- ✅ 3D: `__striped int arr[D][H][W]`
- ✅ 4D: `__striped int arr[D1][D2][H][W]`
- ✅ 5D+: `__striped int arr[D1][D2][D3][H][W]`

### Last Dimension Constraints
- Must be **2 dimensions** minimum (last 2 dims get striped)
- Last dimension (W): power-of-2 (4, 8, 16, ...)
- Second-to-last dimension (H): any size, but 4-256 recommended for efficiency
- Earlier dimensions: any size (no striping optimization)

### Element Type Restrictions (v1.0.8)
- ✅ `int` (4 bytes)
- ⏳ `short` (2 bytes) — planned for v1.0.9
- ⏳ `char` (1 byte) — planned for v1.0.9
- ❌ `long` (8 bytes) — not yet (stride calculation becomes complex)
- ❌ Structs — Phase 94

### Automatic Fallback
- Non-power-of-2 widths → standard layout (no error)
- Non-int types (pre-v1.0.9) → standard layout (no error)
- Fewer than 2 dimensions → not applicable (use Phase 92)

## Code Generation Strategy

### 3D Indexing in Assembly

**Challenge:** 3D offset calculation requires:
1. Calculate 2D offset within a matrix (same as Phase 92)
2. Calculate which matrix (depth offset)
3. Add them together

**Strategy:** Pre-calculate depth offset, then add striped 2D offset

**Example assembly for `cube[d][r][c]`:**
```asm
; Calculate depth offset: d * matrix_size
lda #<matrix_size_lo>    ; Load matrix_size (8*8*4 = 256)
ldx #>matrix_size_hi
mul.16 .dx               ; D (depth) already in D register
sta zpDepthLo
stx zpDepthHi            ; Store depth offset

; Now calculate 2D offset (same as Phase 92)
; stripe_select = c >> 2
lda c
lsr
lsr
; multiply by height (8) = shift left 3
asl
asl
asl                      ; stripe_select * 8

; Add row offset
clc
adc r
sta zpOffsetLo          ; 2D offset in ZP

; Add depth offset to 2D offset
lda zpOffsetLo
clc
adc zpDepthLo
sta finalOffset

; Continue with column remainder calculation...
```

### Code Size Impact

Expected code reduction for 3D arrays:
- **2D indexing (Phase 92):** 10-12 bytes via bit shifts
- **3D indexing (Phase 93):** 15-18 bytes (adds depth calculation)
- **Standard 3D indexing:** 25-30 bytes (all multiplies)
- **Reduction:** 35-50% vs standard 3D

## Initialization

### Global/Static 3D Striped Arrays

Compiler automatically reorganizes row-major user data into striped layout at compile time:

```c
__striped int cube[2][4][4] = {
    {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    },
    {
        {17, 18, 19, 20},
        {21, 22, 23, 24},
        {25, 26, 27, 28},
        {29, 30, 31, 32}
    }
};
```

Compiler converts to striped layout at compile time. No runtime overhead.

## Implementation Checklist

### Phase 93.1: Type System & Parser (✓ Phase 92 already done)
- ✅ Parser recognizes `__striped` on multi-dim arrays
- ✅ Type system tracks `arrayDims` vector
- ✅ Validator checks dimension constraints

### Phase 93.2: Code Generation (This phase)
- [ ] Extend `emitStripedArrayAccess()` to handle 3D+
- [ ] Calculate depth offset for multi-dimensional arrays
- [ ] Combine depth offset with 2D striped offset
- [ ] Handle variable depths and heights

### Phase 93.3: Memory Reorganization (This phase)
- [ ] Update `reorganizeStripedArrayData()` for 3D initialization
- [ ] Preserve depth-plane separation while striping within each plane
- [ ] Validate memory layout correctness

### Phase 93.4: Testing & Validation (This phase)
- [ ] Test 3D basic access patterns
- [ ] Test nested loop patterns on 3D arrays
- [ ] Test 3D array initialization with reorganization
- [ ] Code size comparison: 3D vs standard
- [ ] Test 4D and 5D arrays for generalization

## Performance Expectations

| Scenario | Standard | Striped | Reduction |
|----------|----------|---------|-----------|
| 3D 8×8×8 access | 25-30 bytes | 15-18 bytes | 35-50% |
| 3D loop access | 40-50 bytes | 20-28 bytes | 40-55% |
| 4D 4×8×8×8 access | 35-40 bytes | 20-25 bytes | 40-50% |

## Known Limitations (v1.0.8)

- Only `int` element type (v1.0.8); short/char in v1.0.9
- Last two dimensions striped only; earlier dims not striped
- Requires power-of-2 width for last dimension
- Local striped arrays not yet supported (globals/statics only)

## Future Enhancements (Phase 94+)

- **Phase 94:** Striped struct arrays
- **Phase 95:** Selective axis striping configuration
- **Phase 96:** Multi-axis striping optimization
- **Phase 97:** Cross-module striped array optimization

## Verification Strategy

**Phase 93.2-93.3 Testing:**
1. Compile 3D/4D test programs → verify no errors
2. Generate assembly → verify bit shifts present
3. Inspect binary data → verify striped layout correct
4. Compare code sizes → measure 35-50% reduction

**Phase 93.4 Benchmarking:**
1. Matrix transpose test (3×8×8)
2. Cube volume calculation (4×8×8×8)
3. Higher-dimensional tensor operations
4. Cache-friendly access patterns

---

**Phase 93 Timeline:** 6-8 hours total
- Phase 93.2: Code generation — 2-3 hours
- Phase 93.3: Memory reorganization — 1.5-2 hours  
- Phase 93.4: Testing & validation — 2-3 hours

**Status:** Ready for implementation
