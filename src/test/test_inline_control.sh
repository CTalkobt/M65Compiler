#!/bin/bash
# Inline control: -fmax-inline-ir-size, #pragma cc45 max_inline_ir_size,
# __attribute__((always_inline)), __forceinline, __attribute__((noinline))

CC="./bin/cc45 -O2"
TEMP_C="test_inline_control_tmp.c"
TEMP_S="test_inline_control_tmp.s45"
TEMP_E="test_inline_control_tmp.err"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# Number of calls emitted to function $1 (direct or SAC entry)
calls() { grep -cE "jsr _$1(__[A-Za-z0-9_]+)?\$" "$TEMP_S"; }  # static functions carry a __<module> suffix
check() { # function expected_calls description
    local n; n=$(calls "$1")
    if [ "$n" -eq "$2" ]; then pass "$3"; else fail "$3 (got $n calls)"; fi
}

BIG='{ if (c == 1) return 5; if (c == 2) return 7; if (c > 9) return 9; return c + 3; }'

echo "--- Test: inline control ---"

# IR size limit: CLI flag and pragma
cat > "$TEMP_C" <<END
volatile char *r = (char *)0xC010;
static inline int big(int c) $BIG
static inline int small(int c) { return c; }
void main() { r[0] = big(r[9]); r[1] = small(r[9]); }
END
$CC -S -O1 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
check big 1 "inline function over default IR limit is called"
check small 0 "inline function under default IR limit is inlined"
$CC -S -O1 -fmax-inline-ir-size=100 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
check big 0 "-fmax-inline-ir-size=100 inlines larger function"
sed -i '1i #pragma cc45 max_inline_ir_size(100)' "$TEMP_C"
$CC -S -O1 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
check big 0 "#pragma cc45 max_inline_ir_size(100) inlines larger function"

# Forced / suppressed inlining
cat > "$TEMP_C" <<END
volatile char *r = (char *)0xC010;
__attribute__((always_inline)) static int a_attr(int c) $BIG
static __forceinline int a_force(int c) $BIG
static int a_proto(int c) __attribute__((always_inline));
static int a_proto(int c) $BIG
__attribute__((noinline)) static inline int n_small(int c) { return c; }
__attribute__((always_inline, noinline)) static int both(int c) { return c; }
void main() {
    r[0] = a_attr(r[9]); r[1] = a_force(r[9]); r[2] = a_proto(r[9]);
    r[3] = n_small(r[9]); r[4] = both(r[9]);
}
END
$CC -S -O1 -fmax-inline-ir-size=1 "$TEMP_C" -o "$TEMP_S" 2>"$TEMP_E"
check a_attr 0 "always_inline bypasses IR limit"
check a_force 0 "__forceinline bypasses IR limit"
check a_proto 0 "always_inline on prototype applies to definition"
check n_small 1 "noinline suppresses inlining of inline function"
check both 1 "noinline wins over always_inline"
if grep -q "both always_inline and noinline" "$TEMP_E"; then
    pass "conflicting attributes warn"
else
    fail "conflicting attributes warn"
fi

# Recursive forced-inline function falls back to a normal call with a warning
cat > "$TEMP_C" <<END
volatile char *r = (char *)0xC010;
#pragma cc45 recurse
__forceinline int fact(int n) { if (n < 2) return 1; return n * fact(n - 1); }
void main() { r[0] = fact(r[9]); }
END
$CC -S -O1 "$TEMP_C" -o "$TEMP_S" 2>"$TEMP_E"
check fact 2 "recursive always_inline function is called normally (main + self)"
if [ "$(grep -c "always_inline function 'fact' is recursive" "$TEMP_E")" -eq 1 ]; then
    pass "recursive always_inline warns once"
else
    fail "recursive always_inline warns once"
fi

rm -f "$TEMP_C" "$TEMP_S" "$TEMP_E"

echo ""
echo "inline control tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
