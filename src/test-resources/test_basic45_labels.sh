#!/bin/bash

# Test script for basic45 label support

BASIC45=../../bin/basic45
TMPDIR=$(mktemp -d)
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

pass_count=0
fail_count=0

# Helper function to run a test
run_test() {
    local name="$1"
    local input="$2"
    local expected_bytes="$3"
    local use_labels="${4:-false}"

    local input_file="$TMPDIR/test.bas"
    local output_file="$TMPDIR/test.prg"

    echo "$input" > "$input_file"

    if [ "$use_labels" = "true" ]; then
        if ! $BASIC45 "$input_file" -o "$output_file" --labels > /dev/null 2>&1; then
            echo "✗ FAIL: $name (compilation failed)"
            ((fail_count++))
            return
        fi
    else
        if ! $BASIC45 "$input_file" -o "$output_file" > /dev/null 2>&1; then
            echo "✗ FAIL: $name (compilation failed)"
            ((fail_count++))
            return
        fi
    fi

    local actual_bytes=$(wc -c < "$output_file")

    if [ "$actual_bytes" = "$expected_bytes" ]; then
        echo "✓ PASS: $name (${actual_bytes} bytes)"
        ((pass_count++))
    else
        echo "✗ FAIL: $name (expected ${expected_bytes} bytes, got ${actual_bytes})"
        ((fail_count++))
    fi
}

echo "Testing basic45 label support..."
echo ""

# Test 1: Basic label mode (simple print)
run_test "label_simple_print" \
    "print \"hello\"" \
    "17" true

# Test 2: Label with GOTO
run_test "label_with_goto" \
    "loop:
print \"x\"
goto loop" \
    "25" true

# Test 3: Comment stripping
run_test "comment_stripping" \
    "# This is a comment
print \"hello\"
# Another comment" \
    "17" true

# Test 4: Empty line skipping
run_test "empty_line_skipping" \
    "print \"line1\"

print \"line2\"" \
    "30" true

# Test 5: Normal mode still works
run_test "normal_mode_with_numbers" \
    "10 print \"hello\"
20 print \"world\"" \
    "30" false

# Test 6: Label name resolution in GOTO
run_test "label_goto_resolution" \
    "x:
print \"x\"
y:
goto x" \
    "31" true

# Test 7: Mixed statements without labels
run_test "statements_without_labels" \
    "print \"line1\"
x = 1
y = 2" \
    "33" true

# Test 8: Comments with # at line start
run_test "comment_at_line_start" \
    "# Full line comment
print \"hello\"" \
    "17" true

# Test 9: Label table generation
echo -n "Testing label table generation... "
{
    echo 'start:
print "hello"' > "$TMPDIR/test_label_table.bas"

    $BASIC45 "$TMPDIR/test_label_table.bas" -o "$TMPDIR/test_label_table.prg" \
        --labels --label-table "$TMPDIR/labels.txt" > /dev/null 2>&1

    if [ -f "$TMPDIR/labels.txt" ] && grep -q "start" "$TMPDIR/labels.txt"; then
        echo "✓ PASS"
        ((pass_count++))
    else
        echo "✗ FAIL"
        ((fail_count++))
    fi
}

# Test 10: GOSUB with labels
echo -n "Testing GOSUB with labels... "
{
    echo 'gosub sub1
print "done"

sub1:
print "sub"
return' > "$TMPDIR/test_gosub.bas"

    if $BASIC45 "$TMPDIR/test_gosub.bas" -o "$TMPDIR/test_gosub.prg" --labels > /dev/null 2>&1; then
        echo "✓ PASS"
        ((pass_count++))
    else
        echo "✗ FAIL"
        ((fail_count++))
    fi
}

# Test 11: Multiple labels
echo -n "Testing multiple labels... "
{
    echo 'main:
print "1"

sub:
print "2"

done:
print "3"' > "$TMPDIR/test_multi.bas"

    if $BASIC45 "$TMPDIR/test_multi.bas" -o "$TMPDIR/test_multi.prg" \
        --labels --label-table "$TMPDIR/labels_multi.txt" > /dev/null 2>&1; then

        if [ -f "$TMPDIR/labels_multi.txt" ] && \
           grep -q "main" "$TMPDIR/labels_multi.txt" && \
           grep -q "sub" "$TMPDIR/labels_multi.txt" && \
           grep -q "done" "$TMPDIR/labels_multi.txt"; then
            echo "✓ PASS"
            ((pass_count++))
        else
            echo "✗ FAIL (label table incomplete)"
            ((fail_count++))
        fi
    else
        echo "✗ FAIL (compilation failed)"
        ((fail_count++))
    fi
}

echo ""
echo "========================================"
echo "Test Results: $pass_count passed, $fail_count failed"
echo "========================================"

if [ $fail_count -eq 0 ]; then
    exit 0
else
    exit 1
fi
