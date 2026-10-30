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
$TUNE import $T/song.mod 2>/dev/null | grep -q "^sample s04_c2 $T/song_samples/04_noise_c2.wav" && [ -f $T/song_samples/04_noise_c2.wav ] &&
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
# pattern break; a kept sample (a digi voice: a WAV a note) stopped by a SID
# note on its channel (the silent cut)
python3 - $T/fx.mod <<'EOF'
import sys, random
random.seed(2)
saw = bytes([(i * 8 - 128) & 255 for i in range(32)])
noise = bytes([int(random.uniform(-1, 1) * 120 * (1 - i / 2000)) & 255 for i in range(2000)])
bell = bytes([0]) * 2
h = b"fx test".ljust(20, b"\0")
h += b"saw".ljust(22, b"\0") + bytes([0, 16, 0, 64, 0, 0, 0, 16])
h += b"snare".ljust(22, b"\0") + bytes([3, 232, 0, 64, 0, 0, 0, 1])
h += (bytes(22) + bytes([0, 0, 0, 0, 0, 0, 0, 1])) * 29
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
put(p0, 9, 0, cell(0, 0, 0xD, 0))        # pattern break
put(p1, 0, 0, cell(1, 214, 0xF, 150))    # C-3, 150 BPM
open(sys.argv[1], "wb").write(h + bytes(p0) + bytes(p1) + saw + noise)
EOF
rm -rf $T/fx_samples
$TUNE import $T/fx.mod -o $T/fx.txt >/dev/null 2>&1
v=$(sed -n '/^tune/,$p' $T/fx.txt | tr -d '\\\n' | tr -s ' ')
[ "$v" = "tune fx_test 3 loopv1: @s01 c4:1 arp:3,7 ~:1 arp:off ~:1 @s01v4 f4^5:2 r:2 @s01 a4:3 | tempo:2.5 c5:64v2: r:8 @s01 c4:2 | ~:64d1: r:6 s02_c2:2 cut:2 | r:64" ] &&
    grep -q "^instrument s01v4 saw 00 f3 800 4 0 hr=1 " $T/fx.txt && grep -q "^sample s02_c2 fx_samples/02_snare_c2.wav " $T/fx.txt &&
    [ "$(ls $T/fx_samples | tr '\n' ' ')" = "00_cut.wav 02_snare_c2.wav " ] && $TUNE list $T/fx.txt 2>/dev/null | grep -q "74 ticks" &&
    pass || fail "import of a MOD: $v"
# a SID note's pitch from the sample's own (song.mod: a 32-byte saw at C-2 is c4, a 64-byte square c3)
$TUNE import $T/song.mod -o $T/songmod.txt >/dev/null 2>&1 && grep -q "^v1: @s01 c4:4 @s02 d3:4 @s03 e3:4 " $T/songmod.txt &&
    grep -q "^v2: @s02 c3:16 g3:16 c3:16 g3:16$" $T/songmod.txt && pass || fail "import of a MOD: the notes ($(grep '^v' $T/songmod.txt))"
# the kept samples over 64 KB: the biggest made the SID's (no choices); with
# choices, kept, and convert's error and their sizes said
python3 - $T/big.mod <<'EOF'
import sys, math
bell = bytes([int(sum(math.sin(2 * math.pi * f * i / 8287) / (k + 1) for k, f in enumerate([523, 1443, 2826, 4672])) * 40) & 255 for i in range(40000)])
h = b"big".ljust(20, b"\0") + b"bell".ljust(22, b"\0") + bytes([len(bell) // 2 >> 8, len(bell) // 2 & 255, 0, 64, 0, 0, 0, 1])
h += (bytes(22) + bytes([0, 0, 0, 0, 0, 0, 0, 1])) * 30 + bytes([1, 127]) + bytes(128) + b"M.K."
p = bytearray(1024)
for row, per in ((0, 428), (16, 381), (32, 339)):
    p[row * 16:row * 16 + 4] = bytes([0x00 | per >> 8, per & 255, 0x10, 0])
open(sys.argv[1], "wb").write(h + bytes(p) + bell)
EOF
out=$($TUNE import $T/big.mod -o $T/big.txt 2>&1)
echo "$out" | grep -q "did not fit in 64 KB: made SID instruments of 1 bell" && grep -q "^instrument s01 .*made the SID's so the samples fit in 64 KB" $T/big.txt &&
    $TUNE list $T/big.txt >/dev/null 2>&1 && pass || fail "import of a MOD: fitting 64 KB: $out"
printf '1 sample\n' > $T/big.choices
out=$($TUNE import $T/big.mod -o $T/big2.txt --choices $T/big.choices 2>&1); rc=$?
[ $rc -ne 0 ] && echo "$out" | grep -q "over 64 KB" && echo "$out" | grep -q "the kept samples, about: 1 [0-9]* KB" && pass || fail "import of a MOD with choices over 64 KB: $out"

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
# 2 (a digi voice), no SID voices, its stream after the sample table (at 12:
# offset 0, 1000 bytes, the DMA's rate $000679)
cat > $T/dg.txt <<EOF
sample a $PWD/src/test-resources/pack45/tone.wav rate=4000
tune t 1 once
d1: a:2 r:1 a+a:1
EOF
$TUNE convert $T/dg.txt -o $T/dg.c >/dev/null 2>&1 &&
    [ "$(sed -n '/tune_t\[/,/};/p' $T/dg.c | tr -d ' \n' | sed 's/.*{//; s/,}.*//')" = \
      "2,0,0,1,0,17,12,0,19,0,12,0,0,0,232,3,121,6,0,0,2,128,1,0,1,255" ] &&
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
