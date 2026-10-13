#!/bin/bash
# cc45 integer signedness: char, int and long are unsigned unless declared signed.
# The generated modulo instruction (mod.16 vs mod.s16) shows which rule was applied.

CC="./bin/cc45 -O2"
T="test_signedness_tmp"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# expect_mod "<C source>" "<expected mod.* mnemonic>"
expect_mod() {
    printf '%s\n' "$1" > $T.c
    $CC -S $FLAGS $T.c -o $T.s45 >/dev/null 2>&1
    local got; got=$(grep -oE 'mod\.[a-z0-9]+' $T.s45 | tail -1)
    if [ "$got" = "$2" ]; then pass "$FLAGS $1"; else fail "$FLAGS $1 (got '$got', expected $2)"; fi
}

echo "--- Test: integer signedness defaults ---"

expect_mod 'int v = 5; int f(void) { return v % 14; }'                           mod.16
expect_mod 'int v = 5, w = 1; int f(void) { return w % 14; }'                    mod.16
expect_mod 'signed int v = 5; int f(void) { return v % 14; }'                    mod.s16
expect_mod 'signed int v = 5, w = 1; int f(void) { return w % 14; }'             mod.s16
expect_mod 'char v = 5; int f(void) { return v % 14; }'                          mod.16
expect_mod 'signed char v = 5; int f(void) { return v % 14; }'                   mod.s16
expect_mod 'long v = 5; int f(void) { return v % 14; }'                          mod.32
expect_mod 'signed long v = 5; int f(void) { return v % 14; }'                   mod.s32
expect_mod 'static v = 5; int f(void) { return v % 14; }'                        mod.16
expect_mod 'int f(void) { int v = 5; return v % 14; }'                           mod.16
expect_mod 'int f(signed int v) { return v % 14; }'                              mod.s16
expect_mod 'struct S { int m; }; struct S s; int f(void) { return s.m % 14; }'   mod.16
expect_mod 'typedef signed int T; T v = 5; int f(void) { return v % 14; }'       mod.s16
expect_mod 'signed int g(void); int f(void) { return g() % 14; }'                mod.s16
expect_mod 'int g(void); int f(void) { return g() % 14; }'                       mod.16
expect_mod 'int *p; int f(void) { return *p % 14; }'                             mod.16
expect_mod 'signed int *p; int f(void) { return *p % 14; }'                      mod.s16

echo "--- Test: -fsigned-default (standard C signedness) ---"
FLAGS=-fsigned-default
expect_mod 'int v = 5; int f(void) { return v % 14; }'                           mod.s16
expect_mod 'int v = 5, w = 1; int f(void) { return w % 14; }'                    mod.s16
expect_mod 'unsigned int v = 5; int f(void) { return v % 14; }'                  mod.16
expect_mod 'char v = 5; int f(void) { return v % 14; }'                          mod.s16
expect_mod 'long v = 5; int f(void) { return v % 14; }'                          mod.s32
expect_mod 'static v = 5; int f(void) { return v % 14; }'                        mod.s16
expect_mod 'int f(int v) { return v % 14; }'                                     mod.s16
expect_mod 'int g(void); int f(void) { return g() % 14; }'                       mod.s16
expect_mod 'unsigned int g(void); int f(void) { return g() % 14; }'              mod.16
expect_mod 'typedef unsigned int U; U v = 5; int f(void) { return v % 14; }'     mod.16
FLAGS=

echo "--- Test: #pragma cc45 signed_default / unsigned_default ---"
expect_mod '#pragma cc45 signed_default
int v = 5; int f(void) { return v % 14; }'                                        mod.s16
expect_mod '#pragma cc45 signed_default
#pragma cc45 unsigned_default
int v = 5; int f(void) { return v % 14; }'                                        mod.16

# x < 0 on an unsigned value is always false and warns
warns() {
    printf '%s\n' "$1" > $T.c
    $CC -S $T.c -o $T.s45 2>&1 >/dev/null | grep -q "unsigned expression < 0 is always false"
}
warns 'int f(int x) { return x < 0; }'        && pass "warn: unsigned x < 0"     || fail "warn: unsigned x < 0"
warns 'int g; int f(void) { return 0 > g; }'  && pass "warn: 0 > unsigned g"     || fail "warn: 0 > unsigned g"
warns 'int f(signed int x) { return x < 0; }' && fail "no warning for signed x < 0" || pass "no warning for signed x < 0"
warns 'int f(int x) { return x < 1; }'        && fail "no warning for x < 1"     || pass "no warning for x < 1"

rm -f $T.c $T.s45

echo ""
echo "signedness tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
