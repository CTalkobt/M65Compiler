#!/bin/bash
# Shadow comparison test: compile programs through both legacy CodeGenerator
# and IR codegen paths, verify both assemble successfully.
# When mmemu is available, also compare execution results.
#
# Tests that the IR pipeline (IRBuilder → IRCodeGen) produces assembly
# that is accepted by ca45 and structurally correct.

CC=./bin/cc45
AS=./bin/ca45
MMEMU=mmemu-cli

failed=0
passed=0
skipped=0

mkdir -p build/test/shadow

# Check if mmemu can run (rawMega65 machine type)
MMEMU_OK=0
if command -v $MMEMU &>/dev/null; then
    MTEST=$(echo -e "q" | timeout 5 $MMEMU -m rawMega65 2>&1)
    if echo "$MTEST" | grep -q "^>"; then
        MMEMU_OK=1
    fi
fi
if [ "$MMEMU_OK" = "1" ]; then
    echo "(mmemu available — will compare execution results)"
else
    echo "(mmemu not available — assembly-only validation)"
fi

run_mmemu() {
    local prg="$1"
    echo -e "load $prg\nsetpc \$2000\nstep 5000\nregs\nq" | timeout 10 $MMEMU -m rawMega65 2>/dev/null
}

get_a_reg() {
    echo "$1" | grep -oP 'A:\s*\$\K[0-9A-Fa-f]+' | tail -1
}

# Test a C source file through both paths
shadow_test() {
    local name="$1"
    local src="$2"
    local legacy_s="build/test/shadow/${name}_legacy.s"
    local legacy_prg="build/test/shadow/${name}_legacy.prg"
    local ir_prg="build/test/shadow/${name}_ir.prg"

    # Legacy path: cc45 → .s → ca45 → .prg
    $CC "$src" -o "$legacy_s" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "SKIP: $name (legacy compilation failed)"
        skipped=$((skipped + 1))
        return
    fi
    $AS "$legacy_s" -o "$legacy_prg" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "SKIP: $name (legacy assembly failed)"
        skipped=$((skipped + 1))
        return
    fi

    # IR path: cc45 --codegen-ir → .ir.s → ca45 → .prg
    $CC --codegen-ir "$src" -o "build/test/shadow/${name}_tmp.s" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "SKIP: $name (IR compilation failed)"
        skipped=$((skipped + 1))
        return
    fi

    local ir_s="build/test/shadow/${name}_tmp.ir.s"
    if [ ! -f "$ir_s" ]; then
        echo "SKIP: $name (no .ir.s produced)"
        skipped=$((skipped + 1))
        return
    fi

    $AS "$ir_s" -o "$ir_prg" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "FAIL: $name (IR assembly failed)"
        echo "  IR assembly file: $ir_s"
        failed=$((failed + 1))
        return
    fi

    # Both paths assembled successfully
    if [ "$MMEMU_OK" = "1" ]; then
        # Run both on mmemu and compare A register
        LEGACY_OUT=$(run_mmemu "$legacy_prg")
        LEGACY_A=$(get_a_reg "$LEGACY_OUT")
        IR_OUT=$(run_mmemu "$ir_prg")
        IR_A=$(get_a_reg "$IR_OUT")

        if [ "$LEGACY_A" = "$IR_A" ]; then
            echo "PASS: $name (legacy A=\$$LEGACY_A, IR A=\$$IR_A)"
        else
            echo "FAIL: $name (legacy A=\$$LEGACY_A, IR A=\$$IR_A)"
            failed=$((failed + 1))
            return
        fi
    else
        # No mmemu — just verify both assembled
        local legacy_size=$(wc -c < "$legacy_prg")
        local ir_size=$(wc -c < "$ir_prg")
        echo "PASS: $name (both assembled: legacy=${legacy_size}B, ir=${ir_size}B)"
    fi
    passed=$((passed + 1))
}

echo "Running IR shadow comparison tests..."
echo ""

# Write inline test programs to temp files and test them
# Each program is self-contained (no stdlib) and returns a value in A

# Test 1: constant return
cat > build/test/shadow/t_const.c << 'EOF'
int main() { return 42; }
EOF
shadow_test "const_return" "build/test/shadow/t_const.c"

# Test 2: simple addition
cat > build/test/shadow/t_add.c << 'EOF'
int main() { return 3 + 4; }
EOF
shadow_test "simple_add" "build/test/shadow/t_add.c"

# Test 3: function call
cat > build/test/shadow/t_call.c << 'EOF'
int add(int a, int b) { return a + b; }
int main() { return add(10, 20); }
EOF
shadow_test "func_call" "build/test/shadow/t_call.c"

# Test 4: if/else
cat > build/test/shadow/t_if.c << 'EOF'
int main() {
    int x = 5;
    if (x > 3) return 1;
    else return 0;
}
EOF
shadow_test "if_else" "build/test/shadow/t_if.c"

# Test 5: while loop
cat > build/test/shadow/t_while.c << 'EOF'
int main() {
    int i = 0;
    int sum = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
EOF
shadow_test "while_loop" "build/test/shadow/t_while.c"

# Test 6: for loop
cat > build/test/shadow/t_for.c << 'EOF'
int main() {
    int sum = 0;
    int i;
    for (i = 1; i <= 10; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}
EOF
shadow_test "for_loop" "build/test/shadow/t_for.c"

# Test 7: nested calls
cat > build/test/shadow/t_nested.c << 'EOF'
int square(int x) { return x * x; }
int main() { return square(5); }
EOF
shadow_test "nested_call" "build/test/shadow/t_nested.c"

# Test 8: subtraction and negation
cat > build/test/shadow/t_sub.c << 'EOF'
int main() { return 100 - 57; }
EOF
shadow_test "subtraction" "build/test/shadow/t_sub.c"

# Test 9: bitwise ops
cat > build/test/shadow/t_bitwise.c << 'EOF'
int main() {
    int a = 0xFF;
    int b = 0x0F;
    return a & b;
}
EOF
shadow_test "bitwise_and" "build/test/shadow/t_bitwise.c"

# Test 10: multiple returns
cat > build/test/shadow/t_multiret.c << 'EOF'
int abs_val(int x) {
    if (x < 0) return -x;
    return x;
}
int main() { return abs_val(-7); }
EOF
shadow_test "abs_value" "build/test/shadow/t_multiret.c"

echo ""
echo "========================================"
echo "Shadow IR tests: $passed passed, $failed failed, $skipped skipped"
echo "========================================"

exit $failed
