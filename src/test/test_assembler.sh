#!/bin/bash
# Test script for ca45 assembler - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "Assembler feature tests"

# List of assembler feature tests
AS_TEST_FILES=(
    "src/test-resources/test_basic_upstart.s"
    "src/test-resources/test_data_types.s"
    "src/test-resources/test_mul.s"
    "src/test-resources/test_div.s"
    "src/test-resources/test_expr.s"
    "src/test-resources/test_expr_advanced.s"
    "src/test-resources/hello_call.s"
    "src/test-resources/test_var.s"
    "src/test-resources/test_45gs02.s"
    "src/test-resources/test_equates_advanced.s"
    "src/test-resources/test_flat_z_all.s"
    "src/test-resources/test_accumulator_labels.s"
    "src/test-resources/test_multi_org.s"
    "src/test-resources/test_peephole.s"
    "src/test-resources/test_simulated_ops.s"
    "src/test-resources/test_fill.s"
    "src/test-resources/test_fill_advanced.s"
    "src/test-resources/test_segments.s"
    "src/test-resources/test_res.s"
    "src/test-resources/test_pushpop.s"
    "src/test-resources/test_addr_helpers.s"
    "src/test-resources/test_shifts_16.s"
    "src/test-resources/test_new_16bit_ops.s"
    "src/test-resources/test_global_extern.s"
    "src/test-resources/test_array.s"
    "src/test-resources/test_expressions.s"
)

for f in "${AS_TEST_FILES[@]}"; do
    if [ ! -f "$f" ]; then
        print_skip "$(basename "$f") (not found)"
        continue
    fi

    test_name=$(basename "$f" .s)
    bin_file="build/test/${test_name}.bin"

    if $AS "$f" -o "$bin_file" 2>/dev/null; then
        print_pass "$test_name"
    else
        print_fail "$test_name"
    fi
done

# Specific check for basicUpstart output format
print_section "Output validation tests"

# Expecting: 0b 08 0a 00 9e 32 30 36 31 00 00 00 (for START at $080D = 2061)
ACTUAL=$(hexdump -v -n 12 -e '1/1 "%02x "' build/test/test_basic_upstart.bin 2>/dev/null | xargs)
EXPECTED="0b 08 0a 00 9e 32 30 36 31 00 00 00"

if [ "$ACTUAL" = "$EXPECTED" ]; then
    print_pass ".basicUpstart output format"
else
    print_fail ".basicUpstart output format (expected: $EXPECTED, got: $ACTUAL)"
fi

# Test -Dcc45.zeroPageStart=$10
echo -e ".var ptr = \$20\nexpr .A, *ptr" > build/test_zp_shift.s
if $AS "-Dcc45.zeroPageStart=\$10" -o build/test_zp_shift.bin build/test_zp_shift.s 2>/dev/null; then
    ACTUAL_ZP=$(hexdump -v -e '1/1 "%02x "' build/test_zp_shift.bin 2>/dev/null)
    if echo "$ACTUAL_ZP" | grep -q "85 10"; then
        print_pass "-Dcc45.zeroPageStart=\$10 ZP shifting"
    else
        print_fail "-Dcc45.zeroPageStart=\$10 ZP shifting (bytes: $ACTUAL_ZP)"
    fi
else
    print_fail "-Dcc45.zeroPageStart=\$10 compilation"
fi

test_summary
exit $?
