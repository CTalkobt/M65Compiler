# Phase 21: Vector Operations with Phase 19 Operators

**Status**: Design/Implementation  
**Target Version**: v1.0.12+  
**Priority**: High (Audio/Graphics SIMD)

## Overview

Phase 21 extends Phase 19 advanced operators to fixed-size vector types, enabling SIMD-like batch operations on audio buffers, graphics data, and signal processing arrays. All 9 Phase 19 operators work element-wise on vectors.

## Vector Architecture

### Vector Type Definition

```c
// Template vector type: Vector<ElementType, Size>
// Compile-time fixed size, stack-allocated
Vector<int16_t, 256> audio_frame;        // 256 int16 samples
Vector<uint8_t, 16>  pixel_row;          // 16 pixel colors
Vector<int, 8>       filter_taps;        // 8 filter coefficients
Vector<float, 4>     quaternion;         // 4-element rotation
```

### Type Requirements

- **ElementType**: Any scalar type (int, uint8_t, float, long, short, etc.)
- **Size**: Compile-time constant (1, 2, 4, 8, 16, 32, 64, 128, 256, ...)
- **Storage**: Stack-allocated fixed array (no dynamic allocation)
- **Alignment**: Optionally aligned for hardware SIMD (8-byte on MEGA65)

### Vector Constructor & Access

```c
// Construction
Vector<int, 4> v1{1, 2, 3, 4};         // Aggregate initializer
Vector<int, 4> v2 = v1;                 // Copy construction
Vector<int, 4> v3{};                    // Zero-initialized

// Element access
int val = v1[0];                         // Index access (bounds-checked in debug)
v1[1] = 42;                              // Element assignment
int* data = v1.data();                   // Raw pointer access
int size = v1.size();                    // Always returns template Size
```

## Phase 19 Operators on Vectors

### Element-Wise Saturating Arithmetic

Each operator applies element-by-element to corresponding vector elements.

#### Saturating Addition: `v1 +| v2`
```c
Vector<int8_t, 4> a{100, 50, -100, 0};
Vector<int8_t, 4> b{50, 100, -50, 20};
Vector<int8_t, 4> result = a +| b;
// result = {127, 127, -128, 20}  // Each element saturated

// Audio mixing: safe channel addition
Vector<int16_t, 256> ch1 = load_channel(1);
Vector<int16_t, 256> ch2 = load_channel(2);
Vector<int16_t, 256> mixed = ch1 +| ch2;  // No clipping
```

#### Saturating Subtraction: `v1 -| v2`
```c
Vector<int16_t, 8> a{100, 200, -100, 50, 0, 1000, 500, 255};
Vector<int16_t, 8> b{50, 300, 50, 100, 10, 100, 600, 200};
Vector<int16_t, 8> result = a -| b;
// result = {50, -100, -150, -50, -10, 900, -100, 55}
```

#### Saturating Multiplication: `v1 *| v2`
```c
Vector<uint8_t, 4> brightness{200, 150, 100, 255};
Vector<uint8_t, 4> scale{2, 2, 3, 1};
Vector<uint8_t, 4> amplified = brightness *| scale;
// result = {255, 255, 255, 255}  // Saturated to max
```

### Element-Wise Bit Rotation

#### Rotate Left: `v <<< n`
```c
Vector<uint8_t, 8> pattern{0xA5, 0x5A, 0xF0, 0x0F, 0xAA, 0x55, 0xFF, 0x00};
Vector<uint8_t, 8> rotated = pattern <<< 1;
// result = {0x4B, 0xB4, 0xE1, 0x1E, 0x55, 0xAA, 0xFF, 0x00}

// Sprite animation: rotate pattern for each frame
Vector<uint8_t, 16> sprite = get_sprite_pattern();
for (int frame = 0; frame < 8; frame++) {
    sprite = sprite <<< 1;  // Rotate each sample
    draw_sprite(sprite);
}
```

#### Rotate Right: `v >>> n`
```c
Vector<uint16_t, 4> tiles{0xAAAA, 0x5555, 0xCCCC, 0x3333};
Vector<uint16_t, 4> rotated = tiles >>> 4;
// result = {0xAAAA, 0x5555, 0xCCCC, 0x3333}  // 16-bit rotation
```

### Element-Wise Min/Max

#### Minimum: `v1 <> v2`
```c
Vector<int, 8> values{50, 100, 30, 200, 10, 150, 75, 90};
Vector<int, 8> limit{75, 75, 75, 75, 75, 75, 75, 75};
Vector<int, 8> clamped = values <> limit;
// result = {50, 75, 30, 75, 10, 75, 75, 75}

// Graphics: clamp brightness per pixel
Vector<uint8_t, 16> pixels = load_scanline();
Vector<uint8_t, 16> max_bright = init_vector<uint8_t, 16>(200);
Vector<uint8_t, 16> result = pixels <> max_bright;
```

#### Maximum: `v1 >< v2`
```c
Vector<int8_t, 16> signal = read_adc_samples();
Vector<int8_t, 16> threshold{-10, -10, -10, ...};
Vector<int8_t, 16> gated = signal >< threshold;
// Gate: below threshold becomes threshold
```

### Element-Wise Saturating Shift

#### Saturating Left Shift: `v <<| n`
```c
Vector<int16_t, 4> amplitudes{1000, 2000, 3000, 4000};
Vector<int16_t, 4> amplified = amplitudes <<| 2;  // *4 with saturation
// result = {4000, 8000, 12000, 16000}  // Saturated to INT16_MAX

// Volume boost without clipping
Vector<int16_t, 256> audio = load_audio_frame();
Vector<int16_t, 256> boosted = audio <<| 2;  // 6dB gain
```

#### Saturating Right Shift: `v >>| n`
```c
Vector<uint8_t, 8> samples{200, 150, 100, 50, 25, 12, 6, 3};
Vector<uint8_t, 8> reduced = samples >>| 1;  // Divide by 2
// result = {100, 75, 50, 25, 12, 6, 3, 1}
```

## Vector Operations

### Standard Arithmetic (Element-Wise)

```c
Vector<float, 8> a{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
Vector<float, 8> b{0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8};

Vector<float, 8> sum = a + b;       // Standard addition
Vector<float, 8> diff = a - b;      // Standard subtraction
Vector<float, 8> prod = a * b;      // Standard multiplication
Vector<float, 8> quot = a / b;      // Standard division
```

### Scalar Operations (Broadcast)

```c
Vector<int, 16> values = load_values();

// All elements multiplied by scalar
Vector<int, 16> scaled = values * 2;
Vector<int, 16> shifted = values << 3;

// Saturating ops with scalar
Vector<int8_t, 16> boosted = values +| 50;  // Add 50 to each
```

### Horizontal Operations

```c
Vector<int16_t, 256> audio = load_frame();

int16_t sum = audio.sum();          // Horizontal sum
int16_t min_val = audio.min();      // Horizontal minimum
int16_t max_val = audio.max();      // Horizontal maximum
int16_t avg = audio.sum() / 256;    // Average (with division)

// Audio statistics
if (audio.max() > 30000) {
    audio = audio >>| 1;  // Reduce if clipping detected
}
```

### Masking & Conditional Operations

```c
Vector<int, 8> a{10, 20, 30, 40, 50, 60, 70, 80};
Vector<int, 8> b{15, 15, 15, 15, 15, 15, 15, 15};

// Element-wise comparison
Vector<bool, 8> mask = a > b;  // {false, true, true, ...}

// Conditional assignment (where mask, choose_from_a, choose_from_b)
Vector<int, 8> result = where(mask, a, b);
// result = {15, 20, 30, 40, 50, 60, 70, 80}
```

## Real-World Use Cases

### Audio Mixing (Safe Channel Blending)

```c
// Mix 4 channels without clipping
struct AudioMixer {
    Vector<int16_t, 256> ch1, ch2, ch3, ch4;
    
    Vector<int16_t, 256> mix() {
        // Safe saturating addition
        Vector<int16_t, 256> temp = ch1 +| ch2;
        temp = temp +| ch3;
        temp = temp +| ch4;
        
        // Apply master limiter
        Vector<int16_t, 256> limit = init_vector<int16_t, 256>(32000);
        return temp <> limit;  // Clamp to max
    }
};
```

### Graphics: Pixel Batch Processing

```c
// Apply brightness adjustment to 16-pixel scanline
void adjust_scanline_brightness(Vector<uint8_t, 16>& scanline, int8_t delta) {
    if (delta > 0) {
        scanline = scanline +| delta;  // Clamp at white
    } else {
        scanline = scanline -| (-delta);  // Clamp at black
    }
}

// Sprite rotation (all pixels)
Vector<uint8_t, 32> sprite_pattern;
void rotate_sprite_animation() {
    for (int frame = 0; frame < 8; frame++) {
        sprite_pattern = sprite_pattern <<< 1;  // Rotate all pixels
        draw_sprite(sprite_pattern);
    }
}
```

### Signal Processing: Filter Taps

```c
// Convolution with 8-tap FIR filter
Vector<int16_t, 8> filter_taps{100, 200, 300, 400, 300, 200, 100, 50};
Vector<int16_t, 8> input_samples = read_adc();

// Element-wise multiply and sum
Vector<int16_t, 8> products = input_samples * filter_taps;
int16_t output = products.sum();  // Horizontal reduction
```

### Fixed-Point Math

```c
struct FixedVector {
    Vector<int16_t, 4> values;
    
    // Safe fractional multiply (4.12 fixed point)
    Vector<int16_t, 4> multiply_fixed(int16_t scale) {
        Vector<int32_t, 4> temp;
        for (int i = 0; i < 4; i++) {
            temp[i] = static_cast<int32_t>(values[i]) * scale >> 12;
        }
        // Convert back with saturation
        return saturate_convert<int16_t>(temp);
    }
};
```

## Compiler Implementation

### Template Instantiation

Compiler generates specialized code for each `Vector<T, N>` combination:

```cpp
// Vector<int16_t, 256> instance in IR
class Vector_int16t_256 {
    int16_t data[256];
    
public:
    Vector_int16t_256 operator_plus_sat(const Vector_int16t_256& other) {
        Vector_int16t_256 result;
        for (int i = 0; i < 256; ++i) {
            result.data[i] = saturating_add(data[i], other.data[i]);
        }
        return result;
    }
    
    // Similar for all Phase 19 operators
};
```

### Loop Unrolling Optimization

For power-of-2 vectors and simple operations:

```asm
; Vector<int16_t, 8> a +| b
; Unroll to 4 iterations (8 elements / 2 per iteration)
loop:
    lda a, 0      ; Load a[i]
    clc
    adc b, 0      ; Add b[i] with saturation
    bvc @skip_sat
    lda #127      ; Saturate positive
    bra @store
@skip_sat:
    bmi @skip_neg
    bra @store
    lda #-128     ; Saturate negative
@store:
    sta result, 0
    
    ; Repeat for next element without bounds check
    ...
    
    inx
    cpx #8
    bne loop
```

### SIMD Hints for Linker

Compiler emits optimization hints for loop vectorization:

```cpp
// In IR generation:
// .simd_hint vector_add_loop, vector_type=int16_t, count=256, operator=add_sat
```

## Type Compatibility

### Implicit Conversions

```c
// Vector construction with narrowing (compiler warning in strict mode)
Vector<uint8_t, 4> small{256, 257, 258, 259};  // Narrowed to 0, 1, 2, 3

// Explicit conversion
Vector<float, 4> floats{1.5, 2.5, 3.5, 4.5};
Vector<int, 4> ints = convert_vector<int>(floats);  // {1, 2, 3, 4}
```

### Type Promotion

```c
Vector<int8_t, 8> a;
Vector<int16_t, 8> b;

// Result type promoted to int16_t
Vector<int16_t, 8> result = a + b;
```

## Vector Member Functions

### Access & Inspection
- `T& operator[](int idx)` — Element access
- `const T& at(int idx)` — Bounds-checked access (debug only)
- `T* data()` — Raw pointer
- `size_t size()` — Returns compile-time Size
- `bool empty()` — Always false

### Reductions
- `T sum()` — Horizontal sum (all elements)
- `T min()` — Horizontal minimum
- `T max()` — Horizontal maximum
- `T product()` — Horizontal product (multiply all)

### Utilities
- `void fill(T value)` — Set all elements
- `void swap(Vector& other)` — Element-wise swap
- `Vector reverse()` — Reverse element order
- `Vector rotate(int amount)` — Circular element rotation

## Performance Characteristics

### Code Size (on 6502)

| Operation | Type | Size |
|-----------|------|------|
| Element add | int8_t | 5 bytes + loop |
| Element `+\|` | int8_t | 8 bytes + loop |
| Saturating reduction | int16_t | 20+ bytes |
| Horizontal sum | 256 elements | ~400 bytes |

### Optimization Levels

- **-O0**: Straightforward loops, no unrolling
- **-O1+**: Selective unrolling for small vectors (N ≤ 32)
- **-O2+**: Aggressive loop unrolling, constant folding
- **-O3+**: SIMD-style batch operations where possible

## Testing Strategy

### Test Categories

1. **Basic Vector Operations** (12 tests)
   - Construction, access, modification
   - Standard arithmetic on each element
   - Type conversions and promotions

2. **Phase 19 Operators on Vectors** (20 tests)
   - Saturating arithmetic (9 vector × 3 operator types)
   - Bit rotation with various amounts
   - Min/max with mixed ranges
   - Saturating shift operations

3. **Horizontal Reductions** (8 tests)
   - Sum, min, max
   - Edge cases (empty logic, negative numbers)
   - Overflow handling

4. **Real-World Use Cases** (15 tests)
   - Audio mixing (4-channel, saturation)
   - Graphics operations (scanline processing, rotation)
   - Signal processing (filter taps, convolution)
   - Fixed-point math with vectors

5. **Performance & Optimization** (10 tests)
   - Loop unrolling verification
   - Code size benchmarking
   - Constant folding validation

**Total: 65 tests**

## Integration Points

### Compiler Components
1. **Parser**: Template syntax `Vector<T, N>`
2. **Validator**: Template parameter validation, member function resolution
3. **CodeGenerator**: Loop generation, optimization hints, SIMD emission
4. **IR**: Vector operation nodes, broadcast operations

### Library Support
- `<vector.h>` header with Vector template definition
- Standard library integration for Vector algorithms
- Optimization pragma support: `#pragma cc45 simd(enable/disable)`

## Backward Compatibility

- All new; no changes to existing functionality
- Vector type names don't conflict with std::vector (different namespace)
- Existing code unaffected

## Known Limitations

1. **Fixed-Size Only**: Dynamic vectors deferred (use `std::vector` from libc)
2. **No Cross-Vector Operations**: `Vector<int, 4>` + `Vector<int, 8>` requires conversion
3. **No Complex Element Types**: Vectors of structs with nested vectors not supported
4. **Horizontal Ops**: Limited to reduction (sum/min/max); cross-element shuffles future work

## Future Enhancements (Phase 22+)

- **Phase 22**: Batch SIMD operations (multi-sample frame processing)
- **Phase 23**: Hardware intrinsics for MEGA65 DMA vectorization
- **Phase 24**: Vector element shuffling, permutation operations
- **Phase 25**: Auto-vectorization for array loops

## References

- Phase 19 Operators: `doc/architecture/phase19-advanced-operators.md`
- Phase 20 Overloading: `doc/architecture/phase20-operator-integration.md`
- C++ std::vector (reference): C++17 standard
