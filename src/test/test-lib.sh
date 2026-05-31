#!/bin/bash
# Shared test utilities library for MEGA65 C Compiler Suite
# Reduces code duplication across test scripts

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Global test counters
TESTS_PASSED=0
TESTS_FAILED=0

# Tool paths (can be overridden)
CC="${CC:-./bin/cc45}"
AS="${AS:-./bin/ca45}"
LN="${LN:-./bin/ln45}"
AR="${AR:-./bin/ar45}"

# --- Output utilities ---

print_pass() {
    local msg="$1"
    echo -e "${GREEN}✓${NC} ${msg}"
    ((TESTS_PASSED++))
}

print_fail() {
    local msg="$1"
    echo -e "${RED}✗${NC} ${msg}"
    ((TESTS_FAILED++))
}

print_skip() {
    local msg="$1"
    echo -e "${YELLOW}⊘${NC} ${msg}"
}

print_section() {
    local title="$1"
    echo ""
    echo "=== $title ==="
}

# --- Compilation utilities ---

compile_c() {
    local source="$1"
    local output="$2"
    local opts="${3:-}"

    $CC $opts -v "$source" -o "$output" 2>/dev/null
}

assemble_s() {
    local source="$1"
    local output="$2"

    $AS "$source" -o "$output" 2>/dev/null
}

compile_and_assemble() {
    local source="$1"
    local out_base="$2"
    local opts="${3:-}"

    local s_file="${out_base}.s"
    local bin_file="${out_base}.bin"

    if ! compile_c "$source" "$s_file" "$opts"; then
        return 1
    fi

    if ! assemble_s "$s_file" "$bin_file"; then
        return 2
    fi

    return 0
}

# --- Assertion utilities ---

assert_file_exists() {
    local file="$1"
    local msg="${2:-File exists: $file}"

    if [ -f "$file" ]; then
        print_pass "$msg"
        return 0
    else
        print_fail "$msg"
        return 1
    fi
}

assert_grep() {
    local pattern="$1"
    local file="$2"
    local msg="${3:-Pattern found: $pattern in $file}"

    if grep -q "$pattern" "$file" 2>/dev/null; then
        print_pass "$msg"
        return 0
    else
        print_fail "$msg"
        return 1
    fi
}

assert_not_grep() {
    local pattern="$1"
    local file="$2"
    local msg="${3:-Pattern not found: $pattern in $file}"

    if ! grep -q "$pattern" "$file" 2>/dev/null; then
        print_pass "$msg"
        return 0
    else
        print_fail "$msg"
        return 1
    fi
}

assert_equal() {
    local expected="$1"
    local actual="$2"
    local msg="${3:-$expected == $actual}"

    if [ "$expected" = "$actual" ]; then
        print_pass "$msg"
        return 0
    else
        print_fail "$msg"
        return 1
    fi
}

assert_size_range() {
    local file="$1"
    local min="$2"
    local max="$3"
    local msg="${4:-File size in range: $min-$max bytes}"

    if [ ! -f "$file" ]; then
        print_fail "$msg (file not found)"
        return 1
    fi

    local size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)

    if [ "$size" -ge "$min" ] && [ "$size" -le "$max" ]; then
        print_pass "$msg ($size bytes)"
        return 0
    else
        print_fail "$msg (got $size bytes)"
        return 1
    fi
}

# --- Test result summary ---

test_summary() {
    local total=$((TESTS_PASSED + TESTS_FAILED))
    echo ""
    echo "=========================================="
    echo "Tests: $TESTS_PASSED passed, $TESTS_FAILED failed"
    echo "=========================================="

    if [ "$TESTS_FAILED" -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# --- Test batch utilities ---

test_batch() {
    local test_name="$1"
    local test_file="$2"
    shift 2
    local opts="$@"

    if compile_and_assemble "$test_file" "build/test/${test_name}" "$opts"; then
        print_pass "$test_name"
        return 0
    else
        print_fail "$test_name"
        return 1
    fi
}

test_batch_array() {
    local -n test_array=$1
    local opts="${2:-}"

    for test_file in "${test_array[@]}"; do
        local test_name=$(basename "$test_file" .c)
        test_batch "$test_name" "$test_file" $opts
    done
}

# --- Validation test utilities ---

validate_error() {
    local source="$1"
    local expected_error="$2"
    local msg="${3:-Error validation}"

    if compile_c "$source" /dev/null 2>&1 | grep -q "$expected_error"; then
        print_pass "$msg"
        return 0
    else
        print_fail "$msg"
        return 1
    fi
}

validate_no_error() {
    local source="$1"
    local msg="${2:-Should compile without error}"

    if compile_c "$source" /dev/null 2>&1 | grep -q "error\|Error"; then
        print_fail "$msg"
        return 1
    else
        print_pass "$msg"
        return 0
    fi
}

# --- Setup/teardown utilities ---

setup_test_dir() {
    mkdir -p build/test
}

teardown_test_dir() {
    rm -f build/test/*.{s,bin,o45,prg}
}

# --- Ensure we have a test directory ---
setup_test_dir
