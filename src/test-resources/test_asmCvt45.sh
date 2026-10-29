#!/bin/bash
# asmCvt45 format pair conversion tests - all 20 combinations

ASMCVT45="./bin/asmCvt45"
TEST_DIR="src/test-resources/asmCvt45_tests"
PASS=0
FAIL=0

[[ ! -f "$ASMCVT45" ]] && { echo "Error: asmCvt45 binary not found"; exit 1; }
[[ ! -d "$TEST_DIR" ]] && { echo "Error: test directory not found"; exit 1; }

FORMATS=("ca45" "ca65" "acme" "oscar" "kickassembler")

echo "=== asmCvt45 Format Pair Test Suite ==="
echo "Testing all N×N format conversions..."
echo ""

for input_fmt in "${FORMATS[@]}"; do
  test_file="$TEST_DIR/${input_fmt}_test.asm"
  [[ ! -f "$test_file" ]] && continue

  for output_fmt in "${FORMATS[@]}"; do
    [[ "$input_fmt" == "$output_fmt" ]] && continue

    if timeout 5 "$ASMCVT45" -f "$input_fmt" -t "$output_fmt" "$test_file" > /dev/null 2>&1; then
      echo "✓ $input_fmt → $output_fmt"
      ((PASS++))
    else
      echo "✗ $input_fmt → $output_fmt"
      ((FAIL++))
    fi
  done
done

echo ""
echo "Results: $PASS passed, $FAIL failed"
[[ $FAIL -eq 0 ]] && { echo "All tests passed!"; exit 0; } || { echo "Some tests failed!"; exit 1; }
