#!/bin/bash
# test_font45.sh — font45 (#466): the readers (hand-drawn text, BDF, PSF1,
# PSF2, a C64 charset) and writers (an 8x8 charset, <font.h>'s fonts at 1,
# 2 and 4 bpp; C, assembly, binary), against golden files and bytes worked
# out by hand; the variants, the range and the errors.
cd "$(dirname "$0")/../.." || exit 1
F=./bin/font45
T=src/test-resources/font45
D=build/test/font45
mkdir -p "$D"
passed=0; failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }
hexof() { xxd -p "$1" | tr -d '\n'; }

# the hand-drawn font, proportional 1 bpp: the golden file (checked by hand)
$F convert $T/tiny.txt --bpp 1 --encoding ascii --proportional -o $D/tiny.bin >/dev/null && cmp -s $D/tiny.bin $T/tiny_p1.golden &&
    pass "hand-drawn glyphs: a proportional 1 bpp font (the header, widths 2/4/4/1, A B I's rows)" ||
    fail "hand-drawn glyphs: $(hexof $D/tiny.bin | head -c 80)"
# BDF: ascent 3 + descent 1, A's BBX on the baseline, j's below it
$F convert $T/tiny.bdf --bpp 1 --encoding ascii --proportional -o $D/bdf.bin >/dev/null && cmp -s $D/bdf.bin $T/tinybdf_p1.golden &&
    pass "BDF: the cell from FONT_ASCENT / DESCENT, each BBX placed by its offset (a descender)" ||
    fail "BDF: $(hexof $D/bdf.bin | head -c 80)"
# 2 bpp: A's first row 0110 -> 00 11 11 00 -> $3C, 4 pixels one byte; fixed (no width table)
$F convert $T/tiny.txt --bpp 2 --encoding ascii --range 65-65 -o $D/a2.bin >/dev/null &&
    [ "$(hexof $D/a2.bin)" = "4634010505414102000401003cc3ffc3c3" ] &&
    pass "2 bpp, fixed: coverage as levels 0-3, the leftmost pixel in the top bits" ||
    fail "2 bpp: $(hexof $D/a2.bin)"
# 4 bpp: a full pixel is 15
$F convert $T/tiny.txt --bpp 4 --encoding ascii --range 73-73 -o $D/i4.bin >/dev/null &&
    [ "$(hexof $D/i4.bin)" = "463401050549490400010100f0f0f0f0f0" ] &&
    pass "4 bpp: a full pixel 15 (I's one column)" || fail "4 bpp: $(hexof $D/i4.bin)"
# the 8x8 charset: ASCII order, A at 65; screen-code order, a (none) at 1 and A at 65,
# 128-255 the inverse
$F convert $T/tiny.txt --charset ascii -o $D/cs.bin >/dev/null &&
    [ "$(xxd -s $((65*8)) -l 8 -p $D/cs.bin)" = "6090f09090000000" ] && [ "$(stat -c %s $D/cs.bin)" = 2048 ] &&
    pass "an 8x8 charset in ASCII order (2048 bytes, A at 65)" || fail "charset ascii: $(xxd -s $((65*8)) -l 8 -p $D/cs.bin)"
$F convert $T/tiny.txt --charset screencode -o $D/sc.bin >/dev/null &&
    [ "$(xxd -s $((65*8)) -l 8 -p $D/sc.bin)" = "6090f09090000000" ] &&
    [ "$(xxd -s $((193*8)) -l 8 -p $D/sc.bin)" = "9f6f0f6f6fffffff" ] &&
    pass "screen-code order: A at 65, its inverse at 193" || fail "charset screencode: $(xxd -s $((193*8)) -l 8 -p $D/sc.bin)"
$F convert $T/tiny.txt --charset petscii --count 512 -o $D/pe.bin >/dev/null &&
    [ "$(stat -c %s $D/pe.bin)" = 4096 ] && [ "$(xxd -s $((193*8)) -l 8 -p $D/pe.bin)" = "6090f09090000000" ] &&
    pass "PETSCII order, 512 characters: A at 193 (the shifted letters)" || fail "charset petscii: $(xxd -s $((193*8)) -l 8 -p $D/pe.bin)"
# a C64 charset read back: the same 2048 bytes (and with a load address)
$F convert $D/cs.bin --from ascii --charset ascii -o $D/cs2.bin >/dev/null && cmp -s $D/cs.bin $D/cs2.bin &&
    { printf '\x00\x30'; cat $D/cs.bin; } > $D/cs_la.64c && $F convert $D/cs_la.64c --from ascii --charset ascii -o $D/cs3.bin >/dev/null && cmp -s $D/cs.bin $D/cs3.bin &&
    pass "a C64 charset read back, with or without its load address" || fail "charset read back"
# PSF1 and PSF2: two glyphs each
python3 - "$D" <<'PYEOF'
import sys, struct
d = sys.argv[1]
g0 = bytes([0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00])
psf1 = bytes([0x36, 0x04, 0, 8]) + bytes(8 * 65) + g0 + bytes(8 * 190)
open(d + '/t1.psf', 'wb').write(psf1)
hdr = struct.pack('<8I', 0x864ab572, 0, 32, 0, 66, 16, 8, 10)   # 10 wide: 2 bytes a row
rows = b''.join(bytes([0xff, 0xc0]) if y == 0 else bytes([0x80, 0x40]) for y in range(8))
psf2 = hdr + bytes(16 * 65) + rows
open(d + '/t2.psf', 'wb').write(psf2)
PYEOF
$F convert $D/t1.psf --from ascii --charset ascii -o $D/p1.bin >/dev/null && [ "$(xxd -s $((65*8)) -l 8 -p $D/p1.bin)" = "182442427e424200" ] &&
    pass "PSF1: 256 glyphs, 8 wide" || fail "PSF1: $(xxd -s $((65*8)) -l 8 -p $D/p1.bin)"
$F convert $D/t2.psf --bpp 1 --encoding ascii --range 65-65 -o $D/p2.bin >/dev/null && [ "$(hexof $D/p2.bin)" = "4634010807414101000a0200ffc08040804080408040804080408040" ] &&
    pass "PSF2: the header's height, width and glyph size (10 pixels wide, 2 bytes a row)" || fail "PSF2: $(hexof $D/p2.bin)"
# the variants: bold widens by one, outline by two
$F convert $T/tiny.txt --bpp 1 --encoding ascii --proportional --bold --range 73-73 -o $D/ib.bin >/dev/null &&
    [ "$(xxd -s 12 -l 1 -p $D/ib.bin)" = "02" ] && [ "$(xxd -s 13 -l 1 -p $D/ib.bin)" = "c0" ] &&
    $F convert $T/tiny.txt --bpp 1 --encoding ascii --proportional --outline --range 73-73 -o $D/io.bin >/dev/null &&
    [ "$(xxd -s 12 -l 1 -p $D/io.bin)" = "03" ] && [ "$(xxd -s 13 -l 1 -p $D/io.bin)" = "a0" ] &&
    pass "--bold (I two wide), --outline (I's sides, the glyph cleared)" || fail "variants: $(hexof $D/ib.bin) $(hexof $D/io.bin)"
# --map: a code's glyph under another
$F convert $T/tiny.txt --bpp 1 --encoding ascii --map 65=90 --range 90-90 -o $D/m.bin >/dev/null && [ "$(xxd -s 12 -l 5 -p $D/m.bin)" = "6090f09090" ] &&
    pass "--map 65=90: A's glyph at Z" || fail "--map: $(hexof $D/m.bin)"
# C and assembly
$F convert $T/tiny.txt --bpp 1 --encoding ascii --proportional -o $D/tf.c >/dev/null &&
    grep -q 'const unsigned char tf\[264\] = {' $D/tf.c && grep -q '#define TF_HEIGHT 5' $D/tf.h && grep -q 'extern const unsigned char tf\[264\];' $D/tf.h &&
    $F convert $T/tiny.txt --bpp 1 --encoding ascii --proportional --name tiny -o $D/tf.s45 >/dev/null && grep -q '^tiny:$' $D/tf.s45 && grep -q '    .byte \$46, \$34' $D/tf.s45 &&
    pass "C (an array, a header with its macros) and ca45 assembly (--name)" || fail "C / assembly"
$F convert $T/tiny.txt --charset ascii --load-at 0x3000 -o $D/la.prg >/dev/null && [ "$(xxd -l 2 -p $D/la.prg)" = "0030" ] && [ "$(stat -c %s $D/la.prg)" = 2050 ] &&
    pass "--load-at: a two-byte load address in front" || fail "--load-at"
# the default encoding, PETSCII (cc45's literals): A at 193, B 194, I 201, a space 32
$F convert $T/tiny.txt --bpp 1 --proportional -o $D/pet.bin >/dev/null &&
    [ "$(xxd -s 5 -l 2 -p $D/pet.bin)" = "20c9" ] && [ "$(xxd -s $((12 + 193 - 32)) -l 2 -p $D/pet.bin)" = "0404" ] &&
    [ "$(xxd -s $((12 + 201 - 32)) -l 1 -p $D/pet.bin)" = "01" ] && [ "$(xxd -s $((12 + 65 - 32)) -l 1 -p $D/pet.bin)" = "00" ] &&
    $F convert $T/tiny.txt --bpp 1 --encoding screencode -o $D/scr.bin >/dev/null && [ "$(xxd -s 5 -l 2 -p $D/scr.bin)" = "2049" ] &&
    pass "--encoding: PETSCII by default (A at 193, I at 201), screencode (A at 65)" || fail "--encoding: $(xxd -l 8 -p $D/pet.bin) $(xxd -l 8 -p $D/scr.bin)"
# a C64 charset in screen-code order (the default --from): screen code 1 is a, 65 is A
$F convert $D/sc.bin --charset ascii -o $D/sc2.bin >/dev/null && [ "$(xxd -s $((65*8)) -l 8 -p $D/sc2.bin)" = "6090f09090000000" ] &&
    $F convert $D/sc.bin --charset screencode -o $D/sc3.bin >/dev/null && cmp -s $D/sc.bin $D/sc3.bin &&
    pass "--from screencode (the default): a C64 charset decoded, and written back the same" || fail "--from screencode"

# list and the errors
$F list $T/tiny.bdf | grep -q 'glyphs:   2 (codes 65-106)' && pass "list: the glyphs and their codes" || fail "list"
printf 'A: 01z0\n' > $D/bad.txt
! $F convert $D/bad.txt --bpp 1 --encoding ascii -o $D/x.bin 2>$D/e1 && grep -q "'z' in a row" $D/e1 &&
    ! $F convert $D/cs.bin --charset nonsense -o $D/x.bin 2>$D/e2 && grep -q 'ascii, screencode or petscii' $D/e2 &&
    ! $F convert $D/t1.psf --bpp 3 --encoding ascii -o $D/x.bin 2>$D/e3 && grep -q -- '--bpp is 1, 2 or 4' $D/e3 &&
    ! $F convert /nonexistent.bdf -o $D/x.bin 2>$D/e4 && grep -q 'cannot read' $D/e4 &&
    pass "errors: a bad row character, an unknown order, a bpp, a missing file" || fail "errors: $(cat $D/e1 $D/e2 $D/e3 $D/e4)"

echo "font45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
