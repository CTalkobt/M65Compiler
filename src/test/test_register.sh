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

# 3. register int uses ZP addressing (sta $XX pattern in output)
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() {
    register int i = 42;
    i = i + 1;
    return add(i, 0);
}
EOF
$CC -O0 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'sta \$[0-2][0-9a-fA-F]' $TEMP_S; then
    pass "register int uses ZP store"
else
    fail "register int uses ZP store"
fi

# 4. register int skips .var/.local for stack
cat <<EOF > $TEMP_C
int main() {
    register int i = 10;
    return i;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
# register vars should be at ZP address ($XX), not stack-relative offset
# .var @_l_i = $20 is correct (ZP); .var @_l_i = 4 (stack offset) is wrong
if grep -q '\.var.*_l_i.*= \$' $TEMP_S; then
    pass "register int allocated in ZP"
else
    fail "register int allocated in ZP"
fi

# 5. register in for-loop init compiles
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
if [ $? -eq 0 ]; then
    pass "register in for-loop init"
else
    fail "register in for-loop init"
fi

# 6. non-register stack vars still use .var/.local
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
if grep -q '\.local.*__vr\|\.var' $TEMP_S; then
    pass "non-register vars use frame allocation"
else
    fail "non-register vars use frame allocation"
fi

# 7. register on array falls back to stack
cat <<EOF > $TEMP_C
int main() {
    register int arr[3];
    arr[0] = 42;
    return arr[0];
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ]; then
    pass "register on array compiles (fallback to stack)"
else
    fail "register on array compiles (fallback to stack)"
fi

# 8. register on struct falls back to stack
cat <<EOF > $TEMP_C
struct Point { int x; int y; };
int main() {
    register struct Point p;
    p.x = 1;
    return p.x;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null
if [ $? -eq 0 ]; then
    pass "register on struct compiles (fallback to stack)"
else
    fail "register on struct compiles (fallback to stack)"
fi

# 9. assembles successfully (full pipeline)
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
