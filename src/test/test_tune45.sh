#!/bin/bash
# test_tune45.sh — tune45 (no emulator needed; the model against the real
# player is test_xemu.sh's): the text format and its errors, the player's
# data, list, create (musicgen's melody checks), import (a MIDI file made
# here), play into a WAV
cd "$(dirname "$0")/../.." || exit 1
T=build/test/tune45
mkdir -p $T
TUNE=./bin/tune45
passed=0
failed=0
pass() { passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# the player's data: header, instruments (those used, renumbered), streams
cat > $T/a.txt <<'EOF'
; a comment
instrument unused saw 00 f0 0800 0 0
instrument lead pulse 09 a6 0600 1 2 pwm=-3 vdelay=4 slide=5
tune t 2.5 once
v1: @lead c4:2 r:2 eb4:1 ~:3 arp:4,7 g4:300 \
    arp:off c#4:4 r:4
v2: r:8 c3:308
EOF
$TUNE convert $T/a.txt -o $T/a.c >/dev/null 2>&1 && pass || fail "convert a.txt"
bytes=$(sed -n '/tune_t\[/,/};/p' $T/a.c | tr -d ' \n' | sed 's/.*{//; s/,}.*//')
#  header: once, 2 voices, fpt 2.5 = $0280, 2 instruments of 17 bytes at 12
#  (v2 plays before naming one: the file's first, "unused", is instrument 0;
#  lead 1), the streams at 46 and 71; g4:300 is 255 and a tie of 45
want="0,2,128,2,2,17,12,0,46,0,71,0"
want="$want,32,0,240,0,8,0,0,0,0,0,0,0,0,0,0,0,0,64,9,166,0,6,1,2,253,4,5,0,0,0,0,0,0,0"
want="$want,129,1,60,2,128,2,63,1,130,3,131,4,7,67,255,130,45,131,0,0,61,4,128,4,255"
want="$want,128,8,48,255,130,53,255"
[ "$bytes" = "$want" ] && pass || fail "the blob: $bytes (expected $want)"
# hard restart, filter and a wave table: the instrument's last 7 bytes (hr
# 2, LP+BP ($30) with resonance 9, cutoff $3A0, sweep -6, the table at 27),
# then the table: 3 steps, looping to the second; noise, pulse -12, -24
cat > $T/f.txt <<'EOF'
wavetable drum noise pulse:-12 pulse:-24 loop 2
instrument snare pulse 00 f0 0800 0 0 hr=2 filter=lp+bp res=9 cutoff=3a0 fsweep=-6 table=drum
tune d 1 once
v1: @snare c4:2
EOF
$TUNE convert $T/f.txt -o $T/f.c >/dev/null 2>&1 &&
    [ "$(sed -n '/tune_d\[/,/};/p' $T/f.c | tr -d ' \n' | sed 's/.*{//; s/,}.*//')" = \
      "0,1,0,1,1,17,10,0,35,0,64,0,240,0,8,0,0,0,0,0,2,57,160,3,250,27,0,3,1,128,0,64,244,64,232,129,0,60,2,255" ] &&
    pass || fail "the blob: hard restart, filter, wave table"
grep -q "#define TUNE_T 0" $T/a.h && grep -q "extern const unsigned char tune_t\[" $T/a.h && pass || fail "the header"

# --bin: [tunes][offsets] then the blobs; --asm
$TUNE convert $T/a.txt --bin -o $T/a.bin >/dev/null 2>&1 &&
    [ "$(od -An -tu1 -N3 $T/a.bin | tr -s ' ')" = " 1 3 0" ] && grep -q "TUNE_T_AT 0x0003" $T/a.h && pass || fail "--bin"
$TUNE convert $T/a.txt --asm -o $T/a.s45 >/dev/null 2>&1 && ./bin/ca45 -c $T/a.s45 -o $T/a.o45 >/dev/null 2>&1 &&
    pass || fail "--asm does not assemble"

# a glide: $84 s k before the note (default speed 3); a tune's ticks once through
printf 'instrument a pulse 09 a6 0600 0 0\ntune g 1 loop\nv1: c4^3:4 g4^-5,1:4 r:2\n' > $T/g.txt
$TUNE convert $T/g.txt -o $T/g.c >/dev/null 2>&1 &&
    sed -n '/tune_g\[/,/};/p' $T/g.c | tr -d ' \n' | grep -q '132,3,3,60,4,132,251,1,67,4,128,2,254' &&
    grep -q "#define TUNE_G_TICKS 10" $T/g.h && pass || fail "glide bytes / TUNE_G_TICKS"

# errors, with file and line
err() {   # $1 the text, $2 a part of the message
    printf '%s\n' "$1" > $T/e.txt
    out=$($TUNE convert $T/e.txt -o $T/e.c 2>&1); rc=$?
    [ $rc -ne 0 ] && echo "$out" | grep -q "$2" && pass || fail "no error '$2' for: $1 ($out)"
}
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @b c4:1' "e.txt:3: error: unknown instrument b"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @a h4:1' "not a note"
err $'instrument a pulse 00 f0 0800 0 0 res=4' "need filter="
err $'instrument a pulse 00 f0 0800 0 0 table=none' "no wave table"
err $'wavetable w pulse loop 2' "loop <step>"
err $'instrument a pulse 00 f0 0800 0 0 filter=xx' "filter=lp"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @a c4:2\nv2: @a c4:3' "the voices are not as long (ticks: v1 2, v2 3)"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once 4\nv1: @a c4:3 | c4:4' "bar 1 is 3 ticks, not 4"
err $'instrument a wobble 09 a6 0600 1 1' "not a waveform"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv2: @a c4:1' "v1: expected"
err $'tune 1x 5 once\ninstrument a pulse 09 a6 0600 1 1\nv1: @a c4:1' "C identifier"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @a c4^0:1' "a glide is note^semitones"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @a c4^3,8:1' "speed 1-7"

# pfest's format as it is (its first tune's lines)
cat > $T/pfest.txt <<'EOF'
instrument calliope pulse 09 a6 0600 1 1
instrument bass     tri   0a 00 0000 0 1
instrument stab     pulse 05 00 0800 0 0
tune title 6 loop
v1: @calliope g4:4 c5:4 e5:4 | g5:8 e5:4 | f5:4 e5:4 d5:4 | c5:8 r:4
v2: @bass c3:4 r:8 | c3:4 r:8 | g2:4 r:8 | c3:4 r:8
v3: @stab r:4 e4:3 r:1 g4:3 r:1 | r:4 e4:3 r:1 g4:3 r:1 | r:4 d4:3 r:1 f4:3 r:1 | r:4 e4:3 r:1 g4:3 r:1
EOF
out=$($TUNE list $T/pfest.txt 2>&1)
echo "$out" | grep -qE "^title +3 voices +[0-9]+ bytes +48 ticks +5.8 s +6.00 frames/tick +loop" && pass || fail "list: $out"

# create: every genre, 20 seeds: musicgen's melody checks, and the text parses
out=$($TUNE create --check --count 20 2>&1)
echo "$out" | grep -q "140 tunes composed, 0 failed" && pass || fail "create --check: $(echo "$out" | tail -3)"
# the same seed, the same tune; the options reach it
$TUNE create --genre blues --seed 7 --length 24 --voices 4 --once -o $T/c1.txt 2>/dev/null
$TUNE create --genre blues --seed 7 --length 24 --voices 4 --once -o $T/c2.txt 2>/dev/null
cmp -s $T/c1.txt $T/c2.txt && pass || fail "create --seed is not repeatable"
grep -q "blues in .* 92 bpm, 24 bars, seed 7" $T/c1.txt && grep -q "^v4:" $T/c1.txt && ! grep -q "^v5:" $T/c1.txt &&
    grep -qE "^tune blues [0-9.]+ once 48" $T/c1.txt && pass || fail "create's options: $(head -1 $T/c1.txt)"
$TUNE create --genre chiptune --tempo fast --length 30s --seed 3 -o $T/c3.txt 2>/dev/null &&
    grep -q "chiptune in .* 187 bpm, 24 bars" $T/c3.txt && grep -q "arp:" $T/c3.txt && pass || fail "create --tempo fast --length 30s: $(head -1 $T/c3.txt)"
cp $T/pfest.txt $T/app.txt
$TUNE create --genre pop --seed 5 --append $T/app.txt --name extra >/dev/null 2>&1 &&
    $TUNE list $T/app.txt 2>/dev/null | grep -q "^extra" && pass || fail "create --append"

# import: a MIDI file (format 1: tempo track, melody with an overlap, bass, drums)
python3 - $T/song.mid <<'EOF'
import sys, struct
def vlq(n):
    b = [n & 0x7F]; n >>= 7
    while n: b.insert(0, (n & 0x7F) | 0x80); n >>= 7
    return bytes(b)
def track(ev):
    d = b''.join(vlq(t) + e for t, e in ev) + b'\x00\xff\x2f\x00'
    return b'MTrk' + struct.pack('>I', len(d)) + d
q = 480
meta = [(0, b'\xff\x51\x03' + (600000).to_bytes(3, 'big')), (0, b'\xff\x58\x04\x03\x02\x18\x08')]
mel = [(0, b'\xff\x03\x06melody')]
for n in [60, 62, 64, 65, 67, 69]:
    mel += [(0, bytes([0x90, n, 100])), (q, bytes([0x80, n, 0]))]
mel += [(0, bytes([0x90, 72, 100])), (q, bytes([0x90, 74, 100])), (q, bytes([0x80, 72, 0])), (q // 2, bytes([0x80, 74, 0]))]
bass = [(0, bytes([0x91, 36, 90])), (q * 3, bytes([0x81, 36, 0])), (0, bytes([0x91, 43, 90])), (q * 3, bytes([0x91, 43, 0]))]
drum = [(0, bytes([0x99, 36, 100])), (q, bytes([0x89, 36, 0]))]
open(sys.argv[1], 'wb').write(b'MThd' + struct.pack('>IHHH', 6, 1, 4, q) + track(meta) + track(mel) + track(bass) + track(drum))
EOF
out=$($TUNE import $T/song.mid 2>&1)
echo "$out" | grep -q "^tune song 7.50 once 12$" &&
    echo "$out" | grep -q "^v1: @voice1 c4:4 d4:4 e4:4 | f4:4 g4:4 a4:4 | c5:4 d5:6 r:2$" &&
    echo "$out" | grep -q "^v2: @voice2 c2:12 | g2:12 | r:12$" && ! echo "$out" | grep -q "^v3:" && pass || fail "import: $out"
$TUNE import $T/song.mid --drums 2>/dev/null | grep -q "^v3:" && pass || fail "import --drums"
$TUNE list $T/song.mid 2>/dev/null | grep -q "^song .*36 ticks" && pass || fail "list of a .mid"

# play into a WAV: as long as the tune, and not silent
if $TUNE play $T/pfest.txt --wav $T/p.wav >/dev/null 2>&1 && python3 - $T/p.wav <<'EOF'
import sys, wave, struct
w = wave.open(sys.argv[1])
n = w.getnframes()
s = struct.unpack('<%dh' % (n * 2), w.readframes(n))
# 48 ticks at 6 frames: 5.76 s, and a second of release
assert w.getframerate() == 48000 and w.getnchannels() == 2 and abs(n / 48000 - 6.76) < 0.05, n
assert max(abs(v) for v in s) > 2000
EOF
then pass; else fail "play --wav"; fi

# sample: a sound (AU) at 8000 Hz, held to 48 kHz: a quarter of a second,
# loud; the same samples packed by pack45 play the same
P=src/test-resources/pack45
if $TUNE sample --rate 8000 $P/tone.au --wav $T/s1.wav >/dev/null 2>&1 &&
   ./bin/pack45 --sound --rate 8000 -d 0x40000 $P/tone.au $T/s.pak >/dev/null 2>&1 &&
   $TUNE sample --rate 8000 $T/s.pak --wav $T/s2.wav >/dev/null 2>&1 && cmp -s $T/s1.wav $T/s2.wav &&
   python3 - $T/s1.wav <<'EOF'
import sys, wave, struct
w = wave.open(sys.argv[1])
n = w.getnframes()
s = struct.unpack('<%dh' % (n * 2), w.readframes(n))
assert w.getframerate() == 48000 and n == 12000, n
assert max(abs(v) for v in s) > 15000
EOF
then pass; else fail "sample"; fi
$TUNE sample $T/s.pak >/dev/null 2>&1 && fail "sample: no --rate" || pass

echo "tune45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
