#!/bin/bash
# basic45 without an emulator: numbered programs (their errors), BASIC 65's
# $CE / $FE tokens as the ROM's tables have them, the warning for a statement
# that is not BASIC 65 (SLOW, a typo), decompiling the new tokens.
# (Running them on BASIC 65: test_basic45_numbered_xemu.bas, make test-xemu.)
cd "$(dirname "$0")/../.." || exit 1
B=./bin/basic45
D=build/test_basic45
rm -rf "$D"; mkdir -p "$D"
passed=0; failed=0
pass() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# bytes of a one-line program after the link and line number
bytes() { printf '%s\n' "$1" > "$D/t.bas"; $B "$D/t.bas" -o "$D/t.prg" >/dev/null 2>&1; xxd -p -s 6 "$D/t.prg" | tr -d '\n' | sed 's/000000$//'; }

ok=1; why=""
while IFS='|' read -r src want; do
    got=$(bytes "$src")
    [ "$got" = "$want" ] || { ok=0; why+=" [$src: $got]"; }
done <<'T'
print mod(7,3)|99ce0b28372c3329
print log10(100)|99ce082831303029
print pot(1)|99ce02283129
print wpeek(1)|99ce10283129
print rsprsys(1)|99ce17283129
wpoke 1,2|fe1d20312c32
vsync 100|fe5420313030
mkdir "x"|fe51225822
a = 1 << 3|41b231fe5233
a = 8 >> 1|41b238fe5331
go64|cb3634
T
[ $ok = 1 ] && pass "BASIC 65's \$CE functions, \$FE keywords, << >> and GO64 tokenize as the ROM's tables" ||
    fail "tokens:$why"

# a numbered program keeps its numbers; GOTO 10 stays 10
printf '10 print "hi"\n20 goto 10\n' > "$D/n.bas"
if $B --strict "$D/n.bas" -o "$D/n.prg" >/dev/null 2>&1 &&
   [ "$(xxd -p "$D/n.prg" | tr -d '\n')" = "01200b200a009922484922001420140089203130000000" ]; then
    pass "a numbered program keeps its line numbers"
else
    fail "numbered program: $(xxd -p "$D/n.prg" 2>/dev/null | tr -d '\n')"
fi

# its errors: numbers out of order, a line without one, a missing target
printf '10 print "a"\n5 goto 30\nprint "b"\n' > "$D/bad.bas"
out=$($B "$D/bad.bas" -o "$D/bad.prg" 2>&1); rc=$?
if [ $rc = 1 ] && grep -q "line number 5 after 10: the numbers must ascend" <<< "$out" &&
   grep -q "this line has no line number" <<< "$out" && grep -q "line 30 does not exist" <<< "$out"; then
    pass "a numbered program's numbers are checked (order, missing, GOTO targets)"
else
    fail "numbered program errors (status $rc): $out"
fi

# a statement that is not BASIC 65: a warning (SLOW with its BASIC 65 name)
printf 'slow\nprnt "x"\nx = peekw(1)\n' > "$D/w.bas"
out=$($B "$D/w.bas" -o "$D/w.prg" 2>&1)
if grep -q "'slow' is not a BASIC 65 keyword (BASIC 65 has SPEED)" <<< "$out" &&
   grep -q "'prnt' is not a BASIC 65 keyword: the statement is a ?SYNTAX ERROR" <<< "$out" &&
   grep -q "'peekw' is not a BASIC 65 function (BASIC 65 has WPEEK)" <<< "$out"; then
    pass "a statement BASIC 65 does not know is warned about (SLOW: BASIC 65 has SPEED)"
else
    fail "unknown statement warnings: $out"
fi

# decompiling the new tokens gives the program back
printf 'a = mod(7, 3) << 2\nprint a, log10(100), wpeek(1)\n' > "$D/r.bas"
if $B --strict "$D/r.bas" -o "$D/r.prg" >/dev/null 2>&1 && $B --decompile "$D/r.prg" -o "$D/r2.bas" >/dev/null 2>&1 &&
   $B --strict "$D/r2.bas" -o "$D/r2.prg" >/dev/null 2>&1 && cmp -s "$D/r.prg" "$D/r2.prg" &&
   grep -q "mod(7, 3) << 2" "$D/r2.bas"; then
    pass "--decompile reads the \$CE and << tokens back"
else
    fail "decompile of the new tokens: $(cat "$D/r2.bas" 2>/dev/null)"
fi

echo "basic45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
