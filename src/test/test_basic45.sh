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

# --data-loader (#370): the loader's source, its checks (on xemu:
# test_xemu.sh, basic45_loader_*)
printf '\x00\x70\xa9\x01\x60' > "$D/code.prg"   # lda #1 : rts at $7000
want='10 bank 0:a=28672:n=3
20 for i=0 to n-1:read v:poke a+i,v:next i
30 sys a
40 rem code.prg - $7000, 3 bytes
50 data 169,1,96'
got=$($B --data-loader "$D/code.prg" -o - 2>&1)
[ "$got" = "$want" ] && pass "--data-loader writes the loader's source" || fail "--data-loader source: $got"
got=$($B --data-loader --hex --checksum --per-line 2 --start 100 --increment 5 --no-sys "$D/code.prg" -o - 2>&1)
if grep -q '^100 bank 0:a=28672:n=3:r=2:l=130:k=5$' <<< "$got" && grep -q '^130 data a9,01,aa$' <<< "$got" &&
   grep -q '^135 data 60,60$' <<< "$got" && grep -q 'read v$:v=dec(v$)' <<< "$got" && ! grep -q 'sys' <<< "$got"; then
    pass "--data-loader --hex --checksum --per-line --start --increment --no-sys"
else
    fail "--data-loader options: $got"
fi
printf '\x01\x20\x60' > "$D/low.prg"   # rts at $2001: where the loader is
out=$($B --data-loader "$D/low.prg" -o "$D/low-loader.prg" 2>&1); rc=$?
[ $rc = 1 ] && grep -q 'would overwrite the loader' <<< "$out" && pass "--data-loader refuses code over the loader" ||
    fail "--data-loader overlap (status $rc): $out"
printf '\xa9\x01\x60' > "$D/code.bin"
out=$($B --data-loader "$D/code.bin" -o - 2>&1); rc=$?
out2=$($B --data-loader --load-at '$7000' "$D/code.bin" -o - 2>&1)
[ $rc = 1 ] && grep -q 'needs --load-at' <<< "$out" && grep -q '^50 data 169,1,96$' <<< "$out2" &&
    pass "--data-loader: a raw binary with --load-at" || fail "--data-loader raw binary: $out / $out2"

# --convert=basic7 (#368): a BASIC 7.0 listing or .prg as BASIC 65 source
conv() { printf '%s\n' "$@" > "$D/c7.txt"; $B --convert=basic7 $CONV_OPTS "$D/c7.txt" -o "$D/c7.bas" 2> "$D/c7.err"; }
conv '10 TEMPO 24' '20 PLAY "V1O4T1U15QCDEF V2O3HCG"'
grep -q 'play "t1 u9 o4 qcdef", "h.r o3 cg"' "$D/c7.bas" &&
grep -q 'gosub l[0-9]* : play "t1' "$D/c7.bas" &&
grep -q '^l[0-9]*: do while rplay(1) or rplay(2) or (peek(4702) and peek(4704) and 128) = 0 : loop : return' "$D/c7.bas" &&
    pass "--convert=basic7: PLAY split per voice, voice 2 held back by a rest, U15 -> u9, a wait (a subroutine) in front" ||
    fail "--convert PLAY: $(cat "$D/c7.bas")"
conv '10 TEMPO 24' '20 PLAY "V1QC U0 C U8 C V2 O5 .ID#E V3 M S$B"'
# (voice 2 waits for voice 1's third note: a half rest; M holds voice 3 until voice 2's E ends: 936 counts)
grep -q 'play "o4 qc u0 c u5 c", "hr o5 i.d#e", "h.r sr o5 \$b"' "$D/c7.bas" &&
    pass "--convert=basic7: U (0-15 -> BASIC 65's table), the octave per voice, M as rests, dotted and sharp/flat notes" ||
    fail "--convert PLAY state: $(cat "$D/c7.bas")"
conv '10 TEMPO 24:TEMPO X+1:TEMPO 0'
grep -q 'tempo 25 : tempo int((x + 1) \* 1.042 + .5) : tempo 1' "$D/c7.bas" && grep -q 'TEMPO 0' "$D/c7.err" &&
    pass "--convert=basic7: TEMPO scaled (PAL), an expression wrapped, TEMPO 0 warned" ||
    fail "--convert TEMPO: $(cat "$D/c7.bas" "$D/c7.err")"
CONV_OPTS=--ntsc conv '10 TEMPO 24'
grep -q 'tempo 30' "$D/c7.bas" && grep -q 'NTSC timing' "$D/c7.bas" &&
    pass "--convert=basic7 --ntsc: TEMPO x 1.25" || fail "--convert --ntsc: $(cat "$D/c7.bas")"
conv '10 PLAY "C"'
grep -q '^  gosub l[0-9]* : tempo 8$' "$D/c7.bas" &&
    pass "--convert=basic7: BASIC 7.0's default tempo set first when the program plays and sets none" ||
    fail "--convert default tempo: $(cat "$D/c7.bas")"
conv '10 SLOW:A=PEN(1):B=RCLR(4):C=RGR(0)' '20 DRAW 1,0,0 TO 9,9:FETCH 1,2,3,4:PLAY A$' '30 IF A THEN 10'
grep -q 'speed 1 : a = lpen(1) : b = (rcolor(3) + 1) : c = rgr(0)' "$D/c7.bas" && grep -q '^l10:' "$D/c7.bas" &&
grep -q 'then l10' "$D/c7.bas" && grep -q 'line 10: RGR has no BASIC 65 counterpart' "$D/c7.err" &&
grep -q 'line 20: FETCH -> EDMA' "$D/c7.err" && grep -q 'edma 0, 1, 134414340, 2' "$D/c7.bas" &&
grep -q 'line 20: PLAY of a string that is not a literal' "$D/c7.err" &&
    pass "--convert=basic7: SLOW -> SPEED 1, PEN( -> LPEN(, RCLR( renumbered (1-16), labels; RGR, DRAW, FETCH and PLAY A\$ warned" ||
    fail "--convert renames: $(cat "$D/c7.bas" "$D/c7.err")"
$B "$D/c7.bas" -o "$D/c7.prg" > /dev/null 2>&1 || true
conv '10 BASEX=1:BASEY=2:DT$="A":DTX$="B":SUM1=3' '20 T=TI:PRINT TI$:TI$="000000"'
grep -q 'ba = 1 : ba = 2 : [a-z][a-z0-9]\$ = "a" : [a-z][a-z0-9]\$ = "b" : sum1 = 3' "$D/c7.bas" && ! grep -q 'dt\$' "$D/c7.bas" &&
grep -q 'basex, basey: one variable' "$D/c7.bas" && grep -q 'is renamed .*DT\$ is the date' "$D/c7.err" &&
grep -q 't = int(ti \* 60) : print (left\$(ti\$, 2) + mid\$(ti\$, 4, 2) + right\$(ti\$, 2)) : clr ti' "$D/c7.bas" &&
    pass "--convert=basic7: names that are one C128 variable merged, DT\$ renamed, TI in 1/60 s, TI\$ as HHMMSS, TI\$=\"000000\" -> CLR TI" ||
    fail "--convert variables / TI: $(cat "$D/c7.bas" "$D/c7.err")"
conv '10 SPRITE 1,1,2:SPRCOLOR 3,4:MOVSPR 2,10,20:A=RSPRITE(3,1)+RSPCOLOR(1)+RSPPOS(1,0)' '20 GRAPHIC 3,1:DRAW 1,10,10 TO 20,20:CHAR 1,1,2,"X",1:GRAPHIC 5:SCNCLR 5:SCNCLR 1'
grep -q 'sprite 0, 1, 1 : sprcolor 2, 3 : movspr 1, 10, 20 : a = (rsprite(2, 1) + 1) + (rspcolor(1) + 1) + rsppos(0, 0)' "$D/c7.bas" &&
grep -q 'screen 320, 200, 4 : pen 0, [a-z][a-z0-9](1) : line 20, 10, 40, 20' "$D/c7.bas" &&
grep -q 'char 1, 16, 1, 1, 2, chr\$(18) + "x"' "$D/c7.bas" && grep -q 'gosub l[0-9]* : scnclr$' "$D/c7.bas" &&
grep -q 'SCNCLR 1 (the bit map) with no graphics screen open' "$D/c7.err" &&
grep -q 'then print chr\$(27) + "x";' "$D/c7.bas" &&
    pass "--convert=basic7: sprites 1-8 -> 0-7, colours 1-16 -> 0-15, multicolour x doubled, CHAR rows in pixels, GRAPHIC 5 -> 80 columns, SCNCLR by mode" ||
    fail "--convert sprites / graphics: $(cat "$D/c7.bas" "$D/c7.err")"
conv '10 TEMPO 24' '20 PLAY "V1QCDEF V2O3HCG"' '30 GOTO 20'
$B "$D/c7.bas" -o "$D/c7b.prg" > "$D/c7b.log" 2>&1 && pass "--convert=basic7: the result builds with basic45" ||
    fail "--convert result does not build: $(cat "$D/c7b.log")"
conv '10 GRAPHIC 1:CIRCLE 1,100,100,30:CIRCLE 1,50,50,20,10,0,360,0,60:CIRCLE 1,50,50,20,20,0,90,45,15:BOX 1,0,0,10,10,45' '20 CIRCLE 1,80,80,20,10,0,90,30,10:GRAPHIC 3:CIRCLE 1,10,10,5'
grep -q 'ellipse 100, 100, 30, 30' "$D/c7.bas" &&
grep -q 'polygon 50, 50, 20, 10, 6, 6, 0, 90, 0' "$D/c7.bas" &&
grep -q 'polygon 50, 50, 20, 20, 24, 6, 0, 315, 0' "$D/c7.bas" &&
grep -q ': gosub l[0-9]*$' "$D/c7.bas" && grep -q 'box [a-z][a-z0-9], [a-z][a-z0-9], ' "$D/c7.bas" &&
grep -q 'sin([a-z][a-z0-9]) + [a-z][a-z0-9] \* [a-z][a-z0-9] \* cos(' "$D/c7.bas" &&
grep -q 'ellipse 20, 10, 10, 10' "$D/c7.bas" &&
    pass "--convert=basic7: CIRCLE -> ELLIPSE, POLYGON (steps dividing 360, an arc of whole steps, a circle's rotation folded in; the ROM's argument order), BASIC 7.0's own algorithm by a subroutine (a turned ellipse); BOX turned by a subroutine; multicolour radii" ||
    fail "--convert BOX / CIRCLE: $(cat "$D/c7.bas")"
# a .prg: BASIC 7.0's $CE $08 is XOR (BASIC 65's LOG10), $FE $04 PLAY
printf '\x01\x1c\x10\x1c\x0a\x00\x41\xb2\xce\x08\x28\x31\x2c\x32\x29\x00\x00\x00' > "$D/x.prg"
$B --convert=basic7 "$D/x.prg" -o "$D/x.bas" 2> /dev/null && grep -q 'a = xor(1, 2)' "$D/x.bas" &&
    pass "--convert=basic7: a .prg read with BASIC 7.0's tables (\$CE \$08 is XOR, not LOG10)" ||
    fail "--convert .prg: $(cat "$D/x.bas")"

echo "basic45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
