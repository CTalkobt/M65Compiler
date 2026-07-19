#!/bin/bash
# Test for const keyword in compiler

CC="./bin/cc45"
TEMP_C="test_const.c"
TEMP_S="test_const.s45"

echo "Testing const keyword in compiler..."

# 1. Assignment to const local (should fail)
cat <<EOF > $TEMP_C
int main() {
    const int x = 5;
    x = 10;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Assignment to const local should fail"
    exit 1
fi
echo "PASSED: Assignment to const local fails"

# 2. Assignment to const global (should fail)
cat <<EOF > $TEMP_C
const int g_x = 5;
int main() {
    g_x = 10;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Assignment to const global should fail"
    exit 1
fi
echo "PASSED: Assignment to const global fails"

# 3. Assignment to const struct member (should fail)
cat <<EOF > $TEMP_C
struct S {
    const int x;
    int y;
};
int main() {
    struct S s;
    s.x = 10;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Assignment to const struct member should fail"
    exit 1
fi
echo "PASSED: Assignment to const struct member fails"

# 4. Increment of const variable (should fail)
cat <<EOF > $TEMP_C
int main() {
    const int x = 5;
    x++;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Increment/decrement of read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Increment of const variable should fail"
    exit 1
fi
echo "PASSED: Increment of const variable fails"

# 5. Assignment to const parameter (should fail)
cat <<EOF > $TEMP_C
void foo(const int x) {
    x = 10;
}
int main() {
    foo(5);
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Assignment to const parameter should fail"
    exit 1
fi
echo "PASSED: Assignment to const parameter fails"

# 6. Valid const usage (should pass)
cat <<EOF > $TEMP_C
const int g_const = 123;
int main() {
    const int l_const = 456;
    int x = g_const + l_const;
    return x;
}
EOF

$CC $TEMP_C -o $TEMP_S
if [ $? -ne 0 ]; then
    echo "FAILED: Valid const usage failed to compile"
    exit 1
fi
echo "PASSED: Valid const usage compiles"

# 7. Write through const pointer (should fail)
cat <<EOF > $TEMP_C
void readonly(const int *p) {
    *p = 10;
}
int main() {
    int x = 5;
    readonly(&x);
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Write through const pointer should fail"
    exit 1
fi
echo "PASSED: Write through const pointer fails"

# 8. Casting away const (should compile)
cat <<EOF > $TEMP_C
int main() {
    const int x = 5;
    int *p = (int *)&x;
    *p = 10;
    return *p;
}
EOF

$CC $TEMP_C -o $TEMP_S
if [ $? -ne 0 ]; then
    echo "FAILED: Casting away const should compile"
    exit 1
fi
echo "PASSED: Casting away const compiles"

# 9. Const parameter prevents modification (should fail)
cat <<EOF > $TEMP_C
int add(const int a, const int b) {
    a = 10;
    return a + b;
}
int main() { return add(3, 4); }
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Const parameter assignment should fail"
    exit 1
fi
echo "PASSED: Const parameter assignment fails"

# 10. Const pointer parameter allows read (should compile)
cat <<EOF > $TEMP_C
int read_val(const int *p) {
    return *p;
}
int main() {
    int x = 42;
    return read_val(&x);
}
EOF

$CC $TEMP_C -o $TEMP_S
if [ $? -ne 0 ]; then
    echo "FAILED: Reading through const pointer should compile"
    exit 1
fi
echo "PASSED: Reading through const pointer compiles"

# 11. Warning on passing const address to non-const pointer
cat <<EOF > $TEMP_C
void modify(int *p) {
    *p = 99;
}
int main() {
    const int x = 5;
    modify(&x);
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "discards 'const' qualifier" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: Passing const address to non-const pointer should warn"
    exit 1
fi
echo "PASSED: Passing const address to non-const pointer warns"

# 12. const int *p allows reassigning pointer (should compile)
cat <<EOF > $TEMP_C
int main() {
    int a = 10;
    int b = 20;
    const int *p = &a;
    p = &b;
    return *p;
}
EOF

$CC $TEMP_C -o $TEMP_S
if [ $? -ne 0 ]; then
    echo "FAILED: const int *p should allow p = &b"
    exit 1
fi
echo "PASSED: const int *p allows pointer reassignment"

# 13. int *const p prevents reassigning pointer (should fail)
cat <<EOF > $TEMP_C
int main() {
    int a = 10;
    int b = 20;
    int *const p = &a;
    p = &b;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: int *const p should prevent p = &b"
    exit 1
fi
echo "PASSED: int *const p prevents pointer reassignment"

# 14. int *const p allows writing through pointer (should compile)
cat <<EOF > $TEMP_C
int main() {
    int a = 10;
    int *const p = &a;
    *p = 99;
    return *p;
}
EOF

$CC $TEMP_C -o $TEMP_S
if [ $? -ne 0 ]; then
    echo "FAILED: int *const p should allow *p = 99"
    exit 1
fi
echo "PASSED: int *const p allows write through pointer"

# 15. const int *const p prevents both (should fail)
cat <<EOF > $TEMP_C
int main() {
    int a = 10;
    const int *const p = &a;
    *p = 99;
    return 0;
}
EOF

$CC $TEMP_C -o $TEMP_S 2>&1 | grep "Compile Error: Assignment to read-only location" > /dev/null
if [ $? -ne 0 ]; then
    echo "FAILED: const int *const p should prevent *p = 99"
    exit 1
fi
echo "PASSED: const int *const p prevents write through pointer"

# Clean up
rm -f $TEMP_C $TEMP_S
echo "All compiler const tests passed!"
