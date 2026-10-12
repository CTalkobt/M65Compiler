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

rm -f $T.s45 $T.bin $T.c

echo ""
echo "string escape tests: $passed passed, $failed failed"
if [ $failed -ne 0 ]; then exit 1; fi
