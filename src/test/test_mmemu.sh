#!/bin/bash

# Test script for mmemu-cli validation with ca45 and cc45

CC="./bin/cc45"
AS="./bin/ca45"
MMEMU="mmemu-cli"
mkdir -p build/test

failed=0

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

echo "Testing mmemu-cli with mmemu_compiler_simple.c..."

# 1. Compile
$CC src/test-resources/mmemu_compiler_simple.c -o build/test/mmemu_compiler_simple.s
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation failed for mmemu_compiler_simple.c"
    failed=$((failed + 1))
else
    # 2. Assemble to .prg
    $AS build/test/mmemu_compiler_simple.s -o build/test/mmemu_compiler_simple.prg
    if [ $? -ne 0 ]; then
        echo "FAIL: Assembly failed for mmemu_compiler_simple.s"
        failed=$((failed + 1))
    else
        # 3. Run in mmemu-cli
        # Load .prg (uses header), set PC to $2000 (now jmp main), step 200, dump $4000
        OUTPUT=$(echo -e "load build/test/mmemu_compiler_simple.prg\nsetpc \$2000\nstep 500\nm \$4000 1\nq" | $MMEMU -m rawMega65 2>/dev/null)
        
        if echo "$OUTPUT" | grep -q "4000: AA"; then
            echo "SUCCESS: mmemu_compiler_simple.c executed correctly."
        else
            echo "FAIL: mmemu_compiler_simple.c validation failed."
            echo "Expected AA at \$4000"
            echo "Actual output:"
            echo "$OUTPUT"
            failed=$((failed + 1))
        fi
    fi
fi

echo "Testing mmemu-cli with test_mmemu_control.c (comprehensive control flow)..."

# 1. Compile
$CC -O0 src/test-resources/test_mmemu_control.c -o build/test/test_mmemu_control.s
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation failed for test_mmemu_control.c"
    failed=$((failed + 1))
else
    # 2. Assemble to .prg
    $AS build/test/test_mmemu_control.s -o build/test/test_mmemu_control.prg
    if [ $? -ne 0 ]; then
        echo "FAIL: Assembly failed for test_mmemu_control.s"
        failed=$((failed + 1))
    else
        # 3. Run in mmemu-cli
        # Expected bytes at $4000:
        # 4000: AA (Success flag)
        # 4001: 01 (1 && 1)
        # 4002: 01 (0 || 1)
        # 4003: 0A (while sum 0-4 = 10)
        # 4004: 0A (do-while sum 0-4 = 10)
        # 4005: 12 (for break/continue 3+4+5+6 = 18 = $12)
        # 4006: 0A (switch 1 = 10 = $0A)
        # 4007: 19 (switch 2 = 25 = $19)
        # 4008: 64 (switch default = 100 = $64)
        # 4009: 11 (ternary true = $11)
        # 400A: 22 (ternary false = $22)
        
        EXPECTED_CONTROL="AA 01 01 0A 0A 12 0A 19 64 11 22"
        
        OUTPUT=$(echo -e "load build/test/test_mmemu_control.prg\nsetpc \$2000\nstep 500000\nm \$4000 11\nq" | $MMEMU -m rawMega65 2>/dev/null)
        
        if echo "$OUTPUT" | grep -q "$EXPECTED_CONTROL"; then
            echo "SUCCESS: test_mmemu_control.c executed correctly."
        else
            echo "FAIL: test_mmemu_control.c validation failed."
            echo "Expected at \$4000: $EXPECTED_CONTROL"
            echo "Actual output:"
            echo "$OUTPUT"
            failed=$((failed + 1))
        fi
    fi
fi

echo "Testing mmemu-cli with test_inline_asm.c (inline assembly variable access)..."

# 1. Compile
$CC src/test-resources/test_inline_asm.c -o build/test/test_inline_asm.s
if [ $? -ne 0 ]; then
    echo "FAIL: Compilation failed for test_inline_asm.c"
    failed=$((failed + 1))
else
    # 2. Assemble to .prg
    $AS build/test/test_inline_asm.s -o build/test/test_inline_asm.prg
    if [ $? -ne 0 ]; then
        echo "FAIL: Assembly failed for test_inline_asm.s"
        failed=$((failed + 1))
    else
        # 3. Run in mmemu-cli
        # Expected bytes at $4000:
        # 4000: 01 (global int param via ldax/stax inline asm)
        # 4001: 01 (global char param via lda.sp/sta inline asm)
        # 4002: 01 (global overwrite to zero)
        # 4003: 01 (local variable via ldax/stax inline asm)
        # 4004: AA (success marker)

        EXPECTED_INLINE="01 01 01 01 AA"

        OUTPUT=$(echo -e "load build/test/test_inline_asm.prg\nsetpc \$2000\nstep 500000\nm \$4000 5\nq" | $MMEMU -m rawMega65 2>/dev/null)

        if echo "$OUTPUT" | grep -q "$EXPECTED_INLINE"; then
            echo "SUCCESS: test_inline_asm.c executed correctly."
        else
            echo "FAIL: test_inline_asm.c validation failed."
            echo "Expected at \$4000: $EXPECTED_INLINE"
            echo "Actual output:"
            echo "$OUTPUT"
            failed=$((failed + 1))
        fi
    fi
fi

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

    # Run on mmemu and check A register
    OUTPUT=$(echo -e "load $prg_file\nsetpc \$2000\nrun 500000\nregs\nq" | $MMEMU -m rawMega65 2>/dev/null)
    A_VAL=$(echo "$OUTPUT" | grep -oP 'A:\s*\$\K[0-9A-Fa-f]+' | tail -1)

    if [ "$A_VAL" = "00" ]; then
        echo "PASS: $name (A=\$00)"
    else
        echo "FAIL: $name (A=\$$A_VAL)"
        failed=$((failed + 1))
    fi
done

if [ $failed -eq 0 ]; then
    echo "All mmemu tests passed!"
    exit 0
else
    echo "$failed mmemu tests failed."
    exit 1
fi
