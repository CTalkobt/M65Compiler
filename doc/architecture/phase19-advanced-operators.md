# Phase 19: Advanced Audio/Graphics Operators

**Status**: Implementation
**Target Version**: v1.0.11+
**Priority**: High (Audio/Graphics performance)

## Overview

Phase 19 adds four categories of advanced operators specifically optimized for audio synthesis, effects processing, and graphics manipulation on the MEGA65 platform.

## Operator Categories

### 1. Saturating Arithmetic Operators

Prevent wraparound overflow by clamping results to valid ranges.

#### Syntax
- `a +| b` — Saturating addition
- `a -| b` — Saturating subtraction  
- `a *| b` — Saturating multiplication

#### Semantics
- **Result Type**: Same as operands (must be integral types)
- **Overflow Behavior**: Clamps to min/max of result type instead of wrapping
- **Examples**:
  ```c
  int8_t a = 100, b = 50;
  int8_t result = a +| b;    // Result: 127 (clamped from 150)
  
  uint8_t x = 200, y = 100;
  uint8_t mixed = x +| y;    // Result: 255 (clamped from 300)
  ```

#### Use Cases
- Audio sample mixing (prevent clipping)
- Color channel addition (prevent oversaturation)
- Signal processing (prevent wraparound artifacts)

#### Implementation Strategy
```c
// Compiler generates:
int8_t result = (a > 0 && b > 0 && a > INT8_MAX - b) 
    ? INT8_MAX 
    : ((a < 0 && b < 0 && a < INT8_MIN - b) 
        ? INT8_MIN 
        : a + b);
```

### 2. Bit Rotation Operators

Circular bit shifts using hardware ROL/ROR instructions.

#### Syntax
- `a <<< n` — Rotate left by n bits
- `a >>> n` — Rotate right by n bits

#### Semantics
- **Operands**: Integral types only
- **Rotation Amount**: 0 to type_bits-1 (modulo type width)
- **Examples**:
  ```c
  uint8_t x = 0xA5;      // 10100101
  uint8_t left = x <<< 1;  // 01001011 (0x4B)
  uint8_t right = x >>> 1; // 11010010 (0xD2)
  ```

#### Use Cases
- Sprite graphics rotation
- Pattern generation
- Hardware register bit manipulation
- Circular buffer indexing

#### Implementation Strategy
```c
// Compiler generates:
uint8_t result = (x << n) | (x >> (8 - n));
// But optimizes to single ROL/ROR instruction on 6502
```

### 3. Min/Max Operators

Cleaner syntax for common comparison operations.

#### Syntax
- `a <> b` — Minimum of a and b
- `a >< b` — Maximum of a and b

#### Semantics
- **Operands**: Any comparable types (integral, floating-point)
- **Result**: Same type as operands
- **Examples**:
  ```c
  int x = 50, y = 30;
  int min_val = x <> y;    // Result: 30
  int max_val = x >< y;    // Result: 50
  
  uint8_t clamped = value <> 255;  // Clamp to max 255
  ```

#### Use Cases
- Parameter clamping (volume, frequency, etc.)
- Collision detection (bounding boxes)
- Waveform synthesis (min/max amplitude)
- Graphics rendering (viewport clipping)

#### Implementation Strategy
```c
// Compiler generates:
int result = (x < y) ? x : y;  // For <>
int result = (x > y) ? x : y;  // For ><
// Or calls std::min/std::max at optimization level
```

### 4. Saturating Shift Operators

Bit shifts that clamp instead of wraparound.

#### Syntax
- `a <<| n` — Saturating left shift
- `a >>| n` — Saturating right shift

#### Semantics
- **Operands**: Integral types
- **Left Shift**: Clamps if shift would lose bits
- **Right Shift**: Normal arithmetic shift (sign-extends for signed)
- **Examples**:
  ```c
  int8_t a = 100;
  int8_t result = a <<| 1;   // Result: 127 (clamped from 200)
  
  uint8_t b = 255;
  uint8_t shifted = b <<| 2;  // Result: 255 (clamped from 1020)
  ```

#### Use Cases
- Fixed-point arithmetic
- Signal amplification with safety bounds
- Audio gain control
- Volume/level adjustments

#### Implementation Strategy
```c
// Compiler generates overflow check:
int8_t result = (a > 0 && a > INT8_MAX >> n) 
    ? INT8_MAX
    : (a < 0 && a < INT8_MIN >> n
        ? INT8_MIN
        : a << n);
```

## Operator Precedence

All new operators follow standard C precedence rules:
1. **Multiplicative**: `*, /, %, *|` (highest)
2. **Additive**: `+, -, +|, -|`
3. **Shift**: `<<, >>, <<<, >>>, <<|, >>|`
4. **Relational/Min/Max**: `<, >, <=, >=, <>, ><`
5. **Equality**: `==, !=` (lowest of these)

## Type Compatibility

### Saturating Arithmetic
- **Operands**: Integral types (char, short, int, long)
- **Result**: Same as operands
- **Floating-point**: Not supported (use `std::min/std::max` instead)

### Bit Rotation
- **Operands**: Integral types only
- **Result**: Same type
- **Right operand**: Automatically modulo type width

### Min/Max Operators
- **Operands**: Any comparable types
- **Result**: Same as operands
- **Mixed types**: Follow standard type promotion rules

### Saturating Shift
- **Operands**: Integral types
- **Result**: Same type
- **Right operand**: Amount in range [0, type_bits-1]

## Code Generation Strategy

### Optimization Levels
- **-O0**: Explicit inline checks for all saturating operations
- **-O1+**: Utilize 6502 BIT instruction for overflow detection
- **-O2+**: Recognize patterns and use conditional branches efficiently
- **-O3+**: Recognize and fold constant saturating operations at compile time

### Hardware Utilization
- **Rotation**: Direct ROL/ROR instruction (single byte)
- **Saturation Detection**: Use CMP + BIT instructions for overflow
- **Min/Max**: Conditional branch optimization (CLV/BVS for overflow flag)

## Testing Strategy

### Test Categories
1. **Basic Operations**: Each operator with simple values
2. **Edge Cases**: Min/max values, zero, negative numbers
3. **Type Coverage**: char, short, int, long, unsigned variants
4. **Mixed Expressions**: Combining new operators with standard C
5. **Performance**: Verify code generation is efficient
6. **Audio Use Cases**: Real effects code using new operators
7. **Graphics Use Cases**: Sprite rotation and color manipulation

### Test Files
- `test_phase19_saturating_arithmetic.cpp` (12 tests)
- `test_phase19_bit_rotation.cpp` (12 tests)
- `test_phase19_minmax_operators.cpp` (12 tests)
- `test_phase19_saturating_shift.cpp` (12 tests)
- `test_phase19_integration.cpp` (8 tests)

**Total: 56 tests**

## Integration Points

### Compiler Components
1. **Lexer** (`Lexer.cpp`): Add token recognition for `+|`, `-|`, `*|`, `<<<`, `>>>`, `<>`, `><`, `<<|`, `>>|`
2. **Parser** (`Parser.cpp`): Update operator precedence table and expression parsing
3. **AST** (`ASTNode.h`): Add operator node types for new operations
4. **Validator** (`Validator.cpp`): Type checking for operator compatibility
5. **CodeGenerator** (`CodeGenerator.cpp`): Emission logic for each operator

### Performance Targets
- No runtime overhead for operations that fit in single instruction
- Minimal branching for saturation checks
- Constant folding at compile time when possible

## Backward Compatibility

- All new operators are additions; no breaking changes to existing code
- Standard C operators unchanged in behavior
- Existing code compiles identically

## Known Limitations

1. **Saturating operations on floating-point**: Not implemented (use `std::min/std::max`)
2. **Saturation detection**: Limited to standard C types (not wide integers `__int(N)`)
3. **Rotation amount**: Must be compile-time constant for optimization (runtime OK, less optimal)

## Future Enhancements

- **Phase 20**: Vector operations using these operators
- **Phase 21**: SIMD-like batch operations for audio buffers
- **Phase 22**: Hardware intrinsics for advanced operations
