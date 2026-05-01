#!/bin/bash
# Test for register keyword in compiler — validates parsing, ZP allocation, and codegen

CC="./bin/cc45"
AS="./bin/ca45"
TEMP_C="test_register_tmp.c"
TEMP_S="test_register_tmp.s"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "--- Test: register keyword parsing and codegen ---"

# 1. register int compiles successfully
cat <<EOF > $TEMP_C
int main() {
    register int i = 0;
    return i;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ]; then pass "register int compiles"; else fail "register int compiles"; fi

# 2. register char compiles successfully
cat <<EOF > $TEMP_C
int main() {
    register char c = 42;
    return c;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ]; then pass "register char compiles"; else fail "register char compiles"; fi

# 3. register int allocates in ZP (comment marker present)
cat <<EOF > $TEMP_C
int main() {
    register int i = 0;
    return i;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q "register _l_i" $TEMP_S; then
    pass "register int emits ZP comment"
else
    fail "register int emits ZP comment"
fi

# 4. register char allocates in ZP
cat <<EOF > $TEMP_C
int main() {
    register char c = 42;
    return c;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q "register _l_c" $TEMP_S; then
    pass "register char emits ZP comment"
else
    fail "register char emits ZP comment"
fi

# 5. register var does NOT emit .var directive
cat <<EOF > $TEMP_C
int main() {
    register int i = 10;
    return i;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q "\.var _l_i" $TEMP_S; then
    fail "register int skips .var directive"
else
    pass "register int skips .var directive"
fi

# 6. register var uses direct ZP store (sta $XX) — use -O0 to prevent folding
cat <<EOF > $TEMP_C
int main() {
    register int i = 42;
    i = i + 1;
    return i;
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'sta \$0' $TEMP_S; then
    pass "register int stores via ZP"
else
    fail "register int stores via ZP"
fi

# 7. register var uses direct ZP load for function args
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    register int i = 42;
    return add(i, 1);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'ldax \$0' $TEMP_S; then
    pass "register int loads via ZP for func arg"
else
    fail "register int loads via ZP for func arg"
fi

# 8. register int uses inw for increment — use -O0
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    register int i = 0;
    i = i + 1;
    return add(i, 0);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'inw \$0' $TEMP_S; then
    pass "register int uses inw for +1"
else
    fail "register int uses inw for +1"
fi

# 9. register char uses inc for increment — use -O0
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    register char c = 0;
    c = c + 1;
    return add(c, 0);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'inc \$0' $TEMP_S; then
    pass "register char uses inc for +1"
else
    fail "register char uses inc for +1"
fi

# 10. register in for-loop init compiles and emits ZP
cat <<EOF > $TEMP_C
int main() {
    int sum = 0;
    for (register int i = 0; i < 10; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ] && grep -q "register _l_i" $TEMP_S; then
    pass "register in for-loop init"
else
    fail "register in for-loop init"
fi

# 11. non-register stack vars still use .var — use -O0
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    int a = 10;
    register int b = 20;
    int c = 30;
    return add(add(a, b), c);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q "\.var _l_a" $TEMP_S && grep -q "\.var _l_c" $TEMP_S; then
    pass "non-register vars still use .var"
else
    fail "non-register vars still use .var"
fi

# 12. register var reduces stack cleanup vs non-register
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    register int r = 1;
    int s = 2;
    return add(r, s);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
REG_PLA=$(grep -c "pla" $TEMP_S)
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    int r = 1;
    int s = 2;
    return add(r, s);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
NOREG_PLA=$(grep -c "pla" $TEMP_S)
if [ "$REG_PLA" -lt "$NOREG_PLA" ]; then
    pass "register var reduces stack cleanup ($REG_PLA vs $NOREG_PLA pla)"
else
    fail "register var reduces stack cleanup ($REG_PLA vs $NOREG_PLA pla)"
fi

# 13. multiple register vars get different ZP addresses
cat <<EOF > $TEMP_C
int main() {
    register int a = 1;
    register int b = 2;
    return a + b;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
ADDRS=$(grep "register _l_" $TEMP_S | grep -oP '\$[0-9A-Fa-f]+' | sort -u | wc -l)
if [ "$ADDRS" -eq 2 ]; then
    pass "multiple register vars get distinct ZP addresses"
else
    fail "multiple register vars get distinct ZP addresses (got $ADDRS)"
fi

# 14. register on array falls back to stack
cat <<EOF > $TEMP_C
int main() {
    register int arr[3];
    arr[0] = 42;
    return arr[0];
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ] && grep -q "\.var _l_arr" $TEMP_S; then
    pass "register on array falls back to stack"
else
    fail "register on array falls back to stack"
fi

# 15. register on struct falls back to stack
cat <<EOF > $TEMP_C
struct Point { int x; int y; };
int main() {
    register struct Point p;
    p.x = 1;
    return p.x;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ] && grep -q "\.var _l_p" $TEMP_S; then
    pass "register on struct falls back to stack"
else
    fail "register on struct falls back to stack"
fi

# 16. assembles successfully (full pipeline)
cat <<EOF > $TEMP_C
int main() {
    register int i = 0;
    register char c = 42;
    int sum = 0;
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + c;
    }
    return sum;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o test_register_tmp.bin 2>/dev/null
if [ $? -eq 0 ]; then
    pass "register program assembles to binary"
else
    fail "register program assembles to binary"
fi

# Clean up
rm -f $TEMP_C $TEMP_S test_register_tmp.bin

echo ""
echo "register tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
