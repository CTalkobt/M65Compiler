#!/bin/bash
# Per-function local state must not leak from one top-level function to the next

CC="./bin/cc45 -O2"
TEMP_C="test_function_locals_tmp.c"
TEMP_S="test_function_locals_tmp.s45"
TEMP_E="test_function_locals_tmp.err"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "--- Test: function-local state isolation ---"

# A parameter of an earlier function must not shadow a global in a later one
cat > "$TEMP_C" <<'END'
int a = 7;
int f(int a) { return a + 1; }
int main(void) { return a + f(1); }
END
$CC -S -O1 "$TEMP_C" -o "$TEMP_S" 2>/dev/null
if awk '/proc _main/,/endproc/' "$TEMP_S" | grep -qE 'lda _a$'; then
    pass "global 'a' in main is not resolved to f's parameter"
else
    fail "global 'a' in main is not resolved to f's parameter"
fi

# Unused locals are reported even after a previous function used the same vreg numbers
cat > "$TEMP_C" <<'END'
int f(int a, int b) { return a; }
int main(void) { int unused1; return f(1, 2); }
END
$CC -S -O1 "$TEMP_C" -o "$TEMP_S" 2>"$TEMP_E"
if grep -q "warning: unused variable 'unused1'" "$TEMP_E"; then
    pass "unused local in later function is reported"
else
    fail "unused local in later function is reported"
fi
if grep -q ":1:18: warning: unused parameter 'b'" "$TEMP_E"; then
    pass "unused parameter reported at its name"
else
    fail "unused parameter reported at its name"
fi
if [ "$(grep -c "warning: unused" "$TEMP_E")" -eq 2 ]; then
    pass "each unused name reported once"
else
    fail "each unused name reported once"
fi

rm -f "$TEMP_C" "$TEMP_S" "$TEMP_E"

echo ""
echo "function locals tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
