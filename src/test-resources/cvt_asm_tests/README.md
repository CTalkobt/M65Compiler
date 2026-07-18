# cvt_asm Format Conversion Test Suite

Comprehensive test coverage for all assembler format conversions in the cvt_asm utility.

## Test Files

### Format-Specific Test Cases
- `ca45_test.asm` — ca45 (MEGA65 assembler) format
- `ca65_test.asm` — ca65 (CC65) format with local labels
- `acme_test.asm` — ACME 6502 assembler format
- `oscar_test.asm` — Oscar (vintage Commodore) assembler
- `kickassembler_test.asm` — KickAssembler format

### Coverage

Each test file covers:
- **Addressing Modes**:
  - Immediate: `lda #$20`
  - Zero Page: `lda $80`
  - Absolute: `lda $D000`
  - Indexed X: `lda table, X`
  - Indexed Y: `lda ($80), Y`
  - Indirect: `lda ($80)`

- **Labels and Symbols**:
  - Code labels (main, loop, done)
  - Data labels (table)
  - Global/extern symbols (format-specific)

- **Directives**:
  - ORG (origin)
  - BYTE/DC.B (data)
  - WORD/DC.W (word data)
  - Format-specific directives (segments, namespaces)

- **Instruction Types**:
  - Arithmetic: LDA, STA, ADC, SBC
  - Logical: AND, ORA, EOR, BIT
  - Comparison: CMP, CPX, CPY
  - Branching: BNE, BEQ, JMP, JSR
  - Registers: DEX, DEY, INX, INY, TAX, TYA
  - Stack: PHA, PLA, PHP, PLP
  - Misc: NOP, RTS, RTI, BRK

## Running Tests

### Quick Test (All Pairs)
```bash
bash src/test-resources/test_cvt_asm.sh
```

Runs all 20 format-pair conversions:
- 5 input formats × 4 output formats each = 20 tests
- Expected: 20/20 passing

### Detailed Test Suite
```bash
bash src/test-resources/cvt_asm_test.sh
```

More comprehensive testing with detailed output.

## Test Results

✅ **Current Status: All 20 format-pair conversions passing**

| From | To | Status |
|------|-----|--------|
| ca45 | ca65, acme, oscar, kickassembler | ✓ 4/4 |
| ca65 | ca45, acme, oscar, kickassembler | ✓ 4/4 |
| acme | ca45, ca65, oscar, kickassembler | ✓ 4/4 |
| oscar | ca45, ca65, acme, kickassembler | ✓ 4/4 |
| kickassembler | ca45, ca65, acme, oscar | ✓ 4/4 |

## Known Limitations

### Format-Specific Features Not Preserved
- **KickAssembler**: Namespaces are flattened (`.namespace.label` → `namespace.label`)
- **ca65**: Local labels (`@label`) converted to regular labels in some formats
- **Macros**: Not preserved across formats (Phase 2 feature)

### Output Formatting
- Spacing and indentation may vary between formats
- Comment styles normalized to target format
- Directive argument formatting optimized per format

## Future Enhancements

- [ ] Round-trip fidelity tests (A → B → A equality)
- [ ] Macro preservation system
- [ ] Symbol table comparison
- [ ] Performance benchmarks
- [ ] Semantic IR validation
- [ ] Extended instruction coverage (45GS02-specific opcodes)

## Adding New Format Pair Tests

1. Create test file: `src/test-resources/cvt_asm_tests/<format>_test.asm`
2. Include coverage from this README
3. Test manually: `./bin/cvt_asm -f <format> -t <other> <file>`
4. Run test suite to verify integration
