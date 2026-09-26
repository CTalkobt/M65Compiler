#!/bin/bash
#
# MEGA65 Test Runner via serialtcp
#
# Runs xemu-xmega65 with serialtcp output and validates via Python listener.
# This replaces memory dump-based validation.
#
# Usage: ./run_test_serialtcp.sh <test.prg> [--test TEST_TYPE]
#

set -e

TEST_PRG="${1:-.}"
TEST_TYPE="${3:-array_init}"
PORT=27513
TIMEOUT=30
XEMU_BINARY="$(which xemu-xmega65 2>/dev/null || echo '/home/duck/src/xemu/build/bin/xmega65.native')"
LISTENER_SCRIPT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/serialtcp_listener.py"

# Validate inputs
if [ ! -f "$TEST_PRG" ]; then
    echo "❌ Error: Test file not found: $TEST_PRG"
    exit 1
fi

if [ ! -f "$LISTENER_SCRIPT" ]; then
    echo "❌ Error: Listener script not found: $LISTENER_SCRIPT"
    exit 1
fi

if [ ! -x "$XEMU_BINARY" ]; then
    echo "❌ Error: xemu-xmega65 not found: $XEMU_BINARY"
    exit 1
fi

echo "🚀 Running test via serialtcp: $(basename $TEST_PRG)"
echo "   Test type:  $TEST_TYPE"
echo "   Port:       $PORT"
echo "   Timeout:    ${TIMEOUT}s"
echo ""

# Start listener first (it will listen for xemu connection)
echo "Starting test listener on port $PORT..."
python3 "$LISTENER_SCRIPT" --port $PORT --timeout $TIMEOUT --test "$TEST_TYPE" &
LISTENER_PID=$!

# Give listener time to start
sleep 0.5

# Start xemu (it will connect to the listener)
echo "Starting emulation..."
timeout $((TIMEOUT + 10)) "$XEMU_BINARY" \
    -prgmode 65 \
    -prgtest "SYS 8192" \
    -prg "$TEST_PRG" \
    -serialtcp "127.0.0.1:$PORT" \
    -headless &
XEMU_PID=$!

# Wait for listener to complete (it will exit when test finishes)
wait $LISTENER_PID
LISTENER_EXIT=$?

# Try to kill xemu if still running
kill $XEMU_PID 2>/dev/null || true
wait $XEMU_PID 2>/dev/null || true

if [ $LISTENER_EXIT -eq 0 ]; then
    echo ""
    echo "✅ Test execution successful"
    exit 0
else
    echo ""
    echo "❌ Test execution failed"
    exit 1
fi
