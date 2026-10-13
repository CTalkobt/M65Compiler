#!/bin/bash

# Test script for ca45 assembler specific features (directives, etc.)

AS="./bin/ca45"
mkdir -p build/test

failed=0
passed=0

test_file() {
    local f=$1
    local bin_file="build/test/$(basename "$f" .s45).bin"
    
    echo "Testing $f..."
    if $AS "$f" -o "$bin_file"; then
        echo "SUCCESS: $f"
        passed=$((passed + 1))
    else
        echo "FAIL: Assembly failed for $f"
        failed=$((failed + 1))
    fi
}

# List of assembler feature tests
AS_TEST_FILES=(
    "src/test-resources/test_basic_upstart.s45"
    "src/test-resources/test_data_types.s45"
    "src/test-resources/test_expr_minus.s45"
    "src/test-resources/test_mul.s45"
    "src/test-resources/test_div.s45"
    "src/test-resources/test_expr.s45"
    "src/test-resources/test_expr_advanced.s45"
    "src/test-resources/hello_call.s45"
    "src/test-resources/test_var.s45"
    "src/test-resources/test_45gs02.s45"
    "src/test-resources/test_equates_advanced.s45"
    "src/test-resources/test_flat_z_all.s45"
    "src/test-resources/test_accumulator_labels.s45"
    "src/test-resources/test_multi_org.s45"
    "src/test-resources/test_peephole.s45"
    "src/test-resources/test_simulated_ops.s45"
    "src/test-resources/test_fill.s45"
    "src/test-resources/test_fill_advanced.s45"
    "src/test-resources/test_segments.s45"
    "src/test-resources/test_res.s45"
    "src/test-resources/test_pushpop.s45"
    "src/test-resources/test_addr_helpers.s45"
    "src/test-resources/test_shifts_16.s45"
    "src/test-resources/test_new_16bit_ops.s45"
    "src/test-resources/test_array.s45"
    "src/test-resources/test_expressions.s45"
)

for f in "${AS_TEST_FILES[@]}"; do
    test_file "$f"
done

# Specific check for basicUpstart output format
echo "Verifying .basicUpstart output bytes..."
# Expecting: 0b 08 0a 00 9e 32 30 36 31 00 00 00 (for START at $080D = 2061)
ACTUAL=$(hexdump -v -n 12 -e '1/1 "%02x "' build/test/test_basic_upstart.bin | xargs)
EXPECTED="0b 08 0a 00 9e 32 30 36 31 00 00 00"

if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: .basicUpstart output matches expected format."
else
    echo "FAIL: .basicUpstart output mismatch."
    echo "  Expected start: $EXPECTED"
    echo "  Actual:         $ACTUAL"
    failed=$((failed + 1))
fi

# Expressions: subtraction, unary minus, 0x/0b, data directive expressions
echo "Verifying directive bytes (test_directives.s45)..."
$AS src/test-resources/test_directives.s45 -o build/test/test_directives.bin >/dev/null 2>&1
ACTUAL=$(hexdump -v -e '1/1 "%02x "' build/test/test_directives.bin | xargs)
EXPECTED="ea ea ea 55 55 11 11 02 09 ff ff ff 06 06 06 06 00 00 ad 12 00 bd 12 00 a5 12 1a 20"
if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: directive bytes match."
else
    echo "FAIL: directive bytes mismatch."
    echo "  Expected: $EXPECTED"
    echo "  Actual:   $ACTUAL"
    failed=$((failed + 1))
fi
# .basicUpstart: 5-digit SYS for an address of 10000 or more
printf '.basicUpstart 10000\n* = 10000\n rts\n' > build/test/test_bu5.s45
$AS build/test/test_bu5.s45 -o build/test/test_bu5.prg >/dev/null 2>&1
ACTUAL=$(hexdump -v -n 15 -e '1/1 "%02x "' build/test/test_bu5.prg | xargs)
if [ "$ACTUAL" == "01 20 0c 20 0a 00 9e 31 30 30 30 30 00 00 00" ]; then
    echo "SUCCESS: .basicUpstart 10000 stub."
else
    echo "FAIL: .basicUpstart 10000 stub: $ACTUAL"
    failed=$((failed + 1))
fi

echo "Verifying expression bytes (test_expr_minus.s45)..."
ACTUAL=$(hexdump -v -e '1/1 "%02x "' build/test/test_expr_minus.bin | xargs)
EXPECTED="a9 04 a9 06 a9 04 a9 05 a9 01 a9 10 a9 05 a9 ff 03 fe 01 03 00 06 00 ff ff 3f 42 0f 00 81 80 00 00 00 c3 2d 78 eb c6"
if [ "$ACTUAL" == "$EXPECTED" ]; then
    echo "SUCCESS: expression bytes match."
else
    echo "FAIL: expression bytes mismatch."
    echo "  Expected: $EXPECTED"
    echo "  Actual:   $ACTUAL"
    failed=$((failed + 1))
fi
if $AS src/test-resources/test_expr_minus.s45 -o build/test/test_expr_minus.bin 2>&1 | grep -q "overflow"; then
    echo "FAIL: #-1 warns about an 8-bit overflow"
    failed=$((failed + 1))
fi

echo "Testing -Dcc45.zeroPageStart=\$10..."
# Use a dereference which is guaranteed to use ZP scratch $02/$03
echo -e ".var ptr = \$20\nexpr .A, *ptr" > build/test_zp_shift.s45
$AS "-Dcc45.zeroPageStart=\$10" -o build/test_zp_shift.bin build/test_zp_shift.s45
# 85 10 (STA $10) should be in there (saving eval'd ptr address to ZP scratch)
ACTUAL_ZP=$(hexdump -v -e '1/1 "%02x "' build/test_zp_shift.bin)
if [ -f build/test_zp_shift.bin ] && echo "$ACTUAL_ZP" | grep -q "85 10"; then
    echo "SUCCESS: -Dcc45.zeroPageStart correctly shifted ZP addresses."
else
    echo "FAIL: -Dcc45.zeroPageStart did not shift ZP addresses correctly."
    echo "Actual bytes: $ACTUAL_ZP"
    failed=$((failed + 1))
fi

echo "Testing .global / .extern in an object (test_global_extern.s45)..."
if $AS -c src/test-resources/test_global_extern.s45 -o build/test/test_global_extern.o45 >/dev/null 2>&1 &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " U _printf" &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " T _main" &&
   ./bin/nm45 build/test/test_global_extern.o45 | grep -q " D _count"; then
    echo "SUCCESS: .global exports and .extern imports in the object"
else
    echo "FAIL: test_global_extern.s45 as an object"
    failed=$((failed + 1))
fi

echo "Testing a used .extern in a flat program..."
# no linker resolves it: an error (it would be $0000); declared but unused is fine,
# and -c keeps it as an import
printf '.extern _used, _unused\n* = $2000\n    jsr _used\n    rts\n' > build/test_flat_extern.s45
if $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin 2>&1 | grep -q "'_used' is .extern but no linker resolves it" &&
   ! $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin >/dev/null 2>&1 &&
   ! $AS build/test_flat_extern.s45 -o build/test_flat_extern.bin 2>&1 | grep -q "_unused" &&
   $AS -c build/test_flat_extern.s45 -o build/test_flat_extern.o45 >/dev/null 2>&1; then
    echo "SUCCESS: a used .extern in a flat program is an error"
else
    echo "FAIL: a used .extern in a flat program should be an error (and only that one)"
    failed=$((failed + 1))
fi

# Unknown directives are ignored with a warning (once per line); a known one
# in upper case, #include, conditional assembly and .asciiz get a hint
cat > build/test_unknown_dir.s45 << 'EOF'
    .org $2000
    .bogus 1
    .BYTE 1
    .if 1
    .asciiz "x"
    .byte 2
EOF
OUT=$($AS build/test_unknown_dir.s45 -o build/test_unknown_dir.bin 2>&1)
if echo "$OUT" | grep -q "2:5: warning: unknown directive '.bogus' is ignored" &&
   echo "$OUT" | grep -q "directives are lower case: '.byte'" &&
   echo "$OUT" | grep -q "ca45 has no conditional assembly" &&
   echo "$OUT" | grep -q "use .ascii" &&
   [ "$(echo "$OUT" | grep -c 'unknown directive')" = "4" ] &&
   [ "$(xxd -p build/test_unknown_dir.bin)" = "02" ]; then
    echo "SUCCESS: unknown directives warn and are ignored"
    passed=$((passed + 1))
else
    echo "FAIL: unknown directive warnings ($OUT)"
    failed=$((failed + 1))
fi

# A simulated instruction that can't relocate an address in a relocatable
# segment is an error with -c, not a wrong address
cat > build/test_simop_reloc.s45 << 'EOF'
        .data
_w:     .word 5
        .code
proc _f
        neg.16 _w
endproc
EOF
OUT=$($AS -c build/test_simop_reloc.s45 -o build/test_simop_reloc.o45 2>&1)
if [ $? -ne 0 ] && echo "$OUT" | grep -q "'neg.16' can't relocate '_w'"; then
    echo "SUCCESS: a simulated instruction without a relocation for its operand is an error"
    passed=$((passed + 1))
else
    echo "FAIL: simulated instruction relocation check ($OUT)"
    failed=$((failed + 1))
fi

if [ $failed -eq 0 ]; then
    echo "All assembler feature tests passed!"
    exit 0
else
    echo "$failed tests failed."
    exit 1
fi
