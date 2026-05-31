#!/bin/bash
# Test register keyword parsing, ZP allocation, and codegen - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "Register keyword parsing and codegen"

TEMP_C="test_register_tmp.c"
TEMP_S="test_register_tmp.s"

# register int compiles successfully
cat <<EOF > $TEMP_C
int main() { register int i = 0; return i; }
EOF
compile_c "$TEMP_C" "$TEMP_S" && print_pass "register int compiles" || print_fail "register int compiles"

# register char compiles successfully
cat <<EOF > $TEMP_C
int main() { register char c = 42; return c; }
EOF
compile_c "$TEMP_C" "$TEMP_S" && print_pass "register char compiles" || print_fail "register char compiles"

# register int uses ZP addressing
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() { register int i = 42; i = i + 1; return add(i, 0); }
EOF
$CC -O0 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
grep -q 'sta \$[0-2][0-9a-fA-F]' "$TEMP_S" && print_pass "register int uses ZP store" || print_fail "register int uses ZP store"

# register int allocated in ZP
cat <<EOF > $TEMP_C
int main() { register int i = 10; return i; }
EOF
$CC "$TEMP_C" -o "$TEMP_S" 2>/dev/null
grep -q '\.var.*_l_i.*= \$' "$TEMP_S" && print_pass "register int allocated in ZP" || print_fail "register int allocated in ZP"

# register in for-loop init
cat <<EOF > $TEMP_C
int main() { int sum = 0; for (register int i = 0; i < 10; i = i + 1) { sum = sum + i; } return sum; }
EOF
compile_c "$TEMP_C" "$TEMP_S" && print_pass "register in for-loop init" || print_fail "register in for-loop init"

# non-register vars use frame allocation
cat <<EOF > $TEMP_C
int add(int a, int b);
int main() { int a = 10; register int b = 20; int c = 30; return add(add(a, b), c); }
EOF
$CC -O0 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
grep -q '\.local.*__vr\|\.var' "$TEMP_S" && print_pass "non-register vars use frame allocation" || print_fail "non-register vars use frame allocation"

# register on array (fallback to stack)
cat <<EOF > $TEMP_C
int main() { register int arr[3]; arr[0] = 42; return arr[0]; }
EOF
compile_c "$TEMP_C" "$TEMP_S" && print_pass "register on array compiles (fallback to stack)" || print_fail "register on array compiles (fallback to stack)"

# register on struct (fallback to stack)
cat <<EOF > $TEMP_C
struct Point { int x; int y; };
int main() { register struct Point p; p.x = 1; return p.x; }
EOF
compile_c "$TEMP_C" "$TEMP_S" && print_pass "register on struct compiles (fallback to stack)" || print_fail "register on struct compiles (fallback to stack)"

# Full pipeline assembles to binary
cat <<EOF > $TEMP_C
int main() { register int i = 0; register char c = 42; int sum = 0; for (i = 0; i < 5; i = i + 1) { sum = sum + c; } return sum; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_register" && print_pass "register program assembles to binary" || print_fail "register program assembles to binary"

rm -f $TEMP_C $TEMP_S

test_summary
exit $?
