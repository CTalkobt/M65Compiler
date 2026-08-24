# Phase 29: DMA-Accelerated Far Pointer Operations

**Status**: Design & Architecture  
**Target Version**: v1.0.19+  
**Priority**: Critical (Real-Time Performance)  
**Builds On**: Phase 26-28 Far Pointers, Phase 23 DMA Intrinsics

## Overview

Phase 29 replaces **byte-by-byte cross-bank transfers** with **hardware DMA acceleration**, reducing cross-bank `memcpy` from ~500 µs (256 bytes) to ~5 µs — a **100x speedup** critical for real-time audio, graphics, and data processing.

### The Problem

Phase 27's cross-bank `far_memcpy()` is safe but slow:

```c
// Current Phase 27 (byte-by-byte)
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        uint8_t byte = far_read_uint8(src, i);   // Bank switch + read
        far_write_uint8(dst, i, byte);            // Bank switch + write
    }
}
// Cost: ~2000+ cycles per 256 bytes (~500 µs at 1 MHz)
```

**Why it's slow**: Each byte requires two bank switches (read source, write dest). At ~1-2 cycles per bank switch + ~2 cycles per access = ~5 cycles per byte.

### The Solution

Use MEGA65's F018B DMA controller to transfer blocks in parallel with CPU:

```c
// Phase 29 (DMA-accelerated)
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    if (far_bank(src) == far_bank(dst)) {
        // Same bank: direct memcpy (fast, no DMA needed)
        bank_t bank = far_bank(src);
        memcpy_in_bank(bank, far_offset(dst), far_offset(src), len);
    } else {
        // Cross-bank: use DMA hardware
        dma_transfer_cross_bank(
            far_bank(dst), far_offset(dst),
            far_bank(src), far_offset(src),
            len
        );
    }
}
// Cost: ~10-20 cycles for DMA setup + ~50 cycles transfer (~10 µs at 1 MHz)
// Speedup: 50-100x vs byte-by-byte
```

---

## MEGA65 F018B DMA Architecture

### DMA Registers (I/O Space $DE00-$DFFF)

```
$DE00-$DE02  DMAgic Command
$DE04-$DE06  DMA Length (bytes to transfer)
$DE07-$DE09  DMA Source Address
$DE0A-$DE0C  DMA Dest Address
$DE0D        DMA Source Bank
$DE0E        DMA Dest Bank
$DE0F        DMA Dest Address Skip
$DE10        DMA Source Address Skip
$DE11        DMA List Address / Start
$DE12        DMA List Address Skip
```

### DMA Command Byte ($DE00)

```
Bit 7: Enable
Bit 6-5: Command (00=copy, 01=fill, 10=swap, 11=reserved)
Bit 4: Chain (1 = chain to next list entry)
Bit 3: Interrupt (1 = generate interrupt on completion)
Bit 2-0: Direction (000=source→dest, etc.)
```

### Example: Copy 256 bytes from BANK1:0x1000 to BANK2:0x1000

```asm
; Setup DMA
lda #0x81             ; Copy command, enable
sta $de00

lda #<256             ; Length low
sta $de04
lda #>256             ; Length high
sta $de05

lda #<0x1000          ; Source address low
sta $de07
lda #>0x1000          ; Source address high
sta $de08

lda #<0x1000          ; Dest address low
sta $de0a
lda #>0x1000          ; Dest address high
sta $de0b

lda #1                ; Source bank (BANK1)
sta $de0d

lda #2                ; Dest bank (BANK2)
sta $de0e

lda #0                ; Start transfer
sta $de11
```

---

## Phase 29 Implementation Strategy

### Layer 1: Low-Level DMA Controller

```c
/* mega65/dma_controller.h */

typedef struct {
    uint16_t src_addr;      /* Source address (within bank) */
    uint8_t src_bank;       /* Source bank (0-7) */
    uint16_t dst_addr;      /* Destination address */
    uint8_t dst_bank;       /* Destination bank */
    uint16_t length;        /* Bytes to transfer */
} dma_request_t;

void dma_execute(const dma_request_t* req);
int dma_is_busy(void);
void dma_wait(void);
```

### Layer 2: DMA Helpers for Far Pointers

```c
/* In far_ops.c: Add DMA path */

void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    bank_t src_bank = far_bank(src);
    bank_t dst_bank = far_bank(dst);

    if (src_bank == dst_bank) {
        /* Same bank: direct memcpy (fast) */
        bank_t saved = save_bank();
        restore_bank(src_bank);
        memcpy((void*)far_offset(dst),
               (void*)far_offset(src), len);
        restore_bank(saved);
    } else if (len >= DMA_THRESHOLD) {
        /* Cross-bank, large: use DMA (very fast) */
        dma_request_t req = {
            .src_addr = far_offset(src),
            .src_bank = src_bank,
            .dst_addr = far_offset(dst),
            .dst_bank = dst_bank,
            .length = len
        };
        dma_execute(&req);
        dma_wait();
    } else {
        /* Cross-bank, small: byte-by-byte (safe) */
        for (size_t i = 0; i < len; ++i) {
            uint8_t byte = far_read_uint8(src, i);
            far_write_uint8(dst, i, byte);
        }
    }
}

#define DMA_THRESHOLD 64  /* Use DMA for transfers > 64 bytes */
```

### Layer 3: Automatic Selection

Phase 29 implements an **adaptive strategy**:

```
Transfer Size Analysis
├─ < 32 bytes:    Byte-by-byte (low overhead)
├─ 32-64 bytes:   Mixed (memcpy if same bank, byte-wise if cross)
├─ 64-256 bytes:  DMA (amortizes setup overhead)
└─ > 256 bytes:   DMA (maximum benefit)

DMA Setup Cost: ~50 cycles
DMA Overhead: ~1-2 cycles per byte
Byte-by-Byte Cost: ~5 cycles per byte

Breakeven: ~15-20 bytes (use simple byte-wise)
DMA Wins For: ~64+ bytes (10-100x speedup)
```

---

## Integration with Far Operations

### Current Phase 27 Implementation

```c
// Phase 27: Safe but slow
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        uint8_t byte = far_read_uint8(src, i);
        far_write_uint8(dst, i, byte);
    }
}
// Cross-bank 256B: ~2000 cycles (~500 µs)
```

### New Phase 29 Implementation

```c
// Phase 29: DMA-accelerated
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    if (src_bank == dst_bank) {
        // Same bank: direct (unchanged)
        bank_t saved = save_bank();
        memcpy(dst_offset, src_offset, len);
        restore_bank(saved);
    } else if (len >= 64) {
        // Cross-bank + large: DMA (new)
        dma_request_t req = {
            .src_addr = far_offset(src),
            .src_bank = far_bank(src),
            .dst_addr = far_offset(dst),
            .dst_bank = far_bank(dst),
            .length = len
        };
        dma_execute(&req);
    } else {
        // Cross-bank + small: byte-wise (fallback)
        for (size_t i = 0; i < len; ++i) {
            uint8_t byte = far_read_uint8(src, i);
            far_write_uint8(dst, i, byte);
        }
    }
}
// Cross-bank 256B: ~100 cycles (~25 µs) — 50x faster!
```

---

## DMA Synchronization

### Synchronous Transfer (Simple)

```c
void dma_execute_sync(const dma_request_t* req) {
    write_dma_registers(req);
    start_dma();
    while (dma_is_busy()) {
        /* Spin until done */
    }
}
// Simple but blocks CPU
// Use for small transfers or non-realtime code
```

### Asynchronous Transfer (Advanced, Future Phase)

```c
// Phase 29 does NOT implement async (too complex for now)
// Future: DMA completion interrupt, callback queue

void dma_execute_async(const dma_request_t* req,
                       void (*callback)(void)) {
    queue_dma_request(req, callback);
    enable_dma_interrupt();
}
// CPU free during transfer
// Must handle synchronization carefully
```

---

## Real-World Example: Audio Mixing with DMA

### Before Phase 29

```c
// Cross-bank audio mixing (slow)
far_ptr_t ch1 = far_malloc(512, BANK1);
far_ptr_t ch2 = far_malloc(512, BANK2);  // Different bank!
far_ptr_t mix = far_malloc(512, BANK3);

// Mixing: copy ch1 to mix (byte-by-byte, ~500 µs)
far_memcpy(mix, ch1, 512);  // Same bank, fast (10 µs)

// But copying ch2 is slow:
for (int i = 0; i < 256; ++i) {
    int16_t s = far_read_int16(ch2, i * 2);  // Slow
    int16_t m = far_read_int16(mix, i * 2);
    far_write_int16(mix, i * 2, (int16_t)(s + m));  // Slow
}
// Total: ~1000+ µs (too slow for 5.3ms audio frame @ 48kHz)
```

### After Phase 29

```c
// Smart DMA mixing (fast)
far_ptr_t ch1 = far_malloc(512, BANK1);
far_ptr_t ch2 = far_malloc(512, BANK1);  // Phase 99 suggests co-locate
far_ptr_t ch3 = far_malloc(512, BANK1);

// All same bank: fast direct access
far_memcpy(mix, ch1, 512);  // 10 µs (direct)

for (int i = 0; i < 256; ++i) {
    int16_t s2 = far_read_int16(ch2, i * 2);  // Fast (same bank)
    int16_t s3 = far_read_int16(ch3, i * 2);  // Fast (same bank)
    int16_t m = far_read_int16(mix, i * 2);
    far_write_int16(mix, i * 2, saturate_add(saturate_add(m, s2), s3));
}
// Total: ~50 µs (now real-time safe!)

// If data must be in different banks, DMA helps:
// far_memcpy(mix, ch1_other_bank, 512);  // 25 µs (DMA!)
```

---

## Performance Analysis

### Transfer Speed Comparison

```
Transfer 256 bytes (int16_t audio frame)

Strategy                 Time        Cycles @ 1MHz
──────────────────────────────────────────────────
CPU byte-by-byte         500 µs      500
CPU word-by-word         250 µs      250
DMA transfer            10 µs        10
DMA + setup            25 µs        25

Speedup (DMA vs byte):   20-50x
Speedup (DMA vs word):   10-25x
```

### Real-Time Constraints (48kHz Audio)

```
Sample rate: 48 kHz
Samples/frame: 256
Frame time: 5.3 ms

Budget per channel (4 channels):
  Frame load:      50 µs (DMA copy from disk)
  Processing:     100 µs (mixing, effects)
  Output:          50 µs (DMA to SID)
  ───────────────
  Total:          200 µs per channel
  
Remaining budget: 5.1 ms (96% free for synthesis!)
```

With DMA acceleration, real-time audio with multiple banks becomes practical.

---

## Compatibility and Fallback

### DMA Availability Check

```c
int dma_available(void) {
    /* Some MEGA65 configurations may lack DMA
       Check hardware revision / feature flags */
    return (megainfo->dma_present == 1);
}
```

### Graceful Degradation

```c
void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    if (!dma_available() || len < DMA_THRESHOLD) {
        // Fallback to byte-by-byte (always works)
        byte_wise_copy(dst, src, len);
        return;
    }
    
    // DMA available and worth using
    dma_transfer(dst, src, len);
}
```

All Phase 29 changes are **backward compatible**. If DMA is unavailable, Phase 27's byte-by-byte code still works.

---

## Testing Strategy

### Unit Tests (Phase 29)

1. **DMA Register Setup** (6 tests)
   - Correct register configuration
   - Bank parameter validation
   - Length limits

2. **Synchronization** (4 tests)
   - DMA completion detection
   - Busy-wait correctness
   - Timeout handling

3. **Transfer Correctness** (8 tests)
   - Same-bank (should use memcpy)
   - Cross-bank small (should use byte-wise)
   - Cross-bank large (should use DMA)
   - Data integrity verification

4. **Performance** (5 tests)
   - Speedup measurement (DMA vs byte-wise)
   - Overhead quantification
   - Threshold validation

5. **Edge Cases** (5 tests)
   - Zero-length transfers
   - Bank boundary alignment
   - DMA unavailable graceful fallback
   - Multiple concurrent transfers (not supported yet)

### Integration Tests

1. **With Audio Processing**
   - Multi-bank channel mixing
   - Real-time frame processing
   - CPU latency measurement

2. **With Graphics**
   - Sprite buffer loading
   - Screen buffer updates
   - Z-buffer operations

### Benchmarks

```
Target: 256-byte cross-bank transfer
  Baseline (Phase 27 byte-wise): ~500 µs
  Target (Phase 29 DMA):         ~10-25 µs
  Success criteria:              > 10x speedup
```

---

## Integration Points

### With Phase 26 (Allocation)
- DMA respects bank assignments
- Validates cross-bank transfers

### With Phase 27 (Operations)
- Enhances `far_memcpy()` with DMA path
- Falls back to byte-wise for small transfers
- Other operations (read, write, strlen) use Phase 27 unchanged

### With Phase 28 (Overloads)
- `memcpy(far_ptr, far_ptr, size)` now DMA-accelerated
- Transparent performance improvement

### With Phase 23 (DMA Intrinsics)
- Reuses DMA register definitions
- Integrates with existing DMA infrastructure
- Coordinates with DIGI audio hardware

### With Phase 99 (Bank Optimization)
- Benefits from co-located data placement
- Reduces need for cross-bank transfers
- Synergistic: co-location + DMA = maximum performance

---

## Known Limitations (Phase 29)

### Single Transfer Only

```c
// Phase 29: Sequential only
dma_execute(&req1);
dma_execute(&req2);  // Must wait for req1 first
```

Multiple concurrent DMA transfers not supported (hardware limitation).

### Synchronous Only

Phase 29 implements blocking DMA. Async completion callbacks deferred to Phase 30+.

### Cache Coherency

MEGA65 doesn't have data cache, so no coherency issues (unlike x86).

### Memory Barriers

DMA completion is a full memory barrier (safe for shared data).

---

## Future Enhancements

### Phase 30: Async DMA with Callbacks
- Non-blocking transfers
- Completion interrupt handling
- Queued requests

### Phase 31: Optimized Bank Assignment
- Integrate with Phase 99 hints
- Minimize cross-bank transfers
- Automatic placement analysis

### Phase 32: Hardware Prefetching
- Speculative loading of next frame
- Hide DMA latency for streaming workloads

---

## Architecture Summary

```
User Code
    │
    ├─ far_memcpy(dst, src, len)
    │
    └─ Dispatcher:
        ├─ Same bank? → Direct memcpy (fast)
        ├─ Large cross-bank? → DMA (very fast)
        └─ Small cross-bank? → Byte-wise (safe)
            │
            ├─ DMA Path
            │   ├─ Setup registers
            │   ├─ Start transfer
            │   └─ Wait for completion
            │
            └─ Byte-Wise Path
                ├─ far_read_uint8
                ├─ far_write_uint8
                └─ (repeat for each byte)
```

---

## References

- **MEGA65 F018B DMA**: Hardware documentation
- **Phase 26-28**: Far pointer infrastructure
- **Phase 23**: DMA intrinsics
- **Real-Time Audio**: 48kHz constraints (5.3ms frames)

---

**Phase 29 accelerates far pointer operations from good to exceptional!** 🚀

With DMA hardware providing 50-100x speedup on cross-bank transfers, real-time audio mixing, graphics rendering, and data processing across MEGA65's extended memory becomes practical and efficient. When combined with Phase 99 bank optimization (minimizing cross-bank transfers), the system achieves near-native performance despite memory banking.
