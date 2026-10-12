#!/bin/bash

# Test script for xemu-xmega65 validation with ca45 and cc45
# Alternative to mmemu-cli with better MEGA65 hardware emulation

CC="./bin/cc45 -O2"
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
    shift 5   # any further arguments go to xemu (e.g. -prgtest "RUN : A")

    local dump_file="build/test/${name}_memdump.bin"

    # Run in headless mode with -prgexit (auto-SYS and exit on READY)
    # -prgmode 65 forces MEGA65 mode without dialog
    # -prgexit auto-runs BASIC programs and exits on next READY prompt
    rm -f "$dump_file"
    timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit "$@" -dumpmem "$dump_file" -prg "$prg_file" </dev/null >/dev/null 2>&1
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
    actual=$(xxd -s $addr_dec -l $num_bytes -p -c 256 "$dump_file" 2>/dev/null | \
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

# Test: constant-parameter specialization only for functions every caller of
# which is in the file (two modules), stack and ZP conventions
for cpmode in "stack|-fno-zpcall|$STDLIB_STACK" "zp||$STDLIB"; do
    IFS='|' read -r cpname cpflags cplib <<< "$cpmode"
    echo "Testing test_constparam_xemu.c ($cpname)..."
    if $CC -c $cpflags src/test-resources/test_constparam_xemu.c -o build/test/test_constparam_${cpname}.o45 >/dev/null 2>&1 &&
       $CC -c $cpflags src/test-resources/test_constparam_lib_xemu.c -o build/test/test_constparam_lib_${cpname}.o45 >/dev/null 2>&1 &&
       $LD -basic -o build/test/test_constparam_${cpname}_xemu.prg $CRT0 build/test/test_constparam_${cpname}.o45 \
           build/test/test_constparam_lib_${cpname}.o45 $cplib >/dev/null 2>&1; then
        run_xemu_test "test_constparam_$cpname" "build/test/test_constparam_${cpname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_constparam_xemu.c $cpname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: virtual dispatch (every __vt set: locals, globals, statics, arrays,
# nested members), stack and ZP conventions
for vtmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r vtname vtflags <<< "$vtmode"
    echo "Testing test_virtual_xemu.c ($vtname)..."
    compile_link_test "src/test-resources/test_virtual_xemu.c" "build/test/test_virtual_${vtname}_xemu.prg" "$vtflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_virtual_$vtname" "build/test/test_virtual_${vtname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_virtual_xemu.c $vtname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: R_HIGH relocations of "symbol + n" keep the low byte for the carry
echo "Testing test_reloc_carry_xemu.c (high-byte relocation carry)..."
if $CC -c src/test-resources/test_reloc_carry_xemu.c -o build/test/test_reloc_carry.o45 >/dev/null 2>&1 &&
   $AS -c src/test-resources/test_reloc_carry_asm_xemu.s45 -o build/test/test_reloc_carry_asm.o45 >/dev/null 2>&1 &&
   $LD -basic -o build/test/test_reloc_carry_xemu.prg $CRT0 build/test/test_reloc_carry.o45 \
       build/test/test_reloc_carry_asm.o45 $STDLIB >/dev/null 2>&1; then
    run_xemu_test "test_reloc_carry" "build/test/test_reloc_carry_xemu.prg" "C0C0" 4 "01 01 01 AA"
else
    echo "FAIL: test_reloc_carry_xemu.c (build failed)"
    failed=$((failed + 1))
fi

# Test: continue <value> / continue default in a switch, stack and ZP conventions
for scmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r scname scflags <<< "$scmode"
    echo "Testing test_switch_continue_xemu.c ($scname)..."
    compile_link_test "src/test-resources/test_switch_continue_xemu.c" "build/test/test_switch_continue_${scname}_xemu.prg" "$scflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_switch_continue_$scname" "build/test/test_switch_continue_${scname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_switch_continue_xemu.c $scname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: __regparm (first argument in A / A:X), two modules, both conventions
# with and without static allocation
for rpmode in "stack|-fno-zpcall|$STDLIB_STACK" "stack_nosac|-fno-zpcall -fno-staticalloc|$STDLIB_STACK" \
              "zp||$STDLIB" "zp_nosac|-fno-staticalloc|$STDLIB"; do
    IFS='|' read -r rpname rpflags rplib <<< "$rpmode"
    echo "Testing test_regparm_xemu.c ($rpname)..."
    if $CC -c $rpflags src/test-resources/test_regparm_xemu.c -o build/test/test_regparm_${rpname}.o45 >/dev/null 2>&1 &&
       $CC -c $rpflags src/test-resources/test_regparm_lib_xemu.c -o build/test/test_regparm_lib_${rpname}.o45 >/dev/null 2>&1 &&
       $LD -basic -o build/test/test_regparm_${rpname}_xemu.prg $CRT0 build/test/test_regparm_${rpname}.o45 \
           build/test/test_regparm_lib_${rpname}.o45 $rplib >/dev/null 2>&1; then
        run_xemu_test "test_regparm_$rpname" "build/test/test_regparm_${rpname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_regparm_xemu.c $rpname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the string functions on strings longer than 255 bytes (strlen, puts
# and the rest), strcmp's sign; both conventions
for lsmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r lsname lsflags <<< "$lsmode"
    echo "Testing test_long_strings_xemu.c ($lsname)..."
    compile_link_test "src/test-resources/test_long_strings_xemu.c" "build/test/test_long_strings_${lsname}_xemu.prg" "$lsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_long_strings_$lsname" "build/test/test_long_strings_${lsname}_xemu.prg" "C0C0" 12 "01 01 01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_long_strings_xemu.c $lsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: char arguments through "..." are promoted to int, both conventions
for vcmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r vcname vcflags <<< "$vcmode"
    echo "Testing test_varargs_char_xemu.c ($vcname)..."
    compile_link_test "src/test-resources/test_varargs_char_xemu.c" "build/test/test_varargs_char_${vcname}_xemu.prg" "$vcflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_varargs_char_$vcname" "build/test/test_varargs_char_${vcname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_varargs_char_xemu.c $vcname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: crt0 fills BSS: 0 by default, $FF with -fbss-fill=$FF (xemu's RAM is
# zero at boot, so only the $FF build shows the fill runs); both conventions
for bfmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r bfname bfflags <<< "$bfmode"
    echo "Testing test_bss_fill_xemu.c ($bfname)..."
    compile_link_test "src/test-resources/test_bss_fill_xemu.c" "build/test/test_bss_fill_${bfname}_xemu.prg" "$bfflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_bss_fill_$bfname" "build/test/test_bss_fill_${bfname}_xemu.prg" "C0C0" 7 "00 00 00 00 01 01 AA"
    else
        echo "FAIL: test_bss_fill_xemu.c $bfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    compile_link_test "src/test-resources/test_bss_fill_xemu.c" "build/test/test_bss_fill_ff_${bfname}_xemu.prg" "$bfflags -fbss-fill=\$FF"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_bss_fill_ff_$bfname" "build/test/test_bss_fill_ff_${bfname}_xemu.prg" "C0C0" 7 "FF FF FF FF 01 01 AA"
    else
        echo "FAIL: test_bss_fill_xemu.c -fbss-fill=\$FF $bfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: ln45 -basic -t ADDR: the code is linked and stored at ADDR (the PRG is
# padded after the SYS stub); 4- and 5-digit SYS addresses
for btaddr in 3000 2710; do
    echo "Testing ln45 -basic -t $btaddr..."
    if $CC -c src/test-resources/test_virtual_xemu.c -o build/test/test_basic_t.o45 >/dev/null 2>&1 &&
       $LD -basic -t $btaddr -o build/test/test_basic_t_${btaddr}.prg $CRT0 build/test/test_basic_t.o45 $STDLIB >/dev/null 2>&1; then
        run_xemu_test "test_basic_t_$btaddr" "build/test/test_basic_t_${btaddr}.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: ln45 -basic -t $btaddr (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: a program's own init_features() (weak in crt0) runs before main
for ifmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r ifname ifflags <<< "$ifmode"
    echo "Testing test_init_features_xemu.c ($ifname)..."
    compile_link_test "src/test-resources/test_init_features_xemu.c" "build/test/test_init_features_${ifname}_xemu.prg" "$ifflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_init_features_$ifname" "build/test/test_init_features_${ifname}_xemu.prg" "C0C0" 2 "01 AA"
    else
        echo "FAIL: test_init_features_xemu.c $ifname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: main(argc, argv) from the BASIC line (RUN ["NAME"] : [REM] a, b, ...),
# typed by xemu's -prgtest; stack and ZP conventions
for mamode in "stack|-fno-zpcall|$STDLIB_STACK" "zp||$STDLIB"; do
    IFS='|' read -r maname maflags malib <<< "$mamode"
    echo "Testing test_mainargs_xemu.c ($maname)..."
    compile_link_test "src/test-resources/test_mainargs_xemu.c" "build/test/test_mainargs_${maname}_xemu.prg" "$maflags"
    if [ $? -eq 0 ]; then
        # plain RUN: argc 1, argv[0] ""
        run_xemu_test "test_mainargs_run_$maname" "build/test/test_mainargs_${maname}_xemu.prg" "C100" 4 "01 7C 4E 00"
        # :REM keeps keywords as text; quotes group; empty arguments; trimmed
        run_xemu_test "test_mainargs_rem_$maname" "build/test/test_mainargs_${maname}_xemu.prg" "C100" 22 \
            "06 7C 48 45 4C 4C 4F 7C 41 2C 20 42 7C 54 4F 20 37 7C 7C 58 7C 4E" \
            -prgtest 'RUN:REM HELLO, "A, B" ,  TO 7 , , X'
        # RUN "NAME" from a disk: argv[0] is the name
        rm -f build/test/test_mainargs.d81
        if ./bin/disk45 create build/test/test_mainargs.d81 -n "ARGS" >/dev/null 2>&1 &&
           ./bin/disk45 write build/test/test_mainargs.d81 "build/test/test_mainargs_${maname}_xemu.prg" ARGS >/dev/null 2>&1 &&
           $AS src/test-resources/test_mainargs_nop.s45 -o build/test/test_mainargs_nop.prg >/dev/null 2>&1; then
            run_xemu_test "test_mainargs_disk_$maname" "build/test/test_mainargs_nop.prg" "C100" 10 \
                "02 41 52 47 53 7C 58 59 7C 4E" \
                -8 "$PWD/build/test/test_mainargs.d81" -prgtest 'RUN "ARGS" : XY'
        else
            echo "FAIL: test_mainargs disk image ($maname)"
            failed=$((failed + 1))
        fi
    else
        echo "FAIL: test_mainargs_xemu.c $maname (compilation/linking failed)"
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

# Test: constant initializers of aggregate globals / local statics (struct
# layout, nested braces, brace elision, designators, unions, bitfields)
for simode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r siname siflags <<< "$simode"
    echo "Testing test_struct_init_xemu.c ($siname)..."
    compile_link_test "src/test-resources/test_struct_init_xemu.c" "build/test/test_struct_init_${siname}_xemu.prg" "$siflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_struct_init_$siname" "build/test/test_struct_init_${siname}_xemu.prg" "C0C0" 13 "01 01 01 01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_struct_init_xemu.c $siname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: float <-> integer conversions, -0.0, floor/ceil/trunc, float ternary,
# float global initializers, va_arg after a double parameter
for fcmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r fcname fcflags <<< "$fcmode"
    echo "Testing test_float_conv_xemu.c ($fcname)..."
    compile_link_test "src/test-resources/test_float_conv_xemu.c" "build/test/test_float_conv_${fcname}_xemu.prg" "$fcflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_float_conv_$fcname" "build/test/test_float_conv_${fcname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_float_conv_xemu.c $fcname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: _Complex without <complex.h> (constant initializers, real/complex
# mixing, casts, by-value parameters, int <-> float complex, conjugate)
for cxmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r cxname cxflags <<< "$cxmode"
    echo "Testing test_complex_xemu.c ($cxname)..."
    compile_link_test "src/test-resources/test_complex_xemu.c" "build/test/test_complex_${cxname}_xemu.prg" "$cxflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_complex_$cxname" "build/test/test_complex_${cxname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_complex_xemu.c $cxname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: GCC statement expressions ({ stmt; ...; expr; })
for semode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r sename seflags <<< "$semode"
    echo "Testing test_stmt_expr_xemu.c ($sename)..."
    compile_link_test "src/test-resources/test_stmt_expr_xemu.c" "build/test/test_stmt_expr_${sename}_xemu.prg" "$seflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_stmt_expr_$sename" "build/test/test_stmt_expr_${sename}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_stmt_expr_xemu.c $sename (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: float overflow saturation, float literals, function-pointer argument
# conversion, byte-indexed access to a float's bytes
for frmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r frname frflags <<< "$frmode"
    echo "Testing test_float_range_xemu.c ($frname)..."
    compile_link_test "src/test-resources/test_float_range_xemu.c" "build/test/test_float_range_${frname}_xemu.prg" "$frflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_float_range_$frname" "build/test/test_float_range_${frname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_float_range_xemu.c $frname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: structs through '...', 2-D array members, ++/-- on stack-frame
# locals, signed int -> float conversion
for vsmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r vsname vsflags <<< "$vsmode"
    echo "Testing test_varargs_struct_xemu.c ($vsname)..."
    compile_link_test "src/test-resources/test_varargs_struct_xemu.c" "build/test/test_varargs_struct_${vsname}_xemu.prg" "$vsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_varargs_struct_$vsname" "build/test/test_varargs_struct_${vsname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_varargs_struct_xemu.c $vsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: sign extension (signed shifts, int -> long before a 32-bit add),
# sprintf %f, va_list passed to always_inline functions
for sxmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r sxname sxflags <<< "$sxmode"
    echo "Testing test_signext_xemu.c ($sxname)..."
    compile_link_test "src/test-resources/test_signext_xemu.c" "build/test/test_signext_${sxname}_xemu.prg" "$sxflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_signext_$sxname" "build/test/test_signext_${sxname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_signext_xemu.c $sxname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: local aggregate initializers (nested braces, unions, designators,
# run-time values), pp-numbers and char constants vs macros, float limits
for limode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r liname liflags <<< "$limode"
    echo "Testing test_local_init_xemu.c ($liname)..."
    compile_link_test "src/test-resources/test_local_init_xemu.c" "build/test/test_local_init_${liname}_xemu.prg" "$liflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_local_init_$liname" "build/test/test_local_init_${liname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_local_init_xemu.c $liname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: math domain/range errors return (no ROM ?ILLEGAL QUANTITY/?OVERFLOW
# stop) with EDOM/ERANGE in errno; strtof overflow/underflow
for memode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r mename meflags <<< "$memode"
    echo "Testing test_math_errors_xemu.c ($mename)..."
    compile_link_test "src/test-resources/test_math_errors_xemu.c" "build/test/test_math_errors_${mename}_xemu.prg" "$meflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_math_errors_$mename" "build/test/test_math_errors_${mename}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_math_errors_xemu.c $mename (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: bitfield stores (16/32-bit units, signed fields, char units next to
# other members, unnamed ':0' fields in initializers)
for bsmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r bsname bsflags <<< "$bsmode"
    echo "Testing test_bitfield_store_xemu.c ($bsname)..."
    compile_link_test "src/test-resources/test_bitfield_store_xemu.c" "build/test/test_bitfield_store_${bsname}_xemu.prg" "$bsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_bitfield_store_$bsname" "build/test/test_bitfield_store_${bsname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_bitfield_store_xemu.c $bsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: link-time constant initializers (label tables for goto *, string +
# offset, &a[i][j], arr + n, global char arrays, extern then definition)
for simode2 in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r sname sflags <<< "$simode2"
    echo "Testing test_static_init_xemu.c ($sname)..."
    compile_link_test "src/test-resources/test_static_init_xemu.c" "build/test/test_static_init_${sname}_xemu.prg" "$sflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_static_init_$sname" "build/test/test_static_init_${sname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_static_init_xemu.c $sname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: code generation fixes (variable shifts, sign extension, ++/-- on
# lvalues, array parameters, variable-length arrays, compound literals,
# static locals)
for cgmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r cgname cgflags <<< "$cgmode"
    echo "Testing test_codegen_fixes_xemu.c ($cgname)..."
    compile_link_test "src/test-resources/test_codegen_fixes_xemu.c" "build/test/test_codegen_fixes_${cgname}_xemu.prg" "$cgflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_codegen_fixes_$cgname" "build/test/test_codegen_fixes_${cgname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_codegen_fixes_xemu.c $cgname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: GCC builtins (bit ops, offsetof, expect, choose_expr, return
# address, overflow) and printf formatting
for bimode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r biname biflags <<< "$bimode"
    echo "Testing test_builtins_xemu.c ($biname)..."
    compile_link_test "src/test-resources/test_builtins_xemu.c" "build/test/test_builtins_${biname}_xemu.prg" "$biflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_builtins_$biname" "build/test/test_builtins_${biname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_builtins_xemu.c $biname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: nested struct definitions, zero-length arrays, extended asm
# operands, written constant SAC parameters
for samode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r saname saflags <<< "$samode"
    echo "Testing test_struct_asm_xemu.c ($saname)..."
    compile_link_test "src/test-resources/test_struct_asm_xemu.c" "build/test/test_struct_asm_${saname}_xemu.prg" "$saflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_struct_asm_$saname" "build/test/test_struct_asm_${saname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_struct_asm_xemu.c $saname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: nested functions (static chain, non-local goto, trampolines),
# pointers to arrays, alias/aligned attributes, preprocessor forms
for ndmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r ndname ndflags <<< "$ndmode"
    echo "Testing test_nested_decl_xemu.c ($ndname)..."
    compile_link_test "src/test-resources/test_nested_decl_xemu.c" "build/test/test_nested_decl_${ndname}_xemu.prg" "$ndflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_nested_decl_$ndname" "build/test/test_nested_decl_${ndname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_nested_decl_xemu.c $ndname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: structs with run-time member sizes, large stack frames, __int128 from
# int, wide strings, complex members, label differences, push/pop_macro
for vmmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r vmname vmflags <<< "$vmmode"
    echo "Testing test_vmstruct_misc_xemu.c ($vmname)..."
    compile_link_test "src/test-resources/test_vmstruct_misc_xemu.c" "build/test/test_vmstruct_misc_${vmname}_xemu.prg" "$vmflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_vmstruct_misc_$vmname" "build/test/test_vmstruct_misc_${vmname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_vmstruct_misc_xemu.c $vmname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: GCC vector extension (vector_size) and compound literal initializers
for vcmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r vcname vcflags <<< "$vcmode"
    echo "Testing test_vector_xemu.c ($vcname)..."
    compile_link_test "src/test-resources/test_vector_xemu.c" "build/test/test_vector_${vcname}_xemu.prg" "$vcflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_vector_$vcname" "build/test/test_vector_${vcname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_vector_xemu.c $vcname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the IR optimizer passes keep results (all on at -O2; all off at -O0)
for iomode in "zp|" "stack|-fno-zpcall" "O0|-O0"; do
    IFS='|' read -r ioname ioflags <<< "$iomode"
    echo "Testing test_iropt_xemu.c ($ioname)..."
    compile_link_test "src/test-resources/test_iropt_xemu.c" "build/test/test_iropt_${ioname}_xemu.prg" "$ioflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_iropt_$ioname" "build/test/test_iropt_${ioname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_iropt_xemu.c $ioname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: loop interchange keeps results (nests it swaps and nests it must not)
for limode in "plain|" "zp|-floop-interchange" "stack|-floop-interchange -fno-zpcall"; do
    IFS='|' read -r liname liflags <<< "$limode"
    echo "Testing test_interchange_xemu.c ($liname)..."
    compile_link_test "src/test-resources/test_interchange_xemu.c" "build/test/test_interchange_${liname}_xemu.prg" "$liflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_interchange_$liname" "build/test/test_interchange_${liname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_interchange_xemu.c $liname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: printf/sprintf %f %e %g over the whole float range (BASIC 65 ROM digits)
for pfmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r pfname pfflags <<< "$pfmode"
    echo "Testing test_printf_float_xemu.c ($pfname)..."
    compile_link_test "src/test-resources/test_printf_float_xemu.c" "build/test/test_printf_float_${pfname}_xemu.prg" "$pfflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_printf_float_$pfname" "build/test/test_printf_float_${pfname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_printf_float_xemu.c $pfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: automatic inlining (-O2) keeps results; -O0 for comparison
for inmode in "zp|-O2" "stack|-O2 -fno-zpcall" "O0|-O0"; do
    IFS='|' read -r inname inflags <<< "$inmode"
    echo "Testing test_inline_xemu.c ($inname)..."
    compile_link_test "src/test-resources/test_inline_xemu.c" "build/test/test_inline_${inname}_xemu.prg" "$inflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_inline_$inname" "build/test/test_inline_${inname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_inline_xemu.c $inname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: loop unrolling keeps results (continue, break, nesting, char, zero trips)
for unmode in "plain|" "zp|-floop-unrolling" "stack|-floop-unrolling -fno-zpcall"; do
    IFS='|' read -r unname unflags <<< "$unmode"
    echo "Testing test_unroll_xemu.c ($unname)..."
    compile_link_test "src/test-resources/test_unroll_xemu.c" "build/test/test_unroll_${unname}_xemu.prg" "$unflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_unroll_$unname" "build/test/test_unroll_${unname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_unroll_xemu.c $unname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: malloc / free / calloc / realloc on the default heap
for mlmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r mlname mlflags <<< "$mlmode"
    echo "Testing test_malloc_xemu.c ($mlname)..."
    compile_link_test "src/test-resources/test_malloc_xemu.c" "build/test/test_malloc_${mlname}_xemu.prg" "$mlflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_malloc_$mlname" "build/test/test_malloc_${mlname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_malloc_xemu.c $mlname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: one-step build (cc45 file.c -o file.prg: cc45 runs ca45 and ln45 with
# the standard library of the calling convention)
for osmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r osname osflags <<< "$osmode"
    echo "Testing one-step cc45 -o .prg ($osname)..."
    if $CC $osflags src/test-resources/test_stdint_xemu.c -o "build/test/test_onestep_${osname}_xemu.prg" >/dev/null 2>&1; then
        run_xemu_test "test_onestep_$osname" "build/test/test_onestep_${osname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: one-step cc45 -o .prg $osname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: <stdint.h> signed exact-width types in the default (unsigned) mode
for simode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r siname siflags <<< "$simode"
    echo "Testing test_stdint_xemu.c ($siname)..."
    compile_link_test "src/test-resources/test_stdint_xemu.c" "build/test/test_stdint_${siname}_xemu.prg" "$siflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_stdint_$siname" "build/test/test_stdint_${siname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_stdint_xemu.c $siname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: int64_t (struct __int64): arithmetic, narrowing, parameters, comparisons
for i6mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r i6name i6flags <<< "$i6mode"
    echo "Testing test_int64_xemu.c ($i6name)..."
    compile_link_test "src/test-resources/test_int64_xemu.c" "build/test/test_int64_${i6name}_xemu.prg" "$i6flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_int64_$i6name" "build/test/test_int64_${i6name}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_int64_xemu.c $i6name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: pointer/float compound assignment, float initializers, nested
# function static chain, variably modified parameters
for ammode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r amname amflags <<< "$ammode"
    echo "Testing test_arith_misc_xemu.c ($amname)..."
    compile_link_test "src/test-resources/test_arith_misc_xemu.c" "build/test/test_arith_misc_${amname}_xemu.prg" "$amflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_arith_misc_$amname" "build/test/test_arith_misc_${amname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_arith_misc_xemu.c $amname (compilation/linking failed)"
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

# Test: dma_fill / dma_copy (the fill value goes in the list's source low byte)
echo "Testing test_dma_xemu.c (DMA fill and copy)..."
compile_link_test "src/test-resources/test_dma_xemu.c" "build/test/test_dma_xemu.prg"
if [ $? -eq 0 ]; then
    run_xemu_test "test_dma" "build/test/test_dma_xemu.prg" "C0C0" 3 "01 01 AA"
else
    echo "FAIL: test_dma_xemu.c (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: ca45 simulated instructions (assembled directly with ca45, no linker)
echo "Testing test_ca45_simops_xemu.s45 (ca45 simulated instructions)..."
if ./bin/ca45 src/test-resources/test_ca45_simops_xemu.s45 -o build/test/test_ca45_simops_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_ca45_simops" "build/test/test_ca45_simops_xemu.prg" "C0C0" 39 \
        "04 00 FE FF 2C 01 FB FF 05 00 00 12 FB FF 06 FF 12 34 12 42 00 99 35 12 32 01 34 56 CD AB 5A 5A 77 66 05 5C 03 01 AA"
else
    echo "FAIL: test_ca45_simops_xemu.s45 (assembly failed)"
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
