#!/usr/bin/env python3
"""make_ttf.py — a tiny TrueType font for font45's tests (#466), written from
scratch (no fontTools): 1000 units to the em, ascent 800, descent -200, and
four glyphs of straight lines:

  ' '  empty, advance 300
  'I'  a bar, x 100-300, y 0-700, advance 400
  'L'  an L, the bar and a foot to x 600 (y 0-200), advance 700
  'O'  a box 100-600 x 0-700 with a hole 300-400 x 200-500, advance 700

Only the tables stb_truetype reads: cmap (format 6), glyf, head, hhea, hmtx,
loca (long), maxp.  Usage: make_ttf.py out.ttf
"""
import struct
import sys


def rect(x0, y0, x1, y1, clockwise=True):
    # TrueType (y up): outer contours clockwise, holes counter-clockwise
    pts = [(x0, y0), (x0, y1), (x1, y1), (x1, y0)]
    return pts if clockwise else pts[::-1]


GLYPHS = [  # (code, advance, contours); glyph 0 is .notdef, empty
    (None, 500, []),
    (0x20, 300, []),
    (0x49, 400, [rect(100, 0, 300, 700)]),
    (0x4C, 700, [[(100, 0), (100, 700), (300, 700), (300, 200), (600, 200), (600, 0)]]),
    (0x4F, 700, [rect(100, 0, 600, 700), rect(300, 200, 400, 500, clockwise=False)]),
]


def glyph_data(contours):
    if not contours:
        return b''
    pts = [p for c in contours for p in c]
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    out = struct.pack('>hhhhh', len(contours), min(xs), min(ys), max(xs), max(ys))
    end = -1
    for c in contours:
        end += len(c)
        out += struct.pack('>H', end)
    out += struct.pack('>H', 0)                 # no instructions
    out += bytes([1] * len(pts))                # every point on the curve, 16-bit deltas
    px = py = 0
    for x, _ in pts:
        out += struct.pack('>h', x - px)
        px = x
    for _, y in pts:
        out += struct.pack('>h', y - py)
        py = y
    return out + b'\0' * (-len(out) % 4)


def main(path):
    glyf, loca = b'', []
    for _, _, contours in GLYPHS:
        loca.append(len(glyf))
        glyf += glyph_data(contours)
    loca.append(len(glyf))
    n = len(GLYPHS)
    tables = {}
    tables[b'head'] = struct.pack('>IIIIHHqqhhhhHHhhh', 0x00010000, 0x00010000, 0, 0x5F0F3CF5, 0, 1000, 0, 0,
                                  0, -200, 700, 800, 0, 8, 2, 1, 0)
    tables[b'hhea'] = struct.pack('>Ihhh' 'Hhhh' 'hhh' 'hhhh' 'hH', 0x00010000, 800, -200, 0,
                                  700, 0, 0, 600, 1, 0, 0, 0, 0, 0, 0, 0, n)
    tables[b'maxp'] = struct.pack('>IH', 0x00005000, n)
    tables[b'hmtx'] = b''.join(struct.pack('>Hh', adv, 0) for _, adv, _ in GLYPHS)
    tables[b'loca'] = b''.join(struct.pack('>I', o) for o in loca)
    tables[b'glyf'] = glyf
    # cmap: one Windows Unicode (BMP) subtable, format 6 (codes 32-79 in a row)
    first, last = 0x20, 0x4F
    ids = [0] * (last - first + 1)
    for gid, (code, _, _) in enumerate(GLYPHS):
        if code is not None:
            ids[code - first] = gid
    sub = struct.pack('>HHHHH', 6, 10 + 2 * len(ids), 0, first, len(ids)) + b''.join(struct.pack('>H', i) for i in ids)
    tables[b'cmap'] = struct.pack('>HHHHI', 0, 1, 3, 1, 12) + sub

    tags = sorted(tables)
    count = len(tags)
    entry = 1
    while entry * 2 <= count:
        entry *= 2
    log2 = entry.bit_length() - 1
    out = struct.pack('>IHHHH', 0x00010000, count, entry * 16, log2, count * 16 - entry * 16)
    offset = 12 + 16 * count
    body = b''
    for tag in tags:
        data = tables[tag] + b'\0' * (-len(tables[tag]) % 4)
        s = sum(struct.unpack('>%dI' % (len(data) // 4), data)) & 0xFFFFFFFF
        out += struct.pack('>4sIII', tag, s, offset + len(body), len(tables[tag]))
        body += data
    with open(path, 'wb') as f:
        f.write(out + body)


if __name__ == '__main__':
    main(sys.argv[1])
