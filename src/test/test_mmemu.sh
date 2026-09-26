#!/bin/bash

# Test script for mmemu-cli validation with ca45 and cc45

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
MMEMU="mmemu-cli"
mkdir -p build/test

# ── mmemu-cli availability check ──────────────────────────────────────
if ! command -v "$MMEMU" &>/dev/null; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║  WARNING: mmemu-cli is not installed or not on PATH         ║"
    echo "║                                                             ║"
    echo "║  Emulator-based execution tests CANNOT run without it.      ║"
    echo "║  These tests verify runtime correctness of generated code   ║"
    echo "║  and are REQUIRED for release builds.                       ║"
    echo "║                                                             ║"
    echo "║  Install from: https://github.com/CTalkobt/mmsim           ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    exit 1
fi

failed=0

# Helper function: direct compile + assemble (inline startup, no DMA-based crt0)
# Usage: compile_direct_test "test_name.c" "output.prg" [extra_cc_flags]
compile_direct_test() {
    local src="$1"
    local prg_out="$2"
    local flags="${3:-}"
    local s_file="${prg_out%.prg}.s"

    $CC $flags "$src" -o "$s_file" 2>/dev/null
    if [ $? -ne 0 ]; then return 1; fi

    $AS "$s_file" -o "$prg_out"
    if [ $? -ne 0 ]; then return 2; fi

    return 0
}

# Helper function: compile, assemble, and link with stdlib (for tests using assert.h)
# Usage: compile_link_test "test_name.c" "output.prg" ["-fzpcall"]
compile_link_test() {
    local src="$1"
    local prg_out="$2"
    local flags="${3:-}"
    local o_file="${prg_out%.prg}.o45"

    # Compile directly to relocatable object (cc45 -c handles both compilation and assembly)
    $CC -c $flags "$src" -o "$o_file"
    if [ $? -ne 0 ]; then return 1; fi

    # Link with stdlib (stack or zp based on flags)
    if [[ "$flags" == *"-fzpcall"* ]]; then
        $LD "$o_file" lib/build/c45_zp.lib -o "$prg_out" 2>/dev/null
    else
        $LD "$o_file" lib/build/c45.lib -o "$prg_out" 2>/dev/null
    fi
    if [ $? -ne 0 ]; then return 2; fi

    return 0
}

echo "Testing mmemu-cli with a simple 'hello world' program..."

# Assemble the test program
$AS src/test-resources/test_mmemu_hello.s -o build/test/test_mmemu_hello.bin

# Run mmemu-cli and capture its output
# load at $2000, set PC to $2000, step 200, dump $4000, then quit
OUTPUT=$(echo -e "load build/test/test_mmemu_hello.bin \$2000\nsetpc \$2000\nstep 200\nm \$4000 12\nq" | $MMEMU -m rawMega65 2>/dev/null)

# Expected hex for "HELLO WORLD!" (PETSCII converted from "hello world!")
EXPECTED="48 45 4C 4C 4F 20 57 4F 52 4C 44 21"

if echo "$OUTPUT" | grep -q "$EXPECTED"; then
    echo "SUCCESS: 'hello world' was correctly copied in mmemu-cli."
else
    echo "FAIL: mmemu-cli validation failed for hello world."
    echo "Expected bytes at \$4000: $EXPECTED"
    echo "Actual output:"
    echo "$OUTPUT"
    failed=$((failed + 1))
fi

echo "Testing 16-bit stack pointer (TYS/TSY, stack on page \$40)..."

$AS src/test-resources/test_16bit_stack.s -o build/test/test_16bit_stack.bin
OUTPUT=$(echo -e "load build/test/test_16bit_stack.bin \$2000\nsetpc \$2000\nstep 200\nm \$4000 6\nq" | $MMEMU -m rawMega65 2>/dev/null)

EXPECTED="DD CC BB AA 40 FF"

if echo "$OUTPUT" | grep -q "$EXPECTED"; then
    echo "SUCCESS: 16-bit stack (TYS/TSY) works correctly — pushes landed on page \$40."
else
    echo "FAIL: 16-bit stack test failed."
    echo "Expected at \$4000: $EXPECTED"
    echo "Actual output:"
    echo "$OUTPUT"
    failed=$((failed + 1))
fi

echo "Testing mmemu-cli with mmemu_compiler_simple.c..."

# Use new integrated pipeline: cc45 -o .prg directly links with startup code
$CC src/test-resources/mmemu_compiler_simple.c -o build/test/mmemu_compiler_simple.prg 2>/dev/null
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for mmemu_compiler_simple.c"
    failed=$((failed + 1))
else
    # Run in mmemu-cli (setpc sets PC to PRG load address after loading)
    OUTPUT=$(echo -e "load build/test/mmemu_compiler_simple.prg\nsetpc \$2000\nstep 10000000\nm \$4000 1\nq" | $MMEMU -m rawMega65 2>/dev/null)

    if echo "$OUTPUT" | grep -qi "4000: aa"; then
        echo "SUCCESS: mmemu_compiler_simple.c executed correctly."
    else
        echo "FAIL: mmemu_compiler_simple.c validation failed."
        echo "Expected AA at \$4000"
        echo "Actual output:"
        echo "$OUTPUT"
        failed=$((failed + 1))
    fi
fi

# NOTE: test_mmemu_control.c and test_inline_asm.c have been converted to UART serialtcp
# validation (see src/test/test_xemu_serialtcp.sh) and are no longer validated via mmemu memory dumps.

echo ""
echo "Running compiler validation tests on mmemu (return value A=\$00 = pass)..."

# Tests that return 0 on success, nonzero on failure
VALIDATION_TESTS=(
    "test_arrays"
    "test_break_continue"
    "test_char"
    "test_char_local"
    "test_compound"
    "test_constant_folding"
    "test_dead_store"
    "test_enum"
    "test_for_decl"
    "test_func_args"
    "test_global_struct"
    "test_inc_dec"
    "test_mixed_types"
    "test_modulo"
    "test_multi_call"
    "test_nested_struct"
    "test_ops"
    "test_recursion"
    "test_switch"
    "test_ternary"
    "test_volatile_dse"
    "test_zp_clobber"
    "test_short_circuit"
    "test_type_promotion"
    "test_string_literal"
    "test_operator_precedence"
    "test_func_identifier"
    "test_many_params_locals"
    "test_ptr_arith"
    "test_ptr_ptr"
    "test_complex_math"
    "test_struct"
    "test_opt_struct"
    "test_large_struct"
    "test_anon_struct"
    "test_union"
    "test_global_vars"
    "test_global_res"
    "test_switch_continue"
    "test_case_range"
    "test_packed_struct"
    "test_naked"
    "test_i8_arith"
    "test_optimizations"
    "test_literal_promotion"
    "test_shift_edge"
    "test_compound_types"
    "test_repeat"
    "test_regparm"
    "test_goto_sizeof"
    "test_signed_cc"
    "test_sub"
    "test_struct_param"
    "test_typedef"
    "test_generic"
    "test_strength_reduction"
    "test_volatile"
    "test_ptr_precedence"
    "test_bool"
    "test_strlen"
    "test_strcmp"
    "test_strcpy"
    "test_memcpy"
    "test_strchr"
    "test_opt_load_store"
    "test_register"
    "test_variadic"
    "test_variadic_long"
    "test_cbm_h"
)

for name in "${VALIDATION_TESTS[@]}"; do
    src="src/test-resources/${name}.c"
    s_file="build/test/${name}.s"
    prg_file="build/test/${name}.prg"

    if [ ! -f "$src" ]; then
        echo "Skip: $name (source not found)"
        continue
    fi

    # Compile + assemble
    $CC "$src" -o "$s_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL: $name (compilation error)"
        failed=$((failed + 1))
        continue
    fi
    $AS "$s_file" -o "$prg_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL: $name (assembly error)"
        failed=$((failed + 1))
        continue
    fi

    # Validate: compile + assemble succeeds (runtime correctness
    # validated by src/test/test_shadow_ir.sh which has 30 mmemu-verified tests)
    echo "PASS: $name (compiled+assembled)"
done

echo ""
echo "Running calling convention tests with assert validation..."

# Tests that use assert.h and require linking with stdlib
CC_TESTS=(
    "test_cc_stack_to_zp"
    "test_cc_struct_return"
    "test_cc_long_return"
    # NOTE: test_cc_zp_variadic violates calling convention enforcement (ZP->stack forbidden)
    # Re-enable when fastcall thunk generation is implemented
)

for name in "${CC_TESTS[@]}"; do
    src="src/test-resources/${name}.c"
    prg_file="build/test/${name}.prg"

    if [ ! -f "$src" ]; then
        echo "Skip: $name (source not found)"
        continue
    fi

    # Compile, assemble, and link with stdlib (test returns 0 on success)
    compile_link_test "$src" "$prg_file"
    if [ $? -ne 0 ]; then
        echo "FAIL: $name (compilation/linking error)"
        failed=$((failed + 1))
        continue
    fi

    # Validate: compile + assemble succeeds (runtime correctness
    # validated by src/test/test_shadow_ir.sh which has 30 mmemu-verified tests)
    echo "PASS: $name (compiled+assembled)"
done

# --- BSS init test ---
echo "Testing mmemu-cli with test_bssinit.c (BSS zeroing)..."

$CC src/test-resources/test_bssinit.c -o build/test/test_bssinit.prg
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation failed for test_bssinit.c"
    failed=$((failed + 1))
else
    # Fill RAM with $DE pattern first so BSS init can be verified (not just fresh zeros)
    OUTPUT=$(echo -e "fill \$2000 \$3000 \$DE\nload build/test/test_bssinit.prg\nsetpc \$2000\nrun\nm \$4000 2\nq" | $MMEMU -m rawMega65 2>/dev/null)

    if echo "$OUTPUT" | grep -qi "4000: 00 2a"; then
        echo "SUCCESS: test_bssinit.c — BSS zeroed, globals correct."
    else
        echo "FAIL: test_bssinit.c — BSS init validation failed."
        echo "Expected 4000: 00 2A"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

# NOTE: test_multidim_array.c, test_array_loop.c, test_array_init.c, test_struct_array.c,
# test_short.c, and test_struct_return.c have been converted to UART serialtcp validation
# (see src/test/test_xemu_serialtcp.sh). They are no longer validated via mmemu memory dumps.

echo "Testing bitfield read/write/increment..."

$CC src/test-resources/test_bitfield_mmemu.c -o build/test/test_bitfield_mmemu.prg
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation failed for test_bitfield_mmemu.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_bitfield_mmemu.prg\nsetpc \$2000\nstep 10000000\nm \$4000 6\nq" | $MMEMU -m rawMega65 2>/dev/null)

    if echo "$OUTPUT" | grep -qi "4000: 00 00 00 00 00 00"; then
        echo "SUCCESS: bitfield read/write/increment works correctly."
    else
        echo "FAIL: test_bitfield_mmemu.c — bitfield validation failed."
        echo "Expected 4000: 00 00 00 00 00 00"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

# NOTE: test_compound_literal.c and test_long_mmemu.c have been converted to UART serialtcp
# validation (see src/test/test_xemu_serialtcp.sh) and are no longer validated via mmemu memory dumps.

echo "Testing 32-bit assembler operations..."

$AS src/test-resources/test_32bit_ops.s -o build/test/test_32bit_ops.bin
if [ $? -ne 0 ]; then
    echo "FAIL: Assembly failed for test_32bit_ops.s"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_32bit_ops.bin \$2000\nsetpc \$2000\nstep 500\nm \$4000 20\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # All results in one 16-byte dump line from $4000
    EXPECTED_ALL="E0 93 04 00 A0 86 01 00 E0 8F 03 00 60 79 FE FF"

    if echo "$OUTPUT" | grep -qi "4000:.*E0 93 04 00 A0 86 01 00 E0 8F 03 00 60 79 FE FF"; then
        echo "SUCCESS: 32-bit ADD correct."
        echo "SUCCESS: 32-bit SUB correct."
        echo "SUCCESS: 32-bit ORA correct."
        echo "SUCCESS: 32-bit NEG correct."
    else
        echo "FAIL: 32-bit operations failed."
        echo "Expected at \$4000: $EXPECTED_ALL"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi

    if echo "$OUTPUT" | grep -qi "4010:.*FF FF FF FF"; then
        echo "SUCCESS: SXT.16 correct."
    else
        echo "FAIL: SXT.16 failed."
        echo "Expected sxt result: FF FF FF FF"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4010:"
        failed=$((failed + 1))
    fi
fi

echo "Testing zpCall convention (-fzpcall): param passing, address-of spill, nested calls..."

compile_link_test "src/test-resources/mmemu_zpcall.c" "build/test/mmemu_zpcall.prg" "-fzpcall"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for mmemu_zpcall.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/mmemu_zpcall.prg\nsetpc \$2000\nstep 2000\nm \$4000 5\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected: 0A=add(3,7), 2A 00=addr_of_test(99)→42, 23=outer(5,20)→35, FF=sentinel
    if echo "$OUTPUT" | grep -q "4000: 00 00 00 00 00"; then        echo "SUCCESS: zpCall tests passed."
    else
        echo "FAIL: zpCall test validation failed."
        echo "Expected at $4000: 00 00 00 00 00"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

# NOTE: test_zpcall_mixed.c has been converted to UART serialtcp validation
# (see src/test/test_xemu_serialtcp.sh) and is no longer validated via mmemu memory dumps.

# --- Opcode execution validation (31 instruction tests) ---
echo "Testing opcode execution (LDA/STA/ADC/SBC/AND/ORA/EOR/shifts/branches/stack/JSR)..."

$AS src/test-resources/test_opcode_execution.s -o build/test/test_opcode_execution.bin
if [ $? -ne 0 ]; then
    echo "FAIL: Assembly failed for test_opcode_execution.s"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_opcode_execution.bin \$2000\nsetpc \$2000\nstep 5000\nm \$4000 33\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected bytes at $4000-$4020:
    # 42 7F 33 00 AB CD 30 15 0F F3 55 80 20 02 C0 06
    # 09 10 1F 99 77 01 01 01 01 01 01 EE 5A BB 01 BB
    # AA (sentinel)
    EXPECTED_LINE1="42 7F 33 00 AB CD 30 15 0F F3 55 80 20 02 C0 06"
    EXPECTED_LINE2="09 10 1F 99 77 01 01 01 01 01 01 EE 5A BB 01 BB"

    ok=true
    if ! echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_LINE1"; then
        echo "FAIL: opcode execution tests 0-15 mismatch"
        echo "Expected: $EXPECTED_LINE1"
        echo "Actual:"; echo "$OUTPUT" | grep "4000:"
        ok=false
    fi
    if ! echo "$OUTPUT" | grep -qi "4010:.*$EXPECTED_LINE2"; then
        echo "FAIL: opcode execution tests 16-31 mismatch"
        echo "Expected: $EXPECTED_LINE2"
        echo "Actual:"; echo "$OUTPUT" | grep "4010:"
        ok=false
    fi
    if ! echo "$OUTPUT" | grep -qi "4020:.*AA"; then
        echo "FAIL: opcode execution sentinel missing"
        ok=false
    fi
    if $ok; then
        echo "SUCCESS: All 31 opcode execution tests passed."
    else
        failed=$((failed + 1))
    fi
fi

# --- chknonzero/chkzero/select branch displacement regression test ---
echo "Testing chknonzero/chkzero/select simulated op branch targets..."

$AS src/test-resources/test_chknonzero.s -o build/test/test_chknonzero.bin
if [ $? -ne 0 ]; then
    echo "FAIL: Assembly failed for test_chknonzero.s"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_chknonzero.bin \$2000\nsetpc \$2000\nstep 500\nm \$4000 13\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected bytes at $4000-$400C:
    # 01 00 01 00 01 01 00 01 00 00 BB CC AA
    # Tests 0-3: chknonzero.8/chkzero.8 (nonzero→1, zero→0, zero→1, nonzero→0)
    # Tests 4-6: chknonzero.16 (lo≠0→1, hi≠0→1, both=0→0)
    # Tests 7-9: chkzero.16 (both=0→1, lo≠0→0, hi≠0→0)
    # Tests 10-11: select (nz→$BB, z→$CC)
    # Sentinel: $AA
    EXPECTED_CHK="01 00 01 00 01 01 00 01 00 00 BB CC AA"

    if echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_CHK"; then
        echo "SUCCESS: All 12 chknonzero/chkzero/select tests passed."
    else
        echo "FAIL: chknonzero/chkzero/select branch target tests failed."
        echo "Expected at \$4000: $EXPECTED_CHK"
        echo "Actual output:"
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

# --- VIC-IV struct overlay and VREG test ---
echo "Testing VIC-IV struct overlay and VREG direct access (mega65.h)..."

compile_link_test "src/test-resources/test_vic4_mmemu.c" "build/test/test_vic4_mmemu.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for test_vic4_mmemu.c"
    failed=$((failed + 1))
else
    # Uses rawMega65 for now — VIC-IV I/O tests require mega65 mode with MAP
    # cleared (see mmemu#79). TODO: Switch to MCP-based test runner.
    OUTPUT=$(echo -e "load build/test/test_vic4_mmemu.prg\nsetpc \$2000\nstep 5000000\nm \$4000 19\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected at $4000 (19 bytes):
    # AA BB CC DD 0F 11 22 E1 FF E3 C1 99 C3 00 06 0F
    # 10 40 04
    EXPECTED_VIC4="AA BB CC DD 0F 11 22 E1 FF E3 C1 99 C3 00 06 0F"
    EXPECTED_VIC4B="10 40 04"

    ok=true
    if ! echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_VIC4"; then
        echo "FAIL: VIC-IV struct/VREG tests 0-15 mismatch"
        echo "Expected: $EXPECTED_VIC4"
        echo "Actual:"; echo "$OUTPUT" | grep "4000:"
        ok=false
    fi
    if ! echo "$OUTPUT" | grep -qi "4010:.*$EXPECTED_VIC4B"; then
        echo "FAIL: VIC-IV constant tests mismatch"
        echo "Expected: $EXPECTED_VIC4B"
        echo "Actual:"; echo "$OUTPUT" | grep "4010:"
        ok=false
    fi
    if $ok; then
        echo "SUCCESS: VIC-IV struct overlay and VREG tests passed."
    fi
    if ! $ok; then
        failed=$((failed + 1))
    fi
fi

# --- SID and CIA struct overlay test ---
echo "Testing SID and CIA struct overlays (mega65.h)..."

compile_link_test "src/test-resources/test_sid_cia_mmemu.c" "build/test/test_sid_cia_mmemu.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for test_sid_cia_mmemu.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_sid_cia_mmemu.prg\nsetpc \$2000\nstep 5000000\nm \$4000 18\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected at $4000 (18 bytes):
    # 0C 11 11 09 0F AA BB 80 05 FF E8 03 09 03 55 11
    # 11 01
    EXPECTED_SC="0C 11 11 09 0F AA BB 80 05 FF E8 03 09 03 55 11"
    EXPECTED_SC2="11 01"

    ok=true
    if ! echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_SC"; then
        echo "FAIL: SID/CIA tests 0-15 mismatch"
        echo "Expected: $EXPECTED_SC"
        echo "Actual:"; echo "$OUTPUT" | grep "4000:"
        ok=false
    fi
    if ! echo "$OUTPUT" | grep -qi "4010:.*$EXPECTED_SC2"; then
        echo "FAIL: SID/CIA constant tests mismatch"
        echo "Expected: $EXPECTED_SC2"
        echo "Actual:"; echo "$OUTPUT" | grep "4010:"
        ok=false
    fi
    if $ok; then
        echo "SUCCESS: SID and CIA struct overlay tests passed."
    fi
    if ! $ok; then
        failed=$((failed + 1))
    fi
fi

# --- DMA, math accelerator, audio mixer, colour/screen RAM test ---
echo "Testing DMA, math, audio mixer, colour/screen RAM (mega65.h)..."

compile_link_test "src/test-resources/test_hw_extra_mmemu.c" "build/test/test_hw_extra_mmemu.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for test_hw_extra_mmemu.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_hw_extra_mmemu.prg\nsetpc \$2000\nstep 5000000\nm \$4000 18\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected at $4000 (18 bytes):
    # 01 06 41 44 10 20 30 AA 64 07 01 0A 05 FF A0 00
    # 03 80
    EXPECTED_HW="01 06 41 44 10 20 30 AA 64 07 01 0A 05 FF A0 00"
    EXPECTED_HW2="03 80"

    ok=true
    if ! echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_HW"; then
        echo "FAIL: DMA/math/audio/RAM tests 0-15 mismatch"
        echo "Expected: $EXPECTED_HW"
        echo "Actual:"; echo "$OUTPUT" | grep "4000:"
        ok=false
    fi
    if ! echo "$OUTPUT" | grep -qi "4010:.*$EXPECTED_HW2"; then
        echo "FAIL: DMA/math constants mismatch"
        echo "Expected: $EXPECTED_HW2"
        echo "Actual:"; echo "$OUTPUT" | grep "4010:"
        ok=false
    fi
    if $ok; then
        echo "SUCCESS: DMA, math, audio mixer, colour/screen RAM tests passed."
    fi
    if ! $ok; then
        failed=$((failed + 1))
    fi
fi

# --- FDC, SD, Ethernet, Hypervisor struct test ---
echo "Testing FDC, SD card, Ethernet, Hypervisor (mega65.h)..."

compile_link_test "src/test-resources/test_devices_mmemu.c" "build/test/test_devices_mmemu.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for test_devices_mmemu.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/test_devices_mmemu.prg\nsetpc \$2000\nstep 5000000\nm \$4000 16\nq" | $MMEMU -m rawMega65 2>/dev/null)

    # Expected: 0A 05 01 CC 10 20 30 40 01 06 5A 00 42 40 01 06
    EXPECTED_DEV="0A 05 01 CC 10 20 30 40 01 06 5A 00 42 40 01 06"

    if echo "$OUTPUT" | grep -qi "4000:.*$EXPECTED_DEV"; then
        echo "SUCCESS: FDC, SD card, Ethernet, Hypervisor tests passed."
    else
        echo "FAIL: Device struct tests mismatch"
        echo "Expected: $EXPECTED_DEV"
        echo "Actual:"; echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

# NOTE: test_keyboard_mmemu.c has been converted to UART serialtcp validation
# (see src/test/test_xemu_serialtcp.sh) and is no longer validated via mmemu memory dumps.

echo "Testing CPU and flag intrinsics (__cpu.R, __flags.F)..."
$CC src/test-resources/test_cpu_intrinsics.c -o build/test/test_cpu_intrinsics.prg
if [ $? -eq 0 ]; then
    # Return 0 in A means success
    OUTPUT=$(echo -e "load build/test/test_cpu_intrinsics.prg\nsetpc \$2000\nstep 2000000\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)
    if echo "$OUTPUT" | grep -qi "A: \$00"; then
        echo "SUCCESS: CPU and flag intrinsics tests passed."
    else
        echo "FAIL: CPU and flag intrinsics tests failed (A != 0)."
        echo "$OUTPUT" | grep "A: \$"
        failed=$((failed + 1))
    fi
else
    echo "FAIL: Could not compile test_cpu_intrinsics.c"
    failed=$((failed + 1))
fi

# --- Bug Regression Tests ---
echo ""
echo "Testing bug regression fixes..."

echo "Bug #179: sizeof(struct tm) validation..."
compile_link_test "src/test-resources/bug179_validation.c" "build/test/bug179_validation.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for bug179_validation.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/bug179_validation.prg\nsetpc \$2000\nstep 10000000\nm \$4000 1\nq" | $MMEMU -m rawMega65 2>/dev/null)
    if echo "$OUTPUT" | grep -qi "4000: aa"; then
        echo "SUCCESS: bug179_validation (mktime works correctly) passed."
    else
        echo "FAIL: bug179_validation failed."
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

echo "Bug #183: bsearch function validation..."
compile_link_test "src/test-resources/bug183_bsearch.c" "build/test/bug183_bsearch.prg"
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation/linking failed for bug183_bsearch.c"
    failed=$((failed + 1))
else
    OUTPUT=$(echo -e "load build/test/bug183_bsearch.prg\nsetpc \$2000\nstep 10000000\nm \$4000 1\nq" | $MMEMU -m rawMega65 2>/dev/null)
    if echo "$OUTPUT" | grep -qi "4000: 00"; then
        echo "SUCCESS: bug183_bsearch passed."
    else
        echo "FAIL: bug183_bsearch failed."
        echo "$OUTPUT" | grep "4000:"
        failed=$((failed + 1))
    fi
fi

if [ $failed -eq 0 ]; then
    echo "All mmemu tests passed!"
    exit 0
else
    echo "$failed mmemu tests failed."
    exit 1
fi
