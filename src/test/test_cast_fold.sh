#!/bin/bash
# Test for constant folder cast type preservation — validates that folding
# CastExpression to IntegerLiteral preserves type info (long, pointer, etc.)

# Cast fold tests
CC="./bin/cc45"
TEMP_C="test_cast_fold_tmp.c"
TEMP_S="test_cast_fold_tmp.s45"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "--- Test: constant folder cast type preservation ---"

# 1. (long)42 return with -O1 -fzpcall must emit ldy and ldz
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S; then
    pass "(long)42 return emits ldy+ldz with -O1 -fzpcall"
else
    fail "(long)42 return emits ldy+ldz with -O1 -fzpcall"
fi

# 2. (long)0 return with -O1 -fzpcall must emit ldy and ldz
cat <<EOF > $TEMP_C
long get_long_zero(void) { return (long)0; }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S; then
    pass "(long)0 return emits ldy+ldz with -O1 -fzpcall"
else
    fail "(long)0 return emits ldy+ldz with -O1 -fzpcall"
fi

# 3. -O0 and -O1 produce same instructions for (long)42 return
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -S -O0 -fzpcall $TEMP_C -o ${TEMP_S}.o0 2>/dev/null
$CC -S -O1 -fzpcall $TEMP_C -o ${TEMP_S}.o1 2>/dev/null
O0_LDY=$(grep -c 'ldy' ${TEMP_S}.o0)
O1_LDY=$(grep -c 'ldy' ${TEMP_S}.o1)
O0_LDZ=$(grep -c 'ldz' ${TEMP_S}.o0)
O1_LDZ=$(grep -c 'ldz' ${TEMP_S}.o1)
if [ "$O0_LDY" -eq "$O1_LDY" ] && [ "$O0_LDZ" -eq "$O1_LDZ" ]; then
    pass "-O0 and -O1 emit same ldy/ldz count for (long)42"
else
    fail "-O0 and -O1 emit same ldy/ldz count for (long)42 (O0: $O0_LDY/$O0_LDZ, O1: $O1_LDY/$O1_LDZ)"
fi
rm -f ${TEMP_S}.o0 ${TEMP_S}.o1

# 4. (long)(10 + 20) folds to literal AND preserves long type
cat <<EOF > $TEMP_C
long get_long_expr(void) { return (long)(10 + 20); }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S; then
    pass "(long)(10+20) folded return emits ldy+ldz"
else
    fail "(long)(10+20) folded return emits ldy+ldz"
fi

# 5. (long)42 as function argument with -O1 stores 4 bytes to ZP
cat <<EOF > $TEMP_C
void use_long(long x);
void test(void) { use_long((long)42); }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
# Should store all 4 bytes to ZP param block ($10-$13)
if grep -q '\$12' $TEMP_S && grep -q '\$13' $TEMP_S; then
    pass "(long)42 argument stores all 4 bytes to ZP"
else
    fail "(long)42 argument stores all 4 bytes to ZP"
fi

# 6. (long)42 return with stack convention also emits ldy+ldz
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -S -O1 $TEMP_C -o $TEMP_S 2>/dev/null
if grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S; then
    pass "(long)42 return emits ldy+ldz with -O1 stack convention"
else
    fail "(long)42 return emits ldy+ldz with -O1 stack convention"
fi

# 7. (long)-1 folds correctly to 0xFFFFFFFF (all 4 bytes)
cat <<EOF > $TEMP_C
long get_neg1(void) { return (long)-1; }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
# IR pipeline emits decimal (#255) not hex (#$FF); check all 4 bytes are $FF/255
if grep -q '#255' $TEMP_S && grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S; then
    pass "(long)-1 folds to all-FF bytes"
else
    fail "(long)-1 folds to all-FF bytes"
fi

# 8. Plain int return does NOT emit ldy/ldz (no false positives)
cat <<EOF > $TEMP_C
int get_int42(void) { return (int)42; }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
if ! grep -q 'ldy' $TEMP_S && ! grep -q 'ldz' $TEMP_S; then
    pass "(int)42 return does NOT emit ldy/ldz"
else
    fail "(int)42 return does NOT emit ldy/ldz"
fi

# 9. Char cast folds and truncates correctly
cat <<EOF > $TEMP_C
char get_char(void) { return (char)0x1FF; }
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
# Should fold 0x1FF to 0xFF (char truncation), emit lda #255 or lda #$FF
if grep -q '#255' $TEMP_S || grep -q '#\$FF' $TEMP_S; then
    pass "(char)0x1FF folds to 0xFF"
else
    fail "(char)0x1FF folds to 0xFF"
fi

# 10. Long local propagated through variable still passes 4 bytes
cat <<EOF > $TEMP_C
void use_long(long x);
void test(void) {
    long x = (long)42;
    use_long(x);
}
EOF
$CC -S -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
# Optimizer may propagate constant directly; either way, all 4 ZP param bytes must be stored
if grep -q '\$12' $TEMP_S && grep -q '\$13' $TEMP_S; then
    pass "long propagated through variable stores 4 bytes to ZP"
else
    fail "long propagated through variable stores 4 bytes to ZP"
fi

# Cleanup
rm -f $TEMP_C $TEMP_S

echo ""
echo "cast fold tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
