#!/bin/bash
# Calling convention interoperability tests - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

LN="${LN:-./bin/ln45}"
NM="${NM:-./bin/nm45}"
MMEMU="${MMEMU:-mmemu-cli}"

run_mmemu() {
    local bin=$1
    local addr=$2
    local len=$3
    echo -e "load $bin $addr\nsetpc $addr\nstep 1000\nm $addr $len\nq" | $MMEMU -m rawMega65 2>/dev/null
}

print_section "Test 1: Stack → ZP calls (ALLOWED)"

$CC -S src/test-resources/test_cc_stack_to_zp.c -o build/test/test_cc_stack_to_zp.s 2>build/test/test_cc_stack_to_zp.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_stack_to_zp.s -o build/test/test_cc_stack_to_zp.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_stack_to_zp.bin 0x2000 6)
        echo "$OUTPUT" | grep -qi "4000:.*1e.*9b.*23.*ff" && print_pass "Stack→ZP: zp_add(10,20)=30, zp_add(100,55)=155, zp_char_mul(5,7)=35" || print_fail "Stack→ZP execution results"
    else
        print_fail "Stack→ZP assembly"
    fi
else
    print_fail "Stack→ZP compilation"
fi

print_section "Test 2: ZP → Stack calls (FORBIDDEN)"

$CC -c src/test-resources/test_cc_zp_to_stack.c -o build/test/test_cc_zp_to_stack.o45 2>build/test/test_cc_zp_to_stack_cc.err
if [ $? -eq 0 ]; then
    $LN -t 0x2000 build/test/test_cc_zp_to_stack.o45 -o build/test/test_cc_zp_to_stack.bin 2>build/test/test_cc_zp_to_stack_ld.err
    if [ $? -ne 0 ]; then
        grep -q "calling convention mismatch" build/test/test_cc_zp_to_stack_ld.err && print_pass "ZP→Stack linker error detected" || print_fail "ZP→Stack linker error"
    else
        print_fail "ZP→Stack linker enforcement"
    fi
else
    print_fail "ZP→Stack compilation"
fi

print_section "Test 3: Struct returns across conventions"

$CC -S src/test-resources/test_cc_struct_return.c -o build/test/test_cc_struct_return.s 2>build/test/test_cc_struct_return.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_struct_return.s -o build/test/test_cc_struct_return.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_struct_return.bin 0x2000 7)
        echo "$OUTPUT" | grep -qi "4000:.*0a.*14.*00.*01.*00.*02.*ff" && print_pass "Struct returns: stack Point(10,20) and ZP Rect(256,512)" || print_fail "Struct returns execution"
    else
        print_fail "Struct returns assembly"
    fi
else
    print_fail "Struct returns compilation"
fi

print_section "Test 4: Long (32-bit) returns across conventions"

$CC -S src/test-resources/test_cc_long_return.c -o build/test/test_cc_long_return.s 2>build/test/test_cc_long_return.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_long_return.s -o build/test/test_cc_long_return.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_long_return.bin 0x2000 9)
        echo "$OUTPUT" | grep -qi "4000:.*78.*56.*34.*12.*01.*ef.*cd.*ab.*ff" && print_pass "Long returns: stack (0x12345678) and ZP (0xABCDEF01)" || print_fail "Long returns execution"
    else
        print_fail "Long returns assembly"
    fi
else
    print_fail "Long returns compilation"
fi

print_section "Test 5: Parameter passing edge cases"

$CC -S src/test-resources/test_cc_param_edge_cases.c -o build/test/test_cc_param_edge_cases.s 2>build/test/test_cc_param_edge_cases.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_param_edge_cases.s -o build/test/test_cc_param_edge_cases.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_param_edge_cases.bin 0x2000 5)
        echo "$OUTPUT" | grep -qi "4000:.*0a.*96.*69.*6e.*ff" && print_pass "Parameter edge cases: 4-param ZP, 5-param stack, mixed types" || print_fail "Parameter edge cases execution"
    else
        print_fail "Parameter edge cases assembly"
    fi
else
    print_fail "Parameter edge cases compilation"
fi

print_section "Test 6: Variadic calls from ZP functions"

$CC -S src/test-resources/test_cc_zp_variadic.c -o build/test/test_cc_zp_variadic.s 2>build/test/test_cc_zp_variadic.err
if [ $? -eq 0 ]; then
    $AS build/test/test_cc_zp_variadic.s -o build/test/test_cc_zp_variadic.bin 2>/dev/null
    if [ $? -eq 0 ]; then
        OUTPUT=$(run_mmemu build/test/test_cc_zp_variadic.bin 0x2000 7)
        echo "$OUTPUT" | grep -qi "4000:.*1e.*14.*38.*01.*ff" && print_pass "Variadic from ZP: auto-fallback to stack convention" || print_fail "Variadic from ZP execution"
    else
        print_fail "Variadic from ZP assembly"
    fi
else
    print_fail "Variadic from ZP compilation"
fi

print_section "Test 7: Function attribute verification in .o45 objects"

$CC -c src/test-resources/test_cc_stack_to_zp.c -o build/test/test_cc_stack_to_zp.o45 2>/dev/null
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_cc_stack_to_zp.o45 2>/dev/null)
    echo "$NM_OUT" | grep -q "zp_add.*zp_call" && print_pass "ZP function attribute: zp_add has zp_call flag" || print_fail "ZP function attribute"
    echo "$NM_OUT" | grep -q "main.*stack_call" && print_pass "Stack function attribute: main has stack_call flag" || print_fail "Stack function attribute"
else
    print_fail "Function attribute verification"
fi

print_section "Test 8: Assembly-level .func_flags directive"

$AS -c src/test-resources/test_cc_asm.s -o build/test/test_cc_asm.o45 2>build/test/test_cc_asm.err
if [ $? -eq 0 ]; then
    NM_OUT=$($NM -f build/test/test_cc_asm.o45 2>/dev/null)
    echo "$NM_OUT" | grep -q "zp_add.*zp_call" && print_pass "Assembly: zp_add has zp_call from .func_flags directive" || print_fail "Assembly: zp_add .func_flags"
    echo "$NM_OUT" | grep -q "zp_mul.*zp_call.*leaf" && print_pass "Assembly: zp_mul has zp_call and leaf from .func_flags" || print_fail "Assembly: zp_mul .func_flags"
    echo "$NM_OUT" | grep -q "stack_add.*stack_call" && print_pass "Assembly: stack_add has stack_call from .func_flags" || print_fail "Assembly: stack_add .func_flags"
else
    print_fail "Assembly: .func_flags directive parsing"
fi

test_summary
exit $?
