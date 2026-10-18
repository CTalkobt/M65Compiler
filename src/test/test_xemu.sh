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
    local extra="${4:-}"     # more objects to link (assembled from .s45 sources)
    local o_file="${prg_out%.prg}.o45"

    $CC -c $flags "$src" -o "$o_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 1; fi

    local extra_o=""
    for s in $extra; do
        local eo="${prg_out%.prg}_$(basename "${s%.s45}").o45"
        $AS -c "$s" -o "$eo" >/dev/null 2>&1 || return 1
        extra_o="$extra_o $eo"
    done

    if [[ "$flags" == *"-fno-zpcall"* ]]; then
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $extra_o $STDLIB_STACK 2>/dev/null
    else
        $LD -basic -o "$prg_out" $CRT0 "$o_file" $extra_o $STDLIB 2>/dev/null
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

# Test: __zp globals in the program's base page (two modules): initial
# values, arrays, pointers, addresses, statics, stack and ZP conventions
for zgmode in "stack|-fno-zpcall|$STDLIB_STACK" "zp||$STDLIB"; do
    IFS='|' read -r zgname zgflags zglib <<< "$zgmode"
    echo "Testing test_zp_globals_xemu.c ($zgname)..."
    if $CC -c $zgflags src/test-resources/test_zp_globals_xemu.c -o build/test/test_zp_globals_${zgname}.o45 >/dev/null 2>&1 &&
       $CC -c $zgflags src/test-resources/test_zp_globals_b_xemu.c -o build/test/test_zp_globals_b_${zgname}.o45 >/dev/null 2>&1 &&
       $LD -basic -o build/test/test_zp_globals_${zgname}_xemu.prg $CRT0 build/test/test_zp_globals_${zgname}.o45 \
           build/test/test_zp_globals_b_${zgname}.o45 $zglib >/dev/null 2>&1; then
        run_xemu_test "test_zp_globals_$zgname" "build/test/test_zp_globals_${zgname}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_zp_globals_xemu.c $zgname (compilation/linking failed)"
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

# Test: ca45 simulated instructions and ldq/stq/adcq on relocatable addresses
echo "Testing test_reloc_simops_xemu.c (relocatable operands of simulated/Q instructions)..."
if $CC -c src/test-resources/test_reloc_simops_xemu.c -o build/test/test_reloc_simops.o45 >/dev/null 2>&1 &&
   $AS -c src/test-resources/test_reloc_simops_asm_xemu.s45 -o build/test/test_reloc_simops_asm.o45 >/dev/null 2>&1 &&
   $LD -basic -o build/test/test_reloc_simops_xemu.prg $CRT0 build/test/test_reloc_simops.o45 \
       build/test/test_reloc_simops_asm.o45 $STDLIB >/dev/null 2>&1; then
    run_xemu_test "test_reloc_simops" "build/test/test_reloc_simops_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
else
    echo "FAIL: test_reloc_simops_xemu.c (build failed)"
    failed=$((failed + 1))
fi

# Test: ln45 drops library members nothing live references (a weak printf
# that lost to the float one, sprintf pulled only by it); --no-gc keeps them
echo "Testing ln45 dropping unreferenced library members..."
if $CC -c src/test-resources/test_printf_float_xemu.c -o build/test/test_gc_objects.o45 >/dev/null 2>&1 &&
   $LD -basic --print-gc -o build/test/test_gc_objects_xemu.prg $CRT0 build/test/test_gc_objects.o45 $STDLIB \
       > build/test/test_gc_objects.log 2>&1 &&
   $LD -basic --no-gc -o build/test/test_gc_objects_nogc.prg $CRT0 build/test/test_gc_objects.o45 $STDLIB >/dev/null 2>&1 &&
   grep -q "dropped .*(zp_sprintf.o45)" build/test/test_gc_objects.log &&
   [ "$(stat -c %s build/test/test_gc_objects_xemu.prg)" -lt "$(stat -c %s build/test/test_gc_objects_nogc.prg)" ]; then
    run_xemu_test "test_gc_objects" "build/test/test_gc_objects_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
else
    echo "FAIL: ln45 dropping unreferenced members ($(cat build/test/test_gc_objects.log 2>/dev/null | head -3))"
    failed=$((failed + 1))
fi

# Test: ln45 drops unreachable sections of a sectioned library member
# (a copy of the C conio compiled with -ffunction-sections into a library of
# its own)
echo "Testing test_gc_sections_xemu.c (dropping unused functions)..."
rm -f build/test/test_gc_conio.lib
if $CC -fzpcall -fimplicit-function-declaration -fsigned-default -funsigned-char -ffunction-sections -Ilib/include \
       -c src/test-resources/gc_sample_conio.c -o build/test/test_gc_conio.o45 >/dev/null 2>&1 &&
   ./bin/ar45 c build/test/test_gc_conio.lib build/test/test_gc_conio.o45 >/dev/null 2>&1 &&
   $CC -c src/test-resources/test_gc_sections_xemu.c -o build/test/test_gc_sections.o45 >/dev/null 2>&1 &&
   $LD -basic --print-gc -o build/test/test_gc_sections_xemu.prg lib/build/crt0_zp.o45 build/test/test_gc_sections.o45 \
       build/test/test_gc_conio.lib $STDLIB > build/test/test_gc_sections.log 2>&1 &&
   grep -q "dropped section .*_cprintf " build/test/test_gc_sections.log &&
   grep -q "dropped section .*_clrscr__ar " build/test/test_gc_sections.log &&
   ! grep -q "dropped section .*_gotoxy " build/test/test_gc_sections.log &&
   [ "$(stat -c %s build/test/test_gc_sections_xemu.prg)" -lt 3000 ]; then
    run_xemu_test "test_gc_sections" "build/test/test_gc_sections_xemu.prg" "C0C0" 3 "01 01 AA"
else
    echo "FAIL: test_gc_sections (build, or what was dropped: $(head -3 build/test/test_gc_sections.log 2>/dev/null))"
    failed=$((failed + 1))
fi

# Test: ln45 -d above and below the text; data overlapping the text or the
# BASIC stub is an error
if $CC -c src/test-resources/test_ln45_layout_xemu.c -o build/test/test_ln45_layout.o45 >/dev/null 2>&1; then
    for lymode in "above|-d 8000" "below|-t 3000 -d 2100"; do
        IFS='|' read -r lyname lyflags <<< "$lymode"
        echo "Testing test_ln45_layout_xemu.c (data $lyname the text)..."
        if $LD -basic $lyflags -o "build/test/test_ln45_layout_${lyname}_xemu.prg" $CRT0 \
               build/test/test_ln45_layout.o45 $STDLIB >/dev/null 2>&1; then
            run_xemu_test "test_ln45_layout_$lyname" "build/test/test_ln45_layout_${lyname}_xemu.prg" "C0C0" 3 "01 01 AA"
        else
            echo "FAIL: test_ln45_layout_xemu.c $lyname (link failed)"
            failed=$((failed + 1))
        fi
    done
    for lymode in "overlaps the text segment|-d 2100" "overlaps the BASIC stub|-d 1000"; do
        IFS='|' read -r lymsg lyflags <<< "$lymode"
        if $LD -basic $lyflags -o build/test/test_ln45_layout_err.prg $CRT0 \
               build/test/test_ln45_layout.o45 $STDLIB 2>&1 | grep -q "$lymsg"; then
            echo "PASS: ln45 -basic $lyflags: $lymsg"
            passed=$((passed + 1))
        else
            echo "FAIL: ln45 -basic $lyflags should fail with '$lymsg'"
            failed=$((failed + 1))
        fi
    done
else
    echo "FAIL: test_ln45_layout_xemu.c (compile failed)"
    failed=$((failed + 1))
fi

# Test: basic45 -- a BASIC 65 program (operators, labels, ON GOTO, DATA,
# escapes) built as is and with --minify, run by BASIC on xemu; and the
# validator's undefined-label error
for bsmode in "plain|" "minify|--minify" "spaces|--preserve-spaces"; do
    IFS='|' read -r bsname bsflags <<< "$bsmode"
    echo "Testing test_basic45_xemu.bas ($bsname)..."
    if ./bin/basic45 --strict $bsflags src/test-resources/test_basic45_xemu.bas \
           -o "build/test/test_basic45_${bsname}_xemu.prg" >/dev/null 2>&1; then
        run_xemu_test "test_basic45_$bsname" "build/test/test_basic45_${bsname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_basic45_xemu.bas $bsname (basic45 failed)"
        failed=$((failed + 1))
    fi
done
# basic45 extensions: long names, block if, proc/call, FN (run by BASIC);
# the same program decompiled and rebuilt; --list, --source-map, --disk
echo "Testing test_basic45_features_xemu.bas..."
if ./bin/basic45 --strict src/test-resources/test_basic45_features_xemu.bas \
       -o build/test/test_basic45_features_xemu.prg --source-map build/test/basic45_features.map \
       --disk build/test/basic45_features.d81 --disk-name features --list > build/test/basic45_features.lst 2>&1; then
    run_xemu_test "test_basic45_features" "build/test/test_basic45_features_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    if ./bin/basic45 --decompile build/test/test_basic45_features_xemu.prg -o build/test/basic45_features_rt.bas &&
       ./bin/basic45 --strict build/test/basic45_features_rt.bas -o build/test/basic45_features_rt.prg &&
       cmp -s build/test/test_basic45_features_xemu.prg build/test/basic45_features_rt.prg; then
        echo "PASS: basic45 --decompile round trip gives the same program"
        passed=$((passed + 1))
    else
        echo "FAIL: basic45 --decompile round trip"
        failed=$((failed + 1))
    fi
    if grep -q "test_basic45_features_xemu.bas:[0-9]* *start: score = 10" build/test/basic45_features.lst &&
       grep -q '"FEATURES"' <(./bin/disk45 list build/test/basic45_features.d81 2>&1) &&
       grep -q "test_basic45_features_xemu.bas" build/test/basic45_features.map; then
        echo "PASS: basic45 --list, --source-map and --disk"
        passed=$((passed + 1))
    else
        echo "FAIL: basic45 --list, --source-map or --disk"
        failed=$((failed + 1))
    fi
else
    echo "FAIL: test_basic45_features_xemu.bas (basic45 failed)"
    failed=$((failed + 1))
fi
rm -f build/test/basic45_features.d81

# basic45 --preserve-spaces: the source's blanks between tokens
printf 'top: print  a ;  "x" : goto   top\n' > build/test/basic45_spaces.bas
if ./bin/basic45 --preserve-spaces build/test/basic45_spaces.bas -o build/test/basic45_spaces.prg 2>/dev/null &&
   xxd -p build/test/basic45_spaces.prg | tr -d '\n' | grep -q "99202041203b2020225822203a208920202031" &&
   ./bin/basic45 build/test/basic45_spaces.bas -o build/test/basic45_spaces.prg 2>/dev/null &&
   xxd -p build/test/basic45_spaces.prg | tr -d '\n' | grep -q "9920413b2258223a892031"; then
    echo "PASS: basic45 --preserve-spaces"
    passed=$((passed + 1))
else
    echo "FAIL: basic45 --preserve-spaces"
    failed=$((failed + 1))
fi

# basic45 string escapes, MEGA65 control codes and ESC sequences included
# (the values as the ROM's screen editor has them: control_vectors, funtab)
printf 'print "{flash}{esc-x}{clear-eos}{$c1}{down}{esc}{lower}{orange}{f9}{f14}{help}{default}{shift-f7}{off}{under-off}{back}"\n' > build/test/basic45_escapes.bas
if ./bin/basic45 --strict build/test/basic45_escapes.bas -o build/test/basic45_escapes.prg 2>/dev/null &&
   xxd -p build/test/basic45_escapes.prg | tr -d '\n' | grep -q "220f1b581b40c1111b0e81101a84068c92821422"; then
    echo "PASS: basic45 string escapes"
    passed=$((passed + 1))
else
    echo "FAIL: basic45 string escapes"
    failed=$((failed + 1))
fi

# basic45 --link: BASIC calling C functions (SYS {name}, a, x, y; RREG)
echo "Testing test_basic45_link_xemu.bas..."
if ./bin/basic45 --strict --link src/test-resources/test_basic45_link_xemu.c \
       src/test-resources/test_basic45_link_xemu.bas -o build/test/test_basic45_link_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_basic45_link" "build/test/test_basic45_link_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
else
    echo "FAIL: test_basic45_link_xemu.bas (basic45 --link failed)"
    failed=$((failed + 1))
fi
# basic45 warnings: a variable read but never assigned (a typo), assigned but
# never read, a line that can never run, a proc never called
printf 'top: score = 1 : print scroe : unused = 5\n  goto top\n  print "never"\nproc lonely\nendproc\n' \
    > build/test/basic45_warn.bas
./bin/basic45 build/test/basic45_warn.bas -o build/test/basic45_warn.prg > build/test/basic45_warn.log 2>&1
if grep -q "variable SCROE is read but never assigned" build/test/basic45_warn.log &&
   grep -q "variable UNUSED is assigned but never read" build/test/basic45_warn.log &&
   grep -q "basic45_warn.bas:3:1: warning: line 30 can never run" build/test/basic45_warn.log &&
   grep -q "proc lonely is never called" build/test/basic45_warn.log &&
   ! ./bin/basic45 --strict build/test/basic45_warn.bas -o build/test/basic45_warn.prg >/dev/null 2>&1; then
    echo "PASS: basic45 warnings"
    passed=$((passed + 1))
else
    echo "FAIL: basic45 warnings"
    failed=$((failed + 1))
fi

# basic45 --format: the same program, and formatting again changes nothing;
# --report; --send-cmd
./bin/basic45 --format src/test-resources/test_basic45_features_xemu.bas -o build/test/basic45_fmt.bas
./bin/basic45 --format build/test/basic45_fmt.bas -o build/test/basic45_fmt2.bas
if ./bin/basic45 build/test/basic45_fmt.bas -o build/test/basic45_fmt.prg >/dev/null 2>&1 &&
   cmp -s build/test/basic45_fmt.prg build/test/test_basic45_features_xemu.prg &&
   cmp -s build/test/basic45_fmt.bas build/test/basic45_fmt2.bas &&
   ./bin/basic45 --report src/test-resources/test_basic45_features_xemu.bas -o build/test/basic45_rep.prg |
       grep -q "^Variables .* scalar (of 256" &&
   ./bin/basic45 src/test-resources/test_basic45_features_xemu.bas -o build/test/basic45_rep.prg \
       --send-cmd "cp {prg} build/test/basic45_sent.prg" >/dev/null &&
   cmp -s build/test/basic45_rep.prg build/test/basic45_sent.prg; then
    echo "PASS: basic45 --format, --report, --send-cmd"
    passed=$((passed + 1))
else
    echo "FAIL: basic45 --format, --report or --send-cmd"
    failed=$((failed + 1))
fi

printf 'top: print\n  goto nowhere\n' > build/test/basic45_undef.bas
if ./bin/basic45 build/test/basic45_undef.bas -o build/test/basic45_undef.prg 2>&1 | \
       grep -q "basic45_undef.bas:2:1: error: Undefined label: NOWHERE"; then
    echo "PASS: basic45 reports an undefined label at its line"
    passed=$((passed + 1))
else
    echo "FAIL: basic45 should report 'Undefined label: NOWHERE' at line 2"
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

# Test: file-scope asm("...") (data, a function, an equate), both conventions
for famode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r faname faflags <<< "$famode"
    echo "Testing test_file_asm_xemu.c ($faname)..."
    compile_link_test "src/test-resources/test_file_asm_xemu.c" "build/test/test_file_asm_${faname}_xemu.prg" "$faflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_file_asm_$faname" "build/test/test_file_asm_${faname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_file_asm_xemu.c $faname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: ln45's SAC overlay (functions never active together share storage), both conventions
for somode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r soname soflags <<< "$somode"
    echo "Testing test_sac_overlay_xemu.c ($soname)..."
    compile_link_test "src/test-resources/test_sac_overlay_xemu.c" "build/test/test_sac_overlay_${soname}_xemu.prg" "$soflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_sac_overlay_$soname" "build/test/test_sac_overlay_${soname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_sac_overlay_xemu.c $soname (compilation/linking failed)"
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

# Test: file I/O on a disk (an empty D81 as drive 8): fopen w/r/a/wb/rb,
# replace, missing file, EOF, two open files; both conventions
for fimode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r finame fiflags <<< "$fimode"
    echo "Testing test_fileio_xemu.c ($finame)..."
    compile_link_test "src/test-resources/test_fileio_xemu.c" "build/test/test_fileio_${finame}_xemu.prg" "$fiflags"
    if [ $? -eq 0 ] && rm -f "build/test/test_fileio_${finame}.d81" &&
       ./bin/disk45 create "build/test/test_fileio_${finame}.d81" -n "FILEIO" >/dev/null 2>&1; then
        run_xemu_test "test_fileio_$finame" "build/test/test_fileio_${finame}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA" \
            -8 "$PWD/build/test/test_fileio_${finame}.d81"
    else
        echo "FAIL: test_fileio_xemu.c $finame (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: a[i + c] / a[i - c] folded into a shifted base address (int and
# char elements, a char index, an index changed before the access)
for eomode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r eoname eoflags <<< "$eomode"
    echo "Testing test_elem_offset_xemu.c ($eoname)..."
    compile_link_test "src/test-resources/test_elem_offset_xemu.c" "build/test/test_elem_offset_${eoname}_xemu.prg" "$eoflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_elem_offset_$eoname" "build/test/test_elem_offset_${eoname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_elem_offset_xemu.c $eoname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: dma_copy_far / dma_fill_far (28-bit addresses: chip RAM above
# 64 KB, colour RAM at $FF80000, across a 64 KB boundary)
for dfmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r dfname dfflags <<< "$dfmode"
    echo "Testing test_dma_far_xemu.c ($dfname)..."
    compile_link_test "src/test-resources/test_dma_far_xemu.c" "build/test/test_dma_far_${dfname}_xemu.prg" "$dfflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_dma_far_$dfname" "build/test/test_dma_far_${dfname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_dma_far_xemu.c $dfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: far pointers (far_ptr.h / far_ops.h) and dma_controller.h, all
# through DMA; the comma operator in for clauses and macro arguments that
# contain a parameter's name
for fomode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r foname foflags <<< "$fomode"
    echo "Testing test_far_ops_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_far_ops_xemu.c" "build/test/test_far_ops_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_far_ops_$foname" "build/test/test_far_ops_${foname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_far_ops_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_lib_regress_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_lib_regress_xemu.c" "build/test/test_lib_regress_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_lib_regress_$foname" "build/test/test_lib_regress_${foname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_lib_regress_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_parse_fixes_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_parse_fixes_xemu.c" "build/test/test_parse_fixes_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_parse_fixes_$foname" "build/test/test_parse_fixes_${foname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_parse_fixes_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_methods_nested_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_methods_nested_xemu.c" "build/test/test_methods_nested_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_methods_nested_$foname" "build/test/test_methods_nested_${foname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_methods_nested_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_mulconst_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_mulconst_xemu.c" "build/test/test_mulconst_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_mulconst_$foname" "build/test/test_mulconst_${foname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_mulconst_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_divpow2_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_divpow2_xemu.c" "build/test/test_divpow2_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_divpow2_$foname" "build/test/test_divpow2_${foname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_divpow2_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_smallarray_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_smallarray_xemu.c" "build/test/test_smallarray_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_smallarray_$foname" "build/test/test_smallarray_${foname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_smallarray_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_cmpvalue_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_cmpvalue_xemu.c" "build/test/test_cmpvalue_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_cmpvalue_$foname" "build/test/test_cmpvalue_${foname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_cmpvalue_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_inplace_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_inplace_xemu.c" "build/test/test_inplace_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_inplace_$foname" "build/test/test_inplace_${foname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_inplace_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
    echo "Testing test_intn_native_xemu.c ($foname)..."
    compile_link_test "src/test-resources/test_intn_native_xemu.c" "build/test/test_intn_native_${foname}_xemu.prg" "$foflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_intn_native_$foname" "build/test/test_intn_native_${foname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_intn_native_xemu.c $foname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the forth example's test script with the stack convention and no
# static allocation: ca45's tail-dedup reused a label name in a later
# optimizer round, so exec()'s epilogue jumped into fail()'s code
echo "Testing examples/c/forth (stack, -fno-staticalloc)..."
compile_link_test "examples/c/forth/main.c" "build/test/test_forth_stack_nosac_xemu.prg" "-fno-zpcall -fno-staticalloc -DFORTH_TEST"
if [ $? -eq 0 ]; then
    run_xemu_test "test_forth_stack_nosac" "build/test/test_forth_stack_nosac_xemu.prg" "C0C0" 22 "01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 AA"
else
    echo "FAIL: examples/c/forth stack/no-SAC (compilation/linking failed)"
    failed=$((failed + 1))
fi

# Test: <float.h> constants (exact bytes, epsilon, underflow), complex * and
# / of two complex values, __real__/__imag__, operators on temporaries
for flmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r flname flflags <<< "$flmode"
    echo "Testing test_float_limits_xemu.c ($flname)..."
    compile_link_test "src/test-resources/test_float_limits_xemu.c" "build/test/test_float_limits_${flname}_xemu.prg" "$flflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_float_limits_$flname" "build/test/test_float_limits_${flname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_float_limits_xemu.c $flname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: sscanf, vsnprintf, vfprintf (a file on drive 8), perror (the screen
# row), getchar (the KERNAL keyboard buffer); both conventions
for s2mode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r s2name s2flags <<< "$s2mode"
    echo "Testing test_stdio2_xemu.c ($s2name)..."
    compile_link_test "src/test-resources/test_stdio2_xemu.c" "build/test/test_stdio2_${s2name}_xemu.prg" "$s2flags"
    if [ $? -eq 0 ] && rm -f "build/test/test_stdio2_${s2name}.d81" &&
       ./bin/disk45 create "build/test/test_stdio2_${s2name}.d81" -n "STDIO2" >/dev/null 2>&1; then
        run_xemu_test "test_stdio2_$s2name" "build/test/test_stdio2_${s2name}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA" \
            -8 "$PWD/build/test/test_stdio2_${s2name}.d81"
    else
        echo "FAIL: test_stdio2_xemu.c $s2name (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: CBM file I/O (cbm_read/write/seek/tell/eof/status, cbm_drive_status),
# the KERNAL wrappers, the directory, REL error strings; a D81 as drive 8
for cimode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r ciname ciflags <<< "$cimode"
    echo "Testing test_cbmio_xemu.c ($ciname)..."
    compile_link_test "src/test-resources/test_cbmio_xemu.c" "build/test/test_cbmio_${ciname}_xemu.prg" "$ciflags"
    if [ $? -eq 0 ] && rm -f "build/test/test_cbmio_${ciname}.d81" &&
       ./bin/disk45 create "build/test/test_cbmio_${ciname}.d81" -n "CBMIO" >/dev/null 2>&1; then
        run_xemu_test "test_cbmio_$ciname" "build/test/test_cbmio_${ciname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA" \
            -8 "$PWD/build/test/test_cbmio_${ciname}.d81"
    else
        echo "FAIL: test_cbmio_xemu.c $ciname (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: fscanf/vfscanf/sscanf %f, setvbuf/setbuf, tmpfile, tmpnam, freopen; a
# D81 as drive 8
for s3mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r s3name s3flags <<< "$s3mode"
    echo "Testing test_stdio3_xemu.c ($s3name)..."
    compile_link_test "src/test-resources/test_stdio3_xemu.c" "build/test/test_stdio3_${s3name}_xemu.prg" "$s3flags"
    if [ $? -eq 0 ] && rm -f "build/test/test_stdio3_${s3name}.d81" &&
       ./bin/disk45 create "build/test/test_stdio3_${s3name}.d81" -n "STDIO3" >/dev/null 2>&1; then
        run_xemu_test "test_stdio3_$s3name" "build/test/test_stdio3_${s3name}_xemu.prg" "C0C0" 3 "01 01 AA" \
            -8 "$PWD/build/test/test_stdio3_${s3name}.d81"
    else
        echo "FAIL: test_stdio3_xemu.c $s3name (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: fopen "r+"/"w+"/"a+" (memory streams) and fseek/ftell/rewind on a
# disk; both conventions
for fsmode in "stack|-fno-zpcall" "zp|"; do
    IFS='|' read -r fsname fsflags <<< "$fsmode"
    echo "Testing test_fileseek_xemu.c ($fsname)..."
    compile_link_test "src/test-resources/test_fileseek_xemu.c" "build/test/test_fileseek_${fsname}_xemu.prg" "$fsflags"
    if [ $? -eq 0 ] && rm -f "build/test/test_fileseek_${fsname}.d81" &&
       ./bin/disk45 create "build/test/test_fileseek_${fsname}.d81" -n "FILESEEK" >/dev/null 2>&1; then
        run_xemu_test "test_fileseek_$fsname" "build/test/test_fileseek_${fsname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA" \
            -8 "$PWD/build/test/test_fileseek_${fsname}.d81"
    else
        echo "FAIL: test_fileseek_xemu.c $fsname (build failed)"
        failed=$((failed + 1))
    fi
done

# Test: a REL file made by disk45 on a D81, used by BASIC 65's DOS (and the
# records BASIC wrote read back by disk45)
echo "Testing test_disk45_rel_xemu.bas..."
rm -f build/test/test_disk45_rel.d81
if ./bin/disk45 create build/test/test_disk45_rel.d81 -n RELTEST >/dev/null 2>&1 &&
   ./bin/disk45 rel-create build/test/test_disk45_rel.d81 scores 20 30 >/dev/null 2>&1 &&
   ./bin/disk45 rel-write build/test/test_disk45_rel.d81 scores 5 '"HELLO"' >/dev/null 2>&1 &&
   ./bin/basic45 --strict src/test-resources/test_disk45_rel_xemu.bas -o build/test/test_disk45_rel_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_disk45_rel" "build/test/test_disk45_rel_xemu.prg" "C0C0" 4 "01 01 01 AA" \
        -8 "$PWD/build/test/test_disk45_rel.d81"
    if ./bin/disk45 rel-read build/test/test_disk45_rel.d81 scores 7 2>&1 | grep -q 'Text: "BASIC' &&
       ./bin/disk45 rel-read build/test/test_disk45_rel.d81 scores 40 2>&1 | grep -q 'Text: "FAR' &&
       ./bin/disk45 validate build/test/test_disk45_rel.d81 2>&1 | grep -q 'Status: OK'; then
        echo "PASS: disk45 reads the records BASIC wrote"
        passed=$((passed + 1))
    else
        echo "FAIL: disk45 reading the REL records BASIC wrote"
        failed=$((failed + 1))
    fi
else
    echo "FAIL: test_disk45_rel_xemu.bas (build failed)"
    failed=$((failed + 1))
fi

# Test: an SD card image made by disk45: xemu boots from it (the ROM from the
# card), BASIC reads and writes the card's MEGA65.D81; disk45 then finds the
# file BASIC wrote. Needs xemu's MEGA65.ROM
XEMU_ROM="$HOME/.local/share/xemu-lgb/mega65/MEGA65.ROM"
if [ -f "$XEMU_ROM" ]; then
    echo "Testing test_disk45_sd_xemu.bas..."
    SDC=build/test/test_disk45_sd.img
    rm -f "$SDC" build/test/test_disk45_sd.d81 build/test/test_disk45_sd_back.d81
    printf 'SDCARD\r' > build/test/test_disk45_sd.seq
    if ./bin/disk45 sd create "$SDC" 64M --onboarded >/dev/null 2>&1 &&
       ./bin/disk45 sd add "$SDC" "$XEMU_ROM" MEGA65.ROM >/dev/null 2>&1 &&
       ./bin/disk45 create build/test/test_disk45_sd.d81 -n SDTEST >/dev/null 2>&1 &&
       ./bin/disk45 add build/test/test_disk45_sd.d81 build/test/test_disk45_sd.seq HELLO -t seq >/dev/null 2>&1 &&
       ./bin/disk45 sd add "$SDC" build/test/test_disk45_sd.d81 MEGA65.D81 >/dev/null 2>&1 &&
       ./bin/basic45 --strict src/test-resources/test_disk45_sd_xemu.bas -o build/test/test_disk45_sd_xemu.prg >/dev/null 2>&1; then
        run_xemu_test "test_disk45_sd" "build/test/test_disk45_sd_xemu.prg" "C0C0" 3 "01 01 AA" \
            -sdimg "$PWD/$SDC" -defd81fromsd -romfromsd
        if ./bin/disk45 sd check "$SDC" 2>&1 | grep -q '^OK' &&
           ./bin/disk45 sd extract "$SDC" MEGA65.D81 build/test/test_disk45_sd_back.d81 >/dev/null 2>&1 &&
           ./bin/disk45 list build/test/test_disk45_sd_back.d81 2>&1 | grep -q '"BACK"'; then
            echo "PASS: disk45 finds the file BASIC wrote on the SD card's D81"
            passed=$((passed + 1))
        else
            echo "FAIL: disk45 reading back the SD card's D81"
            failed=$((failed + 1))
        fi
    else
        echo "FAIL: test_disk45_sd_xemu.bas (build failed)"
        failed=$((failed + 1))
    fi
else
    echo "SKIP: test_disk45_sd_xemu.bas (no $XEMU_ROM)"
fi

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
        run_xemu_test "test_aligned_$alname" "build/test/test_aligned_${alname}_xemu.prg" "C0C0" 11 "01 01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_aligned_xemu.c $alname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: sizeof, array dimensions, storage and member offsets agree on struct
# layouts (__unpacked tail padding, bitfields, nested, flexible members)
for slmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r slname slflags <<< "$slmode"
    echo "Testing test_struct_layout_xemu.c ($slname)..."
    compile_link_test "src/test-resources/test_struct_layout_xemu.c" "build/test/test_struct_layout_${slname}_xemu.prg" "$slflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_struct_layout_$slname" "build/test/test_struct_layout_${slname}_xemu.prg" "C0C0" 12 "01 01 01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_struct_layout_xemu.c $slname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: stdlib/string/ctype functions not covered elsewhere (strtol, strtoul,
# atof, div, srand, utoa/ltoa, strtok, ctype, atexit on return and exit());
# a failed assert / assert_msg ends with exit status $FFFE
for s2mode in "zp|" "stack|-fno-zpcall" "zp_exit|-DVIA_EXIT" "stack_exit|-fno-zpcall -DVIA_EXIT"; do
    IFS='|' read -r s2name s2flags <<< "$s2mode"
    echo "Testing test_stdlib2_xemu.c ($s2name)..."
    compile_link_test "src/test-resources/test_stdlib2_xemu.c" "build/test/test_stdlib2_${s2name}_xemu.prg" "$s2flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_stdlib2_$s2name" "build/test/test_stdlib2_${s2name}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_stdlib2_xemu.c $s2name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
for s2mode in "assert|-DVIA_ASSERT" "assert_msg|-DVIA_ASSERT_MSG"; do
    IFS='|' read -r s2name s2flags <<< "$s2mode"
    echo "Testing test_stdlib2_xemu.c ($s2name)..."
    compile_link_test "src/test-resources/test_stdlib2_xemu.c" "build/test/test_stdlib2_${s2name}_xemu.prg" "$s2flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_stdlib2_$s2name" "build/test/test_stdlib2_${s2name}_xemu.prg" "00FD" 2 "FE FF"
    else
        echo "FAIL: test_stdlib2_xemu.c $s2name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: math.h functions not covered elsewhere (atan2, fmod, frexp/ldexp,
# modf, log2, rounding, copysign/fdim/fmax/fmin, the double names)
for m2mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r m2name m2flags <<< "$m2mode"
    echo "Testing test_math2_xemu.c ($m2name)..."
    compile_link_test "src/test-resources/test_math2_xemu.c" "build/test/test_math2_${m2name}_xemu.prg" "$m2flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_math2_$m2name" "build/test/test_math2_${m2name}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_math2_xemu.c $m2name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: time.h (clock from the KERNAL frame counter, CLOCKS_PER_SEC, time from
# the RTC, gmtime/mktime/asctime/ctime/difftime)
for t2mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r t2name t2flags <<< "$t2mode"
    echo "Testing test_time2_xemu.c ($t2name)..."
    compile_link_test "src/test-resources/test_time2_xemu.c" "build/test/test_time2_${t2name}_xemu.prg" "$t2flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_time2_$t2name" "build/test/test_time2_${t2name}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_time2_xemu.c $t2name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: far pointer reads/writes, arrays, arithmetic, bulk copies, banks and
# allocation policies, dma_controller.h, memcpy_optimized
for f2mode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r f2name f2flags <<< "$f2mode"
    echo "Testing test_far2_xemu.c ($f2name)..."
    compile_link_test "src/test-resources/test_far2_xemu.c" "build/test/test_far2_${f2name}_xemu.prg" "$f2flags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_far2_$f2name" "build/test/test_far2_${f2name}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_far2_xemu.c $f2name (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: <far_string.h>: memcpy/memset/strlen/strcpy/strcmp/strcat/memcmp for
# every mix of local and far pointers, typed far array elements
for fsmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r fsname fsflags <<< "$fsmode"
    echo "Testing test_far_string_xemu.c ($fsname)..."
    compile_link_test "src/test-resources/test_far_string_xemu.c" "build/test/test_far_string_${fsname}_xemu.prg" "$fsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_far_string_$fsname" "build/test/test_far_string_${fsname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_far_string_xemu.c $fsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: <audio_sid.h> (register shadow, frequencies, gate/waveform,
# instruments, mirroring, volume/filter), <audio_digi.h> (audio DMA
# registers), <audio_midi.h> (parser, synth voice allocation, pedal, bend)
for aumode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r auname auflags <<< "$aumode"
    echo "Testing test_audio_xemu.c ($auname)..."
    compile_link_test "src/test-resources/test_audio_xemu.c" "build/test/test_audio_${auname}_xemu.prg" "$auflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_audio_$auname" "build/test/test_audio_${auname}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_audio_xemu.c $auname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: constants through variables (one = 1; one * x) and the cases where a
# variable must not be taken for a constant (read first, branches, address
# taken, nested functions, loops, inline asm, a local shadowing a const global)
for cpmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r cpname cpflags <<< "$cpmode"
    echo "Testing test_constprop_xemu.c ($cpname)..."
    compile_link_test "src/test-resources/test_constprop_xemu.c" "build/test/test_constprop_${cpname}_xemu.prg" "$cpflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_constprop_$cpname" "build/test/test_constprop_${cpname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_constprop_xemu.c $cpname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: a local, parameter or for variable hides an enum constant of its
# name until its scope ends; an enum defined in a block ends with it (#286)
for esmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r esname esflags <<< "$esmode"
    echo "Testing test_enum_shadow_xemu.c ($esname)..."
    compile_link_test "src/test-resources/test_enum_shadow_xemu.c" "build/test/test_enum_shadow_${esname}_xemu.prg" "$esflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_enum_shadow_$esname" "build/test/test_enum_shadow_${esname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_enum_shadow_xemu.c $esname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the constants of byte operations are bytes: p & ~0x80 is and #127,
# not and #-129 (an assembler warning) (#288)
for bimode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r biname biflags <<< "$bimode"
    echo "Testing test_byte_imm_xemu.c ($biname)..."
    compile_link_test "src/test-resources/test_byte_imm_xemu.c" "build/test/test_byte_imm_${biname}_xemu.prg" "$biflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_byte_imm_$biname" "build/test/test_byte_imm_${biname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_byte_imm_xemu.c $biname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: break N leaves N enclosing loops/switches (a switch is a level,
# repeat(N) is not), releasing the VLAs of the loops it leaves (#71)
for bnmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r bnname bnflags <<< "$bnmode"
    echo "Testing test_break_n_xemu.c ($bnname)..."
    compile_link_test "src/test-resources/test_break_n_xemu.c" "build/test/test_break_n_${bnname}_xemu.prg" "$bnflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_break_n_$bnname" "build/test/test_break_n_${bnname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_break_n_xemu.c $bnname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: __likely / __unlikely (#75): the branch not expected to run after
# the function's return (else branches, !, __builtin_expect, loops, goto and
# case labels into cold code, stack frames, nested hints)
for lkmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r lkname lkflags <<< "$lkmode"
    echo "Testing test_likely_xemu.c ($lkname)..."
    compile_link_test "src/test-resources/test_likely_xemu.c" "build/test/test_likely_${lkname}_xemu.prg" "$lkflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_likely_$lkname" "build/test/test_likely_${lkname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_likely_xemu.c $lkname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the table-driven parameter thunk of ZP-convention SAC functions
# (__zp_args, #292): static and base-page homes, skipped slots, floats,
# pointers, a recursive caller, calls through pointers, a struct return;
# also without base-page locals
for zamode in "zp|" "zp_nolocals|-fno-zp-locals" "stack|-fno-zpcall"; do
    IFS='|' read -r zaname zaflags <<< "$zamode"
    echo "Testing test_zp_args_xemu.c ($zaname)..."
    compile_link_test "src/test-resources/test_zp_args_xemu.c" "build/test/test_zp_args_${zaname}_xemu.prg" "$zaflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_zp_args_$zaname" "build/test/test_zp_args_${zaname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_zp_args_xemu.c $zaname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: __pure / __attribute__((pure)) and __attribute__((const)) (#76):
# repeated calls share one result, unused calls are dropped, a call in a
# loop condition is hoisted (not one in a conditional block, nor a pure one
# in a loop that stores); the functions count their calls to show it
for pumode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r puname puflags <<< "$pumode"
    echo "Testing test_pure_xemu.c ($puname)..."
    compile_link_test "src/test-resources/test_pure_xemu.c" "build/test/test_pure_${puname}_xemu.prg" "$puflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_pure_$puname" "build/test/test_pure_${puname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_pure_xemu.c $puname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: values put together from bytes (a | b << 8 | (long)c << 16) built
# byte by byte, loads through constant addresses, and the cases that must
# stay OR-ed (overlapping bytes, sign extension)
for bjmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r bjname bjflags <<< "$bjmode"
    echo "Testing test_bytejoin_xemu.c ($bjname)..."
    compile_link_test "src/test-resources/test_bytejoin_xemu.c" "build/test/test_bytejoin_${bjname}_xemu.prg" "$bjflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_bytejoin_$bjname" "build/test/test_bytejoin_${bjname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_bytejoin_xemu.c $bjname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: ~ on a constant in the constant's width: ~0x8000 is 0x7FFF, not the
# long -32769; ~ and - of a char cast are ints (#289)
for bfmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r bfname bfflags <<< "$bfmode"
    echo "Testing test_bnot_fold_xemu.c ($bfname)..."
    compile_link_test "src/test-resources/test_bnot_fold_xemu.c" "build/test/test_bnot_fold_${bfname}_xemu.prg" "$bfflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_bnot_fold_$bfname" "build/test/test_bnot_fold_${bfname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_bnot_fold_xemu.c $bfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the code-size changes: constant arguments into the parameter block,
# ternaries computed into their result, dead pool stores, known tests,
# parameters used in place, wide zero tests (#290 #292 #293 #295)
for csmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r csname csflags <<< "$csmode"
    echo "Testing test_codesize_xemu.c ($csname)..."
    compile_link_test "src/test-resources/test_codesize_xemu.c" "build/test/test_codesize_${csname}_xemu.prg" "$csflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_codesize_$csname" "build/test/test_codesize_${csname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_codesize_xemu.c $csname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the scanf engine (stdlib_asm/scanf_core.s45) through sscanf:
# conversions, widths, sets, sizes, suppression, counts, floats
for scmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r scname scflags <<< "$scmode"
    echo "Testing test_scanf_xemu.c ($scname)..."
    compile_link_test "src/test-resources/test_scanf_xemu.c" "build/test/test_scanf_${scname}_xemu.prg" "$scflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_scanf_$scname" "build/test/test_scanf_${scname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_scanf_xemu.c $scname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: code overlays (#pragma cc45 overlay, ln45), copied into a region
# and (--overlay-map) mapped into a window: calls into, out of and between
# two overlays, recursion bouncing between them, function pointers to
# overlay functions, overlay data kept across swaps, disk I/O from inside
# an overlay; the overlay file on drive 8. Then two link errors: an
# interrupt handler in an overlay, and an overlay too large for the room
# below the stack
# (and in two banks, two files: a range in bank 4 with room for overlay a
# only, then bank 5: OVLTEST.OV1 and OVLTEST.OV2, #299)
OV2="--overlay-store=0x4FC00-0x4FFFF,0x50000-0x5FFFF"
for ovmode in "zp|$STDLIB||" "stack|$STDLIB_STACK|-fno-zpcall|" \
              "zp_map|$STDLIB||--overlay-map" "stack_map|$STDLIB_STACK|-fno-zpcall|--overlay-map" \
              "zp_two|$STDLIB||$OV2" "stack_two_map|$STDLIB_STACK|-fno-zpcall|--overlay-map $OV2"; do
    IFS='|' read -r ovname ovlib ovflags ovlink <<< "$ovmode"
    echo "Testing code overlays ($ovname)..."
    ovd=build/test/overlay_$ovname
    rm -rf $ovd; mkdir -p $ovd
    ovok=1
    for f in main ovl_a ovl_b; do
        $CC $ovflags -c src/test-resources/overlay/$f.c -o $ovd/$f.o45 >/dev/null 2>&1 || ovok=0
    done
    if [ $ovok = 1 ] && $LD -basic $ovlink -o $ovd/ovltest.prg $ovd/main.o45 $ovd/ovl_a.o45 $ovd/ovl_b.o45 $ovlib >/dev/null 2>&1 &&
       ./bin/disk45 create $ovd/ovl.d81 -n OVLTEST >/dev/null 2>&1 &&
       ./bin/disk45 add $ovd/ovl.d81 $ovd/ovltest.ov1 OVLTEST.OV1 >/dev/null 2>&1 &&
       { [ ! -f $ovd/ovltest.ov2 ] || ./bin/disk45 add $ovd/ovl.d81 $ovd/ovltest.ov2 OVLTEST.OV2 >/dev/null 2>&1; }; then
        run_xemu_test "test_overlay_$ovname" "$ovd/ovltest.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA" -8 "$PWD/$ovd/ovl.d81"
    else
        echo "FAIL: code overlays $ovname (build or link failed)"
        failed=$((failed + 1))
    fi
done
# nm45 and objdump45 read the overlay files: the description (.ovd, which
# the program never loads: the code file has none of it, #298) and the code
# (.ov1), either named; their overlays and symbols at the addresses they run
# at (copied and mapped)
ovtools=1
for ovname in zp zp_map; do
    for ovf in build/test/overlay_$ovname/ovltest.ovd build/test/overlay_$ovname/ovltest.ov1; do
        ./bin/nm45 $ovf 2>&1 | grep -q "T _a_add  \[overlay a\]" &&
        ./bin/nm45 -h $ovf 2>&1 | grep -q "^  2 b " &&
        ./bin/objdump45 -h $ovf 2>&1 | grep -q "^  1 a " &&
        ./bin/objdump45 -d $ovf 2>&1 | grep -q "<_b_mul>:" || ovtools=0
    done
    ! grep -q OVL45 build/test/overlay_$ovname/ovltest.ov1 || ovtools=0
done
# the linker map: the overlays kept in bank 5, an overlay object where it
# runs, a stub tagged with its overlay, the overlays' own symbols
ovd=build/test/overlay_zp
$LD -basic -M $ovd/ovltest.map -o $ovd/ovlmap.prg $ovd/main.o45 $ovd/ovl_a.o45 $ovd/ovl_b.o45 $STDLIB >/dev/null 2>&1 &&
grep -qF 'kept in bank 5 $0000 - ' $ovd/ovltest.map &&
grep -qF 'ovl_a.o45  [overlay a]' $ovd/ovltest.map &&
grep -qF '[stub: overlay a, $' $ovd/ovltest.map &&
grep -qF '  overlay b (bank 5 ' $ovd/ovltest.map || ovtools=0
# two banks: overlay b in OVLTEST.OV2 in bank 5, both files in the map and
# read by nm45 / objdump45 from either file
ovd=build/test/overlay_zp_two
[ -f $ovd/ovltest.ov2 ] && [ ! -f $ovd/ovltest.ov3 ] &&
./bin/nm45 -h $ovd/ovltest.ovd 2>&1 | grep -q "^  2 b                kept \$50000" &&
./bin/objdump45 -d $ovd/ovltest.ov2 2>&1 | grep -q "<_b_mul>:" &&
$LD -basic -M $ovd/two.map $OV2 -o $ovd/twomap.prg $ovd/main.o45 $ovd/ovl_a.o45 $ovd/ovl_b.o45 $STDLIB >/dev/null 2>&1 &&
grep -qF '(file TWOMAP.OV2, ' $ovd/two.map && grep -qF 'b              kept in bank 5 $0000' $ovd/two.map || ovtools=0
if [ $ovtools = 1 ]; then
    echo "SUCCESS: nm45, objdump45 and the linker map show code overlays"
    passed=$((passed + 1))
else
    echo "FAIL: nm45, objdump45 and the linker map show code overlays"
    failed=$((failed + 1))
fi
ovd=build/test/overlay_err
mkdir -p $ovd
printf '#pragma cc45 overlay(irq)\n__interrupt void handler(void) { }\nint main(void) { return 0; }\n' > $ovd/isr.c
printf '    .overlay big\n    .code\n.global _big\nproc _big\n    .res 40000\n    rts\n    .func_flags zp_call\nendproc\n' > $ovd/big.s45
printf 'void big(void);\nint main(void) { big(); return 0; }\n' > $ovd/bigmain.c
if $CC -c $ovd/isr.c -o $ovd/isr.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $ovd/isr.prg $ovd/isr.o45 $STDLIB > $ovd/isr.log 2>&1 && grep -q "interrupt handler" $ovd/isr.log &&
   $AS -c $ovd/big.s45 -o $ovd/big.o45 >/dev/null 2>&1 && $CC -c $ovd/bigmain.c -o $ovd/bigmain.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $ovd/big.prg $ovd/bigmain.o45 $ovd/big.o45 $STDLIB > $ovd/big.log 2>&1 && grep -q "does not fit" $ovd/big.log; then
    echo "SUCCESS: code overlay link errors (an interrupt handler, too large)"
    passed=$((passed + 1))
else
    echo "FAIL: code overlay link errors ($(cat $ovd/isr.log $ovd/big.log 2>/dev/null | head -3))"
    failed=$((failed + 1))
fi
# the overlay store (#299): one range too small names the next bank as a
# second range; a range across banks, a second range not in the next bank
# and (two files) a name without a digit to count up are refused
ovd=build/test/overlay_zp
ovs=build/test/overlay_err/store.log
if ! $LD -basic --overlay-store=0x4FC00 -o $ovd/s1.prg $ovd/main.o45 $ovd/ovl_a.o45 $ovd/ovl_b.o45 $STDLIB > $ovs 2>&1 &&
   grep -q "overlay b (.*does not fit in the overlay store.*--overlay-store=0x4FC00-0x4FFFF,0x50000-0x5FFFF" $ovs &&
   ! $LD -basic --overlay-store=0x4FC00-0x50FFF -o $ovd/s2.prg $ovd/main.o45 $STDLIB > $ovs 2>&1 && grep -q "one bank" $ovs &&
   ! $LD -basic --overlay-store=0x40000-0x4FFFF,0x30000-0x3FFFF -o $ovd/s3.prg $ovd/main.o45 $STDLIB > $ovs 2>&1 &&
   grep -q "next bank" $ovs &&
   ! $LD -basic $OV2 --overlay-name=OVLX -o $ovd/s4.prg $ovd/main.o45 $ovd/ovl_a.o45 $ovd/ovl_b.o45 $STDLIB > $ovs 2>&1 &&
   grep -q "must end in a digit" $ovs; then
    echo "SUCCESS: overlay store ranges: too small, across banks, not the next bank, a name to count up"
    passed=$((passed + 1))
else
    echo "FAIL: overlay store ranges ($(head -2 $ovs))"
    failed=$((failed + 1))
fi

# Test: the SAC overlay shares the static-allocation blocks of functions in
# code overlays (#302): a chain overlay o1 -> resident -> overlay o2 keeps
# its storage apart (o1_f's array survives o2_g), two functions never
# active together share theirs (the map: o1_h__ar and o2_k__ar at one
# address, o1_f__ar and o2_g__ar at two)
for somode in "zp|$STDLIB||" "stack|$STDLIB_STACK|-fno-zpcall|" "zp_map|$STDLIB||--overlay-map"; do
    IFS='|' read -r soname solib soflags solink <<< "$somode"
    echo "Testing SAC overlay of code overlays ($soname)..."
    sod=build/test/sacovl_$soname
    rm -rf $sod; mkdir -p $sod
    sook=1
    for f in main o1 o2; do $CC $soflags -c src/test-resources/sacovl/$f.c -o $sod/$f.o45 >/dev/null 2>&1 || sook=0; done
    if [ $sook = 1 ] && $LD -basic $solink -M $sod/t.map -o $sod/sotest.prg $sod/main.o45 $sod/o1.o45 $sod/o2.o45 $solib >/dev/null 2>&1 &&
       ./bin/disk45 create $sod/so.d81 -n SOTEST >/dev/null 2>&1 &&
       ./bin/disk45 add $sod/so.d81 $sod/sotest.ov1 SOTEST.OV1 >/dev/null 2>&1; then
        run_xemu_test "test_sacovl_$soname" "$sod/sotest.prg" "C0C0" 5 "01 01 01 01 AA" -8 "$PWD/$sod/so.d81"
        soaddr() { awk -v s="$1" '$2 == s { print $1; exit }' $sod/t.map; }
        if [ -n "$(soaddr _o1_h__ar)" ] && [ "$(soaddr _o1_h__ar)" = "$(soaddr _o2_k__ar)" ] &&
           [ -n "$(soaddr _o2_g__ar)" ] && [ "$(soaddr _o1_f__ar)" != "$(soaddr _o2_g__ar)" ]; then
            echo "SUCCESS: SAC overlay of code overlays ($soname): shared, and apart along the chain"
            passed=$((passed + 1))
        else
            echo "FAIL: SAC overlay of code overlays ($soname): $(grep -E '_(o1_f|o2_g|o1_h|o2_k)__ar ' $sod/t.map | tr -s ' ' | tr '\n' ';')"
            failed=$((failed + 1))
        fi
    else
        echo "FAIL: SAC overlay of code overlays $soname (build or link failed)"
        failed=$((failed + 1))
    fi
done

# Test: constants kept with an overlay's code (#301): __noescape objects and
# #pragma cc45 noescape_strings literals in a code overlay (copied, mapped)
# and in __far functions; __escape and stored literals stay resident
for nemode in "zp|$STDLIB||" "stack|$STDLIB_STACK|-fno-zpcall|" \
              "zp_map|$STDLIB||--overlay-map" "stack_map|$STDLIB_STACK|-fno-zpcall|--overlay-map"; do
    IFS='|' read -r nename nelib neflags nelink <<< "$nemode"
    echo "Testing __noescape ($nename)..."
    ned=build/test/noescape_$nename
    rm -rf $ned; mkdir -p $ned
    neok=1
    for f in main ovl_a ovl_b far; do
        $CC $neflags -c src/test-resources/noescape/$f.c -o $ned/$f.o45 >/dev/null 2>&1 || neok=0
    done
    if [ $neok = 1 ] && $LD -basic $nelink -M $ned/t.map -o $ned/netest.prg $ned/main.o45 $ned/ovl_a.o45 $ned/ovl_b.o45 \
            $ned/far.o45 $nelib >/dev/null 2>&1 &&
       ./bin/disk45 create $ned/ne.d81 -n NETEST >/dev/null 2>&1 &&
       ./bin/disk45 add $ned/ne.d81 $ned/netest.ov1 NETEST.OV1 >/dev/null 2>&1; then
        run_xemu_test "test_noescape_$nename" "$ned/netest.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA" -8 "$PWD/$ned/ne.d81"
    else
        echo "FAIL: __noescape $nename (build or link failed)"
        failed=$((failed + 1))
    fi
done
# placement: overlay a keeps only what must stay resident (the pad, the
# stored pointer, "stored", the __escape strings: 15 bytes), the far object
# only its resident function's literal; a reference to an overlay's
# __noescape data from outside it, and __noescape on a writable object, are
# errors
ned=build/test/noescape_zp
nee=build/test/noescape_err.log
printf 'extern const unsigned char weights[4];\nunsigned char peek3(void) { return weights[3]; }\n' > $ned/out.c
printf 'static char __noescape buf[4] = "abc";\nchar f(void) { return buf[0]; }\n' > $ned/rw.c
if grep -A3 'ovl_a.o45  \[overlay a\]' $ned/t.map | grep -q 'DATA .*(15 bytes)' &&
   grep -A3 'far.o45$' $ned/t.map | grep -q 'overlay far1' && grep -A3 'far.o45$' $ned/t.map | grep -q 'DATA .*(10 bytes)' &&
   $CC -c $ned/out.c -o $ned/out.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $ned/out.prg $ned/main.o45 $ned/ovl_a.o45 $ned/ovl_b.o45 $ned/far.o45 $ned/out.o45 $STDLIB > $nee 2>&1 &&
   grep -q "'_weights' is __noescape data kept in code overlay a" $nee &&
   ! $CC -c $ned/rw.c -o $ned/rw.o45 > $nee 2>&1 && grep -q "__noescape needs a read-only object" $nee; then
    echo "SUCCESS: __noescape placement; outside references and writable objects refused"
    passed=$((passed + 1))
else
    echo "FAIL: __noescape placement or errors ($(head -2 $nee 2>/dev/null))"
    failed=$((failed + 1))
fi
# the warning (#301 phase 2): overlay a passes kept data to msgf (resident,
# calls vfmt in overlay fmt) and through a function pointer that can reach
# fmt: two warnings; to show (resident), to a function of a itself, and
# __escape literals: none
ned=build/test/noescape_warn
mkdir -p $ned
for f in warn_a warn_fmt warn_main; do $CC -c src/test-resources/noescape/$f.c -o $ned/$f.o45 >/dev/null 2>&1; done
$LD -basic -o $ned/w.prg $ned/warn_main.o45 $ned/warn_a.o45 $ned/warn_fmt.o45 $STDLIB > $ned/w.log 2>&1
if [ "$(grep -c 'passes the address of __noescape data' $ned/w.log)" = 2 ] &&
   grep -q "line 12: '_a_run' (code overlay a) passes the address of __noescape data to '_msgf', which can call into code overlay fmt (_msgf -> _vfmt)" $ned/w.log &&
   grep -q "line 16: '_a_run' (code overlay a) passes the address of __noescape data to a call through a pointer" $ned/w.log; then
    echo "SUCCESS: __noescape data passed to code that can reach another overlay is warned about"
    passed=$((passed + 1))
else
    echo "FAIL: __noescape warning ($(head -3 $ned/w.log))"
    failed=$((failed + 1))
fi

# Test: __far functions (cc45), kept in far memory and mapped into the window
# when called (ln45 packs each file's far code into automatic overlays far1,
# far2, ...): calls from resident code, between far functions of one overlay
# and of two, from far code to resident code, static far functions, function
# pointers both ways, recursion between two overlays, far_malloc filling
# bank 5 without touching the overlays kept there, setjmp/longjmp across
# overlay calls; both conventions, with
# and without function sections. Then: a far interrupt handler is a link
# error, the map shows the automatic overlays, objdump45 disassembles them
for farmode in "zp|$STDLIB|" "stack|$STDLIB_STACK|-fno-zpcall" \
               "zp_sections|$STDLIB|-ffunction-sections" "stack_sections|$STDLIB_STACK|-fno-zpcall -ffunction-sections"; do
    IFS='|' read -r farname farlib farflags <<< "$farmode"
    echo "Testing __far functions ($farname)..."
    fard=build/test/far_$farname
    rm -rf $fard; mkdir -p $fard
    farok=1
    for f in main far_a far_b; do
        $CC $farflags -c src/test-resources/far/$f.c -o $fard/$f.o45 >/dev/null 2>&1 || farok=0
    done
    if [ $farok = 1 ] && $LD -basic -M $fard/fartest.map -o $fard/fartest.prg $fard/main.o45 $fard/far_a.o45 $fard/far_b.o45 $farlib >/dev/null 2>&1 &&
       ./bin/disk45 create $fard/far.d81 -n FARTEST >/dev/null 2>&1 &&
       ./bin/disk45 add $fard/far.d81 $fard/fartest.ov1 FARTEST.OV1 >/dev/null 2>&1; then
        run_xemu_test "test_far_$farname" "$fard/fartest.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA" -8 "$PWD/$fard/far.d81"
    else
        echo "FAIL: __far functions $farname (build or link failed)"
        failed=$((failed + 1))
    fi
done
# The automatic overlays follow the calls: far code of a.c (5 KB) calls
# d.c's (2.5 KB); with b.c (2 KB) and c.c (5 KB) linked between them,
# first fit in command-line order would put a with b and c with d. Packed
# by the calls, a and d share far1, c and b far2
fard=build/test/far_pack
rm -rf $fard; mkdir -p $fard
printf 'void d_fn(void);\n__far void a_fn(void) { d_fn(); }\n__far __naked void a_pad(void) { __asm__(".res 5000"); }\n' > $fard/a.c
printf '__far __naked void b_pad(void) { __asm__(".res 2000"); }\n' > $fard/b.c
printf '__far __naked void c_pad(void) { __asm__(".res 5000"); }\n' > $fard/c.c
printf '__far void d_fn(void) { }\n__far __naked void d_pad(void) { __asm__(".res 2500"); }\nvoid a_fn(void);\nvoid b_pad(void);\nvoid c_pad(void);\nint main(void) { a_fn(); b_pad(); c_pad(); return 0; }\n' > $fard/d.c
farok=1
for f in a b c d; do $CC -c $fard/$f.c -o $fard/$f.o45 >/dev/null 2>&1 || farok=0; done
if [ $farok = 1 ] && $LD -basic -M $fard/pack.map -o $fard/pack.prg $fard/a.o45 $fard/b.o45 $fard/c.o45 $fard/d.o45 $STDLIB >/dev/null 2>&1 &&
   grep -A2 "far_pack/a.o45" $fard/pack.map | grep -q "overlay far1," &&
   grep -A2 "far_pack/d.o45" $fard/pack.map | grep -q "overlay far1," &&
   grep -A2 "far_pack/c.o45" $fard/pack.map | grep -q "overlay far2," &&
   grep -A2 "far_pack/b.o45" $fard/pack.map | grep -q "overlay far2,"; then
    echo "SUCCESS: __far: the automatic overlays follow the calls"
    passed=$((passed + 1))
else
    echo "FAIL: __far: the automatic overlays follow the calls ($(grep 'overlay far' $fard/pack.map 2>/dev/null | tr '\n' ' '))"
    failed=$((failed + 1))
fi
fard=build/test/far_zp
printf '__far __interrupt void handler(void) { }\nint main(void) { return 0; }\n' > $fard/isr.c
# (recurse: stack frames, so the static-allocation check has nothing to say)
printf 'volatile int n;\n#pragma cc45 recurse\n__far void tick(void) { n++; }\n#pragma cc45 recurse\n__attribute__((noinline)) void helper(void) { tick(); }\n__interrupt void handler(void) { helper(); }\nint main(void) { tick(); return 0; }\n' > $fard/isrcall.c
printf 'volatile int n;\n#pragma cc45 recurse\n__far void tick(void) { n++; }\nvoid (*hook)(void) = tick;\n__interrupt void handler(void) { hook(); }\nint main(void) { return 0; }\n' > $fard/isrptr.c
if $CC -c $fard/isr.c -o $fard/isr.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $fard/isr.prg $fard/isr.o45 $STDLIB > $fard/isr.log 2>&1 && grep -q "interrupt handler" $fard/isr.log &&
   $CC -c $fard/isrcall.c -o $fard/isrcall.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $fard/isrcall.prg $fard/isrcall.o45 $STDLIB > $fard/isrcall.log 2>&1 &&
   grep -qF "is reachable from interrupt handler '_handler' (_handler -> _helper -> _tick)" $fard/isrcall.log &&
   $CC -c $fard/isrptr.c -o $fard/isrptr.o45 >/dev/null 2>&1 &&
   ! $LD -basic -o $fard/isrptr.prg $fard/isrptr.o45 $STDLIB > $fard/isrptr.log 2>&1 &&
   grep -qF "is reachable from interrupt handler '_handler'" $fard/isrptr.log &&
   grep -qF '    far1           kept in bank 5 $0000 - ' $fard/fartest.map &&
   grep -qF '    far2           kept in bank 5 ' $fard/fartest.map &&
   grep -qF '(stub $' $fard/fartest.map &&
   ./bin/objdump45 -d $fard/fartest.ov1 2>&1 | grep -q "<_fb_mul>:"; then
    echo "SUCCESS: __far: interrupt handlers refused (far, or reaching far code), map and objdump45 show the automatic overlays"
    passed=$((passed + 1))
else
    echo "FAIL: __far: interrupt handlers refused (far, or reaching far code), map and objdump45 show the automatic overlays ($(cat $fard/isr.log $fard/isrcall.log $fard/isrptr.log 2>/dev/null | head -3))"
    failed=$((failed + 1))
fi

# Test: <input.h> with keys pressed through the virtual key registers
# ($D615/$D616: the real path of typed keys), joysticks
for inmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r inname inflags <<< "$inmode"
    echo "Testing test_input_xemu.c ($inname)..."
    compile_link_test "src/test-resources/test_input_xemu.c" "build/test/test_input_${inname}_xemu.prg" "$inflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_input_$inname" "build/test/test_input_${inname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_input_xemu.c $inname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: conio.h on the 80x25 screen (screen codes, colour RAM, wrapping,
# scrolling, revers, kbhit/getch from the KERNAL keyboard buffer)
for comode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r coname coflags <<< "$comode"
    echo "Testing test_conio_xemu.c ($coname)..."
    compile_link_test "src/test-resources/test_conio_xemu.c" "build/test/test_conio_${coname}_xemu.prg" "$coflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_conio_$coname" "build/test/test_conio_${coname}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_conio_xemu.c $coname (compilation/linking failed)"
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

# Test: function declarators in declaration lists (int f(void), g(void); int x, f(void);)
for dlmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r dlname dlflags <<< "$dlmode"
    echo "Testing test_decl_lists_xemu.c ($dlname)..."
    compile_link_test "src/test-resources/test_decl_lists_xemu.c" "build/test/test_decl_lists_${dlname}_xemu.prg" "$dlflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_decl_lists_$dlname" "build/test/test_decl_lists_${dlname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_decl_lists_xemu.c $dlname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: read-modify-write through a pointer done in place (p->f += v, p->f++)
for rmmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r rmname rmflags <<< "$rmmode"
    echo "Testing test_rmw_ptr_xemu.c ($rmname)..."
    compile_link_test "src/test-resources/test_rmw_ptr_xemu.c" "build/test/test_rmw_ptr_${rmname}_xemu.prg" "$rmflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_rmw_ptr_$rmname" "build/test/test_rmw_ptr_${rmname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_rmw_ptr_xemu.c $rmname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: unsigned comparisons with 0 and 1 (folded to tests for zero)
for ucmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r ucname ucflags <<< "$ucmode"
    echo "Testing test_ucmp_zero_xemu.c ($ucname)..."
    compile_link_test "src/test-resources/test_ucmp_zero_xemu.c" "build/test/test_ucmp_zero_${ucname}_xemu.prg" "$ucflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_ucmp_zero_$ucname" "build/test/test_ucmp_zero_${ucname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_ucmp_zero_xemu.c $ucname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: the heap of a program ending above $B000 (#pragma cc45 stack_size);
# without the pragma ln45 warns that there is no heap
for hlmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r hlname hlflags <<< "$hlmode"
    echo "Testing test_heap_layout_xemu.c ($hlname)..."
    compile_link_test "src/test-resources/test_heap_layout_xemu.c" "build/test/test_heap_layout_${hlname}_xemu.prg" "$hlflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_heap_layout_$hlname" "build/test/test_heap_layout_${hlname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_heap_layout_xemu.c $hlname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
grep -v 'pragma cc45 stack_size' src/test-resources/test_heap_layout_xemu.c > build/test/test_heap_layout_nopragma.c
if $CC build/test/test_heap_layout_nopragma.c -o build/test/test_heap_layout_nopragma.prg 2>&1 |
   grep -qE "ln45: warning: (no heap: the program ends at|only [0-9]+ bytes for the heap)"; then
    echo "SUCCESS: ln45 warns when a program using malloc leaves (almost) no room for the heap"
    passed=$((passed + 1))
else
    echo "FAIL: ln45 'no heap' warning"
    failed=$((failed + 1))
fi

# Test: printf("text") without % is written without the formatter (puts /
# putstr): the same screen as through "%s", and a program far smaller
echo "Testing test_printf_literal_xemu.c (printf of plain text)..."
pl_ok=0
if $CC src/test-resources/test_printf_literal_xemu.c -o build/test/test_printf_literal_xemu.prg >/dev/null 2>&1 &&
   $CC -DFORMATTED src/test-resources/test_printf_literal_xemu.c -o build/test/test_printf_literal_fmt_xemu.prg >/dev/null 2>&1; then
    for plv in "" "_fmt"; do
        timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "build/test/test_printf_literal${plv}.bin" \
            -prg "build/test/test_printf_literal${plv}_xemu.prg" </dev/null >/dev/null 2>&1
        # the program's output starts on the screen row after the banner ($0800 + 9 * 80)
        dd if="build/test/test_printf_literal${plv}.bin" bs=1 skip=$((0x800 + 720)) count=400 2>/dev/null \
            > "build/test/test_printf_literal${plv}.scr"
    done
    plsize=$(stat -c %s build/test/test_printf_literal_xemu.prg)
    if cmp -s build/test/test_printf_literal.scr build/test/test_printf_literal_fmt.scr &&
       [ "$(head -c 5 build/test/test_printf_literal.scr)" = "HELLO" ] && [ "$plsize" -lt 1500 ]; then
        pl_ok=1
    fi
fi
if [ $pl_ok -eq 1 ]; then
    echo "SUCCESS: test_printf_literal (same screen as %s, $plsize bytes)"
    passed=$((passed + 1))
else
    echo "FAIL: test_printf_literal (screen differs from %s, or the formatter was linked)"
    failed=$((failed + 1))
fi

# Test: printf's digit hooks (both conventions), linked only as the format
# strings need them
for phmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r phname phflags <<< "$phmode"
    echo "Testing test_printf_hooks_xemu.c ($phname)..."
    compile_link_test "src/test-resources/test_printf_hooks_xemu.c" "build/test/test_printf_hooks_${phname}_xemu.prg" "$phflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_printf_hooks_$phname" "build/test/test_printf_hooks_${phname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_printf_hooks_xemu.c $phname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
ph_map() {   # ph_map <C body of main> -> the link map
    printf '#include <stdio.h>\nint main(void) { %s return 0; }\n' "$1" > build/test/ph_map.c
    $CC -c build/test/ph_map.c -o build/test/ph_map.o45 >/dev/null 2>&1 &&
        $LD -basic -m -o build/test/ph_map.prg lib/build/crt0_zp.o45 build/test/ph_map.o45 $STDLIB 2>/dev/null
}
M1=$(ph_map 'printf("%s\n", "x");')
M2=$(ph_map 'printf("%d\n", 5);')
M3=$(ph_map 'void *p = (void *)printf; printf("%s", "x"); if (!p) return 1;')
# (the markers: the hooks' names are also the core's weak stubs)
if [ -n "$M1" ] && ! grep -q '___fmt_.*_linked' <<< "$M1" &&
   grep -q '___fmt_int_linked' <<< "$M2" && ! grep -q '___fmt_long_linked' <<< "$M2" &&
   grep -q '___fmt_long_linked' <<< "$M3"; then
    echo "SUCCESS: printf's digit hooks linked as the formats need (%s: none, %d: 16-bit, printf as a value: all)"
    passed=$((passed + 1))
else
    echo "FAIL: printf's digit hooks linked wrongly"
    failed=$((failed + 1))
fi

# Test: cc45 -ffunction-sections trims the program's own code too (ln45 --gc-inputs)
for gimode in "zp|-ffunction-sections" "stack|-ffunction-sections -fno-zpcall"; do
    IFS='|' read -r giname giflags <<< "$gimode"
    echo "Testing test_gc_inputs_xemu.c ($giname)..."
    if $CC $giflags src/test-resources/test_gc_inputs_xemu.c -o build/test/test_gc_inputs_${giname}_xemu.prg >/dev/null 2>&1; then
        run_xemu_test "test_gc_inputs_$giname" "build/test/test_gc_inputs_${giname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_gc_inputs_xemu.c $giname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
if $CC -ffunction-sections -c src/test-resources/test_gc_inputs_xemu.c -o build/test/test_gc_inputs.o45 >/dev/null 2>&1 &&
   GI=$($LD -basic --gc-inputs --print-gc -m -o build/test/test_gc_inputs_map.prg lib/build/crt0_zp.o45 \
        build/test/test_gc_inputs.o45 $STDLIB 2>&1) &&
   grep -q "dropped section .*: _unused " <<< "$GI" && grep -q "dropped section .*: _big_table " <<< "$GI" &&
   grep -q " _via_ptr_b$" <<< "$GI" && grep -q " _inner$" <<< "$GI"; then
    echo "SUCCESS: --gc-inputs drops the program's unreached function and data, keeps what is reached"
    passed=$((passed + 1))
else
    echo "FAIL: --gc-inputs"
    failed=$((failed + 1))
fi

# Test: the printf formatter's flags, width, precision, %n, snprintf
for pfmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r pfname pfflags <<< "$pfmode"
    echo "Testing test_printf_format_xemu.c ($pfname)..."
    compile_link_test "src/test-resources/test_printf_format_xemu.c" "build/test/test_printf_format_${pfname}_xemu.prg" "$pfflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_printf_format_$pfname" "build/test/test_printf_format_${pfname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_printf_format_xemu.c $pfname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: calls through pointers to variadic functions (stack convention)
for fvmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r fvname fvflags <<< "$fvmode"
    echo "Testing test_fnptr_varargs_xemu.c ($fvname)..."
    compile_link_test "src/test-resources/test_fnptr_varargs_xemu.c" "build/test/test_fnptr_varargs_${fvname}_xemu.prg" "$fvflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_fnptr_varargs_$fvname" "build/test/test_fnptr_varargs_${fvname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_fnptr_varargs_xemu.c $fvname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: pointer locals and parameters in ZP registers in leaf functions
for zpmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r zpname zpflags <<< "$zpmode"
    echo "Testing test_zp_pointers_xemu.c ($zpname)..."
    compile_link_test "src/test-resources/test_zp_pointers_xemu.c" "build/test/test_zp_pointers_${zpname}_xemu.prg" "$zpflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_zp_pointers_$zpname" "build/test/test_zp_pointers_${zpname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_zp_pointers_xemu.c $zpname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: stored values taken where they live (byte of a variable, constants)
for svmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r svname svflags <<< "$svmode"
    echo "Testing test_store_value_xemu.c ($svname)..."
    compile_link_test "src/test-resources/test_store_value_xemu.c" "build/test/test_store_value_${svname}_xemu.prg" "$svflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_store_value_$svname" "build/test/test_store_value_${svname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_store_value_xemu.c $svname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: a leaf function's parameters used in the ZP parameter block
for zqmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r zqname zqflags <<< "$zqmode"
    echo "Testing test_zp_params_xemu.c ($zqname)..."
    compile_link_test "src/test-resources/test_zp_params_xemu.c" "build/test/test_zp_params_${zqname}_xemu.prg" "$zqflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_zp_params_$zqname" "build/test/test_zp_params_${zqname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_zp_params_xemu.c $zqname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: large stack frames (pushed in a loop, dropped by setting SP)
for flmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r flname flflags <<< "$flmode"
    echo "Testing test_frame_large_xemu.c ($flname)..."
    compile_link_test "src/test-resources/test_frame_large_xemu.c" "build/test/test_frame_large_${flname}_xemu.prg" "$flflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_frame_large_$flname" "build/test/test_frame_large_${flname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_frame_large_xemu.c $flname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: ++/-- on variables updated in place
for idmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r idname idflags <<< "$idmode"
    echo "Testing test_incdec_xemu.c ($idname)..."
    compile_link_test "src/test-resources/test_incdec_xemu.c" "build/test/test_incdec_${idname}_xemu.prg" "$idflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_incdec_$idname" "build/test/test_incdec_${idname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_incdec_xemu.c $idname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: qsort and bsearch
for qsmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r qsname qsflags <<< "$qsmode"
    echo "Testing test_qsort_xemu.c ($qsname)..."
    compile_link_test "src/test-resources/test_qsort_xemu.c" "build/test/test_qsort_${qsname}_xemu.prg" "$qsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_qsort_$qsname" "build/test/test_qsort_${qsname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_qsort_xemu.c $qsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: division and remainder (library routines; inline for a 32-bit stack-frame dividend)
for dmmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r dmname dmflags <<< "$dmmode"
    echo "Testing test_divmod_xemu.c ($dmname)..."
    compile_link_test "src/test-resources/test_divmod_xemu.c" "build/test/test_divmod_${dmname}_xemu.prg" "$dmflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_divmod_$dmname" "build/test/test_divmod_${dmname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_divmod_xemu.c $dmname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: signed compares with 0 as a sign test
for csmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r csname csflags <<< "$csmode"
    echo "Testing test_cmp_sign_xemu.c ($csname)..."
    compile_link_test "src/test-resources/test_cmp_sign_xemu.c" "build/test/test_cmp_sign_${csname}_xemu.prg" "$csflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_cmp_sign_$csname" "build/test/test_cmp_sign_${csname}_xemu.prg" "C0C0" 7 "01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_cmp_sign_xemu.c $csname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: signed 16-bit compares (subtraction form)
for cgmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r cgname cgflags <<< "$cgmode"
    echo "Testing test_cmp_signed_xemu.c ($cgname)..."
    compile_link_test "src/test-resources/test_cmp_signed_xemu.c" "build/test/test_cmp_signed_${cgname}_xemu.prg" "$cgflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_cmp_signed_$cgname" "build/test/test_cmp_signed_${cgname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_cmp_signed_xemu.c $cgname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: chars in int arithmetic (a + (b + c) kept the carry), as int
# arguments (widened by their own signedness) and against signed ints
# (signed: the char promotes to int), also with -fsigned-default
for cpmode in "zp|" "stack|-fno-zpcall" "zp_sd|-fsigned-default" "stack_sd|-fno-zpcall -fsigned-default"; do
    IFS='|' read -r cpname cpflags <<< "$cpmode"
    echo "Testing test_char_promotion_xemu.c ($cpname)..."
    compile_link_test "src/test-resources/test_char_promotion_xemu.c" "build/test/test_char_promotion_${cpname}_xemu.prg" "$cpflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_char_promotion_$cpname" "build/test/test_char_promotion_${cpname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_char_promotion_xemu.c $cpname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: a stored comparison reloaded for a branch's flags (ca45 redundant
# load, Issue #282)
for rlmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r rlname rlflags <<< "$rlmode"
    echo "Testing test_redundant_load_flags_xemu.c ($rlname)..."
    compile_link_test "src/test-resources/test_redundant_load_flags_xemu.c" "build/test/test_redundant_load_flags_${rlname}_xemu.prg" "$rlflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_redundant_load_flags_$rlname" "build/test/test_redundant_load_flags_${rlname}_xemu.prg" "C0C0" 3 "01 01 AA"
    else
        echo "FAIL: test_redundant_load_flags_xemu.c $rlname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: int parameters every call passes 0-255, passed as bytes and read
# zero-extended from where they live (cc45 parameter narrowing)
for pnmode in "zp|" "stack|-fno-zpcall" "zp_sd|-fsigned-default" "stack_sd|-fno-zpcall -fsigned-default"; do
    IFS='|' read -r pnname pnflags <<< "$pnmode"
    echo "Testing test_param_narrowing_xemu.c ($pnname)..."
    compile_link_test "src/test-resources/test_param_narrowing_xemu.c" "build/test/test_param_narrowing_${pnname}_xemu.prg" "$pnflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_param_narrowing_$pnname" "build/test/test_param_narrowing_${pnname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_param_narrowing_xemu.c $pnname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: multiplication as library calls (-Os = -O2 -fmul-call)
for mcmode in "zp|-Os" "stack|-Os -fno-zpcall"; do
    IFS='|' read -r mcname mcflags <<< "$mcmode"
    echo "Testing test_mul_call_xemu.c ($mcname)..."
    compile_link_test "src/test-resources/test_mul_call_xemu.c" "build/test/test_mul_call_${mcname}_xemu.prg" "$mcflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_mul_call_$mcname" "build/test/test_mul_call_${mcname}_xemu.prg" "C0C0" 4 "01 01 01 AA"
    else
        echo "FAIL: test_mul_call_xemu.c $mcname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: static locals (uninitialized ones in BSS)
for slmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r slname slflags <<< "$slmode"
    echo "Testing test_static_local_xemu.c ($slname)..."
    compile_link_test "src/test-resources/test_static_local_xemu.c" "build/test/test_static_local_${slname}_xemu.prg" "$slflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_static_local_$slname" "build/test/test_static_local_${slname}_xemu.prg" "C0C0" 3 "01 01 AA"
    else
        echo "FAIL: test_static_local_xemu.c $slname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: sizeof of arrays of pointers, array members and array type names
for szmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r szname szflags <<< "$szmode"
    echo "Testing test_sizeof_xemu.c ($szname)..."
    compile_link_test "src/test-resources/test_sizeof_xemu.c" "build/test/test_sizeof_${szname}_xemu.prg" "$szflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_sizeof_$szname" "build/test/test_sizeof_${szname}_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
    else
        echo "FAIL: test_sizeof_xemu.c $szname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: variadic functions with static allocation
for vsmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r vsname vsflags <<< "$vsmode"
    echo "Testing test_varargs_sac_xemu.c ($vsname)..."
    compile_link_test "src/test-resources/test_varargs_sac_xemu.c" "build/test/test_varargs_sac_${vsname}_xemu.prg" "$vsflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_varargs_sac_$vsname" "build/test/test_varargs_sac_${vsname}_xemu.prg" "C0C0" 6 "01 01 01 01 01 AA"
    else
        echo "FAIL: test_varargs_sac_xemu.c $vsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: a weak function with static storage overridden by a strong one while
# its object stays linked (its storage symbols are weak), with and without GC
for wsmode in "zp|||lib/build/c45.lib" "stack|-fno-zpcall||lib/build/c45_stack.lib" "nogc||--no-gc|lib/build/c45.lib"; do
    IFS='|' read -r wsname wsflags wsld wslib <<< "$wsmode"
    echo "Testing test_weak_sac_xemu.c ($wsname)..."
    wsbase="build/test/test_weak_sac_${wsname}"
    if $CC $wsflags -c src/test-resources/test_weak_sac_xemu.c -o ${wsbase}_a.o45 2>/dev/null &&
       $CC $wsflags -c src/test-resources/test_weak_sac_b.c -o ${wsbase}_b.o45 2>/dev/null &&
       $LD $wsld -basic -o ${wsbase}_xemu.prg $CRT0 ${wsbase}_a.o45 ${wsbase}_b.o45 $wslib >/dev/null 2>&1; then
        run_xemu_test "test_weak_sac_$wsname" "${wsbase}_xemu.prg" "C0C0" 3 "01 01 AA"
    else
        echo "FAIL: test_weak_sac_xemu.c $wsname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done

# Test: pointer variables in functions' base-page blocks ($80-$E9, placed by
# ln45), two modules
for zlmode in "zp||lib/build/c45.lib" "stack|-fno-zpcall|lib/build/c45_stack.lib" "off|-fno-zp-locals|lib/build/c45.lib"; do
    IFS='|' read -r zlname zlflags zllib <<< "$zlmode"
    echo "Testing test_zp_locals_xemu.c ($zlname)..."
    zlbase="build/test/test_zp_locals_${zlname}"
    if $CC $zlflags -c src/test-resources/test_zp_locals_xemu.c -o ${zlbase}_a.o45 2>/dev/null &&
       $CC $zlflags -c src/test-resources/test_zp_locals_b.c -o ${zlbase}_b.o45 2>/dev/null &&
       $LD -basic -o ${zlbase}_xemu.prg $CRT0 ${zlbase}_a.o45 ${zlbase}_b.o45 $zllib >/dev/null 2>&1; then
        run_xemu_test "test_zp_locals_$zlname" "${zlbase}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_zp_locals_xemu.c $zlname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
# A call chain whose blocks do not fit in $80-$E9: ln45 warns, and the
# functions sharing bytes with others active together are called through
# save/restore wrappers (f7, wrapped, also through a function pointer)
zlsrc=build/test/zp_locals_overflow.c
{
    echo "#define N __attribute__((noinline))"
    echo "#define OUT ((volatile unsigned char *)0xC0C0)"
    echo "unsigned char g[24] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};"
    echo "N static unsigned char f10(unsigned char *a) { return a[0]; }"
    for lv in 9 8 7 6 5 4 3 2 1; do
        echo "N static unsigned char f$lv(unsigned char *a);"
    done
    echo "static unsigned char (*fp7)(unsigned char *) = f7;"
    for lv in 9 8 7 6 5 4 3 2 1; do
        call="f$((lv + 1))"; [ $lv = 6 ] && call="fp7"
        echo "N static unsigned char f$lv(unsigned char *a) {"
        echo "    unsigned char *p0 = a, *p1 = a + 1, *p2 = a + 2, *p3 = a + 3, *p4 = a + 4, *p5 = a + 5, *p6 = a + 6, *p7 = a + 7;"
        echo "    unsigned char r = $call(a + 1);"
        echo "    return r + *p0 + *p1 + *p2 + *p3 + *p4 + *p5 + *p6 + *p7;"
        echo "}"
    done
    echo "int main(void) { OUT[0] = f1(g); OUT[1] = 0xAA; return 0; }"
} > $zlsrc
# f(lv, a) = f(lv+1, a+1) + g[a..a+7], f(10, a) = g[a]: f(1, 0) = $6E (mod 256)
for zomode in "zp||lib/build/c45.lib" "stack|-fno-zpcall|lib/build/c45_stack.lib"; do
    IFS='|' read -r zoname zoflags zolib <<< "$zomode"
    zobase=build/test/zp_locals_overflow_$zoname
    # (16-byte blocks: the default 8 fits the chain)
    if $CC $zoflags -fzp-locals-max=16 -c $zlsrc -o $zobase.o45 2>/dev/null &&
       $LD -basic -o ${zobase}_xemu.prg $CRT0 $zobase.o45 $zolib 2>$zobase.err >/dev/null; then
        if grep -q "base-page storage: .* save and restore" $zobase.err; then
            run_xemu_test "zp_locals_overflow_$zoname" "${zobase}_xemu.prg" "C0C0" 2 "6E AA"
        else
            echo "FAIL: zp_locals_overflow_$zoname (no base-page storage warning)"
            failed=$((failed + 1))
        fi
    else
        echo "FAIL: zp_locals_overflow_$zoname (compilation/linking failed: $(head -1 $zobase.err))"
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
        run_xemu_test "test_bitfield_store_$bsname" "build/test/test_bitfield_store_${bsname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
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

# Test: string/character literal encoding prefixes p"..." s"..." a"..." @"..."
for spmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r spname spflags <<< "$spmode"
    echo "Testing test_string_prefix_xemu.c ($spname)..."
    compile_link_test "src/test-resources/test_string_prefix_xemu.c" "build/test/test_string_prefix_${spname}_xemu.prg" "$spflags"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_string_prefix_$spname" "build/test/test_string_prefix_${spname}_xemu.prg" "C0C0" 10 "01 01 01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_string_prefix_xemu.c $spname (compilation/linking failed)"
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
        run_xemu_test "test_printf_float_$pfname" "build/test/test_printf_float_${pfname}_xemu.prg" "C0C0" 9 "01 01 01 01 01 01 01 01 AA"
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
# Test: ca45 forgets register/base-page knowledge after Q, indexed/indirect and RMW instructions
echo "Testing test_ca45_tracking_xemu.s45 (ca45 machine-state tracking)..."
if ./bin/ca45 src/test-resources/test_ca45_tracking_xemu.s45 -o build/test/test_ca45_tracking_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_ca45_tracking" "build/test/test_ca45_tracking_xemu.prg" "C0C0" 6 "33 44 11 03 07 AA"
else
    echo "FAIL: test_ca45_tracking_xemu.s45 (assembly failed)"
    failed=$((failed + 1))
fi

echo "Testing test_ca45_simops_xemu.s45 (ca45 simulated instructions)..."
if ./bin/ca45 src/test-resources/test_ca45_simops_xemu.s45 -o build/test/test_ca45_simops_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_ca45_simops" "build/test/test_ca45_simops_xemu.prg" "C0C0" 39 \
        "04 00 FE FF 2C 01 FB FF 05 00 00 12 FB FF 06 FF 12 34 12 42 00 99 35 12 32 01 34 56 CD AB 5A 5A 77 66 05 5C 03 01 AA"
else
    echo "FAIL: test_ca45_simops_xemu.s45 (assembly failed)"
    failed=$((failed + 1))
fi

echo "Testing test_ca45_redundant_load_xemu.s45 (a reload kept for its flags, Issue #282)..."
if ./bin/ca45 src/test-resources/test_ca45_redundant_load_xemu.s45 -o build/test/test_ca45_redundant_load_xemu.prg >/dev/null 2>&1; then
    run_xemu_test "test_ca45_redundant_load" "build/test/test_ca45_redundant_load_xemu.prg" "C0C0" 5 "01 01 01 01 AA"
else
    echo "FAIL: test_ca45_redundant_load_xemu.s45 (assembly failed)"
    failed=$((failed + 1))
fi

echo "Testing test_ca45_seqextract_xemu.s45 (ca45 SeqExtract, .var by position, *+N targets)..."
T=src/test-resources/test_ca45_seqextract_xemu.s45
if ./bin/ca45 -PNoSeqExtract $T -o build/test/test_ca45_seqextract0_xemu.prg >/dev/null 2>&1 &&
   ./bin/ca45 $T -o build/test/test_ca45_seqextract_xemu.prg >/dev/null 2>&1; then
    if [ "$(stat -c %s build/test/test_ca45_seqextract_xemu.prg)" -lt "$(stat -c %s build/test/test_ca45_seqextract0_xemu.prg)" ]; then
        run_xemu_test "test_ca45_seqextract0" "build/test/test_ca45_seqextract0_xemu.prg" "C0C0" 8 "11 03 06 22 AA 02 01 01"
        run_xemu_test "test_ca45_seqextract" "build/test/test_ca45_seqextract_xemu.prg" "C0C0" 8 "11 03 06 22 AA 02 01 01"
    else
        echo "FAIL: test_ca45_seqextract_xemu.s45 (SeqExtract extracted nothing)"
        failed=$((failed + 1))
    fi
else
    echo "FAIL: test_ca45_seqextract_xemu.s45 (assembly failed)"
    failed=$((failed + 1))
fi

# Test: calls to the library's ZP assembly routines without a declaration
# (__builtin_alloca/setjmp/longjmp/abs/labs, implicit alloca/strlen) use the
# ZP convention in a stack-convention unit too: the stack build links with
# ln45 --no-thunks (each stack-mode GTE alloca/setjmp test needed a thunk)
for lcmode in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r lcname lcflags <<< "$lcmode"
    echo "Testing test_libcall_conv_xemu.c ($lcname)..."
    compile_link_test "src/test-resources/test_libcall_conv_xemu.c" "build/test/test_libcall_conv_${lcname}_xemu.prg" \
        "$lcflags -fimplicit-function-declaration"
    if [ $? -eq 0 ]; then
        run_xemu_test "test_libcall_conv_$lcname" "build/test/test_libcall_conv_${lcname}_xemu.prg" "C0C0" 8 "01 01 01 01 01 01 01 AA"
    else
        echo "FAIL: test_libcall_conv_xemu.c $lcname (compilation/linking failed)"
        failed=$((failed + 1))
    fi
done
if $LD --no-thunks -basic -o build/test/test_libcall_conv_nothunks.prg $CRT0 build/test/test_libcall_conv_stack_xemu.o45 $STDLIB_STACK \
       > build/test/test_libcall_conv_nothunks.log 2>&1; then
    echo "SUCCESS: test_libcall_conv_nothunks"
    passed=$((passed + 1))
else
    echo "FAIL: test_libcall_conv_nothunks (ln45 --no-thunks):"
    grep -i "mismatch\|error" build/test/test_libcall_conv_nothunks.log | head -3
    failed=$((failed + 1))
fi

# Test: c45_stack.lib needs no calling-convention thunks: stack-convention
# programs using stdio, file I/O, strings, struct copies and wide integers
# link with ln45 --no-thunks (the library's asm functions are __fastcall__)
for nt in test_printf_float_xemu test_fileio_xemu test_long_strings_xemu test_int64_xemu test_builtins_xemu; do
    [ -f "src/test-resources/$nt.c" ] || continue
    echo "Testing ln45 --no-thunks with $nt.c (stack)..."
    if $CC -c -fno-zpcall "src/test-resources/$nt.c" -o "build/test/${nt}_nothunks.o45" >/dev/null 2>&1 &&
       $LD --no-thunks -basic -o "build/test/${nt}_nothunks.prg" $CRT0 "build/test/${nt}_nothunks.o45" $STDLIB_STACK \
           > "build/test/${nt}_nothunks.log" 2>&1; then
        echo "SUCCESS: ${nt}_nothunks"
        passed=$((passed + 1))
    else
        echo "FAIL: ${nt}_nothunks (ln45 --no-thunks):"
        grep -i "mismatch\|error" "build/test/${nt}_nothunks.log" | head -3
        failed=$((failed + 1))
    fi
done

echo ""
echo "======================================================"
echo "Xemu Test Results: $passed passed, $failed failed"
echo "======================================================"

if [ $failed -gt 0 ]; then
    exit 1
else
    exit 0
fi
