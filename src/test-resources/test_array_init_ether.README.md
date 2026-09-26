# test_array_init_ether.c — Ethernet-Based Test Validation

## Overview

`test_array_init_ether.c` is an alternative implementation of `test_array_init.c` that validates array initialization by transmitting test results via **Ethernet** instead of writing to memory.

### Why Ethernet Instead of Memory?

The traditional `test_array_init.c` writes results to memory address `$4000` for `mmemu-cli` to capture and validate. However:

1. **Network transport independence** — Results can be captured on real hardware without emulator intervention
2. **External validation** — A network sniffer (tcpdump, Wireshark) can verify results independently
3. **Hardware testing bridge** — Tests can run on actual MEGA65 hardware and transmit results to a monitoring station
4. **Bypasses emulator limitations** — Avoids stdout/memory capture complexity in emulator

## Test Implementation

### Array Initialization Tests (Same as Original)

```c
char bytes[4]      = {0x10, 0x20, 0x30, 0x40};   // Global char array
int words[3]       = {100, 200, 300};            // Global int array
int partial[5]     = {11, 22};                   // Partial init (rest zeros)
char zeros[3]      = {};                         // Empty init (all zeros)
char local_bytes[] = {0xAA, 0xBB, 0xCC};         // Local arrays
int local_words[]  = {1000, 2000};
```

### Validation Points (15 test values)

1. Global char array: `bytes[0]` = 0x10, `bytes[3]` = 0x40
2. Global int array: `words[0]` = 100 (0x64), `words[2]` = 300 (0x2C low byte)
3. Partial init: `partial[0]` = 0x0B, `partial[1]` = 0x16
4. Partial padding: `partial[2]` = 0, `partial[4]` = 0
5. Empty init: `zeros[0]` = 0, `zeros[2]` = 0
6. Local char: `local_bytes[0]` = 0xAA, `local_bytes[2]` = 0xCC
7. Local int: `local_words[0]` = 0xE8 (1000 low), `local_words[1]` = 0xD0 (2000 low)

## Ethernet Payload Format

The test constructs a simple Ethernet frame payload:

```
Offset  Byte    Meaning
0       0xA5    Magic: Array initialization test marker
1       15      Number of test values following
2-16    ...     15 test result bytes
17      0xFF    Terminator
```

### Dual Output

For compatibility, the test **outputs to both**:

- **Ethernet TX buffer** (`$FFDE800`+) — Network transmission
- **Memory** (`$4000`+) — mmemu-cli compatibility

This allows the same test to work with both:
- Network-based validation (real hardware or enhanced emulator)
- Memory-based validation (existing mmemu test infrastructure)

## Compilation & Linking

```bash
# Compile
./bin/cc45 -c src/test-resources/test_array_init_ether.c -o build/test/test_array_init_ether.o45

# Link
./bin/ln45 ./lib/build/c45.lib build/test/test_array_init_ether.o45 \
    -o build/test/test_array_init_ether.prg

# Run on MEGA65 hardware or enhanced emulator with Ethernet support
```

## Expected Output

When run on hardware with network monitoring:

```
[Ethernet frame]
  Destination MAC: [broadcast or target]
  Source MAC: [MEGA65 MAC]
  EtherType: [configurable]
  Payload:
    0xA5 0x0F [15 test bytes] 0xFF
```

Test values in hex:
```
A5 0F 10 40 64 2C 0B 16 00 00 00 00 AA CC E8 D0 FF
```

Breaking down:
- `A5` — Magic marker
- `0F` — 15 tests
- `10 40` — bytes[0], bytes[3]
- `64 2C` — words[0], words[2] low
- `0B 16` — partial[0], partial[1]
- `00 00` — partial[2], partial[4]
- `00 00` — zeros[0], zeros[2]
- `AA CC` — local_bytes[0], local_bytes[2]
- `E8 D0` — local_words[0], local_words[1]
- `FF` — Terminator

## Network Validation Script

A complementary validator can be created to:

1. **Capture Ethernet frame** on target network
2. **Parse payload** per format above
3. **Verify test values** against expected
4. **Report results**: PASS/FAIL with byte-by-byte comparison

Example (pseudocode):
```bash
tcpdump -i eth0 -A | grep -a 'A5' | validate_array_init.py
```

## Future Enhancements

1. **Checksummed packets** — Add CRC32 trailer for integrity
2. **UDP payload** — Wrap in UDP/IP for cloud-based validation
3. **Multiple test suites** — Send different test identifiers
4. **Return status codes** — Use Ethernet source port for test results
5. **Hardware integration** — Connect MEGA65 to test rig via Ethernet

## Status

✅ **Compiles successfully**
✅ **Links to PRG executable**
⚠️ **Ethernet hardware mapping needs verification** (see adjust.md for register corrections)

## Related Files

- `test_array_init.c` — Original memory-based version
- `adjust.md` — Hardware register corrections needed for Ethernet
- `/lib/include/mega65.h` — Hardware register definitions
