# Array Element Access Bug - Investigation Summary

## Problem Statement
Arrays allocated on the stack (frame) with runtime-indexed access return 0x00 instead of correct values.

**Test case**: `short arr[3] = {100, 200, 300}; return arr[1];`
- Expected: 200
- Actual: 0x00

**Status**: 9 mmemu test failures (test_short.c, test_array_init.c, test_struct_array.c, test_array_loop.c, test_long_mmemu.c, + 4 others)

## Investigation Timeline

### Phase 1: Simulated Opcode Analysis ✓
**Finding**: struct_elem.16 and addr_elem.16 are NOT the root cause.

- struct_elem.16 works correctly (initialization succeeds)
- addr_elem.16 is never emitted by compiler (doesn't use simulated op)
- Compiler generates direct assembly instead of simulated ops
- All simulated opcode tests pass (compile/assemble successfully)

### Phase 2: Direct Assembly Analysis ✓
**Finding**: Compiler generates manual address calculation instead of simulated ops.

Pattern for array[i] with stride=2:
```asm
leax.fp 0              ; Get frame base (array start)
sta $20; stx $21       ; Save base
lda #i; ldx #0         ; Load index
sta $22; stx $23       ; Save index
lda $20; ldx $21       ; Reload base
sta $1F; stx $1F+1     ; Save base to temp
lda $22; ldx $23       ; Load index
mul.16 .AX, #2         ; Multiply index * stride
clc
adc $1F                ; Add base_lo
pha
txa
adc $1F+1              ; Add base_hi  
tax
pla
sta __zp_scratch       ; Store calculated address
ldy #0
lda (__zp_scratch),y   ; Load from calculated address → 0x00 BUG
```

### Phase 3: Initialization vs Access ✓
**Finding**: Initialization works, access fails.

**Initialization code** (struct_elem.16 with constant offset):
- Uses struct_elem.16 with immediate offsets (0, 2, 4)
- Stores array values correctly (verified: 100, 200, 300)
- No complex address calculation

**Access code** (direct assembly with mul.16):
- Calculates index * stride with mul.16
- Adds base + offset
- Reads 0x00 instead of stored value

## Root Cause Candidates

### Candidate 1: Y Register Not Initialized ❓
**Location**: Initialization code lines 79, 94, 109
**Issue**: Code does `sta (__zp_scratch),y` without preceding `ldy #0`
**Impact**: If Y ≠ 0, stores to wrong address during init
**Counter-evidence**: arr[0] and arr[2] work, only arr[1] fails. Unlikely Y varies between initializations.

### Candidate 2: mul.16 Expansion Broken ❓
**Location**: Assembly lines 126-135
**Issue**: mul.16 might not produce correct result for index * stride
**Impact**: Calculated address would be wrong
**Investigation needed**: Verify mul.16 assembly expansion

### Candidate 3: Address Calculation Carry Propagation ❓
**Location**: After mul.16, lines 127-133
**Issue**: 16-bit addition with carry might corrupt result
```asm
mul.16 .AX, #2       ; offset in .AX
clc
adc $1F              ; offset_lo + base_lo
pha
txa                  ; X (offset_hi) to A
adc $1F+1            ; offset_hi + base_hi + carry
```
**Impact**: Calculated address incorrect
**Status**: Logic looks correct, but needs verification

### Candidate 4: __zp_scratch Collision ❓
**Location**: Lines 134-135, 137
**Issue**: __zp_scratch = $08 might be clobbered by something
**Impact**: Calculated address overwritten
**Investigation needed**: Check all uses of $08-$09

### Candidate 5: Indirect Addressing Mode Failure ❓
**Location**: Line 137: `lda (__zp_scratch),y`
**Issue**: Indirect addressing might fail with certain addresses
**Impact**: Read returns 0x00 even if address is correct
**Investigation needed**: Verify with concrete memory dump

## Next Steps

1. **Get memory dump during execution**
   - Before mul.16: check base address and index
   - After mul.16: check offset value
   - After address calculation: check __zp_scratch value
   - After read: what's actually at that address?

2. **Verify mul.16 assembly**
   - Run test_array_minimal.s through assembler
   - Disassemble .prg to see actual mul.16 expansion
   - Compare with expected instruction sequence

3. **Check ZP allocations**
   - Verify $1F and $1F+1 are free
   - Verify __zp_scratch ($08) is not clobbered
   - Check if busy wait (just added) helps

4. **Create minimal assembly test**
   - Write pure 45GS02 assembly version
   - Test if works when assembly is correct
   - Isolate compiler vs assembler issue

## Test Files Created

- `test_array_minimal.c` - Minimal reproduction
- `test_array_debug.c` - Extended test patterns
- `test_array_yinit.c` - Y register initialization test
- `ARRAY_BUG_MINIMAL.md` - Detailed analysis
- `test_simop_*.s` - Simulated opcode tests (all pass)

## Key Insight

The compiler is NOT using addr_elem.16 or struct_elem.16 for array access. Instead, it generates direct 45GS02 assembly with manual address calculation. This means:

- The bug is NOT in the simulated opcode infrastructure
- The bug IS in the compiler's address calculation code generation
- Or the bug is in the assembler's mul.16 expansion
- Or both mechanisms work but there's a runtime interaction issue

The gap between working initialization (struct_elem.16) and failing access (direct assembly with mul.16) strongly suggests the issue is in the mul.16-based calculation or how the calculated address is used.
