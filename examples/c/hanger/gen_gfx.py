#!/usr/bin/env python3
"""Generate gfx.h and words.h for the hanger example.

Pictures. Each level's animal is drawn here with shapes (PIL) and cut into
8 x 8 character cells for the 30 x 22 cell picture on the left of the
screen. The picture is built up in stages: stage 0 is the gallows and the
rope, stage k adds the animal's k-th part, and a last "dead" stage crosses
out its eyes. For each level the cells' patterns become that level's glyphs
(screen codes GLYPH0.., loaded when the level starts), and each stage is
stored as the cells it changes: offset (2 bytes), screen code, colour.

The screen is 80 columns, where a pixel is half as wide as it is tall, so
the shapes are drawn in square units of 1 x 1 raster lines (a cell is 4 x 8
units) at 4x size, then shrunk to 2 x 1 hires pixels per unit.

Words. words.txt (one word per line, '#' comments) becomes letters 1-26
(screen codes) with a 0 after each word, sorted from easiest to hardest:
the number of misses a guesser makes who tries letters in order of English
frequency. Level n draws its words from the n-th sixth of that order.

Both are written next to this script and kept in the repository, so the
example builds without Python. Run it (make gfx; it needs PIL) after
changing a picture or words.txt. --preview also draws every stage of every
animal into preview.png.

Usage: python3 gen_gfx.py [--preview]
"""
import math
import os
import sys

from PIL import Image, ImageDraw

PW, PH = 30, 22                  # the picture, in cells
UW, UH = PW * 4, PH * 8          # in units
SS = 4                           # supersampling
GLYPH0 = 0x4A                    # first per-level screen code
FIXED0 = 0x40                    # the fixed glyphs: $40-$49
SOLID = 0x47
SPACE = 0x20

# colours (the MEGA65's default palette)
BLACK, WHITE, RED, CYAN, PURPLE, GREEN, BLUE, YELLOW = range(8)
ORANGE, BROWN, PINK, DGREY, GREY, LGREEN, LBLUE, LGREY = range(8, 16)
RGB = [(0x00, 0x00, 0x00), (0xFF, 0xFF, 0xFF), (0xF0, 0x00, 0x00), (0x00, 0xF0, 0xF0),
       (0x60, 0x00, 0x60), (0x00, 0xA0, 0x00), (0x00, 0x00, 0xF0), (0xF0, 0xF0, 0x00),
       (0xFF, 0xA0, 0x00), (0xA0, 0x60, 0x40), (0xFF, 0xA0, 0xA0), (0x50, 0x50, 0x50),
       (0x80, 0x80, 0x80), (0xA0, 0xFF, 0xA0), (0xA0, 0xA0, 0xFF), (0xC0, 0xC0, 0xC0)]

ROPE_X = 74
BEAM_Y = 8

# --------------------------------------------------------------------------
# Shapes: ('circle', cx, cy, r) ('ellipse', cx, cy, rx, ry) ('ring', cx, cy, r, w)
# ('line', w, (x, y), (x, y), ...) ('poly', (x, y), ...) ('rect', x0, y0, x1, y1)
# ('taper', w0, w1, (x, y), ...) a line that narrows from w0 to w1


def s(v):
    return v * SS


def draw(d, shape, fill):
    k = shape[0]
    if k == 'circle':
        _, cx, cy, r = shape
        d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], fill=fill)
    elif k == 'ellipse':
        _, cx, cy, rx, ry = shape
        d.ellipse([s(cx - rx), s(cy - ry), s(cx + rx), s(cy + ry)], fill=fill)
    elif k == 'ring':
        _, cx, cy, r, w = shape
        d.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], outline=fill, width=round(s(w)))
    elif k == 'rect':
        _, x0, y0, x1, y1 = shape
        d.rectangle([s(x0), s(y0), s(x1), s(y1)], fill=fill)
    elif k == 'poly':
        d.polygon([(s(x), s(y)) for x, y in shape[1:]], fill=fill)
    elif k == 'line':
        w, pts = shape[1], shape[2:]
        d.line([(s(x), s(y)) for x, y in pts], fill=fill, width=round(s(w)), joint='curve')
        for x, y in (pts[0], pts[-1]):
            draw(d, ('circle', x, y, w / 2), fill)
    elif k == 'taper':
        w0, w1, pts = shape[1], shape[2], shape[3:]
        n = len(pts) - 1
        for i in range(n):
            (x0, y0), (x1, y1) = pts[i], pts[i + 1]
            steps = max(2, int(math.hypot(x1 - x0, y1 - y0) * 2))
            for j in range(steps + 1):
                t = (i + j / steps) / n
                r = (w0 + (w1 - w0) * t) / 2
                draw(d, ('circle', x0 + (x1 - x0) * j / steps, y0 + (y1 - y0) * j / steps, r), fill)
    else:
        raise ValueError(k)


def cross(x, y, r, w):
    return [('line', w, (x - r, y - r), (x + r, y + r)), ('line', w, (x - r, y + r), (x + r, y - r))]


class Part:
    def __init__(self, name, colour, add, cut=()):
        self.name, self.colour, self.add, self.cut = name, colour, list(add), list(cut)


class Animal:
    """parts in the order they appear; eyes (x, y, r, filled): filled heads
    have eyes cut out of them, outline heads have them drawn in"""
    def __init__(self, name, rope_end, parts, eyes, eyes_part, filled):
        self.name, self.rope_end, self.parts = name, rope_end, parts
        self.eyes, self.eyes_part, self.filled = eyes, eyes_part, filled


def gallows(rope_end):
    return [
        Part('base', BROWN, [('rect', 2, 166, 64, 172)]),
        Part('post', BROWN, [('rect', 12, BEAM_Y, 18, 168), ('rect', 12, BEAM_Y, ROPE_X + 10, BEAM_Y + 5),
                              ('line', 3, (15, 32), (38, BEAM_Y + 3))]),
        Part('rope', YELLOW, [('line', 2, (ROPE_X, BEAM_Y + 4), (ROPE_X, rope_end))]),
    ]


def octopus_arms():
    """two straight segments and a hook each: slopes of 0, 1/4 and 1/2 unit
    per unit move a whole number of cells per cell row, so a segment repeats
    the same few glyphs all the way down"""
    slopes = {0.5: (0, 0), 1.5: (0, 0.25), 2.5: (0.25, 0.25), 3.5: (0.5, 0.25)}
    arms = []
    for i in range(8):
        f = i - 3.5
        side = 1 if f > 0 else -1
        s1, s2 = slopes[abs(f)]
        x0, y0 = ROPE_X + f * 5.5, 68
        x1, y1 = x0 + side * s1 * 24, y0 + 24
        x2, y2 = x1 + side * s2 * 40, y1 + 40
        pts = [(x0, y0), (x1, y1), (x2, y2), (x2 + side * 4, y2 + 4)]
        arms.append((abs(f), side, Part('arm', PINK, [('line', 4, *pts)])))
    # outside arms first, alternating sides
    arms.sort(key=lambda a: (-a[0], a[1]))
    return [a[2] for a in arms]


ANIMALS = [
    Animal('human', 21, [
        Part('head', WHITE, [('ring', ROPE_X, 32, 11, 3)]),
        Part('body', LBLUE, [('line', 3, (ROPE_X, 43), (ROPE_X, 100))]),
        Part('left arm', LBLUE, [('line', 3, (ROPE_X, 56), (ROPE_X - 20, 84))]),
        Part('right arm', LBLUE, [('line', 3, (ROPE_X, 56), (ROPE_X + 20, 84))]),
        Part('left leg', LGREY, [('line', 3, (ROPE_X, 100), (ROPE_X - 16, 142))]),
        Part('right leg', LGREY, [('line', 3, (ROPE_X, 100), (ROPE_X + 16, 142))]),
    ], eyes=[(ROPE_X - 4, 30, 1.6), (ROPE_X + 4, 30, 1.6)], eyes_part=0, filled=False),

    Animal('cat', 79, [
        Part('body', ORANGE, [('ellipse', 74, 92, 26, 13)],
             [('line', 1.5, (x, 82), (x - 3, 96)) for x in (62, 72, 82)]),
        Part('head', ORANGE, [('circle', 104, 80, 12)]),
        Part('ears', ORANGE, [('poly', (93, 74), (96, 60), (103, 70)), ('poly', (106, 69), (113, 59), (116, 75))]),
        Part('front legs', ORANGE, [('line', 5, (88, 98), (88, 124)), ('line', 5, (97, 96), (97, 124))]),
        Part('back legs', ORANGE, [('line', 5, (54, 98), (52, 124)), ('line', 5, (63, 101), (63, 124))]),
        Part('tail', ORANGE, [('line', 4, (50, 88), (40, 82), (34, 70), (34, 58), (40, 52))]),
        Part('whiskers', WHITE, [('line', 1, (113, 84), (119, 81)), ('line', 1, (113, 87), (119, 88))]),
    ], eyes=[(99, 78, 2), (109, 78, 2)], eyes_part=1, filled=True),

    Animal('duck', 85, [
        Part('body', YELLOW, [('ellipse', 70, 100, 25, 15)]),
        Part('head', YELLOW, [('line', 10, (86, 92), (94, 74)), ('circle', 96, 68, 11)]),
        Part('beak', ORANGE, [('poly', (104, 64), (119, 69), (104, 75))]),
        Part('wing', YELLOW, [], [('ring', 64, 97, 11, 1.5)]),
        Part('tail', YELLOW, [('poly', (48, 94), (32, 80), (40, 98))]),
        Part('feet', ORANGE, [('line', 2, (64, 113), (64, 132)), ('line', 2, (76, 113), (76, 132)),
                              ('poly', (64, 130), (56, 137), (71, 137)), ('poly', (76, 130), (68, 137), (83, 137))]),
    ], eyes=[(98, 65, 2)], eyes_part=1, filled=True),

    Animal('giraffe', 88, [
        Part('body', YELLOW, [('ellipse', 66, 100, 25, 12)]),
        Part('neck', YELLOW, [('poly', (78, 98), (90, 92), (106, 40), (97, 36))]),
        Part('head', YELLOW, [('ellipse', 106, 36, 11, 6), ('line', 2, (100, 31), (98, 22)),
                              ('line', 2, (106, 31), (106, 22)), ('circle', 98, 21, 2), ('circle', 106, 21, 2)]),
        Part('front legs', YELLOW, [('line', 4, (80, 106), (82, 150)), ('line', 4, (88, 104), (90, 150))]),
        Part('back legs', YELLOW, [('line', 4, (47, 106), (45, 150)), ('line', 4, (56, 108), (56, 150))]),
        Part('tail', YELLOW, [('line', 2, (42, 96), (34, 120)), ('circle', 34, 121, 3)]),
        Part('spots', YELLOW, [], [('circle', x, y, r) for x, y, r in
                                   ((54, 96, 3), (64, 104, 3), (74, 95, 3), (60, 92, 2), (48, 103, 2),
                                    (88, 82, 2.5), (93, 66, 2.5), (98, 52, 2))]),
    ], eyes=[(104, 33, 1.6)], eyes_part=2, filled=True),

    Animal('elephant', 76, [
        Part('body', GREY, [('ellipse', 62, 96, 30, 20)]),
        Part('head', GREY, [('circle', 96, 86, 16)]),
        Part('ears', GREY, [('ellipse', 88, 78, 9, 14)], [('ring', 88, 82, 8, 1.5)]),
        Part('trunk', GREY, [('taper', 9, 5, (106, 94), (111, 108), (109, 122), (115, 130))]),
        Part('tusk', WHITE, [('taper', 4, 2, (97, 101), (99, 109), (104, 114))]),
        Part('front legs', GREY, [('rect', 72, 108, 80, 140), ('rect', 84, 106, 92, 140)]),
        Part('back legs', GREY, [('rect', 36, 108, 44, 140), ('rect', 48, 112, 56, 140)]),
        Part('tail', GREY, [('line', 2, (33, 92), (25, 112)), ('circle', 25, 113, 2.5)]),
    ], eyes=[(102, 80, 2)], eyes_part=1, filled=True),

    Animal('octopus', 26, [Part('head', PINK, [('ellipse', ROPE_X, 50, 22, 24)])] + octopus_arms(),
           eyes=[(ROPE_X - 9, 56, 3.5), (ROPE_X + 9, 56, 3.5)], eyes_part=0, filled=True),
]

# --------------------------------------------------------------------------


def layer(shapes, cuts=()):
    img = Image.new('L', (s(UW), s(UH)), 0)
    d = ImageDraw.Draw(img)
    for sh in shapes:
        draw(d, sh, 255)
    for sh in cuts:
        draw(d, sh, 0)
    return img


def to_hires(img):
    """units at SS x -> a PW*8 x PH*8 list of rows of 0/1"""
    small = img.resize((PW * 8, PH * 8), Image.BOX)
    px = small.load()
    return [[1 if px[x, y] >= 128 else 0 for x in range(PW * 8)] for y in range(PH * 8)]


def eye_shapes(a, dead):
    out = []
    for x, y, r in a.eyes:
        if dead:
            out += cross(x, y, max(r, 2.5), 1.5)
        else:
            out.append(('circle', x, y, r))
    return out


def stage_layers(a, n, dead=False):
    """(colour, bitmap) per present part, and the whole picture, after n parts"""
    parts = gallows(a.rope_end) + a.parts[:n]
    cuts = [c for p in parts for c in p.cut]
    eyes_in = n > a.eyes_part
    eyes = eye_shapes(a, dead) if eyes_in else []
    if a.filled:
        cuts += eyes
    pics = []
    for i, p in enumerate(parts):
        extra = eyes if (not a.filled and eyes_in and i == len(gallows(0)) + a.eyes_part) else []
        pics.append((p.colour, to_hires(layer(p.add + extra, cuts))))
    whole = [[0] * (PW * 8) for _ in range(PH * 8)]
    for _, b in pics:
        for y in range(PH * 8):
            row, src = whole[y], b[y]
            for x in range(PW * 8):
                row[x] |= src[x]
    return pics, whole


def cells(a, n, dead=False):
    """cell index -> (pattern bytes, colour) for the non-empty cells"""
    pics, whole = stage_layers(a, n, dead)
    out = {}
    for cy in range(PH):
        for cx in range(PW):
            pat = bytes(sum(whole[cy * 8 + r][cx * 8 + c] << (7 - c) for c in range(8)) for r in range(8))
            if not any(pat):
                continue
            best, colour = -1, WHITE
            for col, b in pics:
                cnt = sum(b[cy * 8 + r][cx * 8 + c] for r in range(8) for c in range(8))
                if cnt > best:
                    best, colour = cnt, col
            out[cy * PW + cx] = (pat, colour)
    return out


FIXED = [
    [0, 0, 0, 0xFF, 0xFF, 0, 0, 0],                    # $40 horizontal line
    [0x18] * 8,                                         # $41 vertical line
    [0, 0, 0, 0x1F, 0x1F, 0x18, 0x18, 0x18],            # $42 top left corner
    [0, 0, 0, 0xF8, 0xF8, 0x18, 0x18, 0x18],            # $43 top right
    [0x18, 0x18, 0x18, 0x1F, 0x1F, 0, 0, 0],            # $44 bottom left
    [0x18, 0x18, 0x18, 0xF8, 0xF8, 0, 0, 0],            # $45 bottom right
    [0, 0, 0, 0, 0, 0, 0xFF, 0xFF],                     # $46 a letter's slot
    [0xFF] * 8,                                         # $47 solid
    [0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF],   # $48 hollow box
    [0, 0, 0, 0x3C, 0x3C, 0, 0, 0],                     # $49 a dot: a letter tried
]


def build_level(a):
    stages = [cells(a, n) for n in range(len(a.parts) + 1)] + [cells(a, len(a.parts), True)]
    pats = []
    for st in stages:
        for pat, _ in st.values():
            if pat != bytes([0xFF] * 8) and pat not in pats:
                pats.append(pat)
    if len(pats) > 256 - GLYPH0:
        sys.exit('%s: %d glyphs, room for %d' % (a.name, len(pats), 256 - GLYPH0))
    code = {p: GLYPH0 + i for i, p in enumerate(pats)}
    code[bytes([0xFF] * 8)] = SOLID
    diffs, starts, prev = [], [], {}
    for st in stages:
        starts.append(len(diffs))
        for off in sorted(set(st) | set(prev)):
            new = st.get(off)
            if new == prev.get(off):
                continue
            if new is None:
                diffs.append((off, SPACE, WHITE))
            else:
                diffs.append((off, code[new[0]], new[1]))
        prev = st
    starts.append(len(diffs))
    return pats, diffs, starts, stages


def preview(levels, path):
    sc = 3
    w, h = PW * 8, PH * 16
    ncol = max(len(a.parts) + 2 for a in ANIMALS)
    img = Image.new('RGB', ((w + 8) * ncol, (h + 8) * len(levels)), (40, 40, 40))
    px = img.load()
    for li, (a, (pats, diffs, starts, stages)) in enumerate(zip(ANIMALS, levels)):
        for si, st in enumerate(stages):
            ox, oy = si * (w + 8), li * (h + 8)
            for off, (pat, col) in st.items():
                cx, cy = off % PW, off // PW
                for r in range(8):
                    for c in range(8):
                        if pat[r] >> (7 - c) & 1:
                            for k in range(2):
                                px[ox + cx * 8 + c, oy + (cy * 8 + r) * 2 + k] = RGB[col]
    img = img.resize((img.width * sc // 2, img.height * sc // 2), Image.NEAREST)
    img.save(path)


# --------------------------------------------------------------------------
# Words

FREQ = 'etaoinshrdlucmfwypvbgkjqxz'


def misses(word):
    left, miss = set(word), 0
    for ch in FREQ:
        if not left:
            break
        if ch in left:
            left.discard(ch)
        else:
            miss += 1
    return miss


def load_words(path):
    out = []
    for line in open(path):
        w = line.split('#')[0].strip().lower()
        if not w:
            continue
        if not w.isalpha() or not w.isascii() or not 3 <= len(w) <= 14:
            sys.exit('%s: bad word %r (3-14 letters a-z)' % (path, w))
        if w not in out:
            out.append(w)
    return out


def c_bytes(data, per=16):
    data = list(data)
    return ',\n'.join('    ' + ', '.join('0x%02X' % b for b in data[i:i + per]) for i in range(0, len(data), per))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    build = here
    levels = [build_level(a) for a in ANIMALS]

    o = ['/* generated by gen_gfx.py -- do not edit */', '',
         '#define PIC_W %d' % PW, '#define PIC_H %d' % PH, '#define GLYPH0 0x%02X' % GLYPH0,
         '#define FIXED0 0x%02X' % FIXED0, '#define NLEVELS %d' % len(ANIMALS), '',
         'static const unsigned char fixed_glyphs[%d] = {' % (len(FIXED) * 8),
         c_bytes(b for g in FIXED for b in g), '};', '']
    for i, (a, (pats, diffs, starts, _)) in enumerate(zip(ANIMALS, levels)):
        o += ['/* %s: %d parts, %d glyphs, %d cell changes */' % (a.name, len(a.parts), len(pats), len(diffs)),
              'static const unsigned char glyphs%d[%d] = {' % (i, len(pats) * 8),
              c_bytes(b for p in pats for b in p), '};',
              'static const unsigned char diffs%d[%d] = {' % (i, len(diffs) * 4),
              c_bytes(b for off, ch, col in diffs for b in (off & 0xFF, off >> 8, ch, col)), '};',
              'static const unsigned int stages%d[%d] = {%s};' % (i, len(starts), ', '.join(map(str, starts))), '']
    o += ['static const unsigned char * const lvl_glyphs[NLEVELS] = {%s};' %
          ', '.join('glyphs%d' % i for i in range(len(ANIMALS))),
          'static const unsigned char lvl_nglyphs[NLEVELS] = {%s};' %
          ', '.join(str(len(l[0])) for l in levels),
          'static const unsigned char * const lvl_diffs[NLEVELS] = {%s};' %
          ', '.join('diffs%d' % i for i in range(len(ANIMALS))),
          'static const unsigned int * const lvl_stages[NLEVELS] = {%s};' %
          ', '.join('stages%d' % i for i in range(len(ANIMALS))),
          'static const unsigned char lvl_parts[NLEVELS] = {%s};' %
          ', '.join(str(len(a.parts)) for a in ANIMALS),
          'static const char * const lvl_name[NLEVELS] = {%s};' %
          ', '.join('s"%s"' % a.name for a in ANIMALS), '']
    open(os.path.join(build, 'gfx.h'), 'w').write('\n'.join(o))

    words = load_words(os.path.join(here, 'words.txt'))
    order = sorted(words, key=lambda w: (misses(w), -len(w)))
    data, offs = [], []
    for w in order:
        offs.append(len(data))
        data += [ord(c) - 96 for c in w] + [0]
    n = len(order)
    tiers = [n * i // len(ANIMALS) for i in range(len(ANIMALS) + 1)]
    o = ['/* generated by gen_gfx.py from words.txt -- do not edit */', '',
         '/* %d words, easiest first; level n draws from words tier[n] .. tier[n+1]-1 */' % n,
         '#define NWORDS %d' % n, '',
         'static const unsigned char word_text[%d] = {' % len(data), c_bytes(data), '};',
         'static const unsigned int word_off[NWORDS] = {',
         ',\n'.join('    ' + ', '.join(map(str, offs[i:i + 12])) for i in range(0, n, 12)), '};',
         'static const unsigned int tier[NLEVELS + 1] = {%s};' % ', '.join(map(str, tiers)), '']
    open(os.path.join(build, 'words.h'), 'w').write('\n'.join(o))

    for a, l in zip(ANIMALS, levels):
        print('%-9s %d parts %3d glyphs %4d changes' % (a.name, len(a.parts), len(l[0]), len(l[1])))
    print('%d words' % n)
    if '--preview' in sys.argv:
        preview(levels, os.path.join(build, 'preview.png'))
        print('preview.png')


if __name__ == '__main__':
    main()
