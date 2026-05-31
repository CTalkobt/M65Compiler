#!/bin/bash
# Test C99 restrict qualifier parsing and codegen - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "C99 restrict qualifier"

TEMP_C="test_restrict_tmp.c"
TEMP_S="test_restrict_tmp.s"

# restrict on pointer parameter
cat <<EOF > $TEMP_C
void f(int * restrict p) { *p = 1; }
int main(void) { int x = 0; f(&x); return x; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_1" && print_pass "restrict on pointer parameter" || print_fail "restrict on pointer parameter"

# __restrict variant
cat <<EOF > $TEMP_C
void f(int * __restrict p) { *p = 2; }
int main(void) { int x = 0; f(&x); return x; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_2" && print_pass "__restrict on pointer parameter" || print_fail "__restrict on pointer parameter"

# __restrict__ variant
cat <<EOF > $TEMP_C
void f(int * __restrict__ p) { *p = 3; }
int main(void) { int x = 0; f(&x); return x; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_3" && print_pass "__restrict__ on pointer parameter" || print_fail "__restrict__ on pointer parameter"

# restrict before type
cat <<EOF > $TEMP_C
void f(restrict int *p) { *p = 4; }
int main(void) { int x = 0; f(&x); return x; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_4" && print_pass "restrict before type specifier" || print_fail "restrict before type specifier"

# restrict with const
cat <<EOF > $TEMP_C
void f(const int * restrict p) { int x = *p; }
int main(void) { int x = 5; f(&x); return 0; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_5" && print_pass "restrict combined with const" || print_fail "restrict combined with const"

# restrict on local pointer
cat <<EOF > $TEMP_C
int main(void) { int val = 42; int * restrict p = &val; return *p; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_6" && print_pass "restrict on local pointer variable" || print_fail "restrict on local pointer variable"

# multiple restrict parameters
cat <<EOF > $TEMP_C
void add(int * restrict dst, const int * restrict a, const int * restrict b) { *dst = *a + *b; }
int main(void) { int x = 0; int a = 3; int b = 4; add(&x, &a, &b); return x; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_7" && print_pass "multiple restrict parameters" || print_fail "multiple restrict parameters"

# restrict in for-loop
cat <<EOF > $TEMP_C
int main(void) { int arr = 10; for (int * restrict p = &arr; *p > 0; *p = *p - 1) { } return arr; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_8" && print_pass "restrict in for-loop declaration" || print_fail "restrict in for-loop declaration"

# restrict with struct pointer
cat <<EOF > $TEMP_C
struct Point { int x; int y; };
void set(struct Point * restrict p) { p->x = 1; p->y = 2; }
int main(void) { struct Point pt; set(&pt); return pt.x + pt.y; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_9" && print_pass "restrict with struct pointer" || print_fail "restrict with struct pointer"

# restrict with volatile
cat <<EOF > $TEMP_C
void f(volatile int * restrict p) { int x = *p; }
int main(void) { volatile int x = 5; f(&x); return 0; }
EOF
compile_and_assemble "$TEMP_C" "build/test/test_restrict_10" && print_pass "restrict with volatile" || print_fail "restrict with volatile"

rm -f $TEMP_C $TEMP_S

test_summary
exit $?
