#!/bin/bash
# Profiling suite for optimization effectiveness analysis

COMPILER="./bin/cc45"
TEST_DIR="src/test-resources"
RESULTS_FILE="profiling_results.csv"
SUMMARY_FILE="profiling_summary.txt"

# Test programs representative of different code patterns
PATTERN_TESTS=(
    "test_constant_folding.c:constant-folding"
    "test_loop_unroll.c:loop-unrolling"
    "test_array_init.c:dead-code-elimination"
    "test_recursion.c:function-inlining"
    "test_control_flow.c:branch-optimization"
    "test_cast.c:strength-reduction"
)

echo "=== Optimization Effectiveness Profiling ===" | tee "$SUMMARY_FILE"
echo "Compiler: $COMPILER" | tee -a "$SUMMARY_FILE"
echo "Test date: $(date)" | tee -a "$SUMMARY_FILE"
echo "" | tee -a "$SUMMARY_FILE"

# Initialize CSV
echo "Test,Optimization,Level,CodeSize,Instructions,SizeReduction,ImprovementRate" > "$RESULTS_FILE"

# Helper function to measure code size
measure_code() {
    local test_file="$1"
    local opt_level="$2"
    
    if [ ! -f "$TEST_DIR/$test_file" ]; then
        echo "0:0"
        return
    fi
    
    $COMPILER "$TEST_DIR/$test_file" $opt_level -S -o /tmp/opt_test.s45 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "0:0"
        return
    fi
    
    # Count actual instruction lines (not directives)
    local inst_count=$(grep -E "^\s+(lda|ldx|ldy|ldz|sta|stx|sty|stz|adc|sbc|and|ora|eor|cmp|bit|clc|sec|cli|sei|clv|cld|sed|pha|pla|phx|plx|phy|ply|jsr|jmp|bne|beq|bcs|bcc|bmi|bpl|bvs|bvc|brk|rti|rts)" /tmp/opt_test.s45 2>/dev/null | wc -l)
    
    # Get binary size
    $COMPILER "$TEST_DIR/$test_file" $opt_level -c -o /tmp/opt_test.o45 2>/dev/null
    if [ ! -f /tmp/opt_test.o45 ]; then
        echo "0:0"
        return
    fi
    local size=$(stat -c%s /tmp/opt_test.o45 2>/dev/null || echo 0)
    
    echo "$size:$inst_count"
}

# Test each optimization level
echo "Testing baseline and optimization levels..." | tee -a "$SUMMARY_FILE"
for test_file in test_constant_folding.c test_loop_unroll.c test_array_init.c test_cast.c test_control_flow.c; do
    if [ ! -f "$TEST_DIR/$test_file" ]; then
        continue
    fi
    
    echo -n "Testing $test_file..."
    
    # Baseline (no optimization)
    baseline=$(measure_code "$test_file" "-O0")
    baseline_size=$(echo "$baseline" | cut -d: -f1)
    baseline_inst=$(echo "$baseline" | cut -d: -f2)
    
    # Test each level -O1 through -O3
    for level in -O1 -O2 -O3; do
        result=$(measure_code "$test_file" "$level")
        size=$(echo "$result" | cut -d: -f1)
        inst=$(echo "$result" | cut -d: -f2)
        
        if [ "$baseline_size" -gt 0 ]; then
            reduction=$((100 * (baseline_size - size) / baseline_size))
        else
            reduction=0
        fi
        
        echo "$test_file,$level,$level,$size,$inst,$reduction,$(date +%s)" >> "$RESULTS_FILE"
    done
    
    echo " done"
done

echo "" | tee -a "$SUMMARY_FILE"
echo "=== Results ===" | tee -a "$SUMMARY_FILE"

# Summarize results
if [ -f "$RESULTS_FILE" ]; then
    echo "Average code size reduction by optimization level:" | tee -a "$SUMMARY_FILE"
    awk -F, 'NR>1 {
        level=$3
        reduction=$6
        sum[level] += reduction
        count[level]++
    } END {
        for (level in sum) {
            avg = sum[level] / count[level]
            printf "%s: %.1f%% average reduction\n", level, avg
        }
    }' "$RESULTS_FILE" | sort | tee -a "$SUMMARY_FILE"
fi

echo "" | tee -a "$SUMMARY_FILE"
echo "Full results saved to: $RESULTS_FILE"
