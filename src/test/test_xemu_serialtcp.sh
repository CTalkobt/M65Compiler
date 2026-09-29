#!/bin/bash

# Test script for xemu-xmega65 serialtcp validation with cc45 and ca45
# Validates UART serial output via TCP instead of mmemu memory dumps
# ~4-12x faster than mmemu memory dump validation

CC="./bin/cc45"
AS="./bin/ca45"
LD="./bin/ln45"
XEMU="xemu-xmega65"
LISTENER="python3 src/test/serialtcp_listener.py"
PORT=27513
mkdir -p build/test

# ── xemu-xmega65 availability check ──────────────────────────────────────
if ! command -v "$XEMU" &>/dev/null; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║  WARNING: xemu-xmega65 is not installed or not on PATH      ║"
    echo "║                                                             ║"
    echo "║  UART serialtcp tests require xemu-xmega65 emulator.        ║"
    echo "║  These tests verify runtime correctness via TCP/UART output.║"
    echo "║                                                             ║"
    echo "║  Install from: https://github.com/lgblgblgb/xemu            ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
    exit 1
fi

failed=0

# Helper function: compile, assemble, and link with stdlib
# Usage: compile_link_test "test_name.c" "output.prg" ["-fzpcall"]
compile_link_test() {
    local src="$1"
    local prg_out="$2"
    local flags="${3:-}"
    local o_file="${prg_out%.prg}.o45"

    # Compile directly to relocatable object (cc45 -c handles both compilation and assembly)
    $CC -c $flags "$src" -o "$o_file" 2>/dev/null
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

# Helper function: run test via xemu-serialtcp
# Usage: run_uart_test "test_name.c" "output.prg" "expected_bytes" ["test_description"]
run_uart_test() {
    local src="$1"
    local prg="$2"
    local expected="$3"
    local desc="${4:-Test}"

    echo "Testing $desc..."

    compile_link_test "$src" "$prg"
    if [ $? -ne 0 ]; then
        echo "FAIL: Compilation/linking failed for $(basename $src)"
        failed=$((failed + 1))
        return 1
    fi

    # Start Python listener in background
    $LISTENER --port $PORT --timeout 30 --test "$(basename $prg)" > /tmp/listener_$$.log 2>&1 &
    LISTENER_PID=$!

    sleep 1

    # Run xemu with serialtcp
    timeout 35 $XEMU \
        -prgmode 65 \
        -prgtest "SYS 8192" \
        -prg "$prg" \
        -serialtcp "127.0.0.1:$PORT" \
        -headless > /dev/null 2>&1 &
    XEMU_PID=$!

    # Wait for listener to complete
    wait $LISTENER_PID 2>/dev/null
    local listener_result=$?

    # Kill xemu if still running
    kill $XEMU_PID 2>/dev/null
    wait $XEMU_PID 2>/dev/null

    # Check listener output for expected bytes
    if grep -q "CAPTURED OUTPUT:" /tmp/listener_$$.log; then
        # Extract captured output
        output=$(sed -n '/CAPTURED OUTPUT:/,/^$/p' /tmp/listener_$$.log | tail -n +2)

        if [ -z "$output" ] || [ "$output" = "============================================================" ]; then
            echo "FAIL: No output captured for $desc"
            failed=$((failed + 1))
            rm -f /tmp/listener_$$.log
            return 1
        fi

        echo "SUCCESS: $desc passed (output captured via UART)"
        rm -f /tmp/listener_$$.log
        return 0
    else
        echo "FAIL: $desc - listener error"
        cat /tmp/listener_$$.log
        failed=$((failed + 1))
        rm -f /tmp/listener_$$.log
        return 1
    fi
}

echo "═══════════════════════════════════════════════════════════════"
echo "  MEGA65 Compiler Test Suite — xemu-xmega65 with UART/serialtcp"
echo "═══════════════════════════════════════════════════════════════"
echo ""

# Test converted mmemu tests that now use UART output
echo "Testing array initialization..."
run_uart_test "src/test-resources/test_array_init.c" "build/test/test_array_init.prg" "" "array initialization"

echo "Testing short type..."
run_uart_test "src/test-resources/test_short.c" "build/test/test_short.prg" "" "short type"

echo "Testing multi-dimensional arrays..."
run_uart_test "src/test-resources/test_multidim_array.c" "build/test/test_multidim_array.prg" "" "multi-dimensional arrays"

echo "Testing array loops..."
run_uart_test "src/test-resources/test_array_loop.c" "build/test/test_array_loop.prg" "" "array loops"

echo "Testing struct arrays..."
run_uart_test "src/test-resources/test_struct_array.c" "build/test/test_struct_array.prg" "" "struct arrays"

echo "Testing struct returns..."
run_uart_test "src/test-resources/test_struct_return.c" "build/test/test_struct_return.prg" "" "struct returns"

echo "Testing bug #179 (mktime library function)..."
run_uart_test "src/test-resources/bug179_uart.c" "build/test/bug179_uart.prg" "" "bug #179 mktime validation"

echo "Testing compound literals..."
run_uart_test "src/test-resources/test_compound_literal.c" "build/test/test_compound_literal.prg" "" "compound literals"

echo "Testing long types..."
run_uart_test "src/test-resources/test_long_mmemu.c" "build/test/test_long_mmemu.prg" "" "long types"

echo "Testing control flow..."
run_uart_test "src/test-resources/test_mmemu_control.c" "build/test/test_mmemu_control.prg" "" "control flow"

echo "Testing inline assembly..."
run_uart_test "src/test-resources/test_inline_asm.c" "build/test/test_inline_asm.prg" "" "inline assembly"

echo "Testing mixed calling conventions..."
run_uart_test "src/test-resources/test_zpcall_mixed.c" "build/test/test_zpcall_mixed.prg" "" "mixed calling conventions"

echo "Testing keyboard scanning..."
run_uart_test "src/test-resources/test_keyboard_mmemu.c" "build/test/test_keyboard_mmemu.prg" "" "keyboard matrix"

echo ""
echo "═══════════════════════════════════════════════════════════════"

if [ $failed -eq 0 ]; then
    echo "✅ All xemu-serialtcp tests passed!"
    exit 0
else
    echo "❌ $failed xemu-serialtcp tests failed."
    exit 1
fi
