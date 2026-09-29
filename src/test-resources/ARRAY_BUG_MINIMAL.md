# Minimal Array Element Access Bug Test Case

## Test File
`test_array_minimal.c` - Reproduces array element access bug in minimal form

## What It Tests
```c
short arr[3] = {100, 200, 300};
output[0] = arr[1];  // Should write 200, actually writes 0x00
```

## Expected Output
At memory location $4000:
- `$4000`: 0xC8 (200 low byte)
- `$4001`: 0x00 (200 high byte)
- `$4002`: 0x64 (100)
- `$4003`: 0x2C (300 low byte)
- `$4004`: 0xAA (test complete marker)

## Actual Output (BUG)
At memory location $4000:
- `$4000`: 0x00 (WRONG - should be 0xC8)
- `$4001`: 0x00
- `$4002`: 0x64 (correct)
- `$4003`: 0x2C (correct)
- `$4004`: 0xAA (correct)

## Generated Assembly Pattern

### Initialization (Working)
```asm
leax.fp 0              ; Get array base
sta $20; stx $21       ; Save base
lda #200; ldx #0       ; Load value 200
sta $22; stx $23       ; Save to temp
struct_elem.16 __zp_scratch, $20, #2  ; arr[1] offset = 2
sta (__zp_scratch),y   ; Store to arr[1]
```
**Status**: ✓ Works (initialization stores correct values)

### Access (Broken)
```asm
leax.fp 0              ; Get array base
sta $20; stx $21       ; Save base
lda #1; ldx #0         ; Load index 1
sta $22; stx $23       ; Save index
lda $20; ldx $21       ; Load base
sta $1F; stx $1F+1     ; Save base to temp
lda $22; ldx $23       ; Load index
mul.16 .AX, #2         ; Multiply by stride (index * 2)
clc; adc $1F           ; Add base + offset
pha; txa; adc $1F+1    ; Add with carry (16-bit add)
tax; pla               ; Result in .AX
sta __zp_scratch       ; Store address to __zp_scratch
ldy #0
lda (__zp_scratch),y   ; Load from address
```
**Status**: ✗ Broken (reads 0x00 instead of 0xC8)

## Key Observations

1. **Initialization works** - struct_elem.16 with constant offset stores correct values
2. **Access fails** - Runtime index access with mul.16 reads 0x00
3. **Index calculation** - Index (1) is loaded correctly to $22-$23
4. **Base calculation** - Base via leax.fp should be correct
5. **mul.16 execution** - Multiplier produces index * stride = 2
6. **Address generation** - Base + 2 should give correct address
7. **Memory read** - `lda (__zp_scratch),y` reads 0x00 instead of 0xC8

## Hypothesis

The bug is likely ONE of:
1. **mul.16 result corrupted** - Result of multiplication isn't 2
2. **Base address wrong** - leax.fp doesn't compute correct frame address
3. **Addition corrupted** - Base + offset calculation fails
4. **Address collision** - __zp_scratch points to wrong location
5. **Indirect read fails** - Memory at calculated address can't be read

## Test Integration

This test can be run via:
```bash
make test  # Will compile and assemble
```

To verify with emulator (if mmemu available):
```bash
# Check output buffer at $4000 after execution
# Expected: C8 00 64 2C AA ...
# Actual: 00 00 64 2C AA ...
```

## Related Test Cases

- `test_short.c` - Larger test with multiple data types (7 failures)
- `test_array_init.c` - Array initialization focus (3 failures)
- `test_struct_array.c` - Struct array access (2 failures)
- `test_array_loop.c` - Loop with array access (1 failure)

This minimal case isolates the core issue without confounding factors.
