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
grep -q 'background 1 : border 12 : [a-z][a-z0-9](1) = 10 : [a-z][a-z0-9](1) = val(mid\$("00010203040506070809131003140613", (c) \* 2 - 1, 2))' "$D/c35.bas" &&
grep -q 'print "{light-green}{flash}x{flash-off}"; chr\$(31) : vol 15' "$D/c35.bas" &&
grep -q 'line 10: RLUM has no BASIC 65 counterpart' "$D/c35.err" && grep -q 'luminance of an expression' "$D/c35.err" &&
grep -q 'converted from BASIC 3.5' "$D/c35.bas" &&
    pass "--convert=basic35: TED colours and luminances (white's greys, pink -> light red, an expression by a table), colour codes and flash in strings, CHR\$ codes, VOL 0-8 -> 0-15, RLUM warned" ||
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
$B --convert=basic35 "$D/p4.prg" -o "$D/p4.bas" 2> "$D/p4.err" && grep -q 'a = rlum(1)' "$D/p4.bas" && ! grep -q 'loads at' "$D/p4.err" &&
    pass "--convert=basic35: a .prg read with BASIC 3.5's tables (\$CE is RLUM)" || fail "--convert=basic35 .prg: $(cat "$D/p4.bas" "$D/p4.err")"

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
printf '10 print 1\n20 input a\n' > "$D/cu.bas"
if ! $B --compile "$D/cu.bas" -o "$D/cu.prg" > "$D/cu.log" 2>&1 && grep -q 'cu.bas:2:1: (line 20): error: INPUT is not supported by --compile yet' "$D/cu.log"; then
    pass "--compile: a statement outside the subset (INPUT) is an error at its line"
else
    fail "--compile INPUT: $(cat "$D/cu.log")"
fi
printf '10 goto 30\n' > "$D/cg.bas"
! $B --compile "$D/cg.bas" -o "$D/cg.prg" > "$D/cg.log" 2>&1 && grep -q 'line 30 does not exist' "$D/cg.log" &&
    pass "--compile: GOTO to a missing line is an error" || fail "--compile GOTO 30: $(cat "$D/cg.log")"
# a tokenized .prg compiles too
$B "$D/c.bas" -o "$D/ct.prg" >/dev/null 2>&1
$B --compile "$D/ct.prg" -o "$D/ct2.prg" > "$D/ct.log" 2>&1 && cmp -s "$D/ct2.prg" "$D/c.prg" &&
    pass "--compile: a tokenized .prg in, the same machine code out" || fail "--compile .prg: $(cat "$D/ct.log")"

echo "basic45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
