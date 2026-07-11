# Simulated Opcode Tests: addr_elem.16 and struct_elem.16

## Overview

Comprehensive test suite for validating the `addr_elem.16` and `struct_elem.16` simulated opcodes covering all documented API interfaces.

## Test Files

### 1. `test_simop_minimal.s`
**Purpose**: Basic syntax validation  
**Coverage**: 
- `struct_elem.16 dest, #base_symbol, #offset`
- `addr_elem.16 dest, #base_symbol, index_register, #stride`

**Expected**: Assembles without error  
**Status**: ✓ Working

### 2. `test_simop_comprehensive.s`  
**Purpose**: Full API coverage with 12 test cases  
**Coverage**:

#### struct_elem.16 Tests (5 cases)
1. Global base (#symbol), constant offset → address calculation
2. ZP base address, constant offset
3. Register base (.AX), constant offset  
4. Zero offset (edge case)
5. Large offset (6+ bytes)

#### addr_elem.16 Tests (7 cases)
6. Register index (.AX), stride=1
7. ZP index address, stride=1
8. Stride=2 (16-bit elements)
9. Stride=4 (32-bit elements)
10. Index=0 (edge case)
11. Large index value (7+)
12. Different destination ZP location

**Expected**: All address calculations correct, loads proper byte values  
**Status**: ⚠️ Syntax error (needs .byte syntax fix)

### 3. `test_simop_validation.c`
**Purpose**: C-based validation of array access correctness  
**Coverage**:
- Validates byte arrays work correctly
- Validates struct arrays work correctly
- Error tracking per test case
- Output buffer: $4000-$405F

**Expected**: All array accesses return correct values  
**Status**: Ready to test

## Simulated Opcode APIs

### struct_elem.16 (Constant Offset Only)
```asm
struct_elem.16 dest, base, #offset
```

**Parameters:**
- `dest`: Destination ZP location (2 bytes) or register (.AX)
- `base`: Global (#symbol), ZP address ($zz), or register (.AX)
- `offset`: Constant value (compile-time only)

**Operation**: Calculates `address = base + offset`, stores result to dest

**Use Cases:**
- Accessing struct members (offsets are struct layout, always constant)
- Array access with constant index

### addr_elem.16 (Runtime Index)
```asm
addr_elem.16 dest, base, index, #stride
```

**Parameters:**
- `dest`: Destination ZP location (2 bytes)
- `base`: Global (#symbol), ZP address, or register (.AX)
- `index`: Register (.AX, .AXY, etc), ZP address, or expression
- `stride`: Constant multiplier (element size)

**Operation**: Calculates `address = base + (index * stride)`, stores to dest

**Use Cases:**
- Array access with runtime index: `arr[i]`
- Array of structs with runtime index: `struct_arr[i]` (stride = sizeof(struct))

## Expected Test Results

### Byte Array Tests (8 values)
```
Input:  byte_array = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 }
Test 1-6: Direct access validation
Expected: 0x11, 0x22, 0x33, 0x44, 0x55, 0x66
```

### Struct Array Tests (4 values)
```
Input:  struct_array = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }
Test 7-10: Direct access validation
Expected: 0xAA, 0xBB, 0xCC, 0xDD
```

## Validation Output Format

Output buffer at $4000:
- `$4000-$401F`: Test numbers (0-31)
- `$4020-$403F`: Expected values
- `$4030-$404F`: Actual values
- `$4040-$405F`: Pass/Fail markers (0x00=pass, 0xFF=fail)
- `$401F`: Test complete marker (0xAA)

## Known Issues

1. **test_simop_comprehensive.s**: Syntax error with `.byte` directive
   - Solution: Use inline data or generate via C compiler
   
2. **struct_elem.16 with runtime index**: Not supported (by design)
   - Use addr_elem.16 for runtime array indices
   - struct_elem.16 only for constant offsets

## Next Steps

1. Fix `.byte` syntax in test_simop_comprehensive.s
2. Run test_simop_validation.c to validate array access
3. Compare generated assembly for both opcodes:
   - struct_elem.16 expansion (constant offset path)
   - addr_elem.16 expansion (runtime index path)
4. Validate with mmemu if available

## References

- Simulated opcode dispatch: src/main/AssemblerSimulatedOps.cpp
  - dispatch_StructElem: lines 3290-3391
  - dispatch_AddrElem: lines 3393-3564
- Code generation: src/main/IRCodeGen.cpp
  - ADDR_ELEM case: lines 2970-3040
