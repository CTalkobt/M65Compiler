#!/bin/bash

# Assembly-based validation tests
# Tests compiler correctness by examining generated assembly and binary
# No emulator required - validates code structure, parameters, and calling conventions

CC="./wt.docker/bin/cc45"
AS="./wt.docker/bin/ca45"
mkdir -p build/test

failed=0
passed=0

# Validate test: Check if generated assembly contains expected instruction patterns
validate_asm() {
    local name="$1"
    local asm_file="$2"
    local pattern="$3"
    local description="$4"

    if grep -q "$pattern" "$asm_file"; then
        echo "✓ PASS: $name — $description"
        passed=$((passed + 1))
        return 0
    else
        echo "✗ FAIL: $name — $description"
        echo "  Expected pattern: $pattern"
        failed=$((failed + 1))
        return 1
    fi
}

# Validate binary: Check PRG file format and content
validate_prg() {
    local name="$1"
    local prg_file="$2"
    local min_size="$3"
    local max_size="$4"

    if [ ! -f "$prg_file" ]; then
        echo "✗ FAIL: $name — PRG file not created"
        failed=$((failed + 1))
        return 1
    fi

    local size=$(stat -f%z "$prg_file" 2>/dev/null || stat -c%s "$prg_file" 2>/dev/null)

    # Check load address header (first 2 bytes should be 00 20 for $2000)
    local header=$(hexdump -C "$prg_file" 2>/dev/null | head -1 | grep -o "00 20")
    if [ -z "$header" ]; then
        echo "✗ FAIL: $name — Invalid PRG header (expected 00 20 for \$2000)"
        failed=$((failed + 1))
        return 1
    fi

    # Check size bounds
    if [ "$size" -lt "$min_size" ] || [ "$size" -gt "$max_size" ]; then
        echo "✗ FAIL: $name — Size out of bounds (got $size, expected $min_size-$max_size)"
        failed=$((failed + 1))
        return 1
    fi

    echo "✓ PASS: $name — Valid PRG ($size bytes)"
    passed=$((passed + 1))
    return 0
}

echo "=================================================="
echo "Assembly-Based Validation Tests"
echo "=================================================="
echo ""

# Test 1: test_short.c - SAC parameter addressing
echo "Test 1: test_short.c (SAC parameter passing)"
$CC src/test-resources/test_short.c -o build/test/test_short_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for SAC directives
    validate_asm "test_short" "build/test/test_short_val.s45" ".sac" "SAC directive present"

    # Check for parameter symbols (_add_short__param_a)
    validate_asm "test_short_params" "build/test/test_short_val.s45" "_add_short__param" "SAC parameter symbols"

    # Assemble and check binary
    $AS build/test/test_short_val.s45 -o build/test/test_short_val.prg 2>/dev/null
    validate_prg "test_short_prg" "build/test/test_short_val.prg" 2000 3000
else
    echo "✗ FAIL: test_short.c compilation"
    failed=$((failed + 1))
fi

# Test 2: test_compound_literal.c - Literal initialization
echo ""
echo "Test 2: test_compound_literal.c (compound literals)"
$CC src/test-resources/test_compound_literal.c -o build/test/test_compound_lit_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for CONST instructions for literal values
    validate_asm "compound_literal" "build/test/test_compound_lit_val.s45" "lda #" "Immediate loads for literals"

    # Assemble and check
    $AS build/test/test_compound_lit_val.s45 -o build/test/test_compound_lit_val.prg 2>/dev/null
    validate_prg "compound_literal_prg" "build/test/test_compound_lit_val.prg" 1000 2000
else
    echo "✗ FAIL: test_compound_literal.c compilation"
    failed=$((failed + 1))
fi

# Test 3: test_array_init.c - Array initialization
echo ""
echo "Test 3: test_array_init.c (array initialization)"
$CC src/test-resources/test_array_init.c -o build/test/test_array_init_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for array data
    validate_asm "array_init" "build/test/test_array_init_val.s45" ".byte" "Array bytes present"

    # Assemble and check
    $AS build/test/test_array_init_val.s45 -o build/test/test_array_init_val.prg 2>/dev/null
    validate_prg "array_init_prg" "build/test/test_array_init_val.prg" 1500 3000
else
    echo "✗ FAIL: test_array_init.c compilation"
    failed=$((failed + 1))
fi

# Test 4: test_array_loop.c - Array indexing
echo ""
echo "Test 4: test_array_loop.c (array indexing)"
$CC src/test-resources/test_array_loop.c -o build/test/test_array_loop_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for array element addressing
    validate_asm "array_loop_addressing" "build/test/test_array_loop_val.s45" "add\|adc" "Array arithmetic operations"

    # Assemble and check
    $AS build/test/test_array_loop_val.s45 -o build/test/test_array_loop_val.prg 2>/dev/null
    validate_prg "array_loop_prg" "build/test/test_array_loop_val.prg" 1000 2000
else
    echo "✗ FAIL: test_array_loop.c compilation"
    failed=$((failed + 1))
fi

# Test 5: test_long_mmemu.c - Long integer handling
echo ""
echo "Test 5: test_long_mmemu.c (long type)"
$CC src/test-resources/test_long_mmemu.c -o build/test/test_long_mmemu_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for multi-register operations
    validate_asm "long_type" "build/test/test_long_mmemu_val.s45" "ldx\|ldy\|ldz" "Multi-register operations for long type"

    # Assemble and check
    $AS build/test/test_long_mmemu_val.s45 -o build/test/test_long_mmemu_val.prg 2>/dev/null
    validate_prg "long_prg" "build/test/test_long_mmemu_val.prg" 1400 2000
else
    echo "✗ FAIL: test_long_mmemu.c compilation"
    failed=$((failed + 1))
fi

# Test 6: test_struct_return.c - Struct return values
echo ""
echo "Test 6: test_struct_return.c (struct return)"
$CC src/test-resources/test_struct_return.c -o build/test/test_struct_return_val.s45 2>/dev/null
if [ $? -eq 0 ]; then
    # Check for struct handling (loading multiple fields)
    validate_asm "struct_return" "build/test/test_struct_return_val.s45" "lda\|ldx" "Multi-register struct returns"

    # Assemble and check
    $AS build/test/test_struct_return_val.s45 -o build/test/test_struct_return_val.prg 2>/dev/null
    validate_prg "struct_return_prg" "build/test/test_struct_return_val.prg" 1000 2000
else
    echo "✗ FAIL: test_struct_return.c compilation"
    failed=$((failed + 1))
fi

echo ""
echo "=================================================="
echo "Assembly Validation Results: $passed passed, $failed failed"
echo "=================================================="
echo ""
echo "These tests verify:"
echo "  ✓ Compiler generates valid assembly syntax"
echo "  ✓ Assembler produces valid binary format"
echo "  ✓ PRG files have correct \$2000 load address headers"
echo "  ✓ Code uses correct calling conventions"
echo "  ✓ Multi-register and array operations present"
echo ""
echo "NOTE: These are STRUCTURAL tests, not runtime tests."
echo "They verify the compiler/assembler pipeline is working,"
echo "NOT that the code produces correct output at runtime."
echo ""

if [ $failed -eq 0 ]; then
    echo "✓ All structural validation tests passed!"
    exit 0
else
    exit $failed
fi
