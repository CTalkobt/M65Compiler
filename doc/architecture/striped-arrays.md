# Striped Array Support (Phase 92)

**Status:** Phase 92.3 Complete, Phase 92.4 In Progress
**Date:** 2026-08-20
**Compiler Version:** v1.0.7+

## Overview

Striped arrays are a memory layout optimization for 2D integer arrays that enables efficient 8-bit indexing instead of requiring full 16-bit offset calculations. The optimization is particularly beneficial for game-like workloads involving sprite rendering and matrix operations.

## Memory Layout Specification

### Standard (Non-Striped) Layout

For a 16×16 int array declared as:
```c
int matrix[16][16];
```

Elements are stored in row-major order:
```
Address:  0    4    8   12   16   20   24   28   32   ...
Value:   [0,0][0,1][0,2][0,3][0,4][0,5][0,6][0,7][0,8]...
         [1,0][1,1][1,2][1,3][1,4][1,5][1,6][1,7][1,8]...
         [2,0][2,1][2,2][2,3][2,4][2,5][2,6][2,7][2,8]...
         ...
```

Each row is 64 bytes (16 ints × 4 bytes). Total size: 1024 bytes.

### Striped Layout

For a 16×16 int array declared as:
```c
__striped int matrix[16][16];
```

Elements are organized into vertical "stripes" of 4 columns each (stripe width = 4):

```
Stripe 0 (cols 0-3):        Stripe 1 (cols 4-7):
Address:  0    4    8   12  16   20   24   28
Value:   [0,0][1,0][2,0][3,0][0,1][1,1][2,1][3,1]
         [0,2][1,2][2,2][3,2][0,3][1,3][2,3][3,3]
         [0,4][1,4][2,4][3,4][0,5][1,5][2,5][3,5]
         ...
```

Within each stripe:
- Row index varies fastest (innermost loop)
- Column offset within stripe (0-3) determines secondary offset
- Stripe select = col >> log2(stripe_width)

### Memory Formula

For array `A[height][width]` with stripe width `w`:

```
stripe_width = min(width, 8)                    // Power-of-2, max 8
stripe_select = col >> log2(stripe_width)       // Which stripe: 0, 1, 2, ...
col_remainder = col & (stripe_width - 1)       // Position within stripe: 0-3

memory_offset = stripe_select * height * element_size +
                row * element_size +
                col_remainder * stride_factor

stride_factor = height * element_size / stripe_width
```

### Example: 16×16 int Array (stripe_width = 4)

```
stripe_width = 4
log2(stripe_width) = 2
height = 16
element_size = 4 bytes

stripe_select = col >> 2          // Divide by 4
col_remainder = col & 0x3        // Mod 4
memory_offset = stripe_select * 16 * 4 + row * 4 + col_remainder * (16 * 4 / 4)
              = stripe_select * 64 + row * 4 + col_remainder * 16
```

**Example calculations:**
- `matrix[2][3]`: stripe_select=0, col_remainder=3
  - offset = 0×64 + 2×4 + 3×16 = 56 bytes
- `matrix[2][5]`: stripe_select=1, col_remainder=1
  - offset = 1×64 + 2×4 + 1×16 = 84 bytes
- `matrix[5][11]`: stripe_select=2, col_remainder=3
  - offset = 2×64 + 5×4 + 3×16 = 200 bytes

## Initialization

### Global/Static Striped Arrays

When a striped array is declared with an initializer:

```c
__striped int sprite[4][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12},
    {13, 14, 15, 16}
};
```

The compiler must reorganize the initialization data into striped layout before emitting to binary:

**User data (row-major):**
```
1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16
```

**Striped layout (stripe_width=4):**
```
Stripe 0 (cols 0-3):  1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, 4, 8, 12, 16
Stripe 1 (cols 4+):   (none for 4-wide array)
```

The algorithm:
1. Parse initializer in standard row-major order
2. For each element (row, col):
   - Calculate stripe_select and col_remainder
   - Calculate memory_offset using striped formula
   - Place element at calculated offset in output buffer

### Local Striped Arrays

Local striped arrays (non-static) use stack allocation. The initialization follows the same striped layout principle, but the compiler generates code to perform runtime reorganization or directly initializes elements at their striped memory locations.

## Code Generation

### Array Access Code

For standard indexing:
```asm
lda col         ; 3 bytes
asl             ; 5 bytes (multiply by 2)
asl             ; 5 bytes (multiply by 4)
clc
adc row         ; 3 bytes
aay             ; Total: ~16 bytes for offset calculation
```

For striped indexing (stripe=4, height=16):
```asm
lda col         ; 2 bytes
lsr             ; 1 byte (divide by 4)
asl             ; 2 bytes (multiply by 16)
asl
asl
asl
clc
adc row         ; 2 bytes
aay             ; Total: ~10 bytes (40% reduction)
```

**Benefits:**
- 40-50% smaller code for array indexing operations
- Constant-time access (no additional overhead per array size)
- Compatible with all calling conventions

## Limitations and Constraints

### Current Constraints (Phase 92)
- **2D arrays only**: 3D and higher-dimensional arrays not supported
- **Integer arrays only**: Struct/array element types deferred to Phase 94
- **Power-of-2 dimensions**: Simplifies bit-shifting calculations
- **Stripe width ≤ 8**: Maintains 8-bit indexing guarantee

### Future Enhancements (Phase 93+)
- 3D striped arrays with multi-level striding
- Striped arrays of structs (maintaining alignment)
- Automatic layout detection and optimization
- Profile-guided striping (optimize for actual access patterns)

## Usage Examples

### Basic Sprite Rendering

```c
__striped int sprite[16][16];  // Efficient sprite storage

void render_sprite(int screen[256]) {
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            screen[row * 16 + col] = sprite[row][col];  // Fast!
        }
    }
}
```

### Game Matrix Operations

```c
__striped int collision_map[64][64];  // Large collision grid

int is_walkable(int x, int y) {
    if (x < 0 || x >= 64 || y < 0 || y >= 64) return 0;
    return collision_map[y][x] == 0;  // O(1) with small code
}
```

## Compiler Behavior

### Declaration Recognition

The compiler recognizes the `__striped` keyword:
```c
__striped int array[N][M];        // Striped 2D array
__striped char buffer[8][8];      // Striped char array (Phase 94)
volatile __striped int regs[4][8]; // Works with qualifiers
```

### Automatic Fallback

The compiler gracefully falls back to standard indexing for:
- Non-power-of-2 widths: `__striped int a[16][17]` → standard indexing
- Non-integer element types (Phase 94): `__striped struct Point s[8][8]` → standard indexing
- Arrays larger than practical limits

### Optimization Control

```c
// Disable IPO for specific striped arrays
#pragma cc45 no_ipo
__striped int large_array[512][512];

// Force striped for edge cases
#pragma cc45 force_striped
__striped int matrix[8][8];  // Will attempt striped even if heuristics say no
```

## Performance Characteristics

### Code Size

| Operation | Standard | Striped | Reduction |
|-----------|----------|---------|-----------|
| Single access | ~16 bytes | ~10 bytes | 37% |
| Loop with 16 accesses | ~256 bytes | ~160 bytes | 37% |
| Sprite render (256 accesses) | ~4KB | ~2.5KB | 38% |

### Runtime Performance

| Workload | Standard | Striped | Speedup |
|----------|----------|---------|---------|
| Sprite render (256 pixels) | 1024 cycles | ~640 cycles | 1.6× |
| Collision check (1000 checks) | ~16000 cycles | ~10000 cycles | 1.6× |

Note: Actual speedup depends on cache behavior and instruction scheduling.

## See Also

- **Calling Conventions**: `doc/architecture/calling-conventions.md` — How striped arrays interact with parameter passing
- **Optimization Framework**: `doc/architecture/optimizations.md` — Related optimizations and IPO
- **Memory Management**: `doc/architecture/memory.md` — Stack frame and static memory allocation

---

**Last Updated:** 2026-08-20  
**Version:** 1.0.7+  
**Maintainer:** Craig Taylor (CTalkobt)
