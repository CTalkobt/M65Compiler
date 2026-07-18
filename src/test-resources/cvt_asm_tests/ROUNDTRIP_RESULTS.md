# cvt_asm Round-Trip Preservation Test Results

## Test Overview

Tests label and symbol preservation across format conversions. Each conversion is analyzed to verify that labels and symbols from the original format are present in the output format.

## Test Results Summary

**Passed: 11/20 (55%)**
**Failed: 9/20 (45%)**

## Detailed Results by Format

### ca45 (MEGA65 Assembler)
| Target | Status | Notes |
|--------|--------|-------|
| ca65 | ✅ PASS | Symbols preserved |
| acme | ❌ FAIL | ACME doesn't support symbol directives |
| oscar | ❌ FAIL | Oscar format loses directive symbols |
| kickassembler | ❌ FAIL | KickAssembler doesn't support weak symbols |

### ca65 (CC65 Assembler)
| Target | Status | Notes |
|--------|--------|-------|
| ca45 | ❌ FAIL | ca45 loses export symbols |
| acme | ❌ FAIL | ACME doesn't support symbol directives |
| oscar | ❌ FAIL | Oscar loses both labels and symbols |
| kickassembler | ❌ FAIL | KickAssembler doesn't support export |

### acme (6502 Assembler)
| Target | Status | Notes |
|--------|--------|-------|
| ca45 | ✅ PASS | Labels preserved perfectly |
| ca65 | ✅ PASS | Labels preserved, no symbols to preserve |
| oscar | ❌ FAIL | Oscar writer doesn't output labels correctly |
| kickassembler | ✅ PASS | Labels preserved |

### oscar (Commodore Assembler)
| Target | Status | Notes |
|--------|--------|-------|
| ca45 | ✅ PASS | Labels extracted (includes ORG, WORD) |
| ca65 | ✅ PASS | Labels extracted |
| acme | ✅ PASS | Labels extracted |
| kickassembler | ✅ PASS | Labels extracted |

### kickassembler
| Target | Status | Notes |
|--------|--------|-------|
| ca45 | ✅ PASS | Labels preserved |
| ca65 | ✅ PASS | Labels preserved |
| acme | ✅ PASS | Labels preserved |
| oscar | ❌ FAIL | Oscar writer doesn't output labels correctly |

## Known Issues

### 1. Symbol Directive Loss
Some formats don't support symbol directives:
- **ACME**: No .extern, .global, .weak support
- **Oscar**: Limited symbol support
- **KickAssembler**: Different symbol syntax

**Impact**: Symbols are lost when converting to formats with limited directive support

**Mitigation**: Use intermediate format (ca65) that preserves all symbols

### 2. Oscar Writer Label Issues
Oscar format sometimes doesn't properly output labels when receiving input from certain formats.

**Status**: Known issue, marked for investigation

### 3. Format-Specific Features
Some formats have unique features not representable in other formats:
- KickAssembler namespaces (`.ns.label`) flatten to `ns.label`
- ca65 local labels (`@label`) become regular labels
- ca45 frame-relative addressing `.fp var` may not convert

**Impact**: Low - feature-specific conversions may lose structure but retain semantics

## Recommendations

### For Symbol Preservation
1. **Best path**: Format A → ca65 → Format B
   - ca65 is most complete format for symbols
   - Preserves .export, .import (extern) correctly

2. **For labels only**: Any format → ca45 or kickassembler → target
   - High label preservation rate
   - Good label fidelity across formats

### For Full Fidelity
- Avoid: ca45 → acme, oscar → kickassembler, ca65 → oscar
- These paths lose symbol or label information

## Future Work

- [ ] Fix Oscar writer label output
- [ ] Enhance symbol support in ACME/Oscar formats
- [ ] Add format-specific preservation rules
- [ ] Implement symbol translation for format-specific directives
- [ ] Round-trip validation (A → B → A = A)

## Test Script

Run preservation tests:
```bash
python3 src/test-resources/test_cvt_asm_roundtrip.py
```

Test files analyzed:
- `ca45_test.asm` - 6 labels, 3 symbols
- `ca65_test.asm` - 4 labels, 1 symbol
- `acme_test.asm` - 4 labels, 0 symbols
- `oscar_test.asm` - 4 labels, 0 symbols
- `kickassembler_test.asm` - 4 labels, 0 symbols
