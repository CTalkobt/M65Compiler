#!/bin/bash
# String/char escapes and encodings in ca45 and cc45 (\n is the PETSCII newline $0D)

CC="./bin/cc45 -O2"
AS="./bin/ca45"
T="test_string_escapes_tmp"

passed=0
failed=0

pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# assemble "<source line>" "<expected hex bytes>" "<description>"
asm_bytes() {
    printf '* = $2000\n%s\n' "$1" > $T.s45
    if ! $AS $T.s45 -o $T.bin >/dev/null 2>&1; then fail "$3 (assembly failed)"; return; fi
    local got; got=$(xxd -p $T.bin | tr -d '\n')
    if [ "$got" = "$2" ]; then pass "$3"; else fail "$3 (got $got, expected $2)"; fi
}

echo "--- Test: string escapes and encodings ---"

asm_bytes '    .text "say \"hi\""'      "5341592022484922" ".text with escaped quotes"
asm_bytes '    .text "a\nb\r"'          "410d420a"           ".text \\n is \$0D, \\r is \$0A"
asm_bytes '    .ascii "a\nb\r"'         "610a620d"           ".ascii keeps C newline values"
asm_bytes '    .text "\x41\101A\\"'     "4141615c"           ".text \\x and octal escapes are exact"
asm_bytes '    .text s"hi\n"'           "08090d"             "s\"...\" screen codes with escape"
asm_bytes '    .text "second"'          "5345434f4e44"       "string starting with s is not a prefix"
asm_bytes "    .byte 'A', s'A', p'a', 65" "61014141"         ".byte accepts plain and prefixed chars"

printf '* = $2000\n    .text "say "hi""\n' > $T.s45
if $AS $T.s45 -o $T.bin 2>&1 | grep -q "single string argument"; then
    pass "unescaped quote in .text is an error"
else
    fail "unescaped quote in .text is an error"
fi

# cc45: char literals and string emission
cat > $T.c <<'END'
char c_n(void) { return '\n'; }
char c_r(void) { return '\r'; }
char c_x(void) { return '\x41'; }
char c_o(void) { return '\101'; }
char *s(void) { return "q\"\\\n\x01"; }
void a(void) { asm("lda #1\n    sta $d020"); }
END
$CC -O1 -S $T.c -o $T.s45 2>/dev/null
lda_of() { awk "/proc _$1\$/,/endproc/" $T.s45 | grep -m1 -oE 'lda #[0-9]+'; }
[ "$(lda_of c_n)" = "lda #13" ] && pass "'\\n' is 13" || fail "'\\n' is 13 (got $(lda_of c_n))"
[ "$(lda_of c_r)" = "lda #10" ] && pass "'\\r' is 10" || fail "'\\r' is 10 (got $(lda_of c_r))"
[ "$(lda_of c_x)" = "lda #65" ] && pass "'\\x41' is exact" || fail "'\\x41' is exact (got $(lda_of c_x))"
[ "$(lda_of c_o)" = "lda #65" ] && pass "'\\101' is exact" || fail "'\\101' is exact (got $(lda_of c_o))"
if grep -qF '.text "q\042\134\n\001"' $T.s45; then
    pass "string emitted as one escaped literal"
else
    fail "string emitted as one escaped literal ($(grep -m1 '\.text' $T.s45))"
fi
if awk '/proc _a$/,/endproc/' $T.s45 | grep -q 'sta \$d020'; then
    pass "\\n in inline asm still separates lines"
else
    fail "\\n in inline asm still separates lines"
fi

# cc45 encoding prefixes p"..." s"..." a"..." (as ca45): the directive chosen
# and the character constants; .screencode is the mapping s"..." relies on
asm_bytes '    .screencode "Az@[_ 1"'   "011a001b1f2031"     ".screencode: @ 0, letters 1-26, [\\]^_ 27-31"
cat > $T.c <<'END'
int p, s, a;
char k_p(void) { return p'a'; }
char k_s(void) { return s'A'; }
char k_s2(void) { return s'z'; }
char k_a(void) { return a'a'; }
char *str_s(void) { return s"Hi"; }
char *str_a(void) { return a"Ha"; }
int use(void) { return p + s + a; }
#pragma encoding ascii
char *str_p(void) { return p"Hp"; }
END
if $CC -O1 -S $T.c -o $T.s45 2>/dev/null; then pass "identifiers p, s, a next to prefixed literals"
else fail "identifiers p, s, a next to prefixed literals (compile failed)"; fi
[ "$(lda_of k_p)" = "lda #65" ] && pass "p'a' is \$41" || fail "p'a' is \$41 (got $(lda_of k_p))"
[ "$(lda_of k_s)" = "lda #1" ] && pass "s'A' is 1" || fail "s'A' is 1 (got $(lda_of k_s))"
[ "$(lda_of k_s2)" = "lda #26" ] && pass "s'z' is 26" || fail "s'z' is 26 (got $(lda_of k_s2))"
[ "$(lda_of k_a)" = "lda #97" ] && pass "a'a' is \$61" || fail "a'a' is \$61 (got $(lda_of k_a))"
grep -qF '.screencode "Hi"' $T.s45 && pass "s\"...\" is .screencode" || fail "s\"...\" is .screencode"
grep -qF '.ascii "Ha"' $T.s45 && pass "a\"...\" is .ascii" || fail "a\"...\" is .ascii"
grep -qF '.text "Hp"' $T.s45 && pass "p\"...\" is .text under #pragma encoding ascii" \
    || fail "p\"...\" is .text under #pragma encoding ascii"

rm -f $T.s45 $T.bin $T.c

echo ""
echo "string escape tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
