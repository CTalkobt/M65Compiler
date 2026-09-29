#!/bin/bash
# cvt_asm format conversion test suite
# Tests all N×N format conversions for correctness

set -e

CVT_ASM="./bin/cvt_asm"
TEST_DIR="src/test-resources/cvt_asm_tests"
PASS=0
FAIL=0
SKIP=0

# ANSI colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Supported formats
FORMATS=("ca45" "ca65" "acme" "oscar" "kickassembler")

die() {
    echo -e "${RED}FATAL: $1${NC}" >&2
    exit 1
}

check_binary() {
    if [[ ! -f "$CVT_ASM" ]]; then
        die "cvt_asm binary not found at $CVT_ASM. Run 'make bin/cvt_asm' first."
    fi
}

check_test_files() {
    if [[ ! -d "$TEST_DIR" ]]; then
        die "Test directory $TEST_DIR not found."
    fi
}

# Test a single format pair conversion
# Usage: test_conversion <input_format> <output_format> <test_file>
test_conversion() {
    local input_fmt="$1"
    local output_fmt="$2"
    local test_file="$3"
    local test_name=$(basename "$test_file")

    if [[ ! -f "$test_file" ]]; then
        echo -e "${YELLOW}SKIP${NC}: $test_name ($input_fmt → $output_fmt): file not found"
        ((SKIP++))
        return 0
    fi

    local output=$("$CVT_ASM" -f "$input_fmt" -t "$output_fmt" "$test_file" 2>&1)
    local exit_code=$?

    if [[ $exit_code -ne 0 ]]; then
        echo -e "${RED}FAIL${NC}: $test_name ($input_fmt → $output_fmt)"
        echo "  Error: $(echo "$output" | head -1)"
        ((FAIL++))
        return 1
    fi

    # Verify output is not empty
    if [[ -z "$output" ]]; then
        echo -e "${RED}FAIL${NC}: $test_name ($input_fmt → $output_fmt): empty output"
        ((FAIL++))
        return 1
    fi

    # Verify output has reasonable content (at least one line with content)
    local line_count=$(echo "$output" | wc -l)
    if [[ $line_count -lt 2 ]]; then
        echo -e "${RED}FAIL${NC}: $test_name ($input_fmt → $output_fmt): insufficient output ($line_count lines)"
        ((FAIL++))
        return 1
    fi

    echo -e "${GREEN}PASS${NC}: $test_name ($input_fmt → $output_fmt)"
    ((PASS++))
    return 0
}

# Run all tests
run_all_tests() {
    echo "=== cvt_asm Format Conversion Test Suite ==="
    echo ""

    # Test each format against all test files
    for fmt in "${FORMATS[@]}"; do
        local test_file="$TEST_DIR/${fmt}_test.asm"

        if [[ ! -f "$test_file" ]]; then
            continue
        fi

        echo "Testing $fmt format:"

        # Test conversions from this format to all others
        for target_fmt in "${FORMATS[@]}"; do
            if [[ "$fmt" != "$target_fmt" ]]; then
                test_conversion "$fmt" "$target_fmt" "$test_file"
            fi
        done

        echo ""
    done

    # Summary
    echo "=== Test Summary ==="
    echo -e "${GREEN}PASS:${NC} $PASS"
    echo -e "${RED}FAIL:${NC} $FAIL"
    echo -e "${YELLOW}SKIP:${NC} $SKIP"
    local total=$((PASS + FAIL + SKIP))
    echo "Total: $total"

    if [[ $FAIL -eq 0 ]]; then
        echo -e "${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}Some tests failed.${NC}"
        return 1
    fi
}

# Main
check_binary
check_test_files
run_all_tests
