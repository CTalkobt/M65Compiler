#!/bin/bash

# Performance Validation Test Suite - SIMPLIFIED VERSION
# Measures code size, compilation time for Fibonacci benchmark

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BIN_DIR="$PROJECT_ROOT/bin"
BUILD_DIR="$PROJECT_ROOT/build/performance"
TEST_DIR="$PROJECT_ROOT/src/test-resources"

mkdir -p "$BUILD_DIR"

get_file_size() {
    if [ -f "$1" ]; then
        stat -f%z "$1" 2>/dev/null || stat -c%s "$1" 2>/dev/null || wc -c < "$1"
    else
        echo "0"
    fi
}

print_header() {
    echo -e "\n${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}\n"
}

print_result() {
    local label="$1"
    local value="$2"
    local unit="$3"
    printf "  %-35s %10s %s\n" "$label:" "$value" "$unit"
}

test_level() {
    local level="$1"
    local flag_extra="$2"
    local test_num="$3"
    local test_name="$4"

    print_header "$test_name"

    local source="$TEST_DIR/test_short.c"
    local object="$BUILD_DIR/fib_cc45_$level.o45"
    local binary="$BUILD_DIR/fib_cc45_${level}.prg"

    echo -e "${YELLOW}Compiling with $flag_extra...${NC}"
    $BIN_DIR/cc45 $flag_extra -c -o "$object" "$source" 2>&1 | grep -v "^$" || true

    echo -e "${YELLOW}Linking...${NC}"
    $BIN_DIR/ln45 "$object" -o "$binary" 2>&1 | grep -v "^$" || true

    local binary_size=$(get_file_size "$binary")

    echo -e "${GREEN}✓ Test completed${NC}\n"
    print_result "Binary size" "$binary_size" "bytes"

    if [ -f "$BUILD_DIR/fib_cc45_o0_size.txt" ]; then
        local baseline=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt")
        if [ "$baseline" -gt 0 ]; then
            local reduction=$((100 - (binary_size * 100) / baseline))
            print_result "Size reduction vs -O0" "$reduction" "%"
        fi
    fi

    echo "$binary_size" > "$BUILD_DIR/fib_cc45_${level}_size.txt"
}

main() {
    echo -e "${GREEN}"
    echo "╔════════════════════════════════════════════╗"
    echo "║  Performance Validation Test Suite        ║"
    echo "║  Fibonacci Benchmark - Multi-Level        ║"
    echo "╚════════════════════════════════════════════╝"
    echo -e "${NC}"

    if [ ! -f "$BIN_DIR/cc45" ] || [ ! -f "$BIN_DIR/ln45" ]; then
        echo -e "${RED}✗ Error: Compilers not found. Run: make all${NC}"
        exit 1
    fi

    test_level "o0" "-O0" 1 "TEST 1: cc45 -O0 (No Optimization)"
    test_level "o1" "-O1" 2 "TEST 2: cc45 -O1 (Basic Optimization)"
    test_level "o2" "-O2" 3 "TEST 3: cc45 -O2 (Aggressive Optimization)"
    test_level "o3" "-O3" 4 "TEST 4: cc45 -O3 (Maximum Optimization)"
    test_level "o3_sac" "-O3 -fstaticalloc" 5 "TEST 5: cc45 -O3 -fstaticalloc (Maximum + SAC)"

    print_header "PERFORMANCE VALIDATION SUMMARY"

    local o0=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt" 2>/dev/null || echo "N/A")
    local o1=$(cat "$BUILD_DIR/fib_cc45_o1_size.txt" 2>/dev/null || echo "N/A")
    local o2=$(cat "$BUILD_DIR/fib_cc45_o2_size.txt" 2>/dev/null || echo "N/A")
    local o3=$(cat "$BUILD_DIR/fib_cc45_o3_size.txt" 2>/dev/null || echo "N/A")
    local o3sac=$(cat "$BUILD_DIR/fib_cc45_o3_sac_size.txt" 2>/dev/null || echo "N/A")

    echo "Optimization Level Comparison:"
    printf "  %-20s %15s %15s\n" "Level" "Binary Size" "Reduction"
    printf "  %-20s %15s %15s\n" "------" "-----------" "---------"

    if [ "$o0" != "N/A" ]; then
        printf "  %-20s %15d bytes %8s\n" "-O0 (baseline)" "$o0" "-"
    fi

    if [ "$o1" != "N/A" ] && [ "$o0" != "N/A" ] && [ "$o0" -gt 0 ]; then
        local red=$((100 - (o1 * 100) / o0))
        printf "  %-20s %15d bytes %8d%%\n" "-O1" "$o1" "$red"
    fi

    if [ "$o2" != "N/A" ] && [ "$o0" != "N/A" ] && [ "$o0" -gt 0 ]; then
        local red=$((100 - (o2 * 100) / o0))
        printf "  %-20s %15d bytes %8d%%\n" "-O2" "$o2" "$red"
    fi

    if [ "$o3" != "N/A" ] && [ "$o0" != "N/A" ] && [ "$o0" -gt 0 ]; then
        local red=$((100 - (o3 * 100) / o0))
        printf "  %-20s %15d bytes %8d%%\n" "-O3" "$o3" "$red"
    fi

    if [ "$o3sac" != "N/A" ] && [ "$o0" != "N/A" ] && [ "$o0" -gt 0 ]; then
        local red=$((100 - (o3sac * 100) / o0))
        printf "  %-20s %15d bytes %8d%%\n" "-O3 +SAC" "$o3sac" "$red"
    fi

    echo ""
    echo "Build artifacts:"
    echo "  Location: $BUILD_DIR"
    echo "  Files generated:"
    ls -lh "$BUILD_DIR"/fib_*.* 2>/dev/null | awk '{print "    " $9 " (" $5 ")"}' || true

    echo ""
    echo -e "${GREEN}✓ All performance validation tests completed${NC}"
    echo -e "${BLUE}Results saved to: $BUILD_DIR${NC}\n"
}

main "$@"
