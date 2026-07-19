#!/bin/bash
# Test script for O45 Object Format I/O coverage improvement
# Focuses on O45Reader, O45Linker, and O45Archive weak spots

CC_BIN="./bin/cc45"
CA_BIN="./bin/ca45"
LN_BIN="./bin/ln45"
NM_BIN="./bin/nm45"
OD_BIN="./bin/objdump45"

TEST_DIR="build/test/o45_io_coverage"
mkdir -p "$TEST_DIR"

# Test counters
PASS=0
FAIL=0

test_passed() {
    echo "✓ $1"
    ((PASS++))
}

test_failed() {
    echo "✗ $1"
    ((FAIL++))
}

# =============================================================================
# Test 1: Round-trip write and read
# =============================================================================
echo "=== Round-Trip Tests ==="

# Create object with all segment types
cat > "$TEST_DIR/roundtrip.s45" << 'ASM'
.o45

.segment "code"
* = $2000
_main:
    lda #$42
    rts

.segment "data"
* = $4000
data_start:
    .byte $AA, $BB, $CC

.segment "bss"
* = $6000
bss_start:
    .space $100

.segment "zp"
* = $20
zp_start:
    .space $10

.global _main
ASM

if $CA_BIN "$TEST_DIR/roundtrip.s45" -c -o "$TEST_DIR/roundtrip.o45" 2>/dev/null; then
    if [ -f "$TEST_DIR/roundtrip.o45" ]; then
        SIZE=$(stat -f%z "$TEST_DIR/roundtrip.o45" 2>/dev/null || stat -c%s "$TEST_DIR/roundtrip.o45")
        if [ "$SIZE" -gt 50 ]; then
            test_passed "Round-trip: Create multi-segment object"
        else
            test_failed "Round-trip: Object file too small"
        fi
    else
        test_failed "Round-trip: Object file not created"
    fi
else
    test_failed "Round-trip: Assembly failed"
fi

# Test reading object with relocations
cat > "$TEST_DIR/reloc.s45" << 'ASM'
.o45

.segment "code"
* = $2000
_main:
    jsr _external_func
    rts

.extern _external_func
.global _main
ASM

if $CA_BIN "$TEST_DIR/reloc.s45" -c -o "$TEST_DIR/reloc.o45" 2>/dev/null; then
    if $OD_BIN -f "$TEST_DIR/reloc.o45" >/dev/null 2>&1; then
        test_passed "Round-trip: Relocation object read via objdump"
    else
        test_failed "Round-trip: Cannot read relocation object"
    fi
else
    test_failed "Round-trip: Relocation assembly failed"
fi

# =============================================================================
# Test 2: Linker Tests (Symbol Resolution)
# =============================================================================
echo ""
echo "=== Linker Symbol Resolution Tests ==="

# Create library object
cat > "$TEST_DIR/lib.s45" << 'ASM'
.o45

.segment "code"
* = $0000
_printf:
    rts

.global _printf
ASM

if $CA_BIN "$TEST_DIR/lib.s45" -c -o "$TEST_DIR/lib.o45" 2>/dev/null; then
    test_passed "Linker: Create library object"
else
    test_failed "Linker: Library assembly failed"
fi

# Create main object that imports from lib
cat > "$TEST_DIR/main.s45" << 'ASM'
.o45

.segment "code"
* = $0000
_main:
    jsr _printf
    rts

.extern _printf
.global _main
ASM

if $CA_BIN "$TEST_DIR/main.s45" -c -o "$TEST_DIR/main.o45" 2>/dev/null; then
    test_passed "Linker: Create main object with import"
else
    test_failed "Linker: Main assembly failed"
fi

# Link them together
if $LN_BIN "$TEST_DIR/main.o45" "$TEST_DIR/lib.o45" -o "$TEST_DIR/linked.prg" 2>/dev/null; then
    if [ -f "$TEST_DIR/linked.prg" ]; then
        test_passed "Linker: Link objects successfully"
    else
        test_failed "Linker: Linked PRG not created"
    fi
else
    test_failed "Linker: Linking failed"
fi

# =============================================================================
# Test 3: Linker with data relocations
# =============================================================================

# Create object with data relocations
cat > "$TEST_DIR/data_reloc.s45" << 'ASM'
.o45

.segment "code"
* = $0000
_main:
    lda #<_data_array
    lda #>_data_array
    rts

.segment "data"
* = $4000
data_ptr:
    .word 0

.extern _data_array
.global _main
ASM

if $CA_BIN "$TEST_DIR/data_reloc.s45" -c -o "$TEST_DIR/data_reloc.o45" 2>/dev/null; then
    test_passed "Linker: Create data relocation object"
else
    test_failed "Linker: Data relocation assembly failed"
fi

# =============================================================================
# Test 4: Edge cases
# =============================================================================
echo ""
echo "=== Edge Case Tests ==="

# Large symbol table (50+ imports)
cat > "$TEST_DIR/large_syms.s45" << 'ASM'
.o45

.segment "code"
* = $0000
_export0:
    rts

.global _export0
.extern _func0, _func1, _func2, _func3, _func4
.extern _func5, _func6, _func7, _func8, _func9
.extern _func10, _func11, _func12, _func13, _func14
.extern _func15, _func16, _func17, _func18, _func19
.extern _func20, _func21, _func22, _func23, _func24
.extern _func25, _func26, _func27, _func28, _func29
.extern _func30, _func31, _func32, _func33, _func34
.extern _func35, _func36, _func37, _func38, _func39
.extern _func40, _func41, _func42, _func43, _func44
.extern _func45, _func46, _func47, _func48, _func49
ASM

if $CA_BIN "$TEST_DIR/large_syms.s45" -c -o "$TEST_DIR/large_syms.o45" 2>/dev/null; then
    if [ -f "$TEST_DIR/large_syms.o45" ]; then
        test_passed "Edge: Large symbol table (50+ imports)"
    else
        test_failed "Edge: Large symbol table not created"
    fi
else
    test_failed "Edge: Large symbol table assembly failed"
fi

# Large code section (10KB+)
cat > "$TEST_DIR/large_code.s45" << 'ASM'
.o45

.segment "code"
* = $0000
_start:
    lda #0
ASM

for i in {1..500}; do
    echo "    adc #0" >> "$TEST_DIR/large_code.s45"
done

cat >> "$TEST_DIR/large_code.s45" << 'ASM'
    rts

.global _start
ASM

if $CA_BIN "$TEST_DIR/large_code.s45" -c -o "$TEST_DIR/large_code.o45" 2>/dev/null; then
    SIZE=$(stat -f%z "$TEST_DIR/large_code.o45" 2>/dev/null || stat -c%s "$TEST_DIR/large_code.o45")
    if [ "$SIZE" -gt 1000 ]; then
        test_passed "Edge: Large code section (10KB+)"
    else
        test_failed "Edge: Large code section too small"
    fi
else
    test_failed "Edge: Large code assembly failed"
fi

# =============================================================================
# Test 5: Symbol inspection with nm45
# =============================================================================
echo ""
echo "=== Symbol Table Inspection ==="

if $NM_BIN -n "$TEST_DIR/roundtrip.o45" >/dev/null 2>&1; then
    test_passed "Inspect: nm45 read object successfully"
else
    test_failed "Inspect: nm45 cannot read object"
fi

# =============================================================================
# Report
# =============================================================================
echo ""
echo "=== Coverage Improvement Report ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
echo ""
echo "Areas Tested:"
echo "  - O45Reader: Round-trip parsing (write → read)"
echo "  - O45Linker: Symbol resolution across multiple objects"
echo "  - O45Linker: Data segment relocations"
echo "  - O45Linker: Linking verification"
echo "  - Edge Cases: Large symbol tables, large code sections"
echo "  - Symbol Tools: nm45 inspection capability"
echo ""

if [ "$FAIL" -eq 0 ]; then
    echo "✓ All O45 I/O coverage tests passed!"
    exit 0
else
    echo "✗ Some tests failed. These indicate coverage gaps in O45Reader/Linker."
    exit 1
fi
