#!/bin/bash
# Differential execution test: run small self-contained programs on xemu-xmega65
# and compare main()'s return value (low byte) with the same program compiled
# natively by the host gcc.
#
# cc45 integer types are unsigned unless declared signed (gcc: signed), so
# programs that rely on negative values declare them "signed".
#
# (Originally compared the legacy CodeGenerator with the IR path; the legacy
# path no longer exists, so the host compiler is the reference.)

CC=./bin/cc45
LD=./bin/ln45
CRT0=lib/build/crt0.o45
LIB=lib/build/c45.lib
XEMU=xemu-xmega65
HOSTCC=${HOSTCC:-gcc}

failed=0
passed=0
skipped=0

mkdir -p build/test/shadow

if ! command -v $XEMU &>/dev/null; then
    echo "xemu-xmega65 not found — skipping shadow execution tests"
    exit 0
fi
if ! command -v $HOSTCC &>/dev/null; then
    echo "$HOSTCC not found — skipping shadow execution tests"
    exit 0
fi

# Harness: calls the test's main (renamed) and stores the result at $C0F0
HARNESS=build/test/shadow/harness
cat > $HARNESS.c << 'END_HARNESS'
int __shadow_main(void);
void main(void) {
    int r = __shadow_main();
    *(volatile unsigned char *)0xC0F0 = (unsigned char)r;
    *(volatile unsigned char *)0xC0F1 = 0xAA;
}
END_HARNESS
if ! $CC -c $HARNESS.c -o $HARNESS.o45 2>/dev/null; then
    echo "FAIL: harness compilation"
    exit 1
fi

shadow_test() {
    local name="$1"
    local src="$2"
    # SHADOW_ONLY="a b": run only the named tests
    if [ -n "$SHADOW_ONLY" ] && [[ " $SHADOW_ONLY " != *" $name "* ]]; then return; fi
    local base="build/test/shadow/${name}"

    # Reference: host compiler, exit status = low byte of main's return value
    if ! $HOSTCC -w -O0 -o "$base.host" "$src" 2>/dev/null; then
        echo "SKIP: $name (host compilation failed)"
        skipped=$((skipped + 1))
        return
    fi
    "$base.host"
    local expected=$(( $? & 0xFF ))

    if ! $CC -c -Dmain=__shadow_main "$src" -o "$base.o45" 2>"$base.err"; then
        echo "FAIL: $name (cc45 compilation failed: $(head -1 "$base.err"))"
        failed=$((failed + 1))
        return
    fi
    if ! $LD -basic -o "$base.prg" "$CRT0" "$HARNESS.o45" "$base.o45" "$LIB" >/dev/null 2>"$base.lderr"; then
        echo "FAIL: $name (link failed: $(head -1 "$base.lderr"))"
        failed=$((failed + 1))
        return
    fi
    rm -f "$base.mem"
    timeout 15 $XEMU -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$base.mem" -prg "$base.prg" </dev/null >/dev/null 2>&1
    local out; out=$(xxd -s $((16#C0F0)) -l 2 -p "$base.mem" 2>/dev/null)
    if [ "${out:2:2}" != "aa" ]; then
        echo "FAIL: $name (program did not finish)"
        failed=$((failed + 1))
        return
    fi
    local got=$((16#${out:0:2}))
    if [ "$got" -eq "$expected" ]; then
        echo "PASS: $name (result $got)"
        passed=$((passed + 1))
    else
        echo "FAIL: $name (cc45 $got, $HOSTCC $expected)"
        failed=$((failed + 1))
    fi
}

echo "Running shadow execution tests (cc45 on xemu vs $HOSTCC)..."
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
signed int abs_val(signed int x) {
    if (x < 0) return -x;
    return x;
}
int main() { return abs_val(-7); }
EOF
shadow_test "abs_value" "build/test/shadow/t_multiret.c"

# ============================================================================
# Extended coverage tests
# ============================================================================

# Test 11: do-while loop
cat > build/test/shadow/t_dowhile.c << 'EOF'
int main() {
    int i = 0;
    int sum = 0;
    do {
        sum = sum + i;
        i = i + 1;
    } while (i < 5);
    return sum;
}
EOF
shadow_test "do_while" "build/test/shadow/t_dowhile.c"

# Test 12: nested if
cat > build/test/shadow/t_nested_if.c << 'EOF'
int main() {
    int x = 10;
    int y = 20;
    if (x < y) {
        if (x > 5) {
            return 1;
        } else {
            return 2;
        }
    }
    return 0;
}
EOF
shadow_test "nested_if" "build/test/shadow/t_nested_if.c"

# Test 13: multiple functions with chain
cat > build/test/shadow/t_chain.c << 'EOF'
int double_it(int x) { return x + x; }
int quad(int x) { return double_it(double_it(x)); }
int main() { return quad(3); }
EOF
shadow_test "call_chain" "build/test/shadow/t_chain.c"

# Test 14: comparison operators
cat > build/test/shadow/t_cmp.c << 'EOF'
int main() {
    int result = 0;
    if (5 == 5) result = result + 1;
    if (5 != 3) result = result + 1;
    if (3 < 5)  result = result + 1;
    if (5 > 3)  result = result + 1;
    if (5 >= 5) result = result + 1;
    if (5 <= 5) result = result + 1;
    return result;
}
EOF
shadow_test "comparisons" "build/test/shadow/t_cmp.c"

# Test 15: bitwise or/xor/not
cat > build/test/shadow/t_bitops.c << 'EOF'
int main() {
    int a = 0xF0;
    int b = 0x0F;
    int c = a | b;
    int d = a ^ b;
    int e = c & d;
    return e;
}
EOF
shadow_test "bitwise_ops" "build/test/shadow/t_bitops.c"

# Test 16: shifts
cat > build/test/shadow/t_shift.c << 'EOF'
int main() {
    int x = 1;
    x = x << 4;
    x = x >> 2;
    return x;
}
EOF
shadow_test "shifts" "build/test/shadow/t_shift.c"

# Test 17: char type (8-bit)
cat > build/test/shadow/t_char.c << 'EOF'
int main() {
    char a = 65;
    char b = 10;
    return a + b;
}
EOF
shadow_test "char_type" "build/test/shadow/t_char.c"

# Test 18: recursive function
cat > build/test/shadow/t_recurse.c << 'EOF'
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
int main() { return factorial(5); }
EOF
shadow_test "recursion" "build/test/shadow/t_recurse.c"

# Test 19: modulo
cat > build/test/shadow/t_mod.c << 'EOF'
int main() {
    int x = 17;
    int y = 5;
    return x - (x / y) * y;
}
EOF
shadow_test "modulo_manual" "build/test/shadow/t_mod.c"

# Test 20: complex expression
cat > build/test/shadow/t_expr.c << 'EOF'
int main() {
    int a = 3;
    int b = 4;
    int c = 5;
    return (a + b) * c - a;
}
EOF
shadow_test "complex_expr" "build/test/shadow/t_expr.c"

# Test 21: multiple locals
cat > build/test/shadow/t_locals.c << 'EOF'
int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    return a + b + c + d + e;
}
EOF
shadow_test "many_locals" "build/test/shadow/t_locals.c"

# Test 22: global variable
cat > build/test/shadow/t_global.c << 'EOF'
int g = 42;
int main() { return g; }
EOF
shadow_test "global_var" "build/test/shadow/t_global.c"

# Test 23: logical not
cat > build/test/shadow/t_lognot.c << 'EOF'
int main() {
    int x = 0;
    if (!x) return 1;
    return 0;
}
EOF
shadow_test "logical_not" "build/test/shadow/t_lognot.c"

# Test 24: ternary operator
cat > build/test/shadow/t_ternary.c << 'EOF'
int main() {
    int x = 10;
    int y = (x > 5) ? 100 : 200;
    return y;
}
EOF
shadow_test "ternary" "build/test/shadow/t_ternary.c"

# Test 25: cast
cat > build/test/shadow/t_cast.c << 'EOF'
int main() {
    int x = 300;
    char c = (char)x;
    return c;
}
EOF
shadow_test "cast" "build/test/shadow/t_cast.c"

# Test 26: void function
cat > build/test/shadow/t_void.c << 'EOF'
int result;
void set_result(int x) { result = x; }
int main() {
    set_result(77);
    return result;
}
EOF
shadow_test "void_func" "build/test/shadow/t_void.c"

# Test 27: early return
cat > build/test/shadow/t_early.c << 'EOF'
int find_first_gt(int a, int b, int c, int threshold) {
    if (a > threshold) return a;
    if (b > threshold) return b;
    if (c > threshold) return c;
    return 0;
}
int main() { return find_first_gt(3, 15, 7, 10); }
EOF
shadow_test "early_return" "build/test/shadow/t_early.c"

# Test 28: countdown (stays positive, no sign crossing)
cat > build/test/shadow/t_countdown.c << 'EOF'
int main() {
    int n = 21;
    int count = 0;
    while (n > 7) {
        n = n - 7;
        count = count + 1;
    }
    return count;
}
EOF
shadow_test "countdown" "build/test/shadow/t_countdown.c"

# Test 29: multiply and divide
cat > build/test/shadow/t_muldiv.c << 'EOF'
int main() {
    int x = 6;
    int y = 7;
    int z = x * y;
    return z / 2;
}
EOF
shadow_test "mul_div" "build/test/shadow/t_muldiv.c"

# Test 30: fibonacci
cat > build/test/shadow/t_fib.c << 'EOF'
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
int main() { return fib(8); }
EOF
shadow_test "fibonacci" "build/test/shadow/t_fib.c"

# Regression: unsigned global with the top bit set must use unsigned division
cat > build/test/shadow/t_ugdiv.c << 'EOF'
unsigned int u = 65531u;
int main() { return (int)(u / 7u % 256u) + (int)(u % 14u); }
EOF
shadow_test "unsigned_global_div" "build/test/shadow/t_ugdiv.c"

# Regression: "signed" applies to every global in a comma list
cat > build/test/shadow/t_commasign.c << 'EOF'
signed int a = -100, b = 7;
int main() { return (a / b) + 20; }
EOF
shadow_test "comma_decl_signed" "build/test/shadow/t_commasign.c"

# Regression: code after a return in the same block must not run
cat > build/test/shadow/t_deadret.c << 'EOF'
int g;
void set(void) { g = 99; }
int main() { if (1) return g + 5; set(); return g; }
EOF
shadow_test "return_then_dead_code" "build/test/shadow/t_deadret.c"

echo ""
echo "========================================"
echo "Shadow IR tests: $passed passed, $failed failed, $skipped skipped"
echo "========================================"

exit $failed
