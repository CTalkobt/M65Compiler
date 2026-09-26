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
KICKC=$(which kickc 2>/dev/null || echo "")
OSCARC=$(which oscarc 2>/dev/null || echo "")

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

# Parse mmemu-cli output for execution metrics
parse_mmemu_metrics() {
    local output="$1"
    local cycles=$(echo "$output" | grep -i "cycles" | head -1 | grep -o "[0-9]*" | head -1)

    if [ ! -z "$cycles" ] && [ "$cycles" -gt 0 ]; then
        # Convert cycles to approximate milliseconds (45GS02 @40MHz = 40M cycles/sec)
        local ms=$((cycles / 40000))
        if [ "$ms" -lt 1 ]; then
            ms=1
        fi
        echo "${cycles}c (${ms}ms)"
        return 0
    fi
    return 1
}

# Measure execution time using mmemu-cli with fallback to time command
measure_execution() {
    local prg_file="$1"

    if [ ! -f "$prg_file" ]; then
        echo "---"
        return
    fi

    # Try mmemu-cli first if available
    if [ ! -z "$MMEMU_CLI" ]; then
        local output=""
        local exit_code=0
        output=$(timeout 30 $MMEMU_CLI --prg "$prg_file" --headless 2>&1 || true)
        exit_code=$?

        # Check if program executed successfully
        if [ $exit_code -eq 0 ] || [ $exit_code -eq 124 ]; then
            # Try to parse metrics from output
            local metrics=$(parse_mmemu_metrics "$output")
            if [ ! -z "$metrics" ]; then
                echo "$metrics"
                return 0
            fi

            # If we got output but no metrics, try time-based measurement
            if [ ! -z "$output" ]; then
                # Program ran; use time command for wall-clock measurement
                local time_output=""
                time_output=$( { time timeout 30 $MMEMU_CLI --prg "$prg_file" --headless >/dev/null 2>&1; } 2>&1 | grep real)

                if [ ! -z "$time_output" ]; then
                    local ms=$(echo "$time_output" | grep -o "[0-9]*m[0-9]*\.[0-9]*s" | sed 's/m/*60000+/;s/s//;s/\./*/' | bc 2>/dev/null || echo "")
                    if [ ! -z "$ms" ] && [ "$ms" -gt 0 ]; then
                        echo "${ms}ms"
                        return 0
                    fi
                fi
            fi
        fi
    fi

    # Fallback: Try time command if program is executable
    if [ -x "$prg_file" ] || file "$prg_file" | grep -q "executable"; then
        local time_output=""
        time_output=$( { time timeout 5 "$prg_file" >/dev/null 2>&1; } 2>&1 | grep real 2>/dev/null || true)

        if [ ! -z "$time_output" ]; then
            local ms=$(echo "$time_output" | grep -o "[0-9]*m[0-9]*\.[0-9]*s" | sed 's/m/*60000+/;s/s//;s/\./*/' | bc 2>/dev/null || echo "")
            if [ ! -z "$ms" ] && [ "$ms" -gt 0 ]; then
                echo "${ms}ms (fallback)"
                return 0
            fi
        fi
    fi

    # Unable to measure
    echo "---"
}

# Test cc45 level
test_cc45() {
    local level="$1"
    local flags="$2"
    local description="$3"
    local test_type="${4:-io}"  # "io" for I/O-heavy, "compute" for computation-heavy

    print_header "$description"

    # Use computation-heavy test for better optimization differentiation
    local source="$TEST_DIR/test_short.c"
    if [ "$test_type" = "compute" ]; then
        source="/tmp/fib_compute_test.c"
        # Create computation-heavy test if it doesn't exist
        if [ ! -f "$source" ]; then
            cat > "$source" << 'TESTEOF'
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    volatile int r0 = 0, r1 = 0, r2 = 0, r3 = 0;
    r0 = fib(8);
    r1 = fib(9);
    r2 = fib(10);
    r3 = fib(11);
    return r0 + r1 + r2 + r3;
}
TESTEOF
        fi
    fi
    local object="$BUILD_DIR/fib_cc45_$level.o45"
    local binary="$BUILD_DIR/fib_cc45_${level}.prg"
    local assembly="$BUILD_DIR/fib_cc45_${level}.s45"

    echo -e "${YELLOW}Compiling with: cc45 $flags${NC}"
    $BIN_DIR/cc45 $flags -c -o "$object" "$source" 2>&1 | grep -v "^$" | head -3 || true

    echo -e "${YELLOW}Linking...${NC}"
    LINK_OUTPUT=$($BIN_DIR/ln45 "$object" "$PROJECT_ROOT/lib/build/c45.lib" -o "$binary" 2>&1)
    LINK_EXIT=$?
    if [ $LINK_EXIT -eq 0 ]; then
        echo "$LINK_OUTPUT" | grep -v "^$" | head -3 || true
    else
        echo "Link failed with exit code $LINK_EXIT:"
        echo "$LINK_OUTPUT" | head -5
    fi

    # Generate assembly
    $BIN_DIR/cc45 $flags -S -o "$assembly" "$source" 2>/dev/null || true

    local binary_size=$(get_file_size "$binary")
    local object_size=$(get_file_size "$object")
    local assembly_size=$(get_file_size "$assembly")

    echo -e "${GREEN}✓ Compilation completed${NC}\n"

    # Report both object and binary sizes
    if [ "$binary_size" -gt 0 ]; then
        print_result "Final .prg size" "$binary_size" "bytes"
    else
        print_result "Final .prg size" "---" "(linker issue)"
    fi
    print_result "Object file size" "$object_size" "bytes"
    print_result "Assembly source size" "$assembly_size" "bytes"

    # Size reduction (use binary if available, fallback to object)
    local size_for_reduction="$binary_size"
    if [ "$size_for_reduction" -eq 0 ]; then
        size_for_reduction="$object_size"
    fi

    if [ -f "$BUILD_DIR/fib_cc45_o0_size.txt" ]; then
        local baseline=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt")
        if [ "$baseline" -gt 0 ] && [ "$size_for_reduction" -gt 0 ]; then
            local reduction=$((100 - (size_for_reduction * 100) / baseline))
            print_result "Size reduction vs -O0" "$reduction" "%"
        fi
    fi

    # Execution time measurement (always try, even if no mmemu-cli)
    if [ "$binary_size" -gt 0 ]; then
        echo -e "${YELLOW}Measuring execution...${NC}"
        local exec=$(measure_execution "$binary")
        print_result "Execution" "$exec" ""
        echo "$exec" > "$BUILD_DIR/fib_cc45_${level}_exec.txt"
    else
        echo -e "${YELLOW}Skipping execution measurement (no .prg binary)${NC}"
        echo "---" > "$BUILD_DIR/fib_cc45_${level}_exec.txt"
    fi

    # Store the actual .prg size, or object size as fallback
    if [ "$binary_size" -gt 0 ]; then
        echo "$binary_size" > "$BUILD_DIR/fib_cc45_${level}_size.txt"
    else
        echo "$object_size" > "$BUILD_DIR/fib_cc45_${level}_size.txt"
    fi
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

    # Execution time measurement (if binary exists)
    if [ "$binary_size" -gt 0 ] && [ -f "$binary" ]; then
        echo -e "${YELLOW}Measuring execution...${NC}"
        local exec=$(measure_execution "$binary")
        print_result "Execution" "$exec" ""
        echo "$exec" > "$BUILD_DIR/fib_cc65_${level}_exec.txt"
    fi

    echo "$binary_size" > "$BUILD_DIR/fib_cc65_${level}_size.txt"
}

# Test KickC if available
test_kickc() {
    if [ -z "$KICKC" ]; then
        return
    fi

    print_header "TEST: KickC Compiler"

    local source="$TEST_DIR/test_short.c"
    local object="$BUILD_DIR/fib_kickc.o"

    echo -e "${YELLOW}Compiling with: kickc${NC}"
    if ! $KICKC -o "$object" "$source" 2>&1 | head -3; then
        echo "  (KickC compilation failed)"
        echo "0" > "$BUILD_DIR/fib_kickc_size.txt"
        echo "---" > "$BUILD_DIR/fib_kickc_exec.txt"
        return
    fi

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

    echo "$binary_size" > "$BUILD_DIR/fib_kickc_size.txt"
    echo "---" > "$BUILD_DIR/fib_kickc_exec.txt"
}

# Test Oscar C if available
test_oscarc() {
    if [ -z "$OSCARC" ]; then
        return
    fi

    print_header "TEST: Oscar C Compiler"

    local source="$TEST_DIR/test_short.c"
    local object="$BUILD_DIR/fib_oscarc.o"

    echo -e "${YELLOW}Compiling with: oscarc${NC}"
    if ! $OSCARC -o "$object" "$source" 2>&1 | head -3; then
        echo "  (Oscar C compilation failed)"
        echo "0" > "$BUILD_DIR/fib_oscarc_size.txt"
        echo "---" > "$BUILD_DIR/fib_oscarc_exec.txt"
        return
    fi

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

    echo "$binary_size" > "$BUILD_DIR/fib_oscarc_size.txt"
    echo "---" > "$BUILD_DIR/fib_oscarc_exec.txt"
}

# Generate comparison report
generate_report() {
    print_header "PERFORMANCE COMPARISON REPORT"

    echo "Compiler & Optimization Summary:"
    echo ""
    printf "%-42s %12s %12s %16s\n" "Configuration" "Size (bytes)" "Reduction" "Execution Time"
    printf "%-42s %12s %12s %16s\n" "----------------------------------------------" "-----------" "---------" "----------------"

    local cc45_o0=$(cat "$BUILD_DIR/fib_cc45_o0_size.txt" 2>/dev/null || echo "0")
    local cc45_o0_exec=$(cat "$BUILD_DIR/fib_cc45_o0_exec.txt" 2>/dev/null || echo "---")

    if [ "$cc45_o0" != "0" ]; then
        printf "%-42s %12d %12s %16s\n" "cc45 -O0 (SAC enabled)" "$cc45_o0" "-" "$cc45_o0_exec"

        for level in o1 o2 o3; do
            local size=$(cat "$BUILD_DIR/fib_cc45_${level}_size.txt" 2>/dev/null || echo "0")
            local exec=$(cat "$BUILD_DIR/fib_cc45_${level}_exec.txt" 2>/dev/null || echo "---")
            if [ "$size" != "0" ]; then
                local reduction=$((100 - (size * 100) / cc45_o0))
                printf "%-42s %12d %12d%% %16s\n" "cc45 -O${level:1:1} (SAC enabled)" "$size" "$reduction" "$exec"
            fi
        done

        # Show SAC benefit comparison
        echo ""
        printf "%-42s %12s %12s %16s\n" "--- SAC Benefit (disabled for comparison) ---" "Size" "Overhead" "Savings"
        local o0_nosac=$(cat "$BUILD_DIR/fib_cc45_o0_nosac_size.txt" 2>/dev/null || echo "0")
        local o1_nosac=$(cat "$BUILD_DIR/fib_cc45_o1_nosac_size.txt" 2>/dev/null || echo "0")

        if [ "$o0_nosac" != "0" ]; then
            local sac_savings=$((o0_nosac - cc45_o0))
            printf "%-42s %12d %12d bytes %6s\n" "cc45 -O0 (SAC disabled)" "$o0_nosac" "$sac_savings" "saved"
        fi
        if [ "$o1_nosac" != "0" ]; then
            local o1=$(cat "$BUILD_DIR/fib_cc45_o1_size.txt" 2>/dev/null || echo "0")
            local sac_savings=$((o1_nosac - o1))
            printf "%-42s %12d %12d bytes %6s\n" "cc45 -O1 (SAC disabled)" "$o1_nosac" "$sac_savings" "saved"
        fi
    fi

    # cc65 if available or unavailable
    local cc65_size=$(cat "$BUILD_DIR/fib_cc65_o0_size.txt" 2>/dev/null || echo "0")
    local cc65_exec=$(cat "$BUILD_DIR/fib_cc65_o0_exec.txt" 2>/dev/null || echo "---")
    if [ "$cc65_size" = "0" ]; then
        printf "%-42s %12s %12s %16s\n" "cc65 -O0" "---" "---" "$cc65_exec"
    elif [ "$cc45_o0" != "0" ]; then
        local diff=$((cc65_size - cc45_o0))
        printf "%-42s %12d %12s %16s\n" "cc65 -O0" "$cc65_size" "(+${diff})" "$cc65_exec"
    fi

    # KickC if available or unavailable
    local kickc_size=$(cat "$BUILD_DIR/fib_kickc_size.txt" 2>/dev/null || echo "0")
    local kickc_exec=$(cat "$BUILD_DIR/fib_kickc_exec.txt" 2>/dev/null || echo "---")
    if [ "$kickc_size" = "0" ]; then
        printf "%-42s %12s %12s %16s\n" "kickc" "---" "---" "$kickc_exec"
    elif [ "$cc45_o0" != "0" ]; then
        local diff=$((kickc_size - cc45_o0))
        printf "%-42s %12d %12s %16s\n" "kickc" "$kickc_size" "(+${diff})" "$kickc_exec"
    fi

    # Oscar C if available or unavailable
    local oscarc_size=$(cat "$BUILD_DIR/fib_oscarc_size.txt" 2>/dev/null || echo "0")
    local oscarc_exec=$(cat "$BUILD_DIR/fib_oscarc_exec.txt" 2>/dev/null || echo "---")
    if [ "$oscarc_size" = "0" ]; then
        printf "%-42s %12s %12s %16s\n" "oscarc" "---" "---" "$oscarc_exec"
    elif [ "$cc45_o0" != "0" ]; then
        local diff=$((oscarc_size - cc45_o0))
        printf "%-42s %12d %12s %16s\n" "oscarc" "$oscarc_size" "(+${diff})" "$oscarc_exec"
    fi

    echo ""
    echo "Notes:"
    echo "  Size Metrics:"
    echo "    If .prg linking succeeds: Final executable size"
    echo "    If .prg linking fails:    Object (.o45) file size"
    echo ""
    echo "  Execution Times:"
    echo "    '---'              = Unable to measure"
    echo "    'XXXXc (XXXms)'    = mmemu-cli: cycle count (estimated ms)"
    echo "    'XXXms'            = Wall-clock measurement (native execution)"
    echo "    'XXXms (fallback)' = Fallback time measurement"
    echo ""
    echo "Tools Status:"
    if [ -z "$MMEMU_CLI" ]; then
        echo "  ⓘ mmemu-cli not found (cycle-accurate measurement unavailable)"
    else
        echo "  ✓ mmemu-cli available (cycle-accurate measurement)"
    fi

    echo "Compiler Availability:"
    if [ -z "$CC65" ]; then
        echo "  ⓘ cc65 not available"
    else
        echo "  ✓ cc65 available"
    fi
    if [ -z "$KICKC" ]; then
        echo "  ⓘ kickc not available"
    else
        echo "  ✓ kickc available"
    fi
    if [ -z "$OSCARC" ]; then
        echo "  ⓘ oscarc not available"
    else
        echo "  ✓ oscarc available"
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

    # Test cc45 levels (with SAC enabled by default)
    test_cc45 "o0" "-O0" "TEST 1: cc45 -O0 (No Optimization, SAC default)"
    test_cc45 "o1" "-O1" "TEST 2: cc45 -O1 (Basic Optimization, SAC default)"
    test_cc45 "o2" "-O2" "TEST 3: cc45 -O2 (Aggressive Optimization, SAC default)"
    test_cc45 "o3" "-O3" "TEST 4: cc45 -O3 (Maximum Optimization, SAC default)"

    # Test without SAC for comparison (shows SAC benefit)
    test_cc45 "o0_nosac" "-O0 -fno-staticalloc" "TEST 5: cc45 -O0 (No SAC - baseline comparison)"
    test_cc45 "o1_nosac" "-O1 -fno-staticalloc" "TEST 6: cc45 -O1 (No SAC - baseline comparison)"

    # Test cc65 if available
    if [ ! -z "$CC65" ]; then
        test_cc65 "o0" "" "TEST 6: cc65 -O0 (Baseline)"
        test_cc65 "o2" "-O2" "TEST 7: cc65 -O2 (Optimized)"
    else
        # Create placeholder files for missing cc65
        echo "0" > "$BUILD_DIR/fib_cc65_o0_size.txt"
        echo "---" > "$BUILD_DIR/fib_cc65_o0_exec.txt"
        echo "0" > "$BUILD_DIR/fib_cc65_o2_size.txt"
        echo "---" > "$BUILD_DIR/fib_cc65_o2_exec.txt"
    fi

    # Test KickC if available
    if [ ! -z "$KICKC" ]; then
        test_kickc
    else
        echo "0" > "$BUILD_DIR/fib_kickc_size.txt"
        echo "---" > "$BUILD_DIR/fib_kickc_exec.txt"
    fi

    # Test Oscar C if available
    if [ ! -z "$OSCARC" ]; then
        test_oscarc
    else
        echo "0" > "$BUILD_DIR/fib_oscarc_size.txt"
        echo "---" > "$BUILD_DIR/fib_oscarc_exec.txt"
    fi

    # Generate report
    generate_report

    echo "Build artifacts: $BUILD_DIR"
    ls -lh "$BUILD_DIR"/fib_* 2>/dev/null | wc -l | awk '{print "  Files generated: " $1}' || true

    echo -e "${GREEN}✓ Performance validation completed${NC}"
    echo -e "${BLUE}Results: $BUILD_DIR${NC}\n"
}

main "$@"
