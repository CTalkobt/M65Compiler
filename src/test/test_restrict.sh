#!/bin/bash
# Test for C99 restrict qualifier in compiler

CC="./bin/cc45"
AS="./bin/ca45"
TEMP_C="test_restrict_tmp.c"
TEMP_S="test_restrict_tmp.s45"

passed=0
failed=0

pass() { echo "PASSED: $1"; passed=$((passed + 1)); }
fail() { echo "FAILED: $1"; failed=$((failed + 1)); }

# 1. restrict on pointer parameter
cat <<EOF > $TEMP_C
void f(int * restrict p) { *p = 1; }
int main(void) { int x = 0; f(&x); return x; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict on pointer parameter" || fail "restrict on pointer parameter"

# 2. __restrict on pointer parameter
cat <<EOF > $TEMP_C
void f(int * __restrict p) { *p = 2; }
int main(void) { int x = 0; f(&x); return x; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "__restrict on pointer parameter" || fail "__restrict on pointer parameter"

# 3. __restrict__ on pointer parameter
cat <<EOF > $TEMP_C
void f(int * __restrict__ p) { *p = 3; }
int main(void) { int x = 0; f(&x); return x; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "__restrict__ on pointer parameter" || fail "__restrict__ on pointer parameter"

# 4. restrict before type specifier
cat <<EOF > $TEMP_C
void f(restrict int *p) { *p = 4; }
int main(void) { int x = 0; f(&x); return x; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict before type specifier" || fail "restrict before type specifier"

# 5. restrict combined with const
cat <<EOF > $TEMP_C
void f(const int * restrict p) { int x = *p; }
int main(void) { int x = 5; f(&x); return 0; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict combined with const" || fail "restrict combined with const"

# 6. restrict on local pointer variable
cat <<EOF > $TEMP_C
int main(void) {
    int val = 42;
    int * restrict p = &val;
    return *p;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict on local pointer variable" || fail "restrict on local pointer variable"

# 7. Multiple restrict parameters
cat <<EOF > $TEMP_C
void add(int * restrict dst, const int * restrict a, const int * restrict b) {
    *dst = *a + *b;
}
int main(void) {
    int x = 0;
    int a = 3;
    int b = 4;
    add(&x, &a, &b);
    return x;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "multiple restrict parameters" || fail "multiple restrict parameters"

# 8. restrict in for-loop declaration
cat <<EOF > $TEMP_C
int main(void) {
    int arr = 10;
    for (int * restrict p = &arr; *p > 0; *p = *p - 1) { }
    return arr;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict in for-loop declaration" || fail "restrict in for-loop declaration"

# 9. restrict with struct pointer
cat <<EOF > $TEMP_C
struct Point { int x; int y; };
void set(struct Point * restrict p) { p->x = 1; p->y = 2; }
int main(void) {
    struct Point pt;
    set(&pt);
    return pt.x + pt.y;
}
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict with struct pointer" || fail "restrict with struct pointer"

# 10. restrict with volatile
cat <<EOF > $TEMP_C
void f(volatile int * restrict p) { int x = *p; }
int main(void) { volatile int x = 5; f(&x); return 0; }
EOF
$CC $TEMP_C -o $TEMP_S 2>/dev/null && $AS $TEMP_S -o /dev/null 2>/dev/null \
    && pass "restrict with volatile" || fail "restrict with volatile"

# Clean up
rm -f $TEMP_C $TEMP_S

echo ""
echo "restrict tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
