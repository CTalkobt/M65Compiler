#!/bin/bash

# Test script for xemu-xmega65 validation with ca45 and cc45
# Alternative to mmemu-cli with better MEGA65 hardware emulation

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
CRT0="lib/build/crt0.o45"
STDLIB="lib/build/c45.lib"            # ZP calling convention (cc45 default)
STDLIB_STACK="lib/build/c45_stack.lib" # -fno-zpcall
XEMU="xemu-xmega65"
mkdir -p build/test

# ── xemu-xmega65 availability check ──────────────────────────────────────
if ! command -v "$XEMU" &>/dev/null; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║  WARNING: xemu-xmega65 is not installed or not on PATH      ║"
    echo "║                                                             ║"
    echo "║  Xemu-based execution tests CANNOT run without it.          ║"
    echo "║  These tests verify runtime correctness of generated code.  ║"
    echo "║                                                             ║"
    echo "║  Install from: https://github.com/lgblgblgb/xemu            ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    exit 1
fi

failed=0
passed=0

# Helper: Run PRG with xemu and check memory via binary dump
# Usage: run_xemu_test "name" "prg_file" "memory_addr_hex" "num_bytes" "expected_hex"
#   memory_addr_hex: e.g. "4000" (without 0x prefix)
#   expected_hex: space-separated hex bytes e.g. "01 02 03 04 AA"
run_xemu_test() {
    local name="$1"
    local prg_file="$2"
    local mem_addr="$3"
    local num_bytes="$4"
    local expected="$5"

    local dump_file="build/test/${name}_memdump.bin"

    # Run in headless mode with -prgexit (auto-SYS and exit on READY)
    # -prgmode 65 forces MEGA65 mode without dialog
    # -prgexit auto-runs BASIC programs and exits on next READY prompt
    timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$dump_file" -prg "$prg_file" </dev/null >/dev/null 2>&1
    local exit_code=$?
    # Accept exit 0 (clean) or 124 (timeout — program may halt instead of returning)
    if [ $exit_code -ne 0 ] && [ $exit_code -ne 124 ]; then
        echo "FAIL: $name (xemu exit code $exit_code)"
        failed=$((failed + 1))
        return 1
    fi

    if [ ! -f "$dump_file" ]; then
        echo "FAIL: $name (no memory dump generated)"
        failed=$((failed + 1))
        return 1
    fi

    # Extract bytes from raw binary dump using xxd
    local addr_dec=$((16#$mem_addr))
    actual=$(xxd -s $addr_dec -l $num_bytes -p "$dump_file" 2>/dev/null | \
             sed 's/\(..\)/\1 /g' | sed 's/ $//' | tr '[:lower:]' '[:upper:]')

    if [ -z "$actual" ]; then
        echo "FAIL: $name (could not read memory at \$$mem_addr)"
        failed=$((failed + 1))
        return 1
    fi

    expected_upper=$(echo "$expected" | tr '[:lower:]' '[:upper:]')

    if [ "$actual" = "$expected_upper" ]; then
        echo "SUCCESS: $name"
        passed=$((passed + 1))
        return 0
    else
        echo "FAIL: $name"
        echo "  Expected at \$$mem_addr: $expected_upper"
        echo "  Actual:                $actual"
        failed=$((failed + 1))
        return 1
    fi
}

# Helper: compile and link test
compile_link_test() {
    local src="$1"
    local prg_out="$2"
    local flags="${3:-}"
    local o_file="${prg_out%.prg}.o45"

    $CC -c $flags "$src" -o "$o_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 1; fi

    if [[ "$flags" == *"-fno-zpcall"* ]]; then
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $STDLIB_STACK 2>/dev/null
    else
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $STDLIB 2>/dev/null
    fi
    if [ $? -ne 0 ]; then return 2; fi

    return 0
}

echo "Testing with xemu-xmega65 (MEGA65 Hardware Emulator)"
echo "======================================================"
echo ""

# Test 1: test_short.c (short type, arithmetic, function params/returns)
echo "Testing test_short.c (short type)..."
compile_link_test "src/test-resources/test_short.c" "build/test/test_short_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_short" "build/test/test_short_xemu.prg" "C010" 7 "1E 05 02 0C 0A 14 AA"
else
    echo "FAIL: test_short.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 2: test_struct_return.c (struct return by value)
echo "Testing test_struct_return.c (struct return)..."
compile_link_test "src/test-resources/test_struct_return.c" "build/test/test_struct_return_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_struct_return" "build/test/test_struct_return_xemu.prg" "C010" 7 "01 02 03 04 0A 14 AA"
else
    echo "FAIL: test_struct_return.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 3: test_array_init.c (global array initializers)
# NOTE: may fail until #262 (initialized globals in BSS) is fixed
echo "Testing test_array_init.c (array initialization)..."
compile_link_test "src/test-resources/test_array_init.c" "build/test/test_array_init_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_array_init" "build/test/test_array_init_xemu.prg" "C010" 11 "10 40 64 2C 0B 16 00 00 00 00 AA"
else
    echo "FAIL: test_array_init.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 4: test_compound_literal.c (struct/scalar compound literals)
echo "Testing test_compound_literal.c (compound literals)..."
compile_link_test "src/test-resources/test_compound_literal.c" "build/test/test_compound_literal_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_compound_literal" "build/test/test_compound_literal_xemu.prg" "C010" 8 "1E 2A 07 2C 01 14 00 AA"
else
    echo "FAIL: test_compound_literal.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 5: test_long_mmemu.c (32-bit long type)
# NOTE: may fail until #262 (initialized globals in BSS) is fixed
echo "Testing test_long_mmemu.c (long type)..."
compile_link_test "src/test-resources/test_long_mmemu.c" "build/test/test_long_mmemu_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_long_mmemu" "build/test/test_long_mmemu_xemu.prg" "C010" 12 "04 C0 01 A0 2A A0 00 E0 93 04 00 AA"
else
    echo "FAIL: test_long_mmemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 6: test_alloca_xemu.c (stack-based dynamic allocation)
echo "Testing test_alloca_xemu.c (alloca)..."
compile_link_test "src/test-resources/test_alloca_xemu.c" "build/test/test_alloca_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_alloca" "build/test/test_alloca_xemu.prg" "C000" 4 "01 02 03 AA"
else
    echo "FAIL: test_alloca_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 7: test_mem_ops_xemu.c (stdlib memset/memcpy/memcmp/memmove)
echo "Testing test_mem_ops_xemu.c (memory operations)..."
compile_link_test "src/test-resources/test_mem_ops_xemu.c" "build/test/test_mem_ops_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_mem_ops" "build/test/test_mem_ops_xemu.prg" "C000" 11 "01 02 03 04 05 06 07 08 09 0A AA"
else
    echo "FAIL: test_mem_ops_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 8: test_string_xemu.c (string.h functions)
echo "Testing test_string_xemu.c (string operations)..."
compile_link_test "src/test-resources/test_string_xemu.c" "build/test/test_string_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_string" "build/test/test_string_xemu.prg" "C020" 13 "01 02 03 04 05 06 07 08 09 0A 0B 0C AA"
else
    echo "FAIL: test_string_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 9: test_ctype_xemu.c (ctype.h functions)
echo "Testing test_ctype_xemu.c (character classification)..."
compile_link_test "src/test-resources/test_ctype_xemu.c" "build/test/test_ctype_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_ctype" "build/test/test_ctype_xemu.prg" "C030" 9 "01 02 03 04 05 06 07 08 AA"
else
    echo "FAIL: test_ctype_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 10: test_stdlib_xemu.c (stdlib.h functions)
echo "Testing test_stdlib_xemu.c (stdlib functions)..."
compile_link_test "src/test-resources/test_stdlib_xemu.c" "build/test/test_stdlib_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_stdlib" "build/test/test_stdlib_xemu.prg" "C040" 6 "01 02 03 04 05 AA"
else
    echo "FAIL: test_stdlib_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 11: test_math_xemu.c (integer math + hardware divider)
echo "Testing test_math_xemu.c (integer math)..."
compile_link_test "src/test-resources/test_math_xemu.c" "build/test/test_math_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_math" "build/test/test_math_xemu.prg" "C070" 8 "01 02 03 04 05 06 07 AA"
else
    echo "FAIL: test_math_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 12: test_misc_xemu.c (setjmp, time, errno)
echo "Testing test_misc_xemu.c (setjmp/time/errno)..."
compile_link_test "src/test-resources/test_misc_xemu.c" "build/test/test_misc_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_misc" "build/test/test_misc_xemu.prg" "C080" 7 "01 02 03 04 05 06 AA"
else
    echo "FAIL: test_misc_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 13: test_stdio_xemu.c (sprintf, snprintf)
# NOTE: sprintf hangs with -fno-staticalloc (stack overflow from vsprintf frame nesting).
# This test is expected to FAIL until SAC calling convention thunks are implemented.
echo "Testing test_stdio_xemu.c (formatted I/O)..."
compile_link_test "src/test-resources/test_stdio_xemu.c" "build/test/test_stdio_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_stdio" "build/test/test_stdio_xemu.prg" "C090" 9 "01 02 03 04 05 06 07 08 AA"
else
    echo "FAIL: test_stdio_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test 14: test_float_xemu.c (float arithmetic)
echo "Testing test_float_xemu.c (float arithmetic)..."
compile_link_test "src/test-resources/test_float_xemu.c" "build/test/test_float_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_float" "build/test/test_float_xemu.prg" "C090" 10 "01 02 03 04 05 06 07 08 09 AA"
else
    echo "FAIL: test_float_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: signed comparisons where the operands have different signs
echo "Testing test_signed_compare_xemu.c (signed compare)..."
compile_link_test "src/test-resources/test_signed_compare_xemu.c" "build/test/test_signed_compare_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_signed_compare" "build/test/test_signed_compare_xemu.prg" "C0C0" 10 "01 00 01 00 01 01 01 00 00 AA"
else
    echo "FAIL: test_signed_compare_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: library functions returning negative values
echo "Testing test_stdlib_signed_xemu.c (atoi/min/max/div)..."
compile_link_test "src/test-resources/test_stdlib_signed_xemu.c" "build/test/test_stdlib_signed_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_stdlib_signed" "build/test/test_stdlib_signed_xemu.prg" "C0C0" 11 "85 FF F4 FF 05 00 FD FF FF FF AA"
else
    echo "FAIL: test_stdlib_signed_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: struct copies (assignment / initialisation) of sizes other than 1, 2, 4
echo "Testing test_struct_copy_xemu.c (struct copy)..."
compile_link_test "src/test-resources/test_struct_copy_xemu.c" "build/test/test_struct_copy_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_struct_copy" "build/test/test_struct_copy_xemu.prg" "C0C0" 10 "07 08 09 0A 03 03 07 0A 08 AA"
else
    echo "FAIL: test_struct_copy_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: struct results via the hidden result pointer, struct arguments by copy
echo "Testing test_sret_xemu.c (struct results/arguments)..."
compile_link_test "src/test-resources/test_sret_xemu.c" "build/test/test_sret_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_sret" "build/test/test_sret_xemu.prg" "C0C0" 22 "44 11 88 55 07 08 09 07 08 09 0A 14 0B 0D 0F 02 06 05 07 06 07 AA"
else
    echo "FAIL: test_sret_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: float arguments/results, negative values, ROM math functions
echo "Testing test_float_func_xemu.c (float functions)..."
compile_link_test "src/test-resources/test_float_func_xemu.c" "build/test/test_float_func_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_float_func" "build/test/test_float_func_xemu.prg" "C0C0" 16 "0F 01 01 01 06 07 04 00 0A 1B 03 20 3A 0F F9 AA"
else
    echo "FAIL: test_float_func_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: 16-bit CPU stack (default) and the 8-bit page-1 stack (-fno-stack16),
# stack and ZP conventions: deep recursion with IRQs, alloca, setjmp, ROM calls
for s16mode in "stack|-fno-zpcall|BF 00" "zp||BF 00" "stack8|-fno-zpcall -fno-stack16 -DDEPTH=4 -DVA_N=50|01 20" "zp8|-fno-stack16 -DDEPTH=4 -DVA_N=50|01 20"; do
    IFS='|' read -r s16name s16flags s16sp <<< "$s16mode"
    echo "Testing test_stack16_xemu.c ($s16name)..."
    compile_link_test "src/test-resources/test_stack16_xemu.c" "build/test/test_stack16_${s16name}_xemu.prg" "$s16flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_stack16_$s16name" "build/test/test_stack16_${s16name}_xemu.prg" "C0C0" 9 "$s16sp 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_stack16_xemu.c $s16name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: globals initialized with addresses (&var, arrays, functions) and
# pointer compares against relocated addresses, stack and ZP conventions
for gaimode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r gainame gaiflags <<< "$gaimode"
    echo "Testing test_global_addr_init_xemu.c ($gainame)..."
    compile_link_test "src/test-resources/test_global_addr_init_xemu.c" "build/test/test_global_addr_init_${gainame}_xemu.prg" "$gaiflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_global_addr_init_$gainame" "build/test/test_global_addr_init_${gainame}_xemu.prg" "C0C0" 7 "05 03 07 07 01 01 AA"
    else
        echo "FAIL: test_global_addr_init_xemu.c $gainame (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: pointer differences (element counts, signed), stack and ZP conventions
for pdmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r pdname pdflags <<< "$pdmode"
    echo "Testing test_ptrdiff_xemu.c ($pdname)..."
    compile_link_test "src/test-resources/test_ptrdiff_xemu.c" "build/test/test_ptrdiff_${pdname}_xemu.prg" "$pdflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_ptrdiff_$pdname" "build/test/test_ptrdiff_${pdname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_ptrdiff_xemu.c $pdname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: __interrupt handlers run on their own base page (__zp_page_isr),
# including after ROM calls, in both calling conventions
for isrmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r isrname isrflags <<< "$isrmode"
    echo "Testing test_isr_page_xemu.c ($isrname)..."
    compile_link_test "src/test-resources/test_isr_page_xemu.c" "build/test/test_isr_page_${isrname}_xemu.prg" "$isrflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_isr_page_$isrname" "build/test/test_isr_page_${isrname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_isr_page_xemu.c $isrname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: a C global named fp doesn't clash with the frame-pointer variable
for fpgmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r fpgname fpgflags <<< "$fpgmode"
    echo "Testing test_fp_global_xemu.c ($fpgname)..."
    compile_link_test "src/test-resources/test_fp_global_xemu.c" "build/test/test_fp_global_${fpgname}_xemu.prg" "$fpgflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_fp_global_$fpgname" "build/test/test_fp_global_${fpgname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_fp_global_xemu.c $fpgname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: __attribute__((aligned(N))) / _Alignas on globals, locals, struct
# members and struct types
for almode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r alname alflags <<< "$almode"
    echo "Testing test_aligned_xemu.c ($alname)..."
    compile_link_test "src/test-resources/test_aligned_xemu.c" "build/test/test_aligned_${alname}_xemu.prg" "$alflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_aligned_$alname" "build/test/test_aligned_${alname}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_aligned_xemu.c $alname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: stack frames (recursion via SAC fallback, caller FP preserved, long return)
echo "Testing test_stackframe_xemu.c (stack frames)..."
compile_link_test "src/test-resources/test_stackframe_xemu.c" "build/test/test_stackframe_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_stackframe" "build/test/test_stackframe_xemu.prg" "C0B0" 7 "15 79 00 56 12 34 AA"
else
    echo "FAIL: test_stackframe_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: && / || as values (short-circuit result must be 0/1, not stale)
echo "Testing test_logical_value_xemu.c (logical operators as values)..."
compile_link_test "src/test-resources/test_logical_value_xemu.c" "build/test/test_logical_value_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_logical_value" "build/test/test_logical_value_xemu.prg" "C0A0" 9 "01 00 01 00 01 00 01 01 AA"
else
    echo "FAIL: test_logical_value_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

echo ""
echo "======================================================"
echo "Xemu Test Results: $passed passed, $failed failed"
echo "======================================================"

if [ $failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
