#!/bin/bash

# Test script for mmemu-cli validation with ca45

AS="./bin/ca45"
MMEMU="mmemu-cli"
mkdir -p build/test

echo "Testing mmemu-cli with a simple 'hello world' program..."

# Assemble the test program
$AS src/test-resources/test_mmemu_hello.s -o build/test_mmemu_hello.bin

# Run mmemu-cli and capture its output
# load at $2000, set PC to $2000, step 200, dump $4000, then quit
OUTPUT=$(echo -e "load build/test_mmemu_hello.bin \$2000\nsetpc \$2000\nstep 200\nm \$4000 12\nq" | $MMEMU -m rawMega65 2>/dev/null)

# Expected hex for "HELLO WORLD!" (PETSCII converted from "hello world!")
EXPECTED="48 45 4C 4C 4F 20 57 4F 52 4C 44 21"

if echo "$OUTPUT" | grep -q "$EXPECTED"; then
    echo "SUCCESS: 'hello world' was correctly copied in mmemu-cli."
    exit 0
else
    echo "FAIL: mmemu-cli validation failed."
    echo "Expected bytes at \$4000: $EXPECTED"
    echo "Actual output:"
    echo "$OUTPUT"
    exit 1
fi
