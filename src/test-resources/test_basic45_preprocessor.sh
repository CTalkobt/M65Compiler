#!/bin/bash

# Test script for basic45 preprocessor and advanced features

BASIC45=../../bin/basic45
TMPDIR=$(mktemp -d)
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

pass_count=0
fail_count=0

run_test() {
    local name="$1"
    local input="$2"
    local expected_bytes="$3"
    local extra_args="${4:-}"

    local input_file="$TMPDIR/test.bas"
    local output_file="$TMPDIR/test.prg"

    echo "$input" > "$input_file"

    if ! $BASIC45 "$input_file" -o "$output_file" $extra_args > /dev/null 2>&1; then
        echo "✗ FAIL: $name (compilation failed)"
        ((fail_count++))
        return
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

echo "Testing basic45 preprocessor and advanced features..."
echo ""

# Test 1: Simple #define
run_test "define_simple" \
    "#define X 5
print X" \
    "11" "--labels"

# Test 2: Multiple #define
run_test "define_multiple" \
    "#define A 1
#define B 2
print A;B" \
    "13" "--labels"

# Test 3: #ifdef true
run_test "ifdef_true" \
    "#define DEBUG 1
#ifdef DEBUG
print \"debug\"
#endif" \
    "17" "--labels"

# Test 4: #ifdef false
run_test "ifdef_false" \
    "#ifdef UNDEFINED
print \"hidden\"
#endif
print \"shown\"" \
    "17" "--labels"

# Test 5: #ifdef with #else
run_test "ifdef_else" \
    "#ifdef DEBUG
print \"debug\"
#else
print \"release\"
#endif" \
    "19" "--labels"

# Test 6: #ifndef
run_test "ifndef" \
    "#ifndef RELEASE
print \"debug\"
#endif" \
    "17" "--labels"

# Test 7: Line increment 1
run_test "increment_1" \
    "loop:
print \"x\"" \
    "18" "--labels --increment 1"

# Test 8: Line increment 5
run_test "increment_5" \
    "loop:
print \"x\"" \
    "18" "--labels --increment 5"

# Test 9: Line increment 100
run_test "increment_100" \
    "loop:
print \"x\"" \
    "18" "--labels --increment 100"

# Test 10: Documentation generation
echo -n "Testing documentation generation... "
{
    echo 'start:
print "hello"

finish:
print "done"' > "$TMPDIR/test_docs.bas"

    docs_file="$TMPDIR/docs.md"
    if $BASIC45 "$TMPDIR/test_docs.bas" -o "$TMPDIR/test_docs.prg" --labels --docs "$docs_file" > /dev/null 2>&1; then
        if [ -f "$docs_file" ] && grep -q "start" "$docs_file"; then
            echo "✓ PASS"
            ((pass_count++))
        else
            echo "✗ FAIL (documentation not generated correctly)"
            ((fail_count++))
        fi
    else
        echo "✗ FAIL (compilation failed)"
        ((fail_count++))
    fi
}

# Test 11: Include file support
echo -n "Testing #include support... "
{
    lib_file="$TMPDIR/lib.bas"
    main_file="$TMPDIR/with_include.bas"
    output_file="$TMPDIR/with_include.prg"

    echo '#define WIDTH 40' > "$lib_file"
    echo '#include "lib.bas"' > "$main_file"
    echo 'print WIDTH' >> "$main_file"

    if $BASIC45 "$main_file" -o "$output_file" --labels > /dev/null 2>&1; then
        echo "✓ PASS"
        ((pass_count++))
    else
        echo "✗ FAIL (include failed)"
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
