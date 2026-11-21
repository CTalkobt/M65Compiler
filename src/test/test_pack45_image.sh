#!/bin/bash
# test_pack45_image.sh — pack45 --image, format 2 (#666): images packed and
# unpacked again (pack45 -x: the C++ model of unpack2.s45's decoder, which
# pack45 also runs in place before it writes), byte for byte, and decoded by
# an independent decoder here (the format as include/Pack45.hpp describes it);
# PNG input with and without --palette; the errors.
cd "$(dirname "$0")/../.." || exit 1
d=build/test/pack45_image
rm -rf "$d"
mkdir -p "$d"
pass=0
fail=0
ok() { pass=$((pass + 1)); }
bad() { echo "FAIL: $1"; fail=$((fail + 1)); }

# the images: raw 4-bit pixels (two a byte, the left one the low nibble)
python3 - "$d" <<'PY'
import sys, zlib, struct, random
d = sys.argv[1]
def raw(name, w, h, px):
    out = bytearray()
    for y in range(h):
        for x in range(0, w, 2):
            out.append(px(x, y) | px(x + 1, y) << 4)
    open(f"{d}/{name}.raw", "wb").write(out)
random.seed(666)
noise = [[random.randrange(16) for x in range(256)] for y in range(64)]
raw("one", 10, 5, lambda x, y: 9)                                     # N = 1, a row of 5 bytes
raw("two", 256, 3, lambda x, y: 12 if (x * 37 + (x >> 3)) % 7 < 3 else 3)   # N = 2, w = 256, above
raw("bands", 64, 40, lambda x, y: ((x + (y * 3) // 4) // 4) % 16)     # N = 16, left
raw("odd", 6, 7, lambda x, y: (x * y) % 5)                            # odd height, 3 bytes a row
raw("wide", 250, 1, lambda x, y: x % 3)                               # one row
raw("noise", 256, 64, lambda x, y: noise[y][x])                       # long codes (lengths limited to 8)
raw("skew", 32, 300, lambda x, y: 0 if (x + y) % 97 else 15)          # a rare symbol: halved counts

# a PNG (RGB, 8 bits) from rows of (r, g, b)
def png(name, rows):
    h, w = len(rows), len(rows[0])
    raw_rows = b"".join(b"\0" + bytes(c for px in r for c in px) for r in rows)
    def chunk(t, data):
        return struct.pack(">I", len(data)) + t + data + struct.pack(">I", zlib.crc32(t + data) & 0xFFFFFFFF)
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) + \
        chunk(b"IDAT", zlib.compress(raw_rows)) + chunk(b"IEND", b"")
    open(f"{d}/{name}.png", "wb").write(data)
cols = [(255, 255, 255), (0, 0, 0), (200, 0, 0), (0, 0, 180), (90, 90, 90)]   # by brightness: 1 3 2 4 0
png("five", [[cols[(x // 3 + y // 2) % 5] for x in range(24)] for y in range(10)])
# its expected bytes: colours numbered dark to light
order = sorted(range(5), key=lambda k: (cols[k][0] * 299 + cols[k][1] * 587 + cols[k][2] * 114, cols[k]))
entry = {order[e]: e for e in range(5)}
out = bytearray()
for y in range(10):
    for x in range(0, 24, 2):
        out.append(entry[(x // 3 + y // 2) % 5] | entry[((x + 1) // 3 + y // 2) % 5] << 4)
open(f"{d}/five.expect", "wb").write(out)
open(f"{d}/five.rgb", "wb").write(bytes(c for e in range(5) for c in cols[order[e]]))
# a palette of 16 (R, G, B) with those colours at entries 7, 2, 11, 5, 14
pal = [(i * 3 + 1, i * 5 + 1, i * 7 + 1) for i in range(16)]   # (none of the five)
for e, k in zip((7, 2, 11, 5, 14), range(5)):
    pal[e] = cols[k]
open(f"{d}/pal16.rgb", "wb").write(bytes(c for p in pal for c in p))
out = bytearray()
ent = (7, 2, 11, 5, 14)
for y in range(10):
    for x in range(0, 24, 2):
        out.append(ent[(x // 3 + y // 2) % 5] | ent[((x + 1) // 3 + y // 2) % 5] << 4)
open(f"{d}/five_pal.expect", "wb").write(out)
png("many", [[(x * 15, 0, 0) for x in range(18)] for y in range(2)])   # 18 colours
PY

# an independent decoder of format 2 (from the format's description; bit by
# bit, canonical codes by length then symbol): FILE (with its load address)
# -> the bytes
decode() {
    python3 - "$1" "$2" <<'PY'
import sys
s = open(sys.argv[1], "rb").read()[2:]
assert s[0] == 2, "format"
pitch, h, n, left = s[5], s[6] | s[7] << 8, s[8], s[9]
w = 2 * pitch
order = s[10:10 + n]
lens = s[10 + n:10 + 2 * n]
codes, c = {}, 0
for l in range(1, 9):
    for sym in range(n):
        if lens[sym] == l:
            codes[(l, c)] = sym
            c += 1
    c <<= 1
pos, bit = 10 + 2 * n, 0
def take():
    global pos, bit
    b = s[pos] >> (7 - bit) & 1
    bit += 1
    if bit == 8:
        bit, pos = 0, pos + 1
    return b
prev = [0] * w
out = bytearray()
for y in range(h):
    row = []
    for x in range(w):
        l, c = 0, 0
        while (l, c) not in codes:
            c, l = c << 1 | take(), l + 1
            assert l <= 8, "no code"
        p = (row[x - 1] if x else prev[0]) if left else prev[x]
        row.append((p + codes[(l, c)]) % n)
    prev = row
    for x in range(0, w, 2):
        out.append(order[row[x]] | order[row[x + 1]] << 4)
open(sys.argv[2], "wb").write(out)
PY
}

# round trips: pack45 --image, then pack45 -x and the decoder here
round_trip() {   # name width [options]
    local name=$1 width=$2
    shift 2
    if ./bin/pack45 --image --width "$width" -d 0x40000 "$@" "$d/$name.raw" "$d/$name.pak" > "$d/$name.out" 2>&1 &&
       ./bin/pack45 -x "$d/$name.pak" "$d/$name.back" >/dev/null 2>&1 && cmp -s "$d/$name.raw" "$d/$name.back" &&
       decode "$d/$name.pak" "$d/$name.dec" 2>/dev/null && cmp -s "$d/$name.raw" "$d/$name.dec"; then
        ok
    else
        bad "round trip of $name $* ($(cat "$d/$name.out"))"
    fi
}
round_trip one 10
round_trip two 256
round_trip bands 64
round_trip odd 6
round_trip wide 250
round_trip noise 256
round_trip skew 32
round_trip bands 64 --predict above
round_trip two 256 --predict left

# which prediction was kept: the smaller (a: along the rows, b: down them)
./bin/pack45 --image --width 64 -d 0x40000 "$d/bands.raw" "$d/b1.pak" | grep -q "from the left" && ok || bad "bands not coded from the left"
./bin/pack45 --image --width 256 -d 0x40000 "$d/two.raw" "$d/b2.pak" | grep -q "from above" && ok || bad "two not coded from above"
# one colour: 1 bit a pixel; the header says 1 colour
./bin/pack45 --image --width 10 -d 0x40000 "$d/one.raw" "$d/b3.pak" | grep -q "10x5, 1 colour, " && ok || bad "one: 10x5, 1 colour"
# sizes and the in-place offset (pack45 checks the stream unpacks in place at it)
line=$(./bin/pack45 --image --width 64 -d 0x40000 "$d/bands.raw" "$d/b4.pak")
echo "$line" | grep -q "1280 -> [0-9]* bytes ([0-9]*%; format 1: [0-9]*), unpacks to \$40000; in place: load it at \$4[0-9A-F]* (offset [0-9]*)" && ok || bad "the report: $line"
# the load address written: dest + the in-place offset
off=$(echo "$line" | sed -n 's/.*(offset \([0-9]*\)).*/\1/p')
la=$(od -An -tu2 -N2 "$d/b4.pak" | tr -d ' ')
[ "$la" = "$off" ] && ok || bad "load address $la, offset $off"

# PNG: its colours numbered dark to light (--palette-out), or found in a palette
if ./bin/pack45 --image -d 0x40000 --palette-out "$d/five.pal" "$d/five.png" "$d/five.pak" > "$d/five.out" 2>&1 &&
   ./bin/pack45 -x "$d/five.pak" "$d/five.back" >/dev/null 2>&1 && cmp -s "$d/five.expect" "$d/five.back" &&
   cmp -s "$d/five.rgb" "$d/five.pal" && grep -q "24x10, 5 colours" "$d/five.out" &&
   grep -q "its colours, entry 0 up: #000000 #0000B4 #C80000 #5A5A5A #FFFFFF" "$d/five.out"; then ok; else bad "PNG without a palette ($(cat "$d/five.out"))"; fi
if ./bin/pack45 --image -d 0x40000 --palette "$d/pal16.rgb" "$d/five.png" "$d/fivep.pak" > "$d/fivep.out" 2>&1 &&
   ./bin/pack45 -x "$d/fivep.pak" "$d/fivep.back" >/dev/null 2>&1 && cmp -s "$d/five_pal.expect" "$d/fivep.back"; then ok; else bad "PNG with a palette ($(cat "$d/fivep.out"))"; fi
# the order in the stream: dark to light by the palette (entries 7 2 11 5 14 are
# white black red blue grey: 2 5 11 14 7)
python3 -c "import sys; s=open('$d/fivep.pak','rb').read()[2:]; sys.exit(0 if list(s[10:15]) == [2, 5, 11, 14, 7] else 1)" && ok || bad "the order by the palette"

# errors (exit status 1, a message)
err() {   # what, then the command
    local what=$1
    shift
    if "$@" > "$d/err.out" 2>&1; then bad "$what: accepted"
    elif [ $? -eq 1 ] && grep -q "pack45: " "$d/err.out"; then ok
    else bad "$what: $(cat "$d/err.out")"; fi
}
err "odd width" ./bin/pack45 --image --width 7 -d 0x40000 "$d/one.raw" "$d/x.pak"
err "width over 256" ./bin/pack45 --image --width 258 -d 0x40000 "$d/one.raw" "$d/x.pak"
err "not whole rows" ./bin/pack45 --image --width 8 -d 0x40000 "$d/one.raw" "$d/x.pak"
err "17 colours" ./bin/pack45 --image -d 0x40000 "$d/many.png" "$d/x.pak"
err "a colour not in the palette" ./bin/pack45 --image -d 0x40000 --palette "$d/five.rgb" "$d/many.png" "$d/x.pak"
err "no destination" ./bin/pack45 --image --width 10 "$d/one.raw" "$d/x.pak"
err "--predict" ./bin/pack45 --image --predict up --width 10 -d 0x40000 "$d/one.raw" "$d/x.pak"
err "not an image" ./bin/pack45 --image -d 0x40000 Makefile "$d/x.pak"
# a damaged stream: -x says why
printf '\x00\x00\x02\x00\x00\x04\x00\xC8\x01\x00\x02\x00' > "$d/bad.pak"
err "-x of a width over 256" ./bin/pack45 -x "$d/bad.pak" "$d/x.out"

echo "pack45 image tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
