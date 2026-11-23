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
fwrite#2,1,1:fread#2,1,1|fe1e20322c312c313afe1c20322c312c31
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
conv '10 POKE 53265,PEEK(53265)OR16:IF A THEN POKE 53265,PEEK(53265)AND239:B=1' '20 POKE 53265,155'
# ($D011 bit 7 written 0: the raster compare's bit 8; the MEGA65's KERNAL runs on the raster IRQ at line 255)
grep -q 'poke 53265, (peek(53265) or 16) and 127 : if a then poke 53265, (peek(53265) and 239) and 127 : b = 1' "$D/c7.bas" &&
grep -q 'poke 53265, 27$' "$D/c7.bas" &&
    pass "--convert=basic7: POKE 53265 keeps bit 7 (the raster compare's bit 8) 0" ||
    fail "--convert POKE 53265: $(cat "$D/c7.bas")"
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
conv '10 GRAPHIC 1:SSHAPE A$,10,10,25,13:GSHAPE A$,100,100,4:SSHAPE A$,0,0,99,99:LOCATE 5,5:GSHAPE A$'
grep -q 'gcopy 10, 10, 16, 4 : paste 100, 100 : gcopy 0, 0, 100, 100 : zx = 5 : zy = 5 : paste zx, zy' "$D/c7.bas" &&
grep -q 'GSHAPE mode 4' "$D/c7.err" && grep -q 'SSHAPE of 100 x 100 pixels' "$D/c7.err" &&
conv '10 SSHAPE A$,1,1,5,5:SSHAPE B$,1,1,5,5' && grep -q 'SSHAPE has no BASIC 65 counterpart (several shapes' "$D/c7.err" &&
    pass "--convert=basic7: SSHAPE -> GCOPY, GSHAPE -> PASTE (one shape; a mode, a large area and several shapes warned)" ||
    fail "--convert SSHAPE: $(cat "$D/c7.bas" "$D/c7.err")"
conv '10 SLOW:A=PEN(1):B=RCLR(4):C=RGR(0)' '20 DRAW 1,0,0 TO 9,9:FETCH 1,2,3,4:PLAY A$' '30 IF A THEN 10'
grep -q 'speed 1 : a = lpen(1) : b = (rcolor(3) + 1) : c = [a-z][a-z0-9]$' "$D/c7.bas" && grep -q '^l10:' "$D/c7.bas" &&
grep -q 'then l10' "$D/c7.bas" && ! grep -q 'RGR' "$D/c7.err" &&
grep -q 'line 20: FETCH -> EDMA' "$D/c7.err" && grep -q 'edma 0, 1, 134414340, 2' "$D/c7.bas" &&
grep -q 'line 20: PLAY of a string that is not a literal' "$D/c7.err" &&
    pass "--convert=basic7: SLOW -> SPEED 1, PEN( -> LPEN(, RCLR( renumbered (1-16), RGR( (the GRAPHIC mode kept), labels; FETCH and PLAY A\$ warned" ||
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
$B --convert=basic7 "$D/x.prg" -o "$D/x.bas" 2> /dev/null && grep -q 'a = ((1) xor (2))' "$D/x.bas" &&
    pass "--convert=basic7: a .prg read with BASIC 7.0's tables (\$CE \$08 is XOR, not LOG10; BASIC 65's XOR an operator)" ||
    fail "--convert .prg: $(cat "$D/x.bas")"

# --convert=basic35: BASIC 3.5 (C16 / Plus/4), BASIC 7.0's rules plus the TED's colours and sound
conv35() { printf '%s\n' "$@" > "$D/c35.txt"; $B --convert=basic35 $CONV_OPTS "$D/c35.txt" -o "$D/c35.bas" 2> "$D/c35.err"; }
conv35 '10 COLOR 0,2,7:COLOR 4,2,2:COLOR 1,12:COLOR 1,C,3:A=RLUM(1)' '20 PRINT "{light-red}{$82}X{$84}";CHR$(154):VOL 8'
grep -q 'background 1 : [a-z][a-z0-9](0) = 7 : border 12 : [a-z][a-z0-9](4) = 2 : [a-z][a-z0-9](1) = 10 : [a-z][a-z0-9](1) = val(mid\$("00010203040506070809131003140613", (c) \* 2 - 1, 2))' "$D/c35.bas" &&
grep -q 'print "{light-green}{flash}x{flash-off}"; chr\$(31) : vol 15' "$D/c35.bas" &&
grep -q ' : \([a-z][a-z0-9]\)(1) = 3 : a = \1(1)$' "$D/c35.bas" && ! grep -q 'RLUM' "$D/c35.err" && grep -q 'luminance of an expression' "$D/c35.err" &&
grep -q 'converted from BASIC 3.5' "$D/c35.bas" &&
    pass "--convert=basic35: TED colours and luminances (white's greys, pink -> light red, an expression by a table), colour codes and flash in strings, CHR\$ codes, VOL 0-8 -> 0-15, RLUM from the luminances COLOR set" ||
    fail "--convert=basic35 colours: $(cat "$D/c35.bas" "$D/c35.err")"
conv35 '10 SOUND 1,810,30:SOUND 3,600,10:SOUND V,F,D' '20 GRAPHIC 1:CIRCLE 1,100,100,30:SCNCLR:GRAPHIC 0:SCNCLR'
grep -q 'do while peek(4804) < 128 : loop : sound 1, int(517.946 \* \([a-z][a-z0-9]\)), 30 : do while peek(4805) < 128 : loop : sound 2, int(261.416 \* \1), 10, 0, 0, 0, 3' "$D/c35.bas" &&
grep -q 'sound (v + (v = 3)), int(110840.4375 / (1024 - (f)) \* [a-z][a-z0-9]), d' "$D/c35.bas" &&
grep -q '[a-z][a-z0-9] = 16.519 : if peek(53359) and 128 then [a-z][a-z0-9] = 16.404' "$D/c35.bas" &&
grep -q 'ellipse 100, 100, 30, 30' "$D/c35.bas" && grep -q ': scnclr [a-z][a-z0-9](0) : ' "$D/c35.bas" && grep -q 'gosub l[0-9]* : scnclr$' "$D/c35.bas" &&
    pass "--convert=basic35: SOUND (TED frequencies as the SID's, voice 3 noise on voice 2, waiting for the voice), SCNCLR the bit map in graphics" ||
    fail "--convert=basic35 sound: $(cat "$D/c35.bas" "$D/c35.err")"
CONV_OPTS=--ntsc conv35 '10 SOUND 1,810,30'
grep -q 'sound 1, int(522.714 \* [a-z][a-z0-9]), 25' "$D/c35.bas" &&
    pass "--convert=basic35 --ntsc: the NTSC TED clock, durations in 1/60 s" || fail "--convert=basic35 --ntsc: $(cat "$D/c35.bas")"
$B "$D/c35.bas" -o "$D/c35.prg" > "$D/c35b.log" 2>&1 && pass "--convert=basic35: the result builds with basic45" ||
    fail "--convert=basic35 result does not build: $(cat "$D/c35b.log")"
# a .prg at $1001: $CE is RLUM (BASIC 7.0's prefix), $E7 COLOR
printf '\x01\x10\x0c\x10\x0a\x00\x41\xb2\xce\x28\x31\x29\x00\x00\x00' > "$D/p4.prg"
$B --convert=basic35 "$D/p4.prg" -o "$D/p4.bas" 2> "$D/p4.err" && grep -q 'a = [a-z][a-z0-9](1)$' "$D/p4.bas" && ! grep -q 'loads at' "$D/p4.err" &&
    pass "--convert=basic35: a .prg read with BASIC 3.5's tables (\$CE is RLUM, converted to its luminance array)" || fail "--convert=basic35 .prg: $(cat "$D/p4.bas" "$D/p4.err")"

# --convert=simons: Simons' BASIC (C64)
convs() { printf '%s\n' "$@" > "$D/cs.txt"; $B --convert=simons "$D/cs.txt" -o "$D/cs.bas" 2> "$D/cs.err"; }
convs '10 ON KEY "AB",: GOTO 50' '20 A=INKEY:IF ST THEN 20' '50 DISABLE:PRINT "K":RESUME' '60 RESUME NEXT'
grep -q '^  on sgn(\(..\)) gosub l[0-9]* : \(..\)\$ = "ab" : \1 = 1 : \(..\) = 1$' "$D/cs.bas" &&
grep -q '^l50: on sgn(\(..\)) gosub l[0-9]* : \1 = 0 : print "k" : \1 = .. : return$' "$D/cs.bas" &&
grep -q 'resume next$' "$D/cs.bas" && grep -q 'gosub l[0-9]* : a = .. : if st then' "$D/cs.bas" &&
grep -q '= peek(54809) : if .. = 255 or .. = 0 then return' "$D/cs.bas" && grep -q 'poke 54809, 0 : on .. goto l[0-9]*' "$D/cs.bas" &&
grep -q 'poke 54804, 0 : .. = 255 - peek(54803)' "$D/cs.bas" && grep -q 'ON KEY: the key is in .. (PETSCII), not in ST' "$D/cs.err" &&
! grep -q 'INKEY\|DISABLE\|RESUME' "$D/cs.err" &&
    pass "--convert=simons: ON KEY (the key queue checked at each line's start), DISABLE, RESUME (RESUME NEXT kept), INKEY (the keyboard matrix)" ||
    fail "--convert=simons ON KEY / INKEY: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 BFLASH 5,2,7:BFLASH 0:BFLASH S,1,18' '20 BFLASH OFF:BFLASH ON' '30 IF TI>5 THEN 10' '40 RCOMP PRINT'
grep -q 'poke 5729, 5 : poke 5730, 2 : poke 5731, 7 : poke 5728, 1 : gosub \(l[0-9]*\) : gosub \(l[0-9]*\) : poke 5729, s : poke 5730, 1 : poke 5731, 2 : poke 5728, 1 : on -((s) <> 0) + 1 gosub \2, \1$' "$D/cs.bas" &&
grep -q '^  gosub l[0-9]* : gosub l[0-9]*$' "$D/cs.bas" && grep -q 'then sys 5699$' "$D/cs.bas" && grep -q ': next : sys 5674 : return$' "$D/cs.bas" &&
grep -q '= ((int(ti \* 60) > 5) <> 0) : if .. then l10' "$D/cs.bas" && ! grep -q 'BFLASH' "$D/cs.err" &&
    pass "--convert=simons: BFLASH (an IRQ routine at \$1600 counting frames; 0 / OFF remove it, ON), RCOMP's saved condition converted (TI)" ||
    fail "--convert=simons BFLASH: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 EXEC SUB:CALL SUB' '20 REPEAT:A=A+1:UNTIL A>3:LOOP:B=B+1:EXIT IF B=5:C=1:END LOOP' '30 END' '40 PROC SUB' '50 TRACE 10:RETRACE:END PROC'
grep -q '^  gosub proc_sub : goto proc_sub$' "$D/cs.bas" && grep -q '^proc_sub:' "$D/cs.bas" &&
grep -q ': do : a = a + 1 : loop until a > 3 : do : b = b + 1 : if b = 5 then exit$' "$D/cs.bas" && grep -q '^  c = 1 : loop$' "$D/cs.bas" &&
grep -q 'tron : return' "$D/cs.bas" && grep -q 'RETRACE' "$D/cs.err" &&
    pass "--convert=simons: PROC -> a label, EXEC/CALL -> GOSUB/GOTO, END PROC -> RETURN, REPEAT/UNTIL and LOOP/EXIT IF/END LOOP -> DO/LOOP (statements after EXIT IF on a line of their own), TRACE" ||
    fail "--convert=simons structure: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 A=DIV(7,2)+MOD(7,3)+FRAC(X)+EXOR(5,3)+PLACE("B",A$)+POT(0)+JOY+%0000 0101' '20 B$=INST("X",A$,1)+INSERT("X",A$,1):R=LIN'
grep -q 'a = int(int(7) / int(2)) + mod(int(7), int(3)) + (sgn(x) \* (abs(x) - int(abs(x)))) + ((5) xor (3)) + instr(a\$, "b") + pot(1) + joy(2) + %00000101' "$D/cs.bas" &&
grep -q 'b\$ = (left\$(a\$, 1) + "x" + mid\$(a\$, (1) + len("x") + 1)) + (left\$(a\$, 1) + "x" + mid\$(a\$, (1) + 1)) : rcursor , \([a-z][a-z0-9]\) : r = \1' "$D/cs.bas" &&
    pass "--convert=simons: DIV, MOD, FRAC, EXOR (an operator), PLACE, POT, JOY, a spaced %binary literal, INST (overwrites), INSERT, LIN (by RCURSOR)" ||
    fail "--convert=simons functions: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 ON ERROR:GOTO 50' '20 IF A THEN PRINT 1:ELSE:PRINT 2' '30 RCOMP PRINT 3:ELSE:PRINT 4:CGOTO X' '40 ON ERROR:PRINT ERRN;ERRLN:END' '50 NO ERROR:PRINT AT(5,3)"HI";AT(0,1)"X":CENTRE "C":PAUSE "P",2'
grep -q '^l10: trap l50$' "$D/cs.bas" && grep -q '\([a-z][a-z0-9]\) = ((a) <> 0) : if \1 then print 1 : else print 2' "$D/cs.bas" &&
grep -q 'if [a-z][a-z0-9] then print 3 : else print 4 : [a-z][a-z0-9] = x : goto l[0-9]*' "$D/cs.bas" &&
grep -q 'if [a-z][a-z0-9] = 30 then l30' "$D/cs.bas" && grep -q 'trap l[0-9]*$' "$D/cs.bas" && grep -q 'print er; el : end' "$D/cs.bas" &&
grep -q 'trap : cursor 5, 3 : print "hi"; : cursor 0, 1 : print "x" : print spc(int((40 - pos(0) - len("c")) / 2) \* -(int((40 - pos(0) - len("c")) / 2) > 0)); "c"; : print "p" : [a-z][a-z0-9] = ti : do : get' "$D/cs.bas" &&
    pass "--convert=simons: ON ERROR -> TRAP (a GOTO, or its line as a handler), ELSE, RCOMP (the IF's result kept), CGOTO (a table of lines), ERRN/ERRLN, PRINT AT -> CURSOR, CENTRE, PAUSE" ||
    fail "--convert=simons errors / text: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 HIRES 1,0:PLOT 1,2,1:LINE 0,0,9,9,2:REC 1,1,5,5,1:BLOCK 0,0,4,4,1:CIRCLE 50,50,20,10,1:ARC 50,50,0,90,10,20,20,1' '20 MULTI 2,5,7:PLOT 10,10,3:TEXT 16,8,"A",1,2,8:A=TEST(1,1):NRM:MOB SET 1,13,2,0,0:MMOB 1,24,50,100,60,1,0:WAVE 1,00100001'
grep -q 'screen 320, 200, 4 : \([a-z][a-z0-9]\) = 0 : \([a-z][a-z0-9]\) = 1 : zc(0) = \1 : zc(1) = \2 : zc(5) = (\1 xor \2) : scnclr \1' "$D/cs.bas" &&
grep -q 'pen 0, zc(1) : line 1, 2 : ' "$D/cs.bas" && grep -q 'dmode 0, 1, 0, 0, 1 : pen 0, zc(5) : line 0, 0, 9, 9 : .* : dmode 0, 0, 0, 0, 1' "$D/cs.bas" &&
grep -q 'polygon 50, 50, 20, 10, 30, 30, 0, 90, 0' "$D/cs.bas" && grep -q 'polygon 50, 50, 20, 20, 36, 9, 0, 0, 0' "$D/cs.bas" &&
grep -q 'pen 0, zc(3) : line 20, 10' "$D/cs.bas" && grep -q 'char 4, 8, 2, 2, 2, "a"' "$D/cs.bas" &&
grep -q 'movspr 1, 24, 50 to 100, 60, 15 : do while rsppos(1, 2) : loop' "$D/cs.bas" && grep -q 'poke 54272 + 7 \* ((1) - 1) + 4, 33' "$D/cs.bas" &&
    pass "--convert=simons: HIRES/MULTI -> SCREEN and colour sources, PLOT/LINE/REC/BLOCK/CIRCLE/ARC as BASIC 7.0's (invert by XOR, MULTI x doubled), TEXT -> CHAR, MMOB waits, WAVE -> the SID's control register" ||
    fail "--convert=simons graphics: $(cat "$D/cs.bas" "$D/cs.err")"
$B "$D/cs.bas" -o "$D/cs.prg" > "$D/csb.log" 2>&1 && pass "--convert=simons: the result builds with basic45" ||
    fail "--convert=simons result does not build: $(cat "$D/csb.log")"
convs '10 DESIGN 2,$E000+65*8' '11 @BBBBBBBB' '12 @B  B' '20 DESIGN 1,$C000+64*13' '21 @BCD' '30 MOB SET 1,13,2,0,1:FCHR 1,2,3,4,65:UPW 0,0,5,5'
grep -q 'char def int(mod(\$e000 + 65 \* 8, 4096) / 8), 255, 144, 0, 0, 0, 0, 0, 0' "$D/cs.bas" && grep -q '^  rem @bbbbbbbb$' "$D/cs.bas" &&
grep -q '[a-z][a-z0-9]\$ = "6C0000' "$D/cs.bas" &&
grep -q '[a-z][a-z0-9] = 13 : [a-z][a-z0-9] = 1 : gosub l[0-9]* : sprite 1, 1, 2, 0, , , 1' "$D/cs.bas" &&
grep -q 'poke 1536 + 64 \* [a-z][a-z0-9] + [a-z][a-z0-9], dec(mid\$(' "$D/cs.bas" &&
grep -q '= 1 : [a-z][a-z0-9] = 2 : [a-z][a-z0-9] = 3 : [a-z][a-z0-9] = 4 : [a-z][a-z0-9] = 65 : gosub l[0-9]*' "$D/cs.bas" &&
    pass "--convert=simons: DESIGN of a character -> CHAR DEF (bytes from its @ rows), of a sprite -> data MOB SET copies to \$0600; FCHR and the scrolls by subroutines" ||
    fail "--convert=simons DESIGN / rectangles: $(cat "$D/cs.bas" "$D/cs.err")"
convs '10 FETCH "{home}",5,A$:FETCH "0123",2,N' '20 SCRSV 2,8,2,"S,S,W":SCRLD 2,8,2,"S":OUT' '30 DISAPA:PAGE 5:OPTION 10:DELAY 3:PRINT 1'
grep -q '= "{home}" : [a-z][a-z0-9] = 5 : gosub l[0-9]* : a\$ = [a-z][a-z0-9]\$' "$D/cs.bas" && grep -q ': n = val([a-z][a-z0-9]\$)' "$D/cs.bas" &&
grep -q 'getkey [a-z][a-z0-9]\$' "$D/cs.bas" &&
grep -q 'open 2, 8, 2, "s,s,w" : print# 2, chr\$(0); chr\$(4); : for [a-z][a-z0-9] = 0 to 1023 : print# 2, chr\$(peek(55296 + ' "$D/cs.bas" &&
grep -q 'get# 2, [a-z][a-z0-9]\$, [a-z][a-z0-9]\$ : poke 55296 + ' "$D/cs.bas" && grep -q 'print err\$(er)' "$D/cs.bas" &&
grep -q '^  print 1$' "$D/cs.bas" && ! grep -q 'FETCH\|SCRSV\|SCRLD\|OUT\|DISAPA\|PAGE' "$D/cs.err" &&
    pass "--convert=simons: FETCH (a GET loop), SCRSV/SCRLD (the screen and colours by PRINT# / GET#), OUT, LIST's settings dropped" ||
    fail "--convert=simons FETCH / SCRSV: $(cat "$D/cs.bas" "$D/cs.err")"
$B "$D/cs.bas" -o "$D/cs.prg" > "$D/csb.log" 2>&1 && pass "--convert=simons: FETCH / SCRSV result builds" ||
    fail "--convert=simons FETCH / SCRSV result does not build: $(cat "$D/csb.log")"
# --convert=tsbneo: TSB's statement forms
printf '%s\n' '10 SOUND 3,$3427:SOUND 4,$1E00:WAVE 3,$41,$0400:HIRES 0,0:MULTI OFF:PLOT 10,10,1' '20 KEY GET X$:VOL OFF:COLD' \
    '30 SCRLD DEF $7A,6,0,3:SCRLD 1,8,3,"MC":SCRLD RESTORE:SCRSV 2,8,2,"S,S,W"' '40 MEMCLR SOUND,25:MEM:CHECK:D!10-20' > "$D/ts.txt"
$B --convert=tsbneo "$D/ts.txt" -o "$D/ts.bas" 2> "$D/ts.err"
grep -q 'poke 54272 + 7 \* ((3) - 1), (\$3427) and 255 : poke 54272 + 7 \* ((3) - 1) + 1, int((\$3427) / 256)' "$D/ts.bas" &&
grep -q 'poke 54293, (\$1e00) and 7' "$D/ts.bas" && grep -q '+ 4, \$41 : poke 54272 + 7 \* ((3) - 1) + 2, (\$0400) and 255' "$D/ts.bas" &&
grep -q 'line 20, 10 : ' "$D/ts.bas" && grep -q 'getkey x\$ : vol 0 : end' "$D/ts.bas" &&
grep -q 'bload "mc", p((\$7a) \* 256), u8 : open 2, 8, 2' "$D/ts.bas" &&
grep -q 'for [a-z][a-z0-9] = 0 to (25) - 1 : poke 54272 + [a-z][a-z0-9], 0 : next$' "$D/ts.bas" && grep -q 'D! (TSB: deletes program lines): dropped' "$D/ts.err" &&
    pass "--convert=tsbneo: SOUND / WAVE (TSB's) to the SID's registers, MULTI OFF, KEY GET, VOL OFF, COLD, SCRLD DEF (BLOAD), MEM CLR; MEM, CHECK, D! dropped" ||
    fail "--convert=tsbneo statements: $(cat "$D/ts.bas" "$D/ts.err")"
$B "$D/ts.bas" -o "$D/ts.prg" > "$D/tsb.log" 2>&1 && pass "--convert=tsbneo: the result builds" ||
    fail "--convert=tsbneo result does not build: $(cat "$D/tsb.log")"
# a .prg: $64 $01 is HIRES; $64 $22 (TSB's DO) warned
printf '\x01\x08\x10\x08\x0a\x00\x64\x01\x31\x2c\x30\x3a\x64\x22\x00\x00\x00' > "$D/s.prg"
$B --convert=simons "$D/s.prg" -o "$D/s.bas" 2> "$D/s.err" && grep -q 'screen 320, 200, 4' "$D/s.bas" && grep -q 'DO has no BASIC 65 counterpart (a DO not after IF' "$D/s.err" &&
    pass "--convert=simons: a .prg read with Simons' tokens (\$64 xx; TSB's DO without IF ... THEN warned)" || fail "--convert=simons .prg: $(cat "$D/s.bas" "$D/s.err")"

# --compile (#369): machine code (a SYS line, then the code); the assembly kept
# with --asm; a statement outside the compiled subset an error at its line
printf '10 a=1:print a;"x"\n20 goto 10\n' > "$D/c.bas"
if $B --compile --asm "$D/c.s45" "$D/c.bas" -o "$D/c.prg" > "$D/c.log" 2>&1 &&
   [ "$(xxd -p -l 2 "$D/c.prg")" = "0120" ] && grep -q 'jsr __bas_prn' "$D/c.s45" && grep -q 'jsr __bas_prs' "$D/c.s45" &&
   [ "$(stat -c %s "$D/c.prg")" -gt 1000 ]; then
    pass "--compile: a machine-code program (\$2001, SYS), --asm the generated assembly"
else
    fail "--compile: $(cat "$D/c.log")"
fi
printf '10 print 1\n20 monitor\n' > "$D/cu.bas"
if ! $B --compile "$D/cu.bas" -o "$D/cu.prg" > "$D/cu.log" 2>&1 && grep -q "cu.bas:2:1: (line 20): error: MONITOR is a command of BASIC's editor" "$D/cu.log"; then
    pass "--compile: an editor command (MONITOR) is an error at its line"
else
    fail "--compile MONITOR: $(cat "$D/cu.log")"
fi
# integer narrowing: whole-number variables (FOR's, K) without the float
# runtime; one that grows without a bound (N = N + 1) stays a float
printf '10 for i=1 to 10:k=i*2+5:next\n20 k=5:n=0:for j=1 to 3:n=n+1:next\n' > "$D/cn.bas"
if $B --compile -O2 --asm "$D/cn.s45" "$D/cn.bas" -o "$D/cn.prg" > "$D/cn.log" 2>&1 &&
   [ "$(grep -c 'jsr __float_add' "$D/cn.s45")" = 1 ]; then
    pass "--compile: whole-number variables narrowed to integers"
else
    fail "--compile narrowing: $(grep 'jsr __float' "$D/cn.s45" | tr '\n' ' ') $(cat "$D/cn.log")"
fi
# --speed: -O3 and the RAM float routines (__ffast_*); without it the ROM's
printf '10 a=1.5:b=a*a/3+a:if a<b then print b\n' > "$D/cs.bas"
if $B --compile --speed --asm "$D/cs.s45" "$D/cs.bas" -o "$D/cs.prg" > "$D/cs.log" 2>&1 &&
   grep -q 'jsr __ffast_mul' "$D/cs.s45" && grep -q 'jsr __ffast_add' "$D/cs.s45" && grep -q 'jsr __ffast_cmp' "$D/cs.s45" &&
   grep -q '__bas_fdivx' "$D/cs.s45" &&
   $B --compile --asm "$D/cs2.s45" "$D/cs.bas" -o "$D/cs2.prg" > "$D/cs2.log" 2>&1 && ! grep -q '__ffast' "$D/cs2.s45"; then
    pass "--compile --speed: the RAM float routines (without --speed the ROM's)"
else
    fail "--compile --speed: $(cat "$D/cs.log" "$D/cs2.log")"
fi
# counted loops: a FOR whose body nothing can leave or enter puts nothing
# on BASIC's stack (no __bas_forpush); one with a GOTO in its body does
printf '10 for i=1 to 3:print i:next\n20 for j=1 to 2:if j=3 then 40\n30 next\n40 end\n' > "$D/cf.bas"
if $B --compile --asm "$D/cf.s45" "$D/cf.bas" -o "$D/cf.prg" > "$D/cf.log" 2>&1 &&
   [ "$(grep -c 'jsr __bas_forpush' "$D/cf.s45")" = 1 ]; then
    pass "--compile: a FOR loop nothing leaves or enters is a counted loop"
else
    fail "--compile counted loops: $(grep -c 'jsr __bas_forpush' "$D/cf.s45") forpush $(cat "$D/cf.log")"
fi
# The number reader (basic_fin.s45, #484) is linked only by a program
# INPUTting or READing a number, and strtof by neither: the link maps show
# the objects
printf '10 input a$:read b$:print a$;b$\n20 data x\n' > "$D/cis.bas"
printf '10 input a:read b:print a;b\n20 data 1\n' > "$D/cin.bas"
for n in cis cin; do
    $B --compile --asm "$D/$n.s45" "$D/$n.bas" -o "$D/$n.prg" > "$D/$n.log" 2>&1 &&
        ./bin/ca45 -c "$D/$n.s45" -o "$D/$n.o45" >> "$D/$n.log" 2>&1 &&
        ./bin/ln45 -basic -M "$D/$n.map" -o "$D/$n.m.prg" "$D/$n.o45" lib/build/c45.lib >> "$D/$n.log" 2>&1
done
if ! grep -q 'basic_fin\.o45\|strtof\.o45' "$D/cis.map" && grep -q 'basic_fin\.o45' "$D/cin.map" &&
   ! grep -q 'strtof\.o45' "$D/cin.map"; then
    pass "--compile: INPUT and READ of strings link no number conversion; of numbers the assembly reader, not strtof"
else
    fail "--compile INPUT/READ strings: $(grep -c 'basic_fin\.o45' "$D/cis.map" "$D/cin.map" | tr '\n' ' ') $(cat "$D/cis.log" "$D/cin.log")"
fi
# #486: an array element's subscript in A/X and its array after the jsr;
# PRINT, string assignment and comparison take their strings in A/X / $12
printf '10 dim e(9),n$(3):e(2)=1:n$(1)="a":print e(2);n$(1)\n20 if n$(1)="a" then print "y"\n' > "$D/ca.bas"
if $B --compile --asm "$D/ca.s45" "$D/ca.bas" -o "$D/ca.prg" > "$D/ca.log" 2>&1 &&
   [ "$(grep -A1 'jsr __bas_aelf' "$D/ca.s45" | grep -c '\.word __ba_E')" = 2 ] &&
   grep -q 'jsr __bas_aels' "$D/ca.s45" && grep -q 'jsr __bas_prsa' "$D/ca.s45" &&
   grep -q 'jsr __bas_sseta' "$D/ca.s45" && grep -q 'jsr __bas_scmpa' "$D/ca.s45" && ! grep -qE 'jsr __bas_(prs|sset|scmp|ael1)$' "$D/ca.s45"; then
    pass "--compile: array elements, PRINT, string assignment and comparison with their arguments in registers"
else
    fail "--compile register arguments: $(grep -E 'jsr __bas_(ael|prs|sset|scmp)' "$D/ca.s45" | tr '\n' ' ') $(cat "$D/ca.log")"
fi
# #489: the program's own values and flow checked only with --checks; TRAP
# without it warns that only outside errors reach its handler
printf '10 trap 30:dim e(9)\n15 for i=0 to 9:e(i)=i:next\n20 end\n30 print er\n' > "$D/ck.bas"
if $B --compile --asm "$D/ck0.s45" "$D/ck.bas" -o "$D/ck0.prg" > "$D/ck0.log" 2>&1 &&
   $B --compile --checks --asm "$D/ck1.s45" "$D/ck.bas" -o "$D/ck1.prg" > "$D/ck1.log" 2>&1 &&
   ! grep -q '@Err14' "$D/ck0.s45" && grep -q '@Err14' "$D/ck1.s45" &&
   grep -q 'without --checks only the errors' "$D/ck0.log" && ! grep -q 'without --checks' "$D/ck1.log"; then
    pass "--compile: range checks only with --checks; TRAP without it warns"
else
    fail "--compile --checks: $(grep -c '@Err14' "$D/ck0.s45" "$D/ck1.s45" | tr '\n' ' ') $(cat "$D/ck0.log" "$D/ck1.log")"
fi
printf '10 goto 30\n' > "$D/cg.bas"
! $B --compile "$D/cg.bas" -o "$D/cg.prg" > "$D/cg.log" 2>&1 && grep -q 'line 30 does not exist' "$D/cg.log" &&
    pass "--compile: GOTO to a missing line is an error" || fail "--compile GOTO 30: $(cat "$D/cg.log")"
# a tokenized .prg compiles too
$B "$D/c.bas" -o "$D/ct.prg" >/dev/null 2>&1
$B --compile "$D/ct.prg" -o "$D/ct2.prg" > "$D/ct.log" 2>&1 && cmp -s "$D/ct2.prg" "$D/c.prg" &&
    pass "--compile: a tokenized .prg in, the same machine code out" || fail "--compile .prg: $(cat "$D/ct.log")"

# a reserved word is never a label: "end: print" is END and PRINT (also in a
# numbered program, "80 rcursor:print"); GOTO a reserved word is an error
printf '10 end:print 1\n20 rcursor:print 2\n' > "$D/rw.bas"
if $B "$D/rw.bas" -o "$D/rw.prg" > "$D/rw.log" 2>&1 &&
   [ "$(od -An -tx1 -v "$D/rw.prg" | tr -d ' \n')" = "01200b200a00803a9920310016201400fe423a992032000000" ]; then
    pass "a keyword followed by ':' at a line's start is the statement, not a label"
else
    fail "keyword labels: $(od -An -tx1 -v "$D/rw.prg" | tr -d ' \n') $(cat "$D/rw.log")"
fi
printf 'loop: print 1\ngoto loop\n' > "$D/rw2.bas"
printf 'again: on x goto again, end\n' > "$D/rw3.bas"
if ! $B "$D/rw2.bas" -o "$D/rw2.prg" > "$D/rw2.log" 2>&1 &&
   grep -q "'LOOP' is a reserved word, not a label (GOTO" "$D/rw2.log" &&
   ! $B "$D/rw3.bas" -o "$D/rw3.prg" > "$D/rw3.log" 2>&1 && grep -q "'END' is a reserved word" "$D/rw3.log"; then
    pass "GOTO / ON ... GOTO a reserved word is an error"
else
    fail "GOTO a reserved word: $(cat "$D/rw2.log" "$D/rw3.log")"
fi

# --convert=basic8: BASIC 8 (C128), BASIC 7.0 and its @ graphics commands on a 640 x 200 screen
conv8() { printf '%s\n' "$@" > "$D/c8.txt"; $B --convert=basic8 $CONV_OPTS "$D/c8.txt" -o "$D/c8.bas" 2> "$D/c8.err"; }
conv8 '10 @MODE,0:@COLOR,2,13,1:@DOT,5,6,0:@LINE,1,2,0,3,4,0,1' '20 @BOX,10,10,0,50,40,0,1,5,1:@CIRCLE,100,50,0,20,1:@PAINT,1,2,0,0,0' \
      '30 @ARC,320,100,0,50,30,0,360,2,1,0:@CHAR,255,1,2,16,8,4,"X"' '40 A=@PIXEL,5,6,0:L=@DIR$:@TEXT' \
      '50 IF A THEN @ANGLE,0,0,45,0:PRINT "Z"' '60 MODE1=5:@SCALE,0' '70 @ARC,320,100,0,50,30,0,90,10,1,1'
grep -q '^  ya = 15 : yb = 0 : gosub \(l[0-9]*\) : yb = 2 : ya = 13 : pen 0, ya : border 11 : dot 5, 6 : line 1, 2, 3, 4$' "$D/c8.bas" &&
grep -q '^  box 10, 10, 50, 10, 55, 40, 15, 40, 0 : circle 100, 50, 20 : paint 1, 2, 0$' "$D/c8.bas" &&
grep -q '^  ellipse 320, 100, 50, 30 : char 1, 2, 2, 1, 4, "x", \$29000$' "$D/c8.bas" &&
grep -q '^  a = (-(pixel(5, 6) <> yb)) : l = 0 : gosub l[0-9]* : rem @dir\$$' "$D/c8.bas" &&
grep -q '^  if a then : print "z" : rem @angle,0, 0, 45, 0$' "$D/c8.bas" && grep -q '^  mode1 = 5$' "$D/c8.bas" &&
grep -q '^  y[a-z0-9] = 320 : y[a-z0-9] = 100 : y[a-z0-9] = 50 : y[a-z0-9] = 30 : y[a-z0-9] = 0 : y[a-z0-9] = 90 : y[a-z0-9] = 10 : y[a-z0-9] = 1 : gosub l[0-9]*$' "$D/c8.bas" &&
grep -q 'screen 640, 200, 4 : palette 0, 0, 0, 0, 0 : palette 0, 1, 5, 5, 5' "$D/c8.bas" && grep -q 'palette 0, 15, 15, 15, 15 : pen 0, ya : return' "$D/c8.bas" &&
grep -q 'converted from BASIC 8 (C128) by basic45 --convert=basic8' "$D/c8.bas" &&
grep -q 'line 40: @DIR\$ as a function has no BASIC 65 counterpart: 0 instead' "$D/c8.err" && grep -q 'line 50: @ANGLE (3D)' "$D/c8.err" &&
    pass "--convert=basic8: @MODE, @COLOR (the pen, a VDC border colour), @DOT, @LINE, a sheared @BOX, @CIRCLE, @PAINT, a whole @ARC as ELLIPSE and a part by the subroutine, @CHAR (ROM font, sizes, direction), @PIXEL, @TEXT; the rest in a REM; BASIC 8's names only after @" ||
    fail "--convert=basic8: $(cat "$D/c8.bas" "$D/c8.err")"
# a tokenized BASIC 8 program (bytes as BASIC 8 stores them, at $7001): @ and BASIC 7.0's tokens or $FE $30-$5A
python3 - "$D/c8.prg" <<'PYEOF'
import sys
lines = [(10, bytes.fromhex('40FE4D2C303A40FE1A9F2C312C322C332C342C303A40FE3F2C352C362C303A40FE1AA0')),   # @MODE,0:@WINDOWOPEN,1,2,3,4,0:@DOT,5,6,0:@WINDOWCLOSE
         (20, bytes.fromhex('4DB240FE3E2C322C30')),                                                    # M=@MOUSE,2,0
         (30, bytes.fromhex('40E02C3235342C312C322C382C382C322C224122'))]                              # @CHAR,254,1,2,8,8,2,"A"
out = bytearray([0x01, 0x70]); addr = 0x7001
for n, body in lines:
    nxt = addr + 4 + len(body) + 1
    out += bytes([nxt & 255, nxt >> 8, n & 255, n >> 8]) + body + b'\0'; addr = nxt
open(sys.argv[1], 'wb').write(out + b'\0\0')
PYEOF
$B --decompile --convert=basic8 "$D/c8.prg" -o "$D/c8p.bas" 2> "$D/c8p.err"
grep -q 'gosub l[0-9]* : yc = 1 : yd = 2 : ye = 3 : yf = 4 : dot 5 + yc, 6 + yd : yc = 0 : yd = 0 : ye = 640 : yf = 200$' "$D/c8p.bas" &&
grep -q '^  m = 0 : rem @mouse,2, 0$' "$D/c8p.bas" && grep -q '^  char 1 + int(yc / 8), 2 + yd, 1, 1, 2, "a"$' "$D/c8p.bas" &&
! grep -q 'loads at\|unknown token' "$D/c8p.err" && grep -q '@MOUSE as a function' "$D/c8p.err" &&
    pass "--convert=basic8 of a .prg: \$40 \$FE \$4D is @MODE, \$FE \$1A \$9F @WINDOWOPEN (coordinates from the window's corner), \$40 \$E0 @CHAR; loaded at \$7001" ||
    fail "--convert=basic8 .prg: $(cat "$D/c8p.bas" "$D/c8p.err")"

# each dialect's header line names it and its --convert option (#711)
printf '10 print "hi"\n' > "$D/hdr.txt"
hdr_ok=1
for want in "basic7|BASIC 7.0 (C128)" "basic35|BASIC 3.5 (C16 / Plus/4)" "simons|Simons' BASIC (C64)" "tsbneo|TSBneo (C64)" \
            "basic8|BASIC 8 (C128)"; do
    dia=${want%%|*} name=${want#*|}
    $B --convert=$dia "$D/hdr.txt" -o "$D/hdr_$dia.bas" 2> /dev/null &&
    [ "$(grep -m1 "^' converted from" "$D/hdr_$dia.bas")" \
        = "$(printf "%s" "' converted from $name by basic45 --convert=$dia")$(case $dia in simons|tsbneo) ;; *) printf ' (PAL timing)';; esac): check the warnings" ] ||
        { hdr_ok=0; echo "  --convert=$dia header: $(grep -m1 "converted from" "$D/hdr_$dia.bas")"; }
done
[ $hdr_ok = 1 ] && pass "--convert: each dialect's header names it and its option (TSBneo --convert=tsbneo, #711)" ||
    fail "--convert headers: a dialect named wrongly"

# DIRECTORY: DIR and the ECTORY token ($FE $29), as BASIC 65 crunches it; decompiled as it was typed
printf 'start:\n  directory "x*",u8\n' > "$D/dir.bas"
$B "$D/dir.bas" -o "$D/dir.prg" > "$D/dir.log" 2>&1 && [ "$(xxd -s 6 -l 3 -p "$D/dir.prg")" = "eefe29" ] &&
    ! grep -q "not a BASIC 65 keyword" "$D/dir.log" && $B --decompile "$D/dir.prg" -o - 2>/dev/null | grep -q '^  directory "x\*", u8$' &&
    pass "DIRECTORY: DIR + ECTORY (\$EE \$FE \$29), decompiled as DIRECTORY" || fail "DIRECTORY: $(xxd -l 16 -p "$D/dir.prg") $(cat "$D/dir.log")"

# A disk ID with letters (HEADER "N", IAB): the two characters after the I, no
# long variable name to shorten (it was cut to IA); a variable elsewhere still is
printf '10 header "n",iab,u8\n20 header "n",i5x on u8\n30 idx=1:record#1,idx,2\n' > "$D/id.bas"
$B "$D/id.bas" -o "$D/id.prg" > "$D/id.log" 2>&1 && $B --decompile "$D/id.prg" -o "$D/id2.bas" >/dev/null 2>&1 &&
    grep -q 'header "n", *iab, *u8' "$D/id2.bas" && grep -q 'header "n", *i5x on' "$D/id2.bas" && ! grep -q 'idx' "$D/id2.bas" &&
    ! grep -q 'IAB\|I5X' "$D/id.log" && $B --compile "$D/id.bas" -o "$D/idc.prg" > "$D/idc.log" 2>&1 &&
    pass "a disk ID with letters (HEADER \"N\", IAB) is kept, tokenized and compiled" ||
    fail "disk ID: $(cat "$D/id2.bas" "$D/id.log" "$D/idc.log" 2>/dev/null)"

# --compile: the editor's commands and MERGE are errors that say why; a keyword
# out of place is a ?SYNTAX ERROR, not something "not supported yet"
ok=1
for k in list new cont auto renumber edit find change help info monitor; do
    printf '10 %s\n' "$k" > "$D/ed.bas"
    $B --compile "$D/ed.bas" -o "$D/ed.prg" > "$D/ed.log" 2>&1
    grep -q "is a command of BASIC's editor" "$D/ed.log" || { ok=0; echo "  $k: $(cat "$D/ed.log")"; }
done
printf '10 sin\n20 a=goto\n30 dopen 1\n40 merge "x"\n' > "$D/sx.bas"
$B --compile "$D/sx.bas" -o "$D/sx.prg" > "$D/sx.log" 2>&1
grep -q 'SIN does not start a statement (?SYNTAX ERROR)' "$D/sx.log" && grep -q 'GOTO is not a value in an expression (?SYNTAX ERROR)' "$D/sx.log" &&
    grep -q 'DOPEN: not a parameter' "$D/sx.log" && grep -q "MERGE adds lines to the BASIC program's text" "$D/sx.log" &&
    ! grep -q 'not supported' "$D/sx.log" || { ok=0; cat "$D/sx.log"; }
[ $ok = 1 ] && pass "--compile: the editor's commands and MERGE say why; misplaced keywords are syntax errors" ||
    fail "--compile editor commands / syntax errors"

# #582: a #define name is replaced as a whole identifier only (GET_IMPORT in
# GET_IMPORT_PREP was: A=7_PREP)
printf '#define GET_IMPORT 7\n#define GET_IMPORT_PREP 6\na = GET_IMPORT_PREP\nb = GET_IMPORT\nc = (GET_IMPORT)+GET_IMPORT_PREP\n' > "$D/df.bas"
$B "$D/df.bas" -o "$D/df.prg" > "$D/df.log" 2>&1
[ "$(xxd -p -s 2 "$D/df.prg" | tr -d '\n')" = "09200a0041b236001120140042b237001d201e0043b2283729aa36000000" ] &&
    pass "#define: a name is replaced as a whole identifier only (#582)" ||
    fail "#define in a longer identifier: $(xxd -p -s 2 "$D/df.prg" | tr -d '\n') $(cat "$D/df.log")"

# #583: a ' comment after a statement is dropped; a ' in a string, after REM
# or in DATA is kept
printf 'print a '"'"' a comment\nprint "it'"'"'s":rem don'"'"'t\ndata it'"'"'s\nforeman = 2 '"'"'x\nlab: print foreman '"'"' y\n' > "$D/cm.bas"
$B "$D/cm.bas" -o "$D/cm.prg" > "$D/cm.log" 2>&1
$B --decompile "$D/cm.prg" -o "$D/cm2.bas" >/dev/null 2>&1
if ! grep -qi 'comment' "$D/cm.log" "$D/cm2.bas" && grep -q "\"it's\"" "$D/cm2.bas" && grep -qi "rem don't" "$D/cm2.bas" &&
   grep -qi "data it's" "$D/cm2.bas" && [ "$(grep -c "'" "$D/cm2.bas")" = 2 ] && [ "$(grep -c . "$D/cm2.bas")" -ge 5 ]; then
    pass "a ' comment after a statement is dropped; strings, REM and DATA keep theirs (#583)"
else
    fail "trailing ' comment: $(cat "$D/cm2.bas" "$D/cm.log")"
fi

# #584: a proc named exactly like a keyword is an error at its definition;
# a name that only starts with one (directory1, header1, helper) is a proc
ok=1
for k in directory help Header getkey print; do
    printf 'proc %s\n  print "hi"\nendproc\ncall %s\n' "$k" "$k" > "$D/pk.bas"
    out=$($B "$D/pk.bas" -o "$D/pk.prg" 2>&1); rc=$?
    { [ $rc = 1 ] && grep -q "pk.bas:1:1: error: proc name '$k' is a BASIC keyword" <<< "$out"; } || { ok=0; echo "  $k ($rc): $out"; }
done
printf 'proc directory1\n  print "a"\nendproc\nproc header1\n  print "b"\nendproc\nproc helper\n  print "c"\nendproc\ncall directory1\ncall header1\ncall helper\n' > "$D/pn.bas"
out=$($B "$D/pn.bas" -o "$D/pn.prg" 2>&1); rc=$?
{ [ $rc = 0 ] && ! grep -qi 'error\|warning' <<< "$out" &&
  [ "$($B --decompile "$D/pn.prg" -o - 2>/dev/null | grep -ci 'gosub')" = 3 ]; } || { ok=0; echo "  names ($rc): $out"; }
[ $ok = 1 ] && pass "a proc named exactly like a keyword is an error; directory1, header1, helper are procs (#584)" ||
    fail "proc named like a keyword"

# #598: --minify of a structured program: the generated jumps keep their
# blank (goto __endif1), a proc's local label is found, comment lines go
printf "' a comment line\n  x = 1\n' the comment between statements\n  if x then\n    print \"yes\"\n  else\n    print \"no\"\n  endif\n  call ask\n  end\n' a proc's comment\nproc ask\n.again:\n  getkey c\$\n  if c\$ = \"x\" then goto .done\n  goto .again\n.done:\nendproc\n" > "$D/mn.bas"
out=$($B --strict --minify "$D/mn.bas" -o "$D/mn.prg" 2>&1); rc=$?
$B --strict "$D/mn.bas" -o "$D/mp.prg" >/dev/null 2>&1
if [ $rc = 0 ] && ! grep -qi 'warning\|error' <<< "$out" &&
   [ "$(stat -c %s "$D/mn.prg")" -lt "$(stat -c %s "$D/mp.prg")" ] &&
   [ "$($B --decompile "$D/mn.prg" -o - 2>/dev/null | grep -ci 'comment')" = 0 ] &&
   [ "$($B --decompile "$D/mn.prg" -o - 2>/dev/null | grep -ci 'goto l\|then l\|then goto l')" -ge 3 ]; then
    pass "--minify: block IF, proc and local labels keep their jumps; comment lines go (#598)"
else
    fail "--minify of a structured program ($rc): $out $($B --decompile "$D/mn.prg" -o - 2>/dev/null)"
fi

# #603: --minify keeps the blank between a keyword and a hex or binary
# constant ("poke $d1" joined was the variable poke$)
printf 'poke $d1, 0 : print peek($d1)\nsys $ff81\npoke 1, %%101\n' > "$D/mh.bas"
out=$($B --strict --minify "$D/mh.bas" -o "$D/mh.prg" 2>&1); rc=$?
dec=$($B --decompile "$D/mh.prg" -o - 2>/dev/null)
if [ $rc = 0 ] && ! grep -qi 'warning\|error' <<< "$out" &&
   grep -q 'poke \$d1, 0' <<< "$dec" && grep -q 'sys \$ff81' <<< "$dec" && grep -q 'poke 1, %101' <<< "$dec"; then
    pass "--minify: a hex or binary constant after a keyword stays one (#603)"
else
    fail "--minify of 'poke \$d1' ($rc): $out $dec"
fi

# #605: no GOTO around a proc nothing can run into (after END, after
# another proc); one where the line before runs on. A case's statements on
# its test's line; a case with an IF in it keeps its own lines
printf 'start:\n  i = 2\n  on i gosub one, two\n  end\nproc one\n  print "one"\nendproc\nproc two\n  print "two"\nendproc\n' > "$D/pg.bas"
printf '  call one\n  call two\n  print "on"\nproc one\n  print "one"\nendproc\nproc two\n  print "two"\nendproc\n  print "after"\n' > "$D/ph.bas"
printf '  select k\n    case 133\n      sys 4660, 7\n    case 1, 2\n      print "b"\n    case 5 to 9\n      if x then print "r"\n    case is > 200\n      goto fin\n    case else\n      print "o"\n  endselect\nfin:\n  print "done"\n' > "$D/sc.bas"
$B --strict "$D/pg.bas" -o "$D/pg.prg" >/dev/null 2>&1; r1=$?
$B --strict "$D/ph.bas" -o "$D/ph.prg" >/dev/null 2>&1; r2=$?
$B "$D/sc.bas" -o "$D/sc.prg" >/dev/null 2>&1; r3=$?
pg=$($B --decompile "$D/pg.prg" -o - 2>/dev/null)
ph=$($B --decompile "$D/ph.prg" -o - 2>/dev/null)
sc=$($B --decompile "$D/sc.prg" -o - 2>/dev/null)
if [ $r1 = 0 ] && [ $r2 = 0 ] && [ $r3 = 0 ] &&
   [ "$(grep -ci 'goto' <<< "$pg")" = 0 ] && [ "$(grep -ci 'goto' <<< "$ph")" = 1 ] &&
   grep -q 'if zs = 133 then sys 4660, 7 : goto l' <<< "$sc" &&
   grep -q 'if zs = 1 or zs = 2 then print "b" : goto l' <<< "$sc" &&
   grep -q 'if zs < 5 or zs > 9 then l' <<< "$sc" &&
   grep -q 'if zs > 200 then goto l[0-9]*$' <<< "$sc" && ! grep -q '= 0 then' <<< "$sc"; then
    pass "proc: no GOTO around it where nothing runs into it; select: a case on its test's line (#605)"
else
    fail "proc / select lowering ($r1 $r2 $r3): $pg / $ph / $sc"
fi

# #605: a block IF jumps to its else on the opposite comparison; a condition
# with AND keeps (...)=0
printf '  if a < b then\n    print 1\n  else\n    print 2\n  endif\n  if x then\n    print 3\n  endif\n  if a$ = "x=1" and b then\n    print 4\n  endif\n  if (a = 1) = (b = 2) then\n    print 5\n  endif\n' > "$D/bi.bas"
$B "$D/bi.bas" -o "$D/bi.prg" >/dev/null 2>&1; rc=$?
bi=$($B --decompile "$D/bi.prg" -o - 2>/dev/null)
if [ $rc = 0 ] && grep -q 'if a >= b then l' <<< "$bi" && grep -q 'if x = 0 then l' <<< "$bi" &&
   grep -q 'if(a\$ = "x=1" and b) = 0 then l' <<< "$bi" && grep -q 'if(a = 1) <>(b = 2) then l' <<< "$bi"; then
    pass "block IF: the opposite comparison; (cond)=0 for a condition with AND (#605)"
else
    fail "block IF lowering ($rc): $bi"
fi
# #595: --map writes ln45's -M map of a --link and of a --compile build, with
# the calls through pointers and the base-page blocks the -vv log has; a
# warning when nothing is linked (PRINT USING: basic_using.c, a C module with
# base-page blocks; the string run time is in assembly)
printf 'unsigned char g;\nstatic void a(void){g++;}\nstatic void b(void){g--;}\nvoid (*tab[2])(void)={a,b};\nunsigned int twice(unsigned int x){ unsigned char *p=&g; tab[x&1](); return x*2+*p; }\n' > "$D/mapf.c"
printf 'sys {twice}, 5\nprint 1\n' > "$D/mapl.bas"
printf 'a = 3\nprint using "##.#"; a*2\n' > "$D/mapc.bas"
ok=1
$B "$D/mapl.bas" --link "$D/mapf.c" --map "$D/mapl.map" -o "$D/mapl.prg" > "$D/mapl.log" 2>&1 || ok=0
grep -q '^Symbols (by address)' "$D/mapl.map" 2>/dev/null && grep -q ' _twice ' "$D/mapl.map" &&
    grep -q '^Calls Through Pointers' "$D/mapl.map" && grep -q '^  call graph: _twice calls through pointers ' "$D/mapl.map" &&
    grep -q '^Base Page' "$D/mapl.map" && grep -Eq '^  base-page block: _twice at \$[0-9a-f]+ len [0-9]+$' "$D/mapl.map" || ok=0
$B --compile "$D/mapc.bas" --map "$D/mapc.map" -o "$D/mapc.prg" > "$D/mapc.log" 2>&1 || ok=0
grep -q '^Symbols (by address)' "$D/mapc.map" 2>/dev/null && grep -q '^Call Graph' "$D/mapc.map" &&
    grep -Eq '^  base-page block: [A-Za-z_0-9]+ at \$[0-9a-f]+ len [0-9]+$' "$D/mapc.map" || ok=0
out=$($B "$D/mapc.bas" --map "$D/mapn.map" -o "$D/mapn.prg" 2>&1)
grep -q -- '--map needs --link or --compile' <<< "$out" && [ ! -e "$D/mapn.map" ] || ok=0
[ $ok = 1 ] && pass "--map: ln45's map of a --link and a --compile build, pointer calls and base-page blocks (#595)" ||
    fail "--map: $(cat "$D/mapl.log" "$D/mapc.log" 2>/dev/null) $out"

# #636: --validate with --link / asm blocks still checks the program (an
# undefined label: status 1); an option without its value, --increment 0:
# errors; --help lists --preserve-spaces and the --compile / --convert forms
printf 'sys {twice}, 5\ngoto nowhere\n' > "$D/v1.bas"
printf 'unsigned int twice(unsigned int x){ return x*2; }\n' > "$D/v1.c"
printf 'asm\n  rts\nendasm\ngoto nowhere\n' > "$D/v2.bas"
printf 'sys {twice}, 5\nprint 1\n' > "$D/v3.bas"
ok=1; why=""
out=$($B --validate "$D/v1.bas" --link "$D/v1.c" 2>&1); [ $? = 1 ] && grep -q "Undefined label: NOWHERE" <<< "$out" || { ok=0; why+=" [link: $out]"; }
out=$($B --validate "$D/v2.bas" 2>&1); [ $? = 1 ] && grep -q "Undefined label: NOWHERE" <<< "$out" || { ok=0; why+=" [asm: $out]"; }
out=$($B --validate --compile "$D/v1.bas" --link "$D/v1.c" 2>&1); [ $? = 1 ] || { ok=0; why+=" [compile link: $out]"; }
out=$($B --validate "$D/v3.bas" --link "$D/v1.c" 2>&1); [ $? = 0 ] || { ok=0; why+=" [good: $out]"; }
[ $ok = 1 ] && pass "--validate with --link and asm blocks checks the labels (#636)" || fail "--validate:$why"
ok=1; why=""
for o in -o --increment --link --symbols --encoding; do
    out=$($B "$D/v3.bas" $o 2>&1); [ $? = 1 ] && grep -q -- "$o needs a value" <<< "$out" || { ok=0; why+=" [$o: $out]"; }
done
for v in 0 -5 x 64000; do
    out=$($B "$D/v3.bas" --increment $v -o "$D/v3.prg" 2>&1); [ $? = 1 ] && grep -q -- "--increment takes" <<< "$out" || { ok=0; why+=" [--increment $v: $out]"; }
done
$B "$D/v3.bas" --increment 5 -o "$D/v3.prg" > /dev/null 2>&1 || { ok=0; why+=" [--increment 5]"; }
[ $ok = 1 ] && pass "an option without its value and a bad --increment are errors (#636)" || fail "option values:$why"
h=$($B --help)
if grep -q -- '--preserve-spaces' <<< "$h" && grep -q -- '--compile \[options\]' <<< "$h" && grep -q -- '--convert=<dialect> <' <<< "$h" &&
   grep -A1 -- '--speed, -Ospeed' <<< "$h" | grep -q 'faster than the ROM'; then
    pass "--help: --preserve-spaces, the --compile / --convert forms, --speed's note (#636)"
else
    fail "--help: $h"
fi

# #636: a label named like a keyword that cannot start a statement alone
# (go:) is an error naming it; end: print is still END and PRINT
printf 'go: print 1\ngoto go\n' > "$D/kl.bas"
out=$($B "$D/kl.bas" -o "$D/kl.prg" 2>&1); rc=$?
printf 'print 1\nend: print 2\n' > "$D/kl2.bas"
if [ $rc = 1 ] && grep -q "label 'go' is the BASIC keyword GO" <<< "$out" &&
   $B "$D/kl2.bas" -o "$D/kl2.prg" > /dev/null 2>&1 &&
   [ "$($B --decompile "$D/kl2.prg" -o - 2>/dev/null | grep -c 'end : print 2\|end: print 2\|end:print 2')" = 1 ]; then
    pass "a label named like a keyword (go:) is an error; end: print stays two statements (#636)"
else
    fail "keyword label ($rc): $out / $($B --decompile "$D/kl2.prg" -o - 2>&1)"
fi

# #636: --compile: DT$ is the date (__bas_dts), not a variable; DTX$ kept
# (--keep-names: the interpreter's DT$) is refused
printf 'a$ = dt$\nprint len(a$)\n' > "$D/dt.bas"
printf 'print dtx$\n' > "$D/dt2.bas"
$B --compile "$D/dt.bas" --asm "$D/dt.s45" -o "$D/dt.prg" > "$D/dt.log" 2>&1; rc=$?
out=$($B --compile --keep-names "$D/dt2.bas" -o "$D/dt2.prg" 2>&1); rc2=$?
if [ $rc = 0 ] && grep -q '__bas_dts' "$D/dt.s45" && ! grep -q '__bv_DT_s' "$D/dt.s45" &&
   [ $rc2 = 1 ] && grep -q 'DTX\$ (a reserved variable)' <<< "$out"; then
    pass "--compile: DT\$ reads the date (#636)"
else
    fail "--compile DT\$ ($rc $rc2): $(cat "$D/dt.log") $out"
fi

# #636: --compile --link: {name+1} is substituted (the symbol's address + 1)
printf 'unsigned char buf[4];\nunsigned char *bp(void){ return buf; }\n' > "$D/se.c"
printf 'poke {buf+1}, 7\nprint peek({buf%%3}), {buf/2}, {2*buf}\n' > "$D/se.bas"
if $B --compile "$D/se.bas" --link "$D/se.c" --asm "$D/se.s45" -o "$D/se.prg" > "$D/se.log" 2>&1 &&
   grep -q '_buf' "$D/se.s45" && ! grep -q '{buf' "$D/se.s45"; then
    pass "--compile --link: {name+1} and the other symbol operations are substituted (#636)"
else
    fail "--compile --link {name+1}: $(cat "$D/se.log")"
fi

# #577: compiled procs' misuse is an error at its source line
mp_ok=1
mp_case() {   # file text, expected message
    printf "$1" > "$D/mp.bas"
    out=$($B "$D/mp.bas" -o "$D/mp.prg" 2>&1)
    { [ $? = 1 ] && grep -qF "$2" <<< "$out"; } || { mp_ok=0; echo "  $2: $out"; }
}
mp_case 'call f(1) to a&\nend\nproc f(x) compiled\n  return x\nendproc\n' "mp.bas:1:1: error: call f to a&: a compiled proc's result is a float, % or string variable"
mp_case 'call f(1)\nend\nproc f(x&) compiled\n  print x&\nendproc\n' "mp.bas:3:1: error: proc f compiled: a parameter is a float, % or string variable, or an array (a()), not x&"
mp_case 'call f(1)\nend\nproc f(x) compiled\n  call g\nendproc\nproc g\nendproc\n' "mp.bas:4:1: error: call g: a compiled proc calls only compiled procs (g is not one)"
mp_case 'return 5\n' "mp.bas:1:1: error: return 5: a value is returned only from a proc"
mp_case 'call g to v\nend\n' "mp.bas:1:1: error: call g to v: no proc g"
# #682: a jump out of a compiled proc, or into another
mp_case 'top: call f(1)\nend\nproc f(x) compiled\n  if x then goto top\nendproc\n' "mp.bas:4:1: error: goto top: a compiled proc cannot jump out of itself (top is not one of its labels)"
mp_case 'call f(1)\nend\nproc f(x) compiled\n  on x gosub .a, g2\n.a: return\nendproc\nproc g compiled\ng2: return 1\nendproc\n' "mp.bas:4:1: error: gosub g2: g2 is in compiled proc g (call it)"
# #683: array parameters
mp_case 'dim w%%(3)\ncall f(w%%())\nend\nproc f(a()) compiled\n  a(0) = 1\nendproc\n' "mp.bas:2:1: error: call f: argument 1 (w%()) for a(): an array of its type, as a()"
mp_case 'end\nproc f(a$()) compiled\nendproc\n' "mp.bas:2:1: error: proc f compiled: a\$(): string arrays are not passed yet"
mp_case 'end\nproc f(a())\nendproc\n' "mp.bas:2:1: error: proc f: a(): an array is a parameter of a compiled proc only"
printf 'dim v(3)\ncall f(v())\nend\nproc f(a()) compiled\n  a(0) = 1\nendproc\n' > "$D/ma.bas"
out=$($B --compile "$D/ma.bas" -o "$D/ma.prg" 2>&1)
{ [ $? = 1 ] && grep -qF "not with --compile, which compiles the whole program" <<< "$out"; } || { mp_ok=0; echo "  --compile array: $out"; }
# the call: one WPOKE of the block, the SYS, the error exit
printf 'call f(2, 3) to v%%\nprint v%%\nend\nproc f(x, y%%) compiled\n  return x * y%%\nendproc\n' > "$D/mq.bas"
if $B -v "$D/mq.bas" -o "$D/mq.prg" > "$D/mq.log" 2>&1; then :; else mp_ok=0; cat "$D/mq.log"; fi
if [ $mp_ok = 1 ]; then
    pass "compiled procs: misuse reported at its line (#577, #682, #683)"
else
    fail "compiled procs' errors (#577, #682, #683)"
fi

# #739: --format: NEXT / LOOP later in a line close their block; next j, i closes two
printf 'for i=1 to 3\nprint i : next\nprint 1\nfor i=1 to 2\nfor j=1 to 2\nprint j\nnext j, i\nprint 2\ndo\nprint 3 : loop until x\nprint 4\n' > "$D/f739.bas"
printf '  for i=1 to 3\n    print i : next\n  print 1\n  for i=1 to 2\n    for j=1 to 2\n      print j\n    next j, i\n  print 2\n  do\n    print 3 : loop until x\n  print 4\n' > "$D/f739.want"
$B --format "$D/f739.bas" > "$D/f739.out" 2>&1 && diff -q "$D/f739.out" "$D/f739.want" >/dev/null &&
    pass "--format: a NEXT / LOOP after ':' closes its block, next j, i two (#739)" ||
    fail "--format NEXT later in a line (#739): $(diff "$D/f739.want" "$D/f739.out" | head -5)"

echo "basic45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
