# O45 Object Format I/O Coverage Tests

**Added:** 2026-07-18  
**Status:** Initial implementation with foundation for full coverage  
**Goal:** Improve O45 I/O coverage from 40-63% to >70%

## Purpose

The O45 relocatable object format is critical to the toolchain:
- **O45Reader.cpp** (52.63%): Parses .o45 files
- **O45Linker.cpp** (58.81%): Links objects and applies relocations
- **O45Archive.cpp** (40.57%): Creates/reads static library archives
- **O45Writer.cpp** (53.64%): Generates .o45 files

Low coverage in these modules risks silent failures in multi-file projects.

## Test Suites

### `test_o45_io_coverage.sh` (Executable Test Script)

Comprehensive test script covering:

#### 1. Round-Trip Tests
- **Multi-segment objects**: Text, data, BSS, ZP segments
- **Relocation objects**: With external symbol references
- **Symbol tables**: Imports and exports
- **Options**: Tool metadata (OPT_ASM, OPT_CC)

**Current Result:** Creates and verifies objects (2/9 tests passing)

#### 2. Linker Symbol Resolution
- **Simple linking**: Main → Library linking
- **Multiple symbols**: Complex symbol tables
- **Data relocations**: Address fixup in data segments
- **Import resolution**: Resolving external symbols

**Current Result:** Tests designed, needs linker fixes for directives

#### 3. Edge Cases
- **Large symbol tables**: 50+ imports
- **Large code sections**: 10KB+ code
- **Many relocations**: 500+ relocation entries
- **Mixed relocation types**: All 8 relocation types

#### 4. Tool Integration
- **objdump45**: Object inspection
- **nm45**: Symbol table listing
- **ca45**: Assembly to object conversion
- **ln45**: Linking objects

## Running the Tests

```bash
# Build the compiler
make all -j 12

# Run the O45 I/O coverage test suite
bash src/test/test_o45_io_coverage.sh

# Expected output:
# === Coverage Improvement Report ===
# Passed: 2+
# Failed: (remaining coverage gaps to fix)
```

## Coverage Improvement Roadmap

### Phase 1: Foundation (Completed)
- ✓ Test framework created
- ✓ Round-trip write/read infrastructure
- ✓ Edge case test suite

### Phase 2: Reader Coverage (40-60%)
- [ ] Parse all relocation types correctly
- [ ] Handle symbol table edge cases
- [ ] Test error recovery and malformed files
- [ ] Large object parsing

**Target:** O45Reader.cpp 52.63% → 70%

### Phase 3: Linker Coverage (50-70%)
- [ ] Multi-object symbol resolution
- [ ] Relocation application for all types
- [ ] Segment layout and addressing
- [ ] Error handling (undefined symbols, conflicts)

**Target:** O45Linker.cpp 58.81% → 75%

### Phase 4: Archive Coverage (40-50%)
- [ ] Create and extract archives
- [ ] Multiple member handling
- [ ] Archive metadata
- [ ] Corruption detection

**Target:** O45Archive.cpp 40.57% → 65%

## Known Limitations

1. **Assembly directive gaps**: Some test directives may need tweaking for the current assembler
2. **Symbol import/export syntax**: May need adjustment for .import/.global directive behavior
3. **Relocation testing**: Requires linker implementation adjustments

## How to Extend

### Adding Round-Trip Tests

```bash
cat >> src/test/test_o45_io_coverage.sh << 'EOF'
# Test your scenario
if your_test_command ...; then
    test_passed "Your Test Description"
else
    test_failed "Your Test Description"
fi
EOF
```

### Adding Edge Cases

Focus on:
- Boundary conditions (max sizes, zero lengths)
- Error conditions (malformed files, missing symbols)
- Performance (large file handling)
- Symbol name edge cases (long names, special characters)

## Related Coverage Analysis

See also:
- `doc/COVERAGE_ANALYSIS.md` - Full code coverage report (2026-07-17)
- Coverage before tests: **O45Writer 53.64%, O45Reader 52.63%, O45Linker 58.81%, O45Archive 40.57%**
- Target after tests: **>70% across all O45 modules**

## Integration with CI

To integrate with CI/CD:

```makefile
test-o45-io: all
	@bash src/test/test_o45_io_coverage.sh

# Add to main test target:
test: ... test-o45-io ...
```

## Success Criteria

✓ All edge case tests passing  
✓ Round-trip write→read→verify for all segment types  
✓ Multi-object linking verified  
✓ Symbol resolution tested  
✓ Archive operations verified  
✓ O45 module coverage > 70%

---

**Next Steps:** 
1. Fix assembly directive syntax for test cases
2. Implement missing reader/linker test coverage
3. Add archive operation testing
4. Run against actual GCC coverage metrics to verify improvements
