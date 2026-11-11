#!/bin/bash
# ?: of a signed and a plain (unsigned in cc45) operand: the warning, only
# where the other operand is unsigned by cc45's default alone

CC="./bin/cc45 -O2"
DIR=$(mktemp -d)
trap 'rm -rf "$DIR"' EXIT
SRC="$DIR/t.c"

passed=0
failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "--- Test: ?: signed / plain operand warning ---"

cat > "$SRC" <<'END'
signed char sc;
unsigned int ui;
int pi;
const signed char dx[2] = { -1, 1 };
long w1(unsigned char c) { return c ? sc : 0; }
long w2(unsigned char c) { return c ? 0 : sc; }
long w3(unsigned char c) { return c ? sc : pi; }
long w4(unsigned char c, unsigned char d) { return c ? dx[d] : 0; }
long n1(unsigned char c) { return c ? sc : ui; }
long n2(unsigned char c) { return c ? sc : 0u; }
long n3(unsigned char c) { return c ? sc : (signed)0; }
long n4(unsigned char c) { return c ? -1 : 0; }
long n5(unsigned char c) { return c ? pi : 0; }
int main(void) { return 0; }
END

check() { # flags line expected(1 warn, 0 none) description
    local out; out=$($CC $1 -c "$SRC" -o "$DIR/t.o45" 2>&1)
    if echo "$out" | grep -q "t.c:$2:[0-9]*: warning: ?: of a signed and a plain"; then
        [ "$3" = 1 ] && pass "$4" || fail "$4 (warned)"
    else
        [ "$3" = 0 ] && pass "$4" || fail "$4 (no warning)"
    fi
}

check "" 5 1 "c ? signed : 0"
check "" 6 1 "c ? 0 : signed"
check "" 7 1 "c ? signed : plain int variable"
check "" 8 1 "c ? signed array element : 0"
check "" 9 0 "c ? signed : unsigned int variable"
check "" 10 0 "c ? signed : 0u"
check "" 11 0 "c ? signed : (signed)0"
check "" 12 0 "c ? -1 : 0 (a signed result)"
check "" 13 0 "c ? plain : 0 (no signed operand)"
check "-fsigned-default" 5 1 "-fsigned-default: c ? signed : 0 (the literal is still unsigned)"
check "-fsigned-default" 7 0 "-fsigned-default: c ? signed : int variable"

# the text says which operand, and once a place
out=$($CC -c "$SRC" -o "$DIR/t.o45" 2>&1)
echo "$out" | grep -q "t.c:5:.*the third operand makes the signed one unsigned" && pass "names the third operand" || fail "names the third operand"
echo "$out" | grep -q "t.c:6:.*the second operand makes the signed one unsigned" && pass "names the second operand" || fail "names the second operand"
[ "$(echo "$out" | grep -c "t.c:5:[0-9]*: warning: ?:")" = 1 ] && pass "said once" || fail "said once"

echo "Results: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
