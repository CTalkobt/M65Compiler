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

# tempo: $85 lo hi (8.8 frames per tick); v1 starts with the tune's own (a
# loop starts as it did); one tick, the later voice's wins (list's length:
# 2 ticks at 10, 2 at 25 frames)
printf 'instrument a pulse 09 a6 0600 0 0\ntune s 50 loop\nv1: c4:2 tempo:25 c4:2\nv2: tempo:10 r:4\n' > $T/s.txt
$TUNE convert $T/s.txt -o $T/s.c >/dev/null 2>&1 &&
    sed -n '/tune_s\[/,/};/p' $T/s.c | tr -d ' \n' | grep -q '133,0,50,60,2,133,0,25,60,2,254,133,0,10,128,4,254' &&
    $TUNE list $T/s.txt | grep -q '4 ticks *1.4 s' && pass || fail "tempo: bytes / list length"

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
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: @a tempo:0 c4:1' "tempo:frames per tick"

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

# import of a MOD --report: samples of known kinds (a saw, a 25% pulse and a
# triangle, single cycles looped; a noise burst; a bell of inharmonic
# partials; one unused), each played by the one pattern
python3 - $T/song.mod <<'EOF'
import sys, math, random
random.seed(1)
def s8(v): return max(-128, min(127, int(round(v)))) & 0xFF
samples = [
    ("saw", [s8(-128 + 255 * i / 32) for i in range(32)], 0, 16),
    ("square", [s8(100 if i < 16 else -100) for i in range(64)], 0, 32),
    ("tri", [s8((1 - abs(i - 32) / 16) * 100) for i in range(64)], 0, 32),
    ("noise", [s8(random.uniform(-1, 1) * 120 * math.exp(-i / 400)) for i in range(2000)], 0, 1),
    ("bell", [s8(sum(math.sin(2 * math.pi * f * i / 8287) / (k + 1) for k, f in enumerate([523, 1443, 2826, 4672])) * 60 * math.exp(-i / 1500)) for i in range(4000)], 0, 1),
    ("spare", [s8(100 * math.sin(2 * math.pi * i / 32)) for i in range(32)], 0, 16),
]
h = b"test song".ljust(20, b"\0")
for k in range(31):
    if k < len(samples):
        n, d, ls, ll = samples[k]
        h += n.encode().ljust(22, b"\0") + bytes([len(d) // 2 >> 8, len(d) // 2 & 255, 0, 64, ls >> 8, ls & 255, ll >> 8, ll & 255])
    else:
        h += bytes(22) + bytes([0, 0, 0, 0, 0, 0, 0, 1])
h += bytes([1, 127]) + bytes(128) + b"M.K."
periods = [428, 381, 339, 320, 285, 254, 226, 214]
pat = bytearray(64 * 16)
for row in range(0, 64, 4):
    for ch, smp in enumerate([1 + (row // 4) % 3, 4, 5, 2]):
        if ch == 3 and row % 16: continue
        p = periods[(row // 4) % len(periods)]
        c = (row * 4 + ch) * 4
        pat[c:c + 4] = bytes([(smp & 0xF0) | p >> 8, p & 255, (smp & 15) << 4, 0])
data = b"".join(bytes(d) for _, d, _, _ in samples)
open(sys.argv[1], "wb").write(h + bytes(pat) + data)
EOF
rep=$($TUNE import $T/song.mod --report 2>&1)
echo "$rep" | grep -qE "^ 1  saw .* sid +[0-9]+  saw," &&
    echo "$rep" | grep -qE "^ 2  square .* sid +[0-9]+  pulse 400," &&
    echo "$rep" | grep -qE "^ 3  tri .* sid +[0-9]+  tri," &&
    echo "$rep" | grep -qE "^ 4  noise .* sample .*\(keep the sample: (hat|snare)\)" &&
    echo "$rep" | grep -qE "^ 5  bell .* sample " &&
    echo "$rep" | grep -qE "^ 6  spare .* unused" &&
    echo "$rep" | grep -q "^SID instruments: 3 (0 of them approximately); samples kept: 2" && pass || fail "import --report of a MOD: $rep"
# --extract: the samples kept as WAVs (8-bit, the rate of their most played
# period; a loop in a smpl chunk), --all every one used; they read as samples
rm -rf $T/modx
$TUNE import $T/song.mod --extract $T/modx >/dev/null 2>&1 && [ "$(ls $T/modx | tr '\n' ' ')" = "04_noise.wav 05_bell.wav " ] &&
    $TUNE import $T/song.mod --extract $T/modx --all >/dev/null 2>&1 && [ "$(ls $T/modx | wc -l)" = 5 ] &&
    grep -q smpl $T/modx/01_saw.wav &&
    printf 'sample b modx/05_bell.wav\ntune t 6 once\nd1: b:8\n' > $T/modx.txt && $TUNE list $T/modx.txt 2>&1 | grep -q "hits: b" &&
    pass || fail "import --extract of a MOD ($(ls $T/modx 2>&1 | tr '\n' ' '))"
# (without -o: the text to the output, the WAVs beside the MOD, named from here)
rm -rf $T/song_samples
$TUNE import $T/song.mod 2>/dev/null | grep -q "^sample s04 $T/song_samples/04_noise.wav" && [ -f $T/song_samples/04_noise.wav ] &&
    pass || fail "import of a MOD to the output"
# --audition: a sample's original, its SID stand-in, then both (the original
# left, the SID right), each as loud: three parts, the last two channels apart
out=$($TUNE import $T/song.mod --audition 1 --wav $T/aud.wav 2>&1)
echo "$out" | grep -q "^ 1  saw .* SID: saw 00 f3 800 0 0$" &&
python3 - $T/aud.wav <<'EOF' && pass || fail "import --audition of a MOD: $out"
import sys, wave, struct, math
w = wave.open(sys.argv[1]); n = w.getnframes(); d = struct.unpack('<%dh' % (2 * n), w.readframes(n))
assert w.getnchannels() == 2 and w.getframerate() == 48000
parts, cur, quiet = [], None, 0
for b in range(0, n, 2400):
    L, R = d[2 * b:2 * (b + 2400):2], d[2 * b + 1:2 * (b + 2400):2]
    if max(map(abs, L + R)) > 300:
        cur = cur or [[], []]; cur[0] += L; cur[1] += R; quiet = 0
    else:
        quiet += 1
        if cur and quiet >= 3: parts.append(cur); cur = None
if cur: parts.append(cur)
rms = lambda v: math.sqrt(sum(x * x for x in v) / len(v)) / 32768
assert len(parts) == 3, len(parts)
for L, R in parts: assert 0.1 < rms(L) < 0.25 and 0.1 < rms(R) < 0.25
assert parts[0][0] == parts[0][1] and parts[1][0] == parts[1][1] and parts[2][0] != parts[2][1]
EOF
$TUNE import $T/song.mod --audition 6 --wav $T/aud6.wav 2>&1 | grep -q "sample 6 is not one the song plays" && pass || fail "import --audition of an unused sample"
# asking (--ask: the answers from the input; with --wav nothing is played):
# 1 kept as a sample, 2 as shown (SID), back, 2 the SID, 3 as shown, 4 the
# SID, quit; the choices saved as made, then the verdicts of --report and
# --extract
rm -f $T/song.choices
out=$(printf 's\n\nb\nd\n\nd\nq\n' | $TUNE import $T/song.mod --audition --ask --wav $T/aud.wav 2>&1)
[ "$(grep -v '^;' $T/song.choices | sed 's/ *;.*//' | tr '\n' ' ')" = "1 sample 2 sid 3 sid 4 sid " ] &&
    echo "$out" | grep -q "song.choices: 3 SID instruments, 1 kept as sample" && pass || fail "import --audition --ask: $out"
rep=$($TUNE import $T/song.mod --report --choices $T/song.choices 2>&1)
echo "$rep" | grep -qE "^ 1  saw .* sample\* .*\(keep the sample\)" && echo "$rep" | grep -qE "^ 4  noise .* sid\* .*noise 00 [0-9a-f]{2} 0800 0 0 \(a stand-in\)" &&
    echo "$rep" | grep -qE "^ 5  bell .* sample " && echo "$rep" | grep -q "your choice: sample (measured: sid, 0)" && pass || fail "import --report --choices: $rep"
rm -rf $T/modc
$TUNE import $T/song.mod --extract $T/modc --choices $T/song.choices >/dev/null 2>&1 &&
    [ "$(ls $T/modc | tr '\n' ' ')" = "01_saw.wav 05_bell.wav " ] && pass || fail "import --extract --choices ($(ls $T/modc 2>&1 | tr '\n' ' '))"
printf '2 maybe\n' > $T/bad.choices
$TUNE import $T/song.mod --report --choices $T/bad.choices 2>&1 | grep -q "bad.choices:1: expected <sample> sid|sample" && pass || fail "a bad choices file"

# import of a MOD as text: the rows in play order a tick each; the speed
# (the tune's frames a tick) and a BPM change (tempo:); an arpeggio on a row,
# a slide to a note (a glide), vibrato (the instrument's), a note cut, a
# pattern break; kept samples (digi voices: a WAV each, hit at its note)
# stopped by a SID note on their channel and by a cut (-), a loop (loop=),
# the sample's volume and C (vol:)
python3 - $T/fx.mod <<'EOF'
import sys, random
random.seed(2)
saw = bytes([(i * 8 - 128) & 255 for i in range(32)])
noise = bytes([int(random.uniform(-1, 1) * 120 * (1 - i / 2000)) & 255 for i in range(2000)])
bell = bytes([0]) * 2
h = b"fx test".ljust(20, b"\0")
h += b"saw".ljust(22, b"\0") + bytes([0, 16, 0, 64, 0, 0, 0, 16])
h += b"snare".ljust(22, b"\0") + bytes([3, 232, 0, 64, 0, 0, 0, 1])
h += b"wind".ljust(22, b"\0") + bytes([1, 244, 0, 48, 0, 150, 1, 94])
h += (bytes(22) + bytes([0, 0, 0, 0, 0, 0, 0, 1])) * 28
h += bytes([2, 127, 0, 1]) + bytes(126) + b"M.K."
def cell(smp, per, fx, prm): return bytes([(smp & 0xF0) | per >> 8, per & 255, (smp & 15) << 4 | fx, prm])
p0 = bytearray(1024); p1 = bytearray(1024)
def put(p, row, ch, c): p[(row * 4 + ch) * 4:(row * 4 + ch) * 4 + 4] = c
put(p0, 0, 0, cell(1, 428, 0xF, 3))      # C-2, speed 3
put(p0, 1, 0, cell(0, 0, 0x0, 0x37))     # arpeggio 3,7
put(p0, 3, 0, cell(0, 320, 0x3, 8))      # slide to F-2
put(p0, 4, 0, cell(0, 0, 0x4, 0x84))     # vibrato, depth 4
put(p0, 5, 0, cell(0, 0, 0xE, 0xC0))     # note cut
put(p0, 7, 0, cell(1, 254, 0, 0))        # A-2
put(p0, 6, 1, cell(2, 428, 0, 0))        # the snare (kept), ringing...
put(p0, 8, 1, cell(1, 428, 0, 0))        # ...a SID note on its channel stops it
put(p0, 7, 1, cell(0, 0, 0xC, 0x20))     # its volume 32
put(p0, 2, 2, cell(3, 428, 0, 0))        # wind (kept, looped, volume 48)...
put(p0, 9, 2, cell(0, 0, 0xE, 0xC0))     # ...cut
put(p0, 9, 0, cell(0, 0, 0xD, 0))        # pattern break
put(p1, 0, 0, cell(1, 214, 0xF, 150))    # C-3, 150 BPM
wind = bytes([int(random.uniform(-1, 1) * 100) & 255 for i in range(1000)])
open(sys.argv[1], "wb").write(h + bytes(p0) + bytes(p1) + saw + noise + wind)
EOF
rm -rf $T/fx_samples
$TUNE import $T/fx.mod -o $T/fx.txt >/dev/null 2>&1
v=$(sed -n '/^tune/,$p' $T/fx.txt | tr -d '\\\n' | tr -s ' ')
[ "$v" = "tune fx_test 3 loopv1: @s01 c4:1 arp:3,7 ~:1 arp:off ~:1 @s01v4 f4^5:2 r:2 @s01 a4:3 | tempo:2.5 c5:64v2: r:8 @s01 c4:2 | ~:64d1: r:6 s02:c4:1 vol:32 r:1 -:2 | r:64d2: r:2 vol:48 s03:c4:7 -:1 | r:64" ] &&
    grep -q "^instrument s01v4 saw 00 f3 800 4 0 hr=1 " $T/fx.txt && grep -q "^sample s02 fx_samples/02_snare.wav note=c4 " $T/fx.txt &&
    grep -q "^sample s03 fx_samples/03_wind.wav note=c4 loop=0.0362 " $T/fx.txt &&
    [ "$(ls $T/fx_samples | tr '\n' ' ')" = "02_snare.wav 03_wind.wav " ] && $TUNE list $T/fx.txt 2>/dev/null | grep -q "74 ticks" &&
    pass || fail "import of a MOD: $v"
# a SID note's pitch from the sample's own (song.mod: a 32-byte saw at C-2 is c4, a 64-byte square c3)
$TUNE import $T/song.mod -o $T/songmod.txt >/dev/null 2>&1 && grep -q "^v1: @s01 c4:4 @s02 d3:4 @s03 e3:4 " $T/songmod.txt &&
    grep -q "^v2: @s02 c3:16 g3:16 c3:16 g3:16$" $T/songmod.txt && pass || fail "import of a MOD: the notes ($(grep '^v' $T/songmod.txt))"
# the kept samples over 64 KB: the biggest made the SID's (no choices); with
# choices, kept, and convert's error and their sizes said
python3 - $T/big.mod <<'EOF'
import sys, math
def bell(fs): return bytes([int(sum(math.sin(2 * math.pi * f * i / 8287) / (k + 1) for k, f in enumerate(fs)) * 40) & 255 for i in range(40000)])
b1, b2 = bell([523, 1443, 2826, 4672]), bell([440, 1270, 2510, 3990])
h = b"big".ljust(20, b"\0")
for n, d in ((b"bell", b1), (b"gong", b2)):
    h += n.ljust(22, b"\0") + bytes([len(d) // 2 >> 8, len(d) // 2 & 255, 0, 64, 0, 0, 0, 1])
h += (bytes(22) + bytes([0, 0, 0, 0, 0, 0, 0, 1])) * 29 + bytes([1, 127]) + bytes(128) + b"M.K."
p = bytearray(1024)
for row, ch, smp, per in ((0, 0, 1, 428), (16, 0, 1, 381), (32, 1, 2, 339)):
    c = (row * 4 + ch) * 4
    p[c:c + 4] = bytes([per >> 8, per & 255, smp << 4, 0])
open(sys.argv[1], "wb").write(h + bytes(p) + b1 + b2)
EOF
out=$($TUNE import $T/big.mod -o $T/big.txt 2>&1)
echo "$out" | grep -q "did not fit in 64 KB: made SID instruments of 2 gong" && grep -q "^instrument s02 .*made the SID.s so the samples fit in 64 KB" $T/big.txt &&
    $TUNE list $T/big.txt >/dev/null 2>&1 && pass || fail "import of a MOD: fitting 64 KB: $out"
printf '1 sample\n2 sample\n' > $T/big.choices
out=$($TUNE import $T/big.mod -o $T/big2.txt --choices $T/big.choices 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$out" | grep -q "over 64 KB" && echo "$out" | grep -q "the kept samples, about: 1 [0-9]* KB" && pass || fail "import of a MOD with choices over 64 KB: $out"

# pitched hits (one copy of the samples, a table entry a note at its rate),
# a loop (the entry's last word: from the start), vol: ($83 v: 0-64 as
# 0-255), a stop ($81 d); past 127 hits: $84 h d
cat > $T/dp.txt <<EOF
sample a $PWD/src/test-resources/pack45/tone.wav rate=4000 note=c4 loop=0.05
tune t 1 once
d1: a:2 a:g4:1 vol:32 -:1 a:c5:1
EOF
$TUNE convert $T/dp.txt -o $T/dp.c >/dev/null 2>&1 && grep -q "#define TUNE_SAMPLES_SIZE 1000" $T/dp.h &&
python3 - $T/dp.c <<'EOF' && pass || fail "digi voices: pitched hits, a loop, vol:, a stop"
import sys, re
src = open(sys.argv[1]).read()
b = [int(x) for x in re.search(r'tune_t\[\d+\] = \{([^}]*)\}', src).group(1).replace('\n', '').split(',') if x.strip()]
assert b[0] == 0x12 and b[1] == 0, b[:2]
st = b[10] | b[11] << 8
rate = lambda hz: round(hz * (1 << 24) / 40500000)
want = [(0, 1000, rate(4000), 200), (0, 1000, rate(4000 * 2 ** (7 / 12)), 200), (0, 1000, rate(4000 * 2), 200)]
for k, (at, n, r, lp) in enumerate(want):
    e = b[st + 9 * k:st + 9 * k + 9]
    got = (e[0] | e[1] << 8, e[2] | e[3] << 8, e[4] | e[5] << 8 | e[6] << 16, e[7] | e[8] << 8)
    assert abs(got[2] - r) <= 1 and got[:2] == (at, n) and got[3] == lp, (k, got, (at, n, r, lp))
s = b[8] | b[9] << 8
assert b[s:] == [0, 2, 1, 1, 0x83, 128, 0x81, 1, 2, 1, 0xFF], b[s:]
EOF
python3 - $T/df.txt $PWD/src/test-resources/pack45/tone.wav <<'EOF'
import sys
notes = ['c', 'c#', 'd', 'd#', 'e', 'f', 'f#', 'g', 'g#', 'a', 'a#', 'b']
hits = ['%s:%s%d:1' % (s, notes[n % 12], 2 + n // 12) for s in 'abcd' for n in range(37)]
open(sys.argv[1], 'w').write(''.join('sample %s %s rate=4000 note=c4\n' % (s, sys.argv[2]) for s in 'abcd') +
                              'tune t 1 once\nd1: ' + ' '.join(hits) + '\n')
EOF
$TUNE convert $T/df.txt -o $T/df.c >/dev/null 2>&1 && grep -q "#define TUNE_SAMPLES_SIZE 4000" $T/df.h &&
    sed -n '/tune_t\[/,/};/p' $T/df.c | tr -d ' \n' | grep -q ',127,1,132,128,1,132,129,1,' && pass || fail "digi voices: hits past 127 (\$84 h d)"
err $'sample a x.wav\ntune t 5 once\nd1: vol:65 a:1' "vol:0-64"
err $'sample a x.wav\ntune t 5 once\nd1: a:h4:1' "not a note"
err $'sample vol x.wav' "is not r, - or vol"
err $'sample a x.wav note=h2' "note=<note>"

# a legato note: $88 n d (the pitch, the note going on)
printf 'instrument a pulse 09 a6 0600 0 0\ntune t 1 once\nv1: c4:2 ~e4:3\n' > $T/lg.txt
$TUNE convert $T/lg.txt -o $T/lg.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/lg.c | tr -d ' \n' | grep -q '60,2,136,64,3,255,}' &&
    grep -q "#define TUNE_T_TICKS 5" $T/lg.h && pass || fail "a legato note"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: ~e4^3:1' "a legato note is"

# tail (the hr byte's bit 7), arp2: ($83 a $80), pw: ($89 lo hi s), cut: ($8A lo hi s)
printf 'instrument a pulse 09 a6 0600 0 0 hr=2 tail\ntune t 1 once\nv1: arp2:12 c4:2 pw:6a0,-16 cut:1f0,9 ~:1\n' > $T/tl.txt
$TUNE convert $T/tl.txt -o $T/tl.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/tl.c | tr -d ' \n' |
    grep -q '64,9,166,0,6,0,0,0,0,0,130,0,0,0,0,0,0,131,12,128,60,2,137,160,6,240,138,240,1,9,130,1,255,}' && pass || fail "tail, arp2:, pw:, cut:"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: pw:1000 c4:1' "pw:hex 0-fff"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: cut:200,200 c4:1' "cut:hex 0-7ff"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: arp2:30 c4:1' "arp2:a"

# free (the hr byte's bit 6) and vspeed (the vibrato byte's high nibble + 1)
printf 'instrument a pulse 09 a6 0600 5 0 vspeed=3 free hr=1\ntune t 1 once\nv1: c4:2\n' > $T/fr.txt
$TUNE convert $T/fr.txt -o $T/fr.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/fr.c | tr -d ' \n' | grep -q '64,9,166,0,6,37,0,0,0,0,65,' &&
    pass || fail "free, vspeed"
err $'instrument a pulse 09 a6 0600 5 0 vspeed=17' "vspeed 1-16"

# pwrun, cutrun (the hr byte's bits 5 and 4)
printf 'instrument a pulse 09 a6 0600 0 0 pwrun cutrun filter=lp\ntune t 1 once\nv1: c4:2\n' > $T/pr.txt
$TUNE convert $T/pr.txt -o $T/pr.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/pr.c | tr -d ' \n' | grep -q '64,9,166,0,6,0,0,0,0,0,48,16,0,4,0,0,0,' &&
    pass || fail "pwrun, cutrun"

# a pattern called transposed: $8B offset times semitones (a note and a
# legato note in it shifted by the player)
printf 'instrument a pulse 09 a6 0600 0 0\npattern p: c4:2 ~e4:1\ntune t 1 once\nv1: *p *p+5 *p-2*2\n' > $T/tp.txt
$TUNE convert $T/tp.txt -o $T/tp.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/tp.c | tr -d ' \n' |
    grep -q '134,42,0,1,139,42,0,1,5,139,42,0,2,254,255,60,2,136,64,1,135,}' && pass || fail "a pattern called transposed"
err $'instrument a pulse 09 a6 0600 1 1\npattern p: c4:1\ntune t 5 once\nv1: *p+200' "semitones (-127..127)"
err $'sample a x.wav\npattern p: a:1\ntune t 5 once\nd1: *p+2' "a digi voice calls a pattern untransposed"

# a pulse-width cycle: pwlo=, pwhi= (19-byte instruments: its limits / 16 at the end)
printf 'instrument a pulse 09 a6 0600 0 0 pwm=32 pwlo=400 pwhi=c00 pwrun\ntune t 1 once\nv1: c4:2\n' > $T/pc.txt
$TUNE convert $T/pc.txt -o $T/pc.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/pc.c | tr -d ' \n' |
    grep -q '{0,1,0,1,1,19,10,0,29,0,64,9,166,0,6,0,0,32,0,0,32,0,0,0,0,0,0,64,192,60,2,255,}' && pass || fail "a pulse-width cycle"
err $'instrument a pulse 09 a6 0600 0 0 pwhi=1000' "pwhi=hex 0-fff"

# a sawtooth pulse-width cycle (the hr byte's bit 2: from its top back to its bottom)
printf 'instrument a pulse 09 a6 0600 0 0 pwm=32 pwlo=400 pwhi=c00 pwsaw\ntune t 1 once\nv1: c4:2\n' > $T/ps.txt
$TUNE convert $T/ps.txt -o $T/ps.c >/dev/null 2>&1 && sed -n '/tune_t\[/,/};/p' $T/ps.c | tr -d ' \n' |
    grep -q ',4,0,0,0,0,0,0,64,192,60,2,255,}' && pass || fail "a sawtooth pulse-width cycle"

# patterns: in the data once ($86 offset times a call, $87 their end), after
# the voices; the ticks and bars counted through them
printf 'instrument a pulse 09 a6 0600 0 0\npattern p: c4:2 e4:2\npattern q: g4:1\ntune t 1 loop 4\nv1: *p | *q*3 r:1 | *p\n' > $T/pt.txt
$TUNE convert $T/pt.txt -o $T/pt.c >/dev/null 2>&1 && grep -q "#define TUNE_T_TICKS 12" $T/pt.h &&
    sed -n '/tune_t\[/,/};/p' $T/pt.c | tr -d ' \n' | grep -q '134,42,0,1,134,47,0,3,128,1,134,42,0,1,254,60,2,64,2,135,67,1,135,}' &&
    pass || fail "patterns: the data"
err $'instrument a pulse 09 a6 0600 1 1\ntune t 5 once\nv1: *nope' "unknown pattern nope"
err $'pattern p: c4:1 *p' "a pattern calls no pattern"
err $'instrument a pulse 09 a6 0600 1 1\npattern p: c4:1\ntune t 5 once\nv1: *p*0' "1-255, not"
err $'pattern p: c4:1\npattern p: c4:2' "pattern p defined twice"

# import of an XM, S3M and IT: the same song in each, read as a MOD is (the
# middle note on a 16-bit saw tuned to 8363 Hz sounds c4; speed 3, 125 BPM;
# an arpeggio, a note cut or note-off, 150 BPM, a pattern break; S3M and IT
# with +++ and --- in their orders)
python3 - $T/fx.xm <<'EOF'
# fx.xm: one channel, speed 3 at 125 BPM; C-4 (the middle) on a 16-bit
# 32-sample saw (8363 Hz: c4), an arpeggio (037), a note-off, E-4 at 150 BPM
# (F96), a pattern break (D00) on row 7
import sys, struct
saw = [-32768 + 2048 * i for i in range(32)]
delta, prev = b"", 0
for v in saw: delta += struct.pack("<h", (v - prev + 32768) % 65536 - 32768); prev = v
h = b"Extended Module: " + b"fx xm".ljust(20, b"\0") + b"\x1a" + b"gen".ljust(20, b"\0") + struct.pack("<H", 0x104)
h += struct.pack("<I", 276) + struct.pack("<8H", 1, 0, 1, 1, 1, 1, 3, 125) + bytes(256)
rows = [b"\x80"] * 16
rows[0] = bytes([0x9B, 49, 1, 0x0F, 3])      # C-4, instrument 1, speed 3
rows[1] = bytes([0x98, 0x00, 0x37])          # arpeggio 3,7
rows[3] = bytes([0x81, 97])                  # note off
rows[4] = bytes([0x99, 53, 0x0F, 150])       # E-4, 150 BPM
rows[7] = bytes([0x88, 0x0D])                # pattern break
pat = b"".join(rows)
h += struct.pack("<IBHH", 9, 0, 16, len(pat)) + pat
ins = struct.pack("<I", 263) + b"saw".ljust(22, b"\0") + bytes([0]) + struct.pack("<HI", 1, 40)
ins += bytes(263 - len(ins))
smp = struct.pack("<III", 64, 0, 64) + bytes([64, 0, 0x11, 128, 0, 0]) + b"saw16".ljust(22, b"\0")
open(sys.argv[1], "wb").write(h + ins + smp + delta)
EOF
python3 - $T/fx.s3m <<'EOF'
# fx.s3m: one channel, speed 3 at 125 BPM; C-4 (ST3's middle) on a 16-bit
# unsigned 32-sample saw (C2 speed 8363: c4), an arpeggio (J37), a note cut
# (^^), E-4 at 150 BPM (T96), a pattern break (C00) on row 7; the orders
# 0, +++, ---
import sys, struct
saw = b"".join(struct.pack("<H", 2048 * i) for i in range(32))   # (unsigned)
h = bytearray(b"fx s3m".ljust(28, b"\0") + bytes([0x1A, 16, 0, 0]) + struct.pack("<6H", 4, 1, 1, 0, 0x1320, 2) + b"SCRM")
h += bytes([64, 3, 125, 0xB0, 0, 0]) + bytes(10) + bytes([0] + [255] * 31)
h += bytes([0, 254, 255, 255]) + struct.pack("<HH", 7, 12)     # orders; the paragraphs: sample 0x70, pattern 0xC0
h += bytes(0x70 - len(h))
h += bytes([1]) + b"saw.raw".ljust(12, b"\0") + bytes([0]) + struct.pack("<H", 0x12)   # data at 0x120
h += struct.pack("<III", 32, 0, 32) + bytes([64, 0, 0, 5]) + struct.pack("<I", 8363) + bytes(12)
h += b"saw16".ljust(28, b"\0") + b"SCRS"
pat = bytes([0x20 | 0x80, 0x40, 1, 1, 3, 0])      # C-4, instrument 1, A03
pat += bytes([0x80, 10, 0x37, 0])                 # J37
pat += bytes([0]) + bytes([0x20, 254, 0, 0])      # (row 2) ^^ on row 3
pat += bytes([0x20 | 0x80, 0x44, 0, 20, 0x96, 0]) # E-4, T96
pat += bytes([0, 0]) + bytes([0x80, 3, 0, 0])     # (rows 5, 6) C00 on row 7
pat += bytes(64 - 7)
h += struct.pack("<H", len(pat) + 2) + pat
h += bytes(0x120 - len(h))
open(sys.argv[1], "wb").write(bytes(h) + saw)
EOF
python3 - $T/fx.it <<'EOF'
# fx.it: instrument mode (its keyboard: every note the sample), one
# channel, speed 3 at 125 BPM; C-5 (IT's middle) on a 16-bit 32-sample saw
# (C5 speed 8363: c4), an arpeggio (J37), a note cut (^^^), E-5 at 150 BPM
# (T96), a pattern break (C00) on row 7; the orders 0, +++, ---
import sys, struct
saw = b"".join(struct.pack("<h", -32768 + 2048 * i) for i in range(32))
h = b"IMPM" + b"fx it".ljust(26, b"\0") + bytes(2)
h += struct.pack("<4H", 3, 1, 1, 1) + struct.pack("<4H", 0x214, 0x214, 0x0D, 0) + bytes([128, 48, 3, 125, 128, 0])
h += struct.pack("<HII", 0, 0, 0) + bytes([32] * 64) + bytes([64] * 64) + bytes([0, 254, 255])
o_ins = len(h) + 12
ins = b"IMPI" + b"saw.iti".ljust(12, b"\0") + bytes([0, 0, 0, 0]) + struct.pack("<H", 0) + bytes([0, 0, 128, 32, 0, 0])
ins += struct.pack("<H", 0x214) + bytes([1, 0]) + b"saw".ljust(26, b"\0") + bytes(6)
ins += b"".join(bytes([n, 1]) for n in range(120))
ins += bytes(554 - len(ins))
o_smp = o_ins + len(ins)
o_pat = o_smp + 0x50
pat = bytes([0x81, 0x0B, 60, 1, 1, 3, 0])         # C-5, instrument 1, A03
pat += bytes([0x81, 0x08, 10, 0x37, 0])           # J37
pat += bytes([0]) + bytes([0x81, 0x01, 254, 0])   # (row 2) ^^^ on row 3
pat += bytes([0x81, 0x09, 64, 20, 0x96, 0])       # E-5, T96
pat += bytes([0, 0]) + bytes([0x81, 0x08, 3, 0, 0])   # (rows 5, 6) C00 on row 7
pat += bytes(16 - 7)
o_dat = o_pat + 8 + len(pat)
smp = b"IMPS" + b"saw.raw".ljust(12, b"\0") + bytes([0, 64, 0x13, 64]) + b"saw16".ljust(26, b"\0") + bytes([1, 32])
smp += struct.pack("<IIIIIII", 32, 0, 32, 8363, 0, 0, o_dat) + bytes(4)
h += struct.pack("<III", o_ins, o_smp, o_pat) + ins + smp + struct.pack("<HH", len(pat), 16) + bytes(4) + pat
open(sys.argv[1], "wb").write(h + saw)
EOF
for x in xm s3m it; do
    $TUNE import $T/fx.$x -o $T/fx_$x.txt >/dev/null 2>&1
    v=$(sed -n '/^tune/,$p' $T/fx_$x.txt | tr '\n' '/')
    [ "$v" = "tune fx_$x 3 loop/v1: @s01 c4:1 arp:3,7 ~:1 arp:off ~:1 r:1 tempo:2.5 e4:4/" ] &&
        grep -q "^instrument s01 saw 00 f3 800 0 0 hr=1 " $T/fx_$x.txt && head -1 $T/fx_$x.txt | grep -qi "(tune45 import: $x, 1 channels" &&
        $TUNE list $T/fx_$x.txt 2>/dev/null | grep -q "8 ticks *0.4 s" && pass || fail "import of an $x: $v"
done

# import of a .sid: a PSID whose player plays back a list of SID register
# writes a call (a frame's: register, value ... $FF), so what it writes is
# known: a bass as Hubbard's (a frame gated, noise, an octave bounce through
# the release: a wave table, arp2:, tail; AD/SR 0 before the next: hr=3),
# an arpeggio (c4 e4 g4 a frame each) whose pulse width rises then falls
# (pwm free, then pw:), a saw through the low-pass (resonance 15) going
# from a3 to c4 without a new gate (legato);
# called a frame (vbi), or by its own interrupt on CIA 1's timer ($2626:
# 100.88 calls a second, a tick each)
cat > $T/gen_sid.py <<'EOF'
import sys, struct
def reg(n): return round(440 * 2 ** ((n - 69) / 12) * 16777216 / 985248)
frames = [[] for _ in range(600)]
def w(f, r, v): frames[f].append((r, v))
w(0, 0x18, 0x1F); w(0, 0x15, 0); w(0, 0x16, 0x40); w(0, 0x17, 0xF4)
# voice 1: a bass as Hubbard's: the gate a frame, a noise frame, then the
# pulse an octave up and down a frame through the release; AD/SR 0 for the
# last 3 frames before the next (a hard restart); hit at 0, 16, 32
for at in (0, 16, 32):
    w(at, 5, 0x06); w(at, 6, 0x4B); w(at, 2, 0x80); w(at, 3, 1)
    w(at, 0, reg(36) & 255); w(at, 1, reg(36) >> 8); w(at, 4, 0x41)
    w(at + 1, 4, 0x80)
    w(at + 2, 4, 0x40)
    for f in range(2, 16):
        n = 36 if f % 2 else 48
        w(at + f, 0, reg(n) & 255); w(at + f, 1, reg(n) >> 8)
    w(at + 13, 5, 0); w(at + 13, 6, 0)
# voice 2: an arpeggio c4 e4 g4, a frame each, 24 frames; its pulse width
# up $40 a frame, then down from frame 12
w(0, 7 + 5, 0x09); w(0, 7 + 6, 0xA0)
for f in range(24):
    n = (60, 64, 67)[f % 3]
    pw = 0x400 + 0x40 * min(f, 12) - 0x40 * max(0, f - 12)
    w(f, 7, reg(n) & 255); w(f, 8, reg(n) >> 8); w(f, 7 + 2, pw & 255); w(f, 7 + 3, pw >> 8)
w(0, 7 + 4, 0x41); w(24, 7 + 4, 0x40)
# voice 3: a saw through the low-pass, a3 then c4 legato (no new gate), 16 frames
w(4, 14 + 5, 0x08); w(4, 14 + 6, 0xC4)
w(4, 14, reg(57) & 255); w(4, 15, reg(57) >> 8); w(4, 14 + 4, 0x21)
w(10, 14, reg(60) & 255); w(10, 15, reg(60) >> 8)
w(20, 14 + 4, 0x20)
if len(sys.argv) > 2 and sys.argv[2] in ("rep", "trep", "loop"):   # instead: a phrase of four notes, four times
    frames = [[] for _ in range(600)]                                 # (trep: up 5, 7, back: transposed; loop:
    w(0, 0x18, 0x0F); w(0, 5, 0x09); w(0, 6, 0xA0)                    # after a note, the four again and again)
    at = 8 if sys.argv[2] == "loop" else 0
    if at: w(0, 0, reg(48) & 255); w(0, 1, reg(48) >> 8); w(0, 4, 0x11); w(6, 4, 0x10)
    for r in range(8 if at else 4):
        up = (0, 5, 7, 0)[r % 4] if sys.argv[2] != "rep" else 0
        for q, n in enumerate((60 + up, 64 + up, 67 + up, 72 + up)):
            f = at + r * 16 + q * 4
            w(f, 0, reg(n) & 255); w(f, 1, reg(n) >> 8); w(f, 4, 0x11); w(f + 3, 4, 0x10)
    if at: frames = frames[:at] + frames[at:at + 64] * 9
data = b"".join(bytes(sum(([r, v] for r, v in fr), [])) + b"\xff" for fr in frames)
# the player at $1000: init (the frames' pointer; then SETUP), play (a
# frame's writes: register, value ... $FF), SETUP (cia: $0314 at HANDLER,
# CIA 1 timer A at $2626 with its interrupt), HANDLER (JSR play, ack, JMP $EA31)
cia = len(sys.argv) > 2 and sys.argv[2] == "cia"
PLAY = 0x100B
play = [0xA0, 0x00, 0xB1, 0xFB, 0xC9, 0xFF, 0xF0, 0x0A, 0xAA, 0xC8, 0xB1, 0xFB,
        0x9D, 0x00, 0xD4, 0xC8, 0xD0, 0xF0,
        0xC8, 0x98, 0x18, 0x65, 0xFB, 0x85, 0xFB, 0x90, 0x02, 0xE6, 0xFC, 0x60]
SETUP = PLAY + len(play)
setup = [0x60]
if cia:
    HANDLER = SETUP + 32
    setup = [0x78, 0xA9, HANDLER & 255, 0x8D, 0x14, 0x03, 0xA9, HANDLER >> 8, 0x8D, 0x15, 0x03,
             0xA9, 0x81, 0x8D, 0x0D, 0xDC, 0xA9, 0x26, 0x8D, 0x04, 0xDC, 0x8D, 0x05, 0xDC,
             0xA9, 0x11, 0x8D, 0x0E, 0xDC, 0x60]
    setup += [0xEA] * (32 - len(setup)) + [0x20, PLAY & 255, PLAY >> 8, 0xAD, 0x0D, 0xDC, 0x4C, 0x31, 0xEA]
DATA = SETUP + len(setup)
code = bytes([0xA9, DATA & 255, 0x85, 0xFB, 0xA9, DATA >> 8, 0x85, 0xFC, 0x4C, SETUP & 255, SETUP >> 8]) + bytes(play) + bytes(setup)
assert len(code) == DATA - 0x1000
h = bytearray(0x7C)
h[0:4] = b"PSID"
struct.pack_into(">HHHHHHHI", h, 4, 2, 0x7C, 0x1000, 0x1000, 0 if cia else PLAY, 1, 1, 1 if cia else 0)   # (cia: its speed bit)
h[0x16:0x16 + 9] = b"sid notes"
struct.pack_into(">H", h, 0x76, 0x14)
open(sys.argv[1], "wb").write(bytes(h) + code + data)
EOF
for v in vbi cia; do python3 $T/gen_sid.py $T/n_$v.sid $v 2>/dev/null; done
$TUNE import $T/n_vbi.sid --seconds 0.8 --no-loop -o $T/n_vbi.txt >/dev/null 2>&1
v=$(sed -n '/^wavetable/p;/^instrument/p;/^tune/,$p' $T/n_vbi.txt | tr '\n' '/')
[ "$v" = "wavetable t01 pulse noise pulse/instrument i01 pulse 06 4b 180 0 0 table=t01 hr=3 tail free/instrument i02 pulse 06 4b 180 0 0 table=t01 tail free/instrument i03 pulse 09 a0 400 0 0 pwm=64 free/instrument i04 saw 08 c4 000 0 0 filter=lp res=15 cutoff=200/tune sid_notes 1 once/v1: arp2:12 @i01 c2:1 r:15 c2:1 r:15 @i02 c2:1 r:7/v2: arp:4,7 @i03 c4:13 pw:700,-64 ~:11 r:16/v3: r:4 @i04 a3:6 ~c4:10 r:20/" ] &&
    $TUNE list $T/n_vbi.txt 2>/dev/null | grep -q "40 ticks *0.8 s" && pass || fail "import of a .sid: $v"
$TUNE import $T/n_cia.sid --seconds 0.8 --no-loop -o $T/n_cia.txt >/dev/null 2>&1 &&
    grep -q "by its CIA timer, 100.88 calls a second" $T/n_cia.txt && grep -q "^tune sid_notes 0.49609375 once" $T/n_cia.txt &&
    grep -q "^v1: arp2:12 @i01 c2:1 r:15 c2:1 r:15 @i02 c2:1 r:48$" $T/n_cia.txt && pass || fail "import of a .sid timed by the CIA"
# its repeats as patterns (a phrase four times: one pattern, called four
# times running), playing as the text written out does (--no-patterns)
python3 $T/gen_sid.py $T/n_rep.sid rep 2>/dev/null
$TUNE import $T/n_rep.sid --seconds 1.28 --no-loop -o $T/np.txt >/dev/null 2>&1
python3 $T/gen_sid.py $T/n_trep.sid trep 2>/dev/null
$TUNE import $T/n_trep.sid --seconds 1.28 --no-loop -o $T/tnp.txt >/dev/null 2>&1
$TUNE import $T/n_trep.sid --seconds 1.28 --no-loop --no-patterns -o $T/tnf.txt >/dev/null 2>&1
$TUNE import $T/n_rep.sid --seconds 1.28 --no-loop --no-patterns -o $T/nf.txt >/dev/null 2>&1
grep -q "^pattern p1: c4:3 r:1 e4:3 r:1 g4:3 r:1 c5:3 r:1$" $T/np.txt && grep -q "^v1: @i01 \*p1\*4$" $T/np.txt &&
    [ "$($TUNE trace $T/np.txt --frames 64 2>/dev/null | md5sum)" = "$($TUNE trace $T/nf.txt --frames 64 2>/dev/null | md5sum)" ] &&
    pass || fail "import of a .sid: its repeats as patterns ($(sed -n '/^pattern/p;/^v1/p' $T/np.txt | tr '\n' '/'))"
# (a phrase played at other pitches: one pattern, called transposed)
grep -q "^pattern p1: c4:3 r:1 e4:3 r:1 g4:3 r:1 c5:3 r:1$" $T/tnp.txt && grep -q "^v1: @i01 \*p1 \*p1+5 \*p1+7 \*p1$" $T/tnp.txt &&
    [ "$($TUNE trace $T/tnp.txt --frames 64 2>/dev/null | md5sum)" = "$($TUNE trace $T/tnf.txt --frames 64 2>/dev/null | md5sum)" ] &&
    pass || fail "import of a .sid: repeats transposed ($(sed -n '/^pattern/p;/^v1/p' $T/tnp.txt | tr '\n' '/'))"
# where it plays again as before: a tune that loops there (|: after the
# first note, a period of 64 calls), or, gone silent, one that ends (a
# second of release after)
python3 $T/gen_sid.py $T/n_loop.sid loop 2>/dev/null
$TUNE import $T/n_loop.sid --seconds 10 --search 10 -o $T/nl.txt >/dev/null 2>&1
grep -q "it loops from 0.2 s, every 1.3 s" $T/nl.txt && grep -q "^tune sid_notes 1 loop" $T/nl.txt &&
    grep -q "^v1: @i01 c3:6 r:2 |: \*p1 \*p1+5 \*p1+7 \*p1$" $T/nl.txt &&
    $TUNE list $T/nl.txt 2>/dev/null | grep -q " 72 ticks" && pass || fail "import of a .sid that loops ($(sed -n '1p;/^tune/,$p' $T/nl.txt | tr '\n' '/'))"
$TUNE import $T/n_rep.sid --search 10 -o $T/ne.txt >/dev/null 2>&1
grep -q "it ends at 1.3 s" $T/ne.txt && grep -q "^tune sid_notes 1 once" $T/ne.txt &&
    $TUNE list $T/ne.txt 2>/dev/null | grep -q " 113 ticks" && pass || fail "import of a .sid that ends ($(sed -n '1p;/^tune/,$p' $T/ne.txt | tr '\n' '/'))"
python3 -c "
import struct, sys
h = bytearray(0x7C); h[0:4] = b'PSID'; struct.pack_into('>HHHHHHHI', h, 4, 2, 0x7C, 0x1000, 0x1000, 0x1003, 1, 1, 0)
open(sys.argv[1], 'wb').write(bytes(h) + bytes([0x4C, 0x00, 0x10, 0x60]))" $T/loop.sid
$TUNE import $T/loop.sid 2>&1 | grep -q "init" && pass || fail "import of a .sid whose init does not return"

# coded data (read as it plays): a tune made by create, plain by default
# (list says what coding would make of it), coded with --coded (a third of
# the plain at most), playing as the plain data does (the model's trace
# alike over the tune and round its loop); --coded leaves a small tune
# plain (the codes' tables cost more), --coded-all does not
$TUNE create --genre chiptune --length 64 --seed 5 -o $T/cr.txt >/dev/null 2>&1
cp=$($TUNE list $T/cr.txt 2>/dev/null | awk 'NR==1 && /\(coded [0-9]+\)$/{print $4}')
cc=$($TUNE list --coded $T/cr.txt 2>/dev/null | awk 'NR==1 && / coded$/{print $4}')
[ -n "$cp" ] && [ -n "$cc" ] && [ "$cc" -lt $((cp / 3)) ] &&
    [ "$($TUNE trace $T/cr.txt --frames 6000 2>/dev/null | md5sum)" = "$($TUNE trace --coded $T/cr.txt --frames 6000 2>/dev/null | md5sum)" ] &&
    pass || fail "coded data: $cc bytes (plain $cp), or its trace differs"
$TUNE convert --bin --coded $T/cr.txt -o $T/cr.bin >/dev/null 2>&1 && $TUNE convert --bin $T/cr.txt -o $T/crp.bin >/dev/null 2>&1 &&
    [ "$(od -An -tx1 -j3 -N1 $T/cr.bin | tr -d ' ')" = "21" ] && [ "$(od -An -tx1 -j3 -N1 $T/crp.bin | tr -d ' ')" = "01" ] &&
    ! $TUNE list --coded $T/pfest.txt 2>/dev/null | grep -q " coded$" && $TUNE list --coded-all $T/pfest.txt 2>/dev/null | grep -q " coded$" &&
    grep -q "define tune_play(t) (tune_codec(), tune_play(t))" $T/cr.h && ! grep -q "tune_codec" $T/crp.h 2>/dev/null &&
    pass || fail "coded data: --coded, plain by default, --coded-all, music.h's tune_codec"

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

# digi voices: a sample (a quarter second at 4000 Hz: 1000 bytes), hit,
# ringing on, hit together with itself (one hit: a+a is a); the data: flags
# $12 (a digi voice, 9-byte sample entries), no SID voices, its stream after
# the sample table (at 12: offset 0, 1000 bytes, the DMA's rate $000679, no
# loop $FFFF)
cat > $T/dg.txt <<EOF
sample a $PWD/src/test-resources/pack45/tone.wav rate=4000
tune t 1 once
d1: a:2 r:1 a+a:1
EOF
$TUNE convert $T/dg.txt -o $T/dg.c >/dev/null 2>&1 &&
    [ "$(sed -n '/tune_t\[/,/};/p' $T/dg.c | tr -d ' \n' | sed 's/.*{//; s/,}.*//')" = \
      "18,0,0,1,0,17,12,0,21,0,12,0,0,0,232,3,121,6,0,255,255,0,2,128,1,0,1,255" ] &&
    grep -q "#define TUNE_SAMPLES_SIZE 1000" $T/dg.h && grep -q "tune_sample_bank\[TUNE_SAMPLES_SIZE\] = {" $T/dg.c &&
    pass || fail "digi voices: the data"
# --bin needs --samples, and writes the samples there
$TUNE convert $T/dg.txt --bin -o $T/dg.bin >/dev/null 2>&1 && fail "digi --bin without --samples" || pass
$TUNE convert $T/dg.txt --bin -o $T/dg.bin --samples $T/dg.smp >/dev/null 2>&1 &&
    [ $(stat -c %s $T/dg.smp) = 1000 ] && pass || fail "digi --bin --samples"
# a hit cut short by the next: a warning; play mixes the samples in
cat > $T/dw.txt <<EOF
sample a $PWD/src/test-resources/pack45/tone.wav rate=4000
tune t 2 once
d1: a:2 a:30
EOF
$TUNE list $T/dw.txt 2>&1 | grep -q "warning: tune t d1: a (0.25 s) is cut short by the next hit 1 time" && pass || fail "digi: the cut-short warning"
if $TUNE play $T/dw.txt --wav $T/dw.wav >/dev/null 2>&1 && python3 - $T/dw.wav <<'EOF'
import sys, wave, struct
w = wave.open(sys.argv[1])
n = w.getnframes()
s = struct.unpack('<%dh' % (n * 2), w.readframes(n))
assert max(abs(v) for v in s) > 5000
EOF
then pass; else fail "digi: play mixes the samples in"; fi
# from= and len=: a part of the file (0.1 s from 0.05 s: 400 bytes at 4000 Hz)
cat > $T/dp.txt <<EOF
sample a $PWD/src/test-resources/pack45/tone.wav rate=4000 from=0.05 len=0.1
tune t 1 once
d1: a:2
EOF
$TUNE list $T/dp.txt 2>&1 | grep -q "hits: a (400 bytes, 4000 Hz)" && pass || fail "digi: from= len="
err $'sample a x.wav from=x' "from=seconds"
err $'sample a x.wav len=0' "len=seconds"
err "sample a $PWD/src/test-resources/pack45/tone.wav from=5"$'\ntune t 1 once\nd1: a:1' "past its end"
err $'sample a x.wav\ntune t 1 once\nd1: b:1' "unknown sample"
err $'sample a x.wav\ntune t 1 once\nd2: a:1' "d1: expected"
err $'sample a x.wav\ntune t 1 once\nd5: a:1' "d1: to d4:"
err $'sample a x.wav\nsample a y.wav' "defined twice"
err $'sample a nothere.wav\ntune t 1 once\nd1: a:1' "cannot read"
err $'instrument i pulse 00 f0 0800 0 0\nsample a x.wav\ntune t 1 once\nv1: c4:2\nd1: r:1' "d1 1"

echo "tune45 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
