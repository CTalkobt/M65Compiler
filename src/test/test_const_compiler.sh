#!/bin/bash
# Test const keyword parsing and validation - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "Const keyword validation"

TEMP_C="test_const.c"
TEMP_S="test_const.s"

# Assignment to const local (should fail)
cat <<EOF > $TEMP_C
int main() { const int x = 5; x = 10; return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "assignment to const local fails"

# Assignment to const global (should fail)
cat <<EOF > $TEMP_C
const int g_x = 5;
int main() { g_x = 10; return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "assignment to const global fails"

# Assignment to const struct member (should fail)
cat <<EOF > $TEMP_C
struct S { const int x; int y; };
int main() { struct S s; s.x = 10; return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "assignment to const struct member fails"

# Increment of const variable (should fail)
cat <<EOF > $TEMP_C
int main() { const int x = 5; x++; return 0; }
EOF
validate_error "$TEMP_C" "Increment/decrement of read-only location" "increment of const variable fails"

# Assignment to const parameter (should fail)
cat <<EOF > $TEMP_C
void foo(const int x) { x = 10; }
int main() { foo(5); return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "assignment to const parameter fails"

# Valid const usage (should pass)
cat <<EOF > $TEMP_C
const int g_const = 123;
int main() { const int l_const = 456; int x = g_const + l_const; return x; }
EOF
validate_no_error "$TEMP_C" "valid const usage compiles"

# Write through const pointer (should fail)
cat <<EOF > $TEMP_C
void readonly(const int *p) { *p = 10; }
int main() { int x = 5; readonly(&x); return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "write through const pointer fails"

# Casting away const (should compile)
cat <<EOF > $TEMP_C
int main() { const int x = 5; int *p = (int *)&x; *p = 10; return *p; }
EOF
validate_no_error "$TEMP_C" "casting away const compiles"

# Const parameter prevents modification (should fail)
cat <<EOF > $TEMP_C
int add(const int a, const int b) { a = 10; return a + b; }
int main() { return add(3, 4); }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "const parameter assignment fails"

# Const pointer parameter allows read (should compile)
cat <<EOF > $TEMP_C
int read_val(const int *p) { return *p; }
int main() { int x = 42; return read_val(&x); }
EOF
validate_no_error "$TEMP_C" "reading through const pointer compiles"

# Warning on passing const address to non-const pointer
cat <<EOF > $TEMP_C
void modify(int *p) { *p = 99; }
int main() { const int x = 5; modify(&x); return 0; }
EOF
validate_error "$TEMP_C" "discards 'const' qualifier" "passing const address to non-const pointer warns"

# const int *p allows reassigning pointer (should compile)
cat <<EOF > $TEMP_C
int main() { int a = 10; int b = 20; const int *p = &a; p = &b; return *p; }
EOF
validate_no_error "$TEMP_C" "const int *p allows pointer reassignment"

# int *const p prevents reassigning pointer (should fail)
cat <<EOF > $TEMP_C
int main() { int a = 10; int b = 20; int *const p = &a; p = &b; return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "int *const p prevents pointer reassignment"

# int *const p allows writing through pointer (should compile)
cat <<EOF > $TEMP_C
int main() { int a = 10; int *const p = &a; *p = 99; return *p; }
EOF
validate_no_error "$TEMP_C" "int *const p allows write through pointer"

# const int *const p prevents both (should fail)
cat <<EOF > $TEMP_C
int main() { int a = 10; const int *const p = &a; *p = 99; return 0; }
EOF
validate_error "$TEMP_C" "Assignment to read-only location" "const int *const p prevents write through pointer"

rm -f $TEMP_C $TEMP_S

test_summary
exit $?
