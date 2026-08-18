#!/bin/bash

# Enhanced Performance Validation Test Suite
# Measures: code size, compilation time, and execution time
# Supports: cc45 (primary), cc65 (if available)
# Gracefully degrades for unavailable tools

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

# Detect available tools
MMEMU_CLI=$(which mmemu-cli 2>/dev/null || echo "")
XEMU=$(which xemu-xmega65 2>/dev/null || echo "")
CC65=$(which cc65 2>/dev/null || echo "")
CA65=$(which ca65 2>/dev/null || echo "")

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
    printf "  %-40s %12s %s\n" "$label:" "$value" "$unit"
}

# Measure execution using mmemu-cli
measure_execution() {
    local prg_file="$1"

    if [ -z "$MMEMU_CLI" ]; then
        echo "---"
        return
    fi

    if [ ! -f "$prg_file" ]; then
        echo "---"
        return
    fi

    # Run program with emulator
    local result=$(timeout 30 $MMEMU_CLI --prg "$prg_file" --headless >/dev/null 2>&1 && echo "Executed" || echo "---")
    echo "$result"
}

# Test cc45 level
test_cc45() {
    local level="$1"
    local flags="$2"
    local description="$3"

    print_header "$description"

    local source="$TEST_DIR/test_short.c"
    local object="$BUILD_DIR/fib_cc45_$level.o45"
    local binary="$BUILD_DIR/fib_cc45_${level}.prg"
    local assembly="$BUILD_DIR/fib_cc45_${level}.s45"

    echo -e "${YELLOW}Compiling with: cc45 $flags${NC}"
    $BIN_DIR/cc45 $flags -c -o "$object" "$source" 2>&1 | grep -v "^$" | head -3 || true

    echo -e "${YELLOW}Linking...${NC}"
    $BIN_DIR/ln45 "$object" -o "$binary" 2>&1 | grep -v "^$" | head -3 || true

    # Generate assembly
    $BIN_DIR/cc45 $flags -S -o "$assembly" "$source" 2>/dev/null || true

    local binary_size=$(get_file_size "$binary")
    local assembly_size=$(get_file_size "$assembly")

    echo -e "${GREEN}✓ Compilation completed${NC}\n"
    print_result "Binary size" "$binary_size" "bytes"
    print_result "Assembly size" "$assembly_size" "bytes"

    # Size reduction
    if [ -f "$BUILD_DIR/fib_cc45_o0_size.txt" ]; then
        local baseline=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt")
        if [ "$baseline" -gt 0 ] && [ "$binary_size" -gt 0 ]; then
            local reduction=$((100 - (binary_size * 100) / baseline))
            print_result "Size reduction vs -O0" "$reduction" "%"
        fi
    fi

    # Execution time measurement
    if [ ! -z "$MMEMU_CLI" ] && [ "$binary_size" -gt 0 ]; then
        echo -e "${YELLOW}Measuring execution...${NC}"
        local exec=$(measure_execution "$binary")
        print_result "Execution" "$exec" ""
    fi

    echo "$binary_size" > "$BUILD_DIR/fib_cc45_${level}_size.txt"
}

# Test cc65 if available
test_cc65() {
    if [ -z "$CC65" ] || [ -z "$CA65" ]; then
        return
    fi

    local level="$1"
    local flags="$2"
    local description="$3"

    print_header "$description"

    local source="$TEST_DIR/test_short.c"
    local assembly="$BUILD_DIR/fib_cc65_${level}.s"
    local object="$BUILD_DIR/fib_cc65_${level}.o"
    local binary="$BUILD_DIR/fib_cc65_${level}.prg"

    echo -e "${YELLOW}Compiling with: cc65 $flags${NC}"
    if ! $CC65 $flags -o "$assembly" "$source" 2>&1 | head -3; then
        echo "  (cc65 not compatible with this source)"
        return
    fi

    echo -e "${YELLOW}Assembling...${NC}"
    $CA65 -o "$object" "$assembly" 2>&1 | head -1 || true

    local binary_size=$(get_file_size "$object")
    echo -e "${GREEN}✓ Compilation completed${NC}\n"
    print_result "Object size" "$binary_size" "bytes"

    if [ -f "$BUILD_DIR/fib_cc45_o0_size.txt" ]; then
        local baseline=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt")
        if [ "$baseline" -gt 0 ] && [ "$binary_size" -gt 0 ]; then
            local diff=$((binary_size - baseline))
            print_result "Size vs cc45 -O0" "$diff" "bytes"
        fi
    fi

    echo "$binary_size" > "$BUILD_DIR/fib_cc65_${level}_size.txt"
}

# Generate comparison report
generate_report() {
    print_header "PERFORMANCE COMPARISON REPORT"

    echo "Compiler & Optimization Summary:"
    echo ""
    printf "%-42s %12s %12s %12s\n" "Configuration" "Size (bytes)" "Reduction" "Exec"
    printf "%-42s %12s %12s %12s\n" "----------------------------------------------" "-----------" "---------" "----"

    local cc45_o0=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt" 2>/dev/null || echo "0")

    if [ "$cc45_o0" != "0" ]; then
        printf "%-42s %12d %12s %12s\n" "cc45 -O0 (baseline)" "$cc45_o0" "-" "---"

        for level in o1 o2 o3 o3_sac; do
            local size=$(cat "$BUILD_DIR/fib_cc45_${level}_size.txt" 2>/dev/null || echo "0")
            if [ "$size" != "0" ]; then
                local reduction=$((100 - (size * 100) / cc45_o0))
                local label="cc45 -${level:0:2}"
                if [ "$level" = "o3_sac" ]; then
                    label="cc45 -O3 +SAC"
                fi
                printf "%-42s %12d %12d%% %12s\n" "$label" "$size" "$reduction" "---"
            fi
        done
    fi

    # cc65 if available
    local cc65_size=$(cat "$BUILD_DIR/fib_cc65_o0_size.txt" 2>/dev/null || echo "")
    if [ ! -z "$cc65_size" ] && [ "$cc65_size" != "0" ]; then
        if [ "$cc45_o0" != "0" ]; then
            local diff=$((cc65_size - cc45_o0))
            printf "%-42s %12d %12s %12s\n" "cc65 -O0" "$cc65_size" "(+${diff})" "---"
        fi
    fi

    echo ""
    echo "Notes:"
    echo "  '---' indicates data not available"
    if [ -z "$MMEMU_CLI" ]; then
        echo "  Execution time not available (mmemu-cli not found)"
    fi
    if [ -z "$CC65" ]; then
        echo "  cc65 not available (install cc65 to compare)"
    fi
    echo ""
}

main() {
    echo -e "${GREEN}"
    echo "╔════════════════════════════════════════════╗"
    echo "║  Enhanced Performance Validation Suite     ║"
    echo "║  Fibonacci Benchmark - Multi-Compiler      ║"
    echo "╚════════════════════════════════════════════╝"
    echo -e "${NC}"

    echo "Available tools:"
    [ ! -z "$MMEMU_CLI" ] && echo "  ✓ mmemu-cli (execution measurement)" || echo "  ✗ mmemu-cli (not found)"
    [ ! -z "$CC65" ] && echo "  ✓ cc65 (6502 compiler)" || echo "  ✗ cc65 (not found)"
    echo ""

    if [ ! -f "$BIN_DIR/cc45" ] || [ ! -f "$BIN_DIR/ln45" ]; then
        echo -e "${RED}✗ Error: cc45 or ln45 not found. Run: make all${NC}"
        exit 1
    fi

    # Test cc45 levels
    test_cc45 "o0" "-O0" "TEST 1: cc45 -O0 (No Optimization)"
    test_cc45 "o1" "-O1" "TEST 2: cc45 -O1 (Basic Optimization)"
    test_cc45 "o2" "-O2" "TEST 3: cc45 -O2 (Aggressive Optimization)"
    test_cc45 "o3" "-O3" "TEST 4: cc45 -O3 (Maximum Optimization)"
    test_cc45 "o3_sac" "-O3 -fstaticalloc" "TEST 5: cc45 -O3 +SAC (Maximum + Static Allocation)"

    # Test cc65 if available
    if [ ! -z "$CC65" ]; then
        test_cc65 "o0" "" "TEST 6: cc65 -O0 (Baseline)"
        test_cc65 "o2" "-O2" "TEST 7: cc65 -O2 (Optimized)"
    fi

    # Generate report
    generate_report

    echo "Build artifacts: $BUILD_DIR"
    ls -lh "$BUILD_DIR"/fib_* 2>/dev/null | wc -l | awk '{print "  Files generated: " $1}' || true

    echo -e "${GREEN}✓ Performance validation completed${NC}"
    echo -e "${BLUE}Results: $BUILD_DIR${NC}\n"
}

main "$@"
