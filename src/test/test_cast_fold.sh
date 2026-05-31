#!/bin/bash
# Test constant folder cast type preservation - uses shared test utilities

source "$(dirname "$0")/test-lib.sh"

print_section "Constant folder cast type preservation"

TEMP_C="test_cast_fold_tmp.c"
TEMP_S="test_cast_fold_tmp.s"

# (long)42 return with -O1 -fzpcall must emit ldy and ldz
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S && print_pass "(long)42 return emits ldy+ldz with -O1 -fzpcall" || print_fail "(long)42 return emits ldy+ldz with -O1 -fzpcall"

# (long)0 return with -O1 -fzpcall
cat <<EOF > $TEMP_C
long get_long_zero(void) { return (long)0; }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S && print_pass "(long)0 return emits ldy+ldz with -O1 -fzpcall" || print_fail "(long)0 return emits ldy+ldz with -O1 -fzpcall"

# -O0 and -O1 produce same instruction count
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -O0 -fzpcall $TEMP_C -o ${TEMP_S}.o0 2>/dev/null
$CC -O1 -fzpcall $TEMP_C -o ${TEMP_S}.o1 2>/dev/null
O0_LDY=$(grep -c 'ldy' ${TEMP_S}.o0 || echo 0)
O1_LDY=$(grep -c 'ldy' ${TEMP_S}.o1 || echo 0)
O0_LDZ=$(grep -c 'ldz' ${TEMP_S}.o0 || echo 0)
O1_LDZ=$(grep -c 'ldz' ${TEMP_S}.o1 || echo 0)
[ "$O0_LDY" -eq "$O1_LDY" ] && [ "$O0_LDZ" -eq "$O1_LDZ" ] && print_pass "-O0 and -O1 emit same ldy/ldz count for (long)42" || print_fail "-O0 and -O1 emit same ldy/ldz count for (long)42"
rm -f ${TEMP_S}.o0 ${TEMP_S}.o1

# (long)(10+20) folded return preserves long type
cat <<EOF > $TEMP_C
long get_long_expr(void) { return (long)(10 + 20); }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S && print_pass "(long)(10+20) folded return emits ldy+ldz" || print_fail "(long)(10+20) folded return emits ldy+ldz"

# (long)42 argument stores 4 bytes to ZP
cat <<EOF > $TEMP_C
void use_long(long x);
void test(void) { use_long((long)42); }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q '\$12' $TEMP_S && grep -q '\$13' $TEMP_S && print_pass "(long)42 argument stores all 4 bytes to ZP" || print_fail "(long)42 argument stores all 4 bytes to ZP"

# (long)42 return with stack convention
cat <<EOF > $TEMP_C
long get_long42(void) { return (long)42; }
EOF
$CC -O1 $TEMP_C -o $TEMP_S 2>/dev/null
grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S && print_pass "(long)42 return emits ldy+ldz with -O1 stack convention" || print_fail "(long)42 return emits ldy+ldz with -O1 stack convention"

# (long)-1 folds to all-FF bytes
cat <<EOF > $TEMP_C
long get_neg1(void) { return (long)-1; }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q '#255' $TEMP_S && grep -q 'ldy' $TEMP_S && grep -q 'ldz' $TEMP_S && print_pass "(long)-1 folds to all-FF bytes" || print_fail "(long)-1 folds to all-FF bytes"

# (int)42 return does NOT emit ldy/ldz
cat <<EOF > $TEMP_C
int get_int42(void) { return (int)42; }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
! grep -q 'ldy' $TEMP_S && ! grep -q 'ldz' $TEMP_S && print_pass "(int)42 return does NOT emit ldy/ldz" || print_fail "(int)42 return does NOT emit ldy/ldz"

# (char)0x1FF folds to 0xFF
cat <<EOF > $TEMP_C
char get_char(void) { return (char)0x1FF; }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
(grep -q '#255' $TEMP_S || grep -q '#\$FF' $TEMP_S) && print_pass "(char)0x1FF folds to 0xFF" || print_fail "(char)0x1FF folds to 0xFF"

# Long propagated through variable stores 4 bytes
cat <<EOF > $TEMP_C
void use_long(long x);
void test(void) { long x = (long)42; use_long(x); }
EOF
$CC -O1 -fzpcall $TEMP_C -o $TEMP_S 2>/dev/null
grep -q '\$12' $TEMP_S && grep -q '\$13' $TEMP_S && print_pass "long propagated through variable stores 4 bytes to ZP" || print_fail "long propagated through variable stores 4 bytes to ZP"

rm -f $TEMP_C $TEMP_S

test_summary
exit $?
