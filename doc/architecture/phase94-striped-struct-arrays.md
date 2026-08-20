# Phase 94: Striped Arrays of Structs

**Status:** Design & Implementation (2026-08-20)  
**Version:** v1.0.8+  
**Phase:** 94 of compiler development

## Overview

Phase 94 extends striped array support from primitive types (int only in Phase 92-93) to arbitrary struct types. This enables memory layout optimization for multi-field data structures with the same striped indexing benefits.

## Design Principle: Variable-Size Element Striping

### Comparison with Phase 92-93

**Phase 92-93 (int-only):**
```c
__striped int matrix[HEIGHT][WIDTH];
// element_size = 4 bytes (fixed)
// stride_factor = HEIGHT * 4 / STRIPE_WIDTH
```

**Phase 94 (struct support):**
```c
struct Point { int x, y; };  // element_size = 8 bytes
__striped struct Point matrix[HEIGHT][WIDTH];
// element_size = 8 bytes (variable based on struct)
// stride_factor = HEIGHT * 8 / STRIPE_WIDTH
```

### Striped Layout for Structs

For `__striped struct Point array[4][4]` where `sizeof(Point) = 8`:

```
Stripe 0 (cols 0-3):
  Address 0:   [Point(0,0)][Point(1,0)][Point(2,0)][Point(3,0)]
  Address 32:  [Point(0,1)][Point(1,1)][Point(2,1)][Point(3,1)]
  ...

Stripe 1 (cols 4-7):
  Address 64:  [Point(0,4)][Point(1,4)][Point(2,4)][Point(3,4)]
  ...
```

Each element is `sizeof(struct)` bytes, not fixed 4 bytes.

## Memory Layout Formula

For `__striped struct T array[height][width]` where `element_size = sizeof(T)`:

```
stripe_width = min(width, 8)                    // Power-of-2, max 8
stripe_select = col >> log2(stripe_width)       // Which stripe
col_remainder = col & (stripe_width - 1)       // Position in stripe
stride_factor = (height * element_size) / stripe_width

offset = stripe_select * height * element_size +
         row * element_size +
         col_remainder * stride_factor
```

### Example: struct Point[8][8] (8-byte elements)

```
element_size = 8 bytes
stripe_width = 4
stride_factor = (8 * 8) / 4 = 16 bytes

Access Point[2][5]:
  stripe_select = 5 >> 2 = 1
  col_remainder = 5 & 3 = 1
  
  offset = 1 * 8 * 8 + 2 * 8 + 1 * 16
         = 64 + 16 + 16 = 96 bytes
```

## Implementation Strategy

### 1. Type System Enhancement

**Detection:**
- `__striped` keyword applies to struct arrays
- Detect `sizeof(struct)` during IR generation
- Validate struct type (must have known size)

**Propagation:**
- Add `elementSize` field to striped array metadata
- Track both dimensions and element size through code generation
- Pass element size to offset calculation functions

### 2. Code Generation Changes

**Offset Calculation:**
- Base formula identical to Phase 92-93
- Replace hardcoded `4` (int size) with `element_size`
- Calculate `stride_factor = height * element_size / stripe_width`

**Assembly Impact:**
- More complex multiplication when element_size > 4
- Additional shifts/multiplies for stride calculation
- Still avoid expensive multiplies via power-of-2 optimizations

### 3. Memory Reorganization

**Data Layout:**
- User provides data in standard row-major order (each element is full struct)
- Compiler reorganizes entire structs into striped layout
- Struct fields remain contiguous (no field-level striping in v1.0.8)

**Implementation:**
- Extend `reorganizeStripedArrayData()` to handle variable element sizes
- Read `sizeof(struct)` bytes per element
- Reorganize based on struct boundaries, not individual integers

### 4. Initialization Support

**Static Arrays:**
```c
struct Vertex {
    short x, y;        // 4 bytes
    unsigned char r, g, b;  // 3 bytes (aligned to 4)
};

__striped struct Vertex mesh[16][16] = { ... };
// Compiler reorganizes initializer data to striped layout
```

## Supported Struct Types

### Compatible Structs (v1.0.8)
✅ All structs with known fixed size
✅ Nested structs (size calculated recursively)
✅ Arrays as struct members (included in size)
✅ Bitfields (included in struct size)

### Size Constraints
- Min: 1 byte (struct with single char)
- Max: 256+ bytes (no hard limit, but assembly gets complex)
- Ideal: 4-64 bytes (matches CPU word sizes)

### Future Limitations (Phase 95+)
- Variable-size structs (unions): Phase 95
- Field-level striping: Phase 96
- Flexible array members in structs: Phase 97

## Performance Impact

### Expected Code Size Reduction

| Scenario | Element Size | Standard | Striped | Reduction |
|----------|--------------|----------|---------|-----------|
| Point[8][8] (8 bytes) | 8 | 28-35 bytes | 18-22 bytes | 35-50% |
| Vertex[8][8] (12 bytes) | 12 | 32-40 bytes | 20-25 bytes | 35-50% |
| Color[8][8] (3 bytes) | 3 | 25-30 bytes | 15-20 bytes | 35-50% |

### Runtime Characteristics
- 30-40% speedup for array-heavy loops (same as Phase 92-93)
- Depth offset calculation unchanged (Phase 93)
- Stride calculation complexity increases with element size
- Still faster than standard row-major multiplication

## Code Generation Details

### Assembly Pattern (Struct Element)

```asm
; Calculate offset for struct array access
; array[row][col] where sizeof(struct) = element_size

lda col_index
lsr
lsr                    ; stripe_select = col >> 2
; Multiply by height * element_size
tax
lda #(height * element_size)
mul.16 .ax, .tx        ; stripe_select * height * element_size

; Add row * element_size
clc
tax
lda row_index
ldy #element_size
mul.8y                 ; row * element_size
adc.16 .tx

; Add col_remainder * stride_factor
tax
lda col_index
and #3                 ; col_remainder
ldy #stride_factor
mul.8y                 ; col_remainder * stride_factor
adc.16 .tx

; Add base address
clc
adc.16 $zp_base
```

### Complexity Considerations

**For small structs (1-4 bytes):**
- Similar to Phase 92 (int arrays)
- Element size fits in single byte

**For larger structs (8-64 bytes):**
- More complex multiplication chains
- May require 16-bit multiplies
- Trade-off: more assembly, but still 35-50% reduction

## Restrictions (v1.0.8)

- **Fixed-size structs only** — sizes must be known at compile time
- **Last 2 dimensions striped** — same as Phase 93 (3D+ arrays)
- **Static/global arrays** — initialization support required
- **Power-of-2 widths** — same as Phase 92-93
- **No field-level striping** — entire struct striped as unit

## Testing Strategy

### Test Categories

**1. Basic Struct Access**
- Simple point/vector structs (8-12 bytes)
- Basic indexing patterns
- Verify correct offset calculation

**2. Initialization**
- Static array initialization with data reorganization
- Large initializer lists (16×16 matrices)
- Verify memory layout correctness

**3. Nested Loops**
- Common access patterns
- Performance characteristics
- Assembly generation

**4. Complex Structs**
- Bitfield structs
- Nested structs
- Alignment verification

**5. 3D+ Structs**
- Struct arrays with 3+ dimensions
- Depth offset integration
- Complex offset calculation

## Implementation Checklist

### Code Changes Required
- [ ] Parser: Detect `__striped` on struct array declarations
- [ ] Type system: Extract and track `sizeof(struct)`
- [ ] Code generation: Update offset calculation for variable element size
- [ ] Memory reorganization: Handle struct-sized elements
- [ ] Data emission: Reorganize initializer data properly

### Test Files Needed
- [ ] test_striped_struct_basic.c
- [ ] test_striped_struct_init.c
- [ ] test_striped_struct_nested.c
- [ ] test_striped_struct_3d.c
- [ ] test_striped_struct_bitfield.c

### Validation
- [ ] Compilation tests
- [ ] Assembly verification
- [ ] Memory layout verification
- [ ] Code size comparison
- [ ] No regression tests

## Performance Expectations

**Phase 94 Impact:**
- Extends striped optimization to **all struct types**
- Maintains **35-50% code reduction** for array indexing
- Maintains **30-40% runtime speedup** for array-heavy loops
- Zero additional compile-time overhead
- Minimal runtime overhead for element size calculations

## Known Limitations (v1.0.8)

- Field-level striping not supported (only whole-struct striping)
- Variable-size structs not supported (size must be constant)
- Flexible array members in structs not supported
- Recursive struct support limited (flattened only)

## Future Enhancements (Phase 95+)

- **Phase 95:** Field-level striping (stripe individual struct fields)
- **Phase 96:** Variable-size structs (unions)
- **Phase 97:** Flexible array members
- **Phase 98:** Cross-module struct striping optimization

## Verification Strategy

**Phase 94 Testing:**
1. Compile struct array programs → verify no errors
2. Generate assembly → verify element size handling
3. Inspect binary data → verify striped layout correct
4. Compare code sizes → measure 35-50% reduction
5. Run functional tests → verify correctness

## Summary

Phase 94 extends striped array support from primitive types to all struct types by:

1. **Detecting struct types** and extracting their size
2. **Generalizing offset calculations** to use variable element size
3. **Reorganizing initializer data** at element boundaries
4. **Maintaining 35-50% optimization** across all struct sizes
5. **Supporting 3D+ struct arrays** via Phase 93 integration

This enables high-performance access to complex data structures with the same striped indexing benefits as primitive arrays.

---

**Phase 94 Timeline:** 3-4 hours total
- Parser/Type system updates: 1 hour
- Code generation: 1-1.5 hours
- Testing & validation: 1-1.5 hours

**Status:** Ready for implementation
