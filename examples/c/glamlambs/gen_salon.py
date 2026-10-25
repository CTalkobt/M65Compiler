#!/usr/bin/env python3
"""Generate the salon scene for Glam Lambs: a full-colour picture.

Usage: python3 gen_salon.py <outdir> [--preview FILE.png [wool,lips,lash,bow,ram,blink]]

The salon is a full-screen room under the score line (screen rows 1-23,
80 x 23 cells): the sheep in a salon chair, facing a vanity mirror lit by
bulbs that shows its face from the front, the menu on the cabinet's
doors, the season's trend on a sign. The VIC-IV shows it in full-colour
character mode (FCM: 8 x 8 pixels a character, a byte a pixel, any of
256 colours) with 16-bit character numbers (SEAM), so every cell can be
its own character; identical cells share one. In 80 columns a pixel is
half as wide as it is tall: the picture is drawn at 640 x 368 square
units (a cell 8 x 16) and halved vertically.

Palette entries: 0-15 the game's (14: the sign's and the doors'
background, the colour the text cells show: $D021 in the salon); 16-31
the wool, shades from dark to light (the game writes them for the dye);
32-39 the lipstick; 40-43 and 44-47 the mirror's two sets of bulbs (the
game swaps them); 48-254 the rest. (A pixel never uses 0, transparent,
or 255, the cell's colour.)

The head's looks: the eyes (eyelashes, a blink) and the head's top (a
ram's horns, a bow), in profile and in the mirror, are runs of cells
along the rows stored once per combination of their group (four each);
the generator checks that every one of the 16 looks is the two groups'.

Output: salon1.dat (GLAMSALON on the disk, loaded by the game into bank
4 at $40000: the first 960 characters, numbers from $1000), salon2.dat
(GLAMSALON2, into bank 5 at $52800: the other characters, then the
screen, the looks and the palette) and salon.h (where things are). The
Makefile's "salon" target runs this (it needs Python's PIL); the files
are kept in the repository.
"""
import math
import random
import sys

from PIL import Image, ImageDraw

COLS, ROWS = 80, 23
UW, UH = 640, 368                       # square units (a cell: 8 x 16)
SS = 2                                  # supersampling of the drawing
PW, PH = 640, 184                       # the pixels shown

LOAD_LIN = 0x40000                      # salon1.dat: characters (to the game's character sets)
LIMIT = 0x4F000
LOAD2_LIN = 0x52800                     # salon2.dat: the rest, then the tables
LIMIT2 = 0x5A000                        # (the game's code overlays from here)

P_WOOL, N_WOOL = 16, 16
P_LIPS, N_LIPS = 32, 8
P_BULBA, P_BULBB, N_BULB = 40, 44, 4
P_SHADOW, P_BLUSH, P_HOOF, N_TONE = 48, 52, 56, 4
P_GLASS = 14                            # (the text cells' background: $D021)
P_FIRST = 60                            # the picture's own colours: 60-254
P_LAST = 254

GLASS_RGB = (0x2A, 0x16, 0x34)          # the sign's and the doors' background (palette 14)
MIRROR_RGB = (0x30, 0x24, 0x44)         # the mirror's glass
WALL = (0x5A, 0x26, 0x4E)

# the preview's dye and lipstick (the game's colours)
WOOL_RGB = [(0xF4, 0xF0, 0xE8), (0xFF, 0x88, 0xC8), (0x70, 0xA8, 0xFF), (0xF8, 0xD8, 0x30)]
LIPS_RGB = [(0x30, 0x18, 0x20), (0xE0, 0x20, 0x30), (0xA8, 0x48, 0xD0), (0xFF, 0x88, 0xC8)]
BULB_ON, BULB_OFF = (0xFF, 0xF0, 0xA0), (0xC0, 0x90, 0x40)

# roles of a pixel (the role image): what the game recolours
R_ART, R_WOOL, R_LIPS, R_BULBA, R_BULBB, R_GLASS, R_SHADOW, R_BLUSH, R_HOOF = 0, 1, 2, 3, 4, 5, 6, 7, 8
TONE_ROLES = {R_SHADOW: P_SHADOW, R_BLUSH: P_BLUSH, R_HOOF: P_HOOF}

# the looks' groups of cells: (name, the number of looks, the scene's
# parameters for look k); look 0 is the plain one (the room's)
GROUPS = [
    ("EYES", 4, lambda k: {"lash": k >> 1, "blink": k & 1}),
    ("HORNS", 2, lambda k: {"ram": k}),
    ("HAT", 6, lambda k: {"hat": k}),       # none, red bow, dotty bow, flower, tiara, top hat
    ("NECK", 3, lambda k: {"neck": k}),     # none, pearls, a ribbon collar
]

# the tones' choices (palette entries P_SHADOW.., P_BLUSH.., P_HOOF..: four
# shades each, dark to light); choice 0 is the sheep's own
SHADOWS = [None, (0x50, 0x90, 0xFF), (0xB0, 0x50, 0xE0), (0xF0, 0xC0, 0x40)]   # none, blue, purple, gold
BLUSHES = [None, (0xFF, 0x70, 0x90)]                                            # none, rosy
HOOVES = [(0x30, 0x2C, 0x30), (0xE0, 0x20, 0x30), (0xFF, 0x80, 0xC0), (0xE8, 0xB8, 0x40)]  # natural, red, pink, gold

# where things are (cells; rows of the picture: screen row - 1)
SIGN = (2, 21, 0, 3)                    # columns, rows of the sign over the chair (text: rows 1-2)
MIRROR = (50, 76, 1, 11)                # the frame
GLASS = (51, 75, 2, 10)                 # the glass: the sheep's reflection
CABINET = (50, 76, 14, 21)              # the doors: the menu and the help

# where sparkles may twinkle (cells on the wall and the glass, away from
# the looks' cells), by the part they celebrate
SPARKS = {
    "WOOL": [(6, 5), (20, 15), (40, 15), (24, 4)],
    "EYE": [(53, 3), (73, 3), (48, 5), (53, 8)],
    "LIPS": [(73, 9), (53, 10), (48, 9), (72, 6)],
    "BOW": [(30, 3), (44, 1), (60, 2), (66, 2)],
    "NECK": [(49, 9), (36, 8), (52, 11), (74, 11)],
    "HOOF": [(30, 19), (47, 20), (40, 18), (48, 18)],
}


def U(c):                               # a column's left edge in units
    return c * 8


def V(r):                               # a picture row's top edge in units
    return r * 16


class Pic:
    """the picture: colour (RGB) and role, drawn at SS x the units"""

    def __init__(self):
        self.rgb = Image.new("RGB", (UW * SS, UH * SS), WALL)
        self.role = Image.new("L", (UW * SS, UH * SS), R_ART)
        self.d = ImageDraw.Draw(self.rgb)
        self.r = ImageDraw.Draw(self.role)

    def _box(self, b):
        return [v * SS for v in b]

    def rect(self, b, fill, role=R_ART):
        self.d.rectangle(self._box(b), fill=fill)
        self.r.rectangle(self._box(b), fill=role)

    def rrect(self, b, rad, fill, role=R_ART):
        self.d.rounded_rectangle(self._box(b), rad * SS, fill=fill)
        self.r.rounded_rectangle(self._box(b), rad * SS, fill=role)

    def ellipse(self, b, fill, role=R_ART):
        self.d.ellipse(self._box(b), fill=fill)
        self.r.ellipse(self._box(b), fill=role)

    def circle(self, x, y, rad, fill, role=R_ART):
        self.ellipse((x - rad, y - rad, x + rad, y + rad), fill, role)

    def poly(self, pts, fill, role=R_ART):
        p = [(x * SS, y * SS) for x, y in pts]
        self.d.polygon(p, fill=fill)
        self.r.polygon(p, fill=role)

    def line(self, pts, fill, w, role=R_ART):
        p = [(x * SS, y * SS) for x, y in pts]
        self.d.line(p, fill=fill, width=max(1, int(w * SS)), joint="curve")
        self.r.line(p, fill=role, width=max(1, int(w * SS)))

    def arc(self, b, a0, a1, fill, w, role=R_ART):
        self.d.arc(self._box(b), a0, a1, fill=fill, width=max(1, int(w * SS)))
        self.r.arc(self._box(b), a0, a1, fill=role, width=max(1, int(w * SS)))

    def limb(self, pts, w, fill, role=R_ART):
        """a thick line with round ends and joints"""
        self.line(pts, fill, w, role)
        for x, y in pts:
            self.circle(x, y, w / 2.0, fill, role)


def mix(a, b, t):
    return tuple(int(round(a[i] + (b[i] - a[i]) * t)) for i in range(3))


def shaded_ball(p, x, y, rad, dark, light, role=R_ART, steps=6):
    """a ball lit from the top left: rings toward the highlight"""
    for i in range(steps):
        t = i / (steps - 1)
        r = rad * (1 - 0.75 * t)
        cx, cy = x - rad * 0.35 * t, y - rad * 0.35 * t
        p.circle(cx, cy, r, mix(dark, light, t), role)


WOOL_DARK, WOOL_LIGHT = (104, 104, 104), (255, 255, 255)   # (the fleece's base: depth between tufts)


def grey(v):
    v = int(max(0, min(255, v)))
    return (v, v, v)


def tuft(p, x, y, r, lit):
    """a tuft of wool, lit lit (0-255) at its top: a soft shadow under it,
    its body, a lighter top and the gleam of a curl"""
    p.circle(x + r * 0.2, y + r * 0.35, r, grey(lit * 0.55), R_WOOL)
    p.circle(x, y, r * 0.9, grey(lit * 0.78), R_WOOL)
    p.circle(x - r * 0.15, y - r * 0.25, r * 0.62, grey(lit * 0.92), R_WOOL)
    p.arc((x - r * 0.6, y - r * 0.7, x + r * 0.3, y + r * 0.15), 200, 330, grey(min(255, lit * 1.08)), max(1.0, r * 0.18), R_WOOL)


def curl(p, x, y, rad, seed=0):
    """a little cluster of tufts (a forelock's, a tail's)"""
    rnd = random.Random(int(x * 7 + y * 13 + seed))
    for i in range(4):
        a = rnd.uniform(0, 2 * math.pi)
        d = rad * rnd.uniform(0.15, 0.5)
        tuft(p, x + d * math.cos(a), y + d * math.sin(a) - rad * 0.1 * i, rad * rnd.uniform(0.5, 0.7),
             rnd.uniform(205, 240) - 12 * (3 - i))


def fleece(p, region, rad, seed):
    """wool over a region (a test of a point): small tufts close together,
    a jittered grid drawn top to bottom (the lower ones in front), lit from
    above: light on the back, darker toward the belly"""
    rnd = random.Random(seed)
    x0, y0, x1, y1, inside = region
    pts = []
    y, row = y0, 0
    step = rad * 0.85
    while y <= y1:
        x = x0 + (step * 0.5 if row % 2 else 0)
        while x <= x1:
            cx, cy = x + rnd.uniform(-0.45, 0.45) * step, y + rnd.uniform(-0.4, 0.4) * step
            if inside(cx, cy):
                pts.append((cy, cx, rad * rnd.uniform(0.75, 1.2)))
            elif any(inside(cx + dx * rad, cy + dy * rad) for dx, dy in ((0.9, 0), (-0.9, 0), (0, 0.9), (0, -0.9))):
                pts.append((cy, cx, rad * rnd.uniform(0.55, 0.85)))     # (a lumpy edge)
            x += step
        y += step * 0.8
        row += 1
    pts.sort()
    for cy, cx, r in pts:
        t = (cy - y0) / max(1.0, y1 - y0)
        tuft(p, cx, cy, r, 250 - 85 * t + rnd.uniform(-10, 10))


def in_ellipse(cx, cy, rx, ry):
    return lambda x, y: ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0


# ---------------------------------------------------------------------------
# the room

WALL_DARK = (0x4A, 0x1E, 0x40)
GOLD = (0xE0, 0xB4, 0x50)
GOLD_DARK = (0x9C, 0x74, 0x2C)
GOLD_LIGHT = (0xFF, 0xE8, 0x9C)
WOOD = (0x80, 0x4C, 0x2C)
WOOD_DARK = (0x5C, 0x34, 0x1C)


def room(p):
    # the wall: a damask of small diamonds (every two cells across, a row down)
    for r in range(0, 12):
        for c in range(0, 50, 2):
            x, y = U(c) + 8 + (8 if r % 2 else 0), V(r) + 8
            p.poly([(x, y - 5), (x + 3, y), (x, y + 5), (x - 3, y)], WALL_DARK)
    # the dado rail and the panelled wall under it
    p.rect((0, V(12) - 2, U(50) - 1, V(12) + 3), GOLD)
    p.rect((0, V(12) + 4, U(50) - 1, V(12) + 5), GOLD_DARK)
    for c in range(1, 50, 4):
        p.rrect((U(c) + 2, V(12) + 12, U(c + 3) + 4, V(16) - 6), 3, WALL_DARK)
        p.rrect((U(c) + 4, V(12) + 14, U(c + 3) + 2, V(16) - 8), 3, WALL)
    # the skirting board and the floor: black and white tiles
    p.rect((0, V(16) + 4, UW - 1, V(17) - 1), WOOD)
    p.rect((0, V(16) + 4, UW - 1, V(16) + 5), GOLD)
    for ty in range(V(17), UH, 16):
        for tx in range(0, UW, 16):
            white = ((tx // 16) + (ty // 16)) % 2 == 0
            p.rect((tx, ty, tx + 15, ty + 15), (0xE8, 0xE4, 0xDC) if white else (0x24, 0x22, 0x28))
    # the sign over the chair (its text: the game writes it)
    x0, x1, y0, y1 = U(SIGN[0]) + 4, U(SIGN[1] + 1) - 4, V(SIGN[2]) + 4, V(SIGN[3] + 1) - 4
    p.rrect((x0 - 3, y0 - 3, x1 + 3, y1 + 3), 6, GOLD_DARK)
    p.rrect((x0 - 1, y0 - 1, x1 + 1, y1 + 1), 5, GOLD)
    p.rrect((x0 + 3, y0 + 3, x1 - 3, y1 - 3), 3, GLASS_RGB, R_GLASS)
    p.rect((U(SIGN[0] + 1), V(SIGN[2] + 1), U(SIGN[1]) - 1, V(SIGN[3]) - 1), GLASS_RGB, R_GLASS)
    # the sign's chains
    for x in (x0 + 12, x1 - 12):
        p.line([(x, 0), (x, y0 - 3)], GOLD_DARK, 2)


def vanity(p, look):
    c0, c1, r0, r1 = MIRROR
    x0, x1, y0, y1 = U(c0), U(c1 + 1) - 1, V(r0), V(r1 + 1) - 1
    p.rrect((x0, y0 + 2, x1, y1 - 2), 6, GOLD_DARK)
    p.rrect((x0 + 2, y0 + 4, x1 - 2, y1 - 4), 5, GOLD)
    p.rect((x0 + 6, y0 + 12, x1 - 6, y1 - 12), GOLD_LIGHT)
    # the glass, the sheep's face in it, a shine in two corners
    gx0, gx1, gy0, gy1 = U(GLASS[0]), U(GLASS[1] + 1) - 1, V(GLASS[2]), V(GLASS[3] + 1) - 1
    p.rect((gx0, gy0, gx1, gy1), MIRROR_RGB)
    reflection(p, (gx0 + gx1) / 2.0, gy0, gy1, look)
    shine = (0x8C, 0x7C, 0xA8)
    p.line([(gx0 + 4, gy0 + 34), (gx0 + 22, gy0 + 4)], shine, 2)
    p.line([(gx0 + 4, gy0 + 50), (gx0 + 12, gy0 + 36)], shine, 2)
    p.line([(gx1 - 22, gy1 - 4), (gx1 - 4, gy1 - 34)], shine, 2)
    p.line([(gx1 - 12, gy1 - 4), (gx1 - 4, gy1 - 18)], shine, 2)
    # the frame again over the glass's edges (the reflection stays inside)
    for b in ((x0, y0 + 2, x1, gy0 - 1), (x0, gy1 + 1, x1, y1 - 2), (x0, y0 + 2, gx0 - 1, y1 - 2), (gx1 + 1, y0 + 2, x1, y1 - 2)):
        p.rect(b, GOLD)
    p.rect((x0, y0 + 2, x1, y0 + 3), GOLD_DARK)
    p.rect((x0, y1 - 3, x1, y1 - 2), GOLD_DARK)
    p.rect((gx0 - 2, gy0 - 2, gx1 + 2, gy0 - 1), GOLD_LIGHT)
    p.rect((gx0 - 2, gy1 + 1, gx1 + 2, gy1 + 2), GOLD_DARK)
    p.rect((gx0 - 2, gy0 - 2, gx0 - 1, gy1 + 2), GOLD_LIGHT)
    p.rect((gx1 + 1, gy0 - 2, gx1 + 2, gy1 + 2), GOLD_DARK)
    # (the wall round the frame, where the reflection's wool spread)
    p.rect((x0 - 8, y1 - 1, x1 + 8, V(13) - 1), WALL)
    p.rect((x0 - 8, y0, x0 - 1, y1), WALL)
    p.rect((x1 + 1, y0, x1 + 8, y1), WALL)
    # the bulbs: round, on the frame, two sets that take turns
    k = 0
    for c in range(c0 + 1, c1, 2):
        p.circle(U(c) + 4, y0 + 8, 5, BULB_ON, R_BULBA if k % 2 == 0 else R_BULBB)
        k += 1
    for r in range(r0 + 1, r1, 2):
        for c in (c0, c1):
            p.circle(U(c) + 4, V(r) + 8, 5, BULB_ON, R_BULBA if (r + c) % 4 < 2 else R_BULBB)
    # the counter and what stands on it
    cx0, cx1 = U(49), U(78) - 1
    p.rect((cx0, V(13), cx1, V(13) + 3), GOLD)
    p.rect((cx0, V(13) + 4, cx1, V(14) - 1), WOOD)
    p.rect((cx0, V(14) - 3, cx1, V(14) - 1), WOOD_DARK)
    base = V(13)
    for i, col in enumerate(((0xFF, 0x88, 0xC8), (0x70, 0xA8, 0xFF), (0xF8, 0xD8, 0x30))):   # the dyes
        x = U(52 + 2 * i)
        p.rrect((x - 1, base - 12, x + 8, base - 1), 3, mix(col, (0, 0, 0), 0.35))
        p.rrect((x, base - 11, x + 6, base - 2), 2, col)
        p.rect((x + 2, base - 16, x + 4, base - 12), GOLD)
    x = U(59)                                                       # a lipstick
    p.rect((x, base - 9, x + 6, base - 1), GOLD)
    p.poly([(x + 1, base - 9), (x + 6, base - 9), (x + 6, base - 17), (x + 2, base - 15)], (0xE0, 0x20, 0x30))
    x = U(61)                                                       # mascara
    p.rect((x + 2, base - 14, x + 5, base - 1), (0x18, 0x18, 0x1C))
    p.rect((x + 2, base - 18, x + 5, base - 15), GOLD)
    x = U(63)                                                       # a ribbon
    p.circle(x + 8, base - 7, 7, (0xC0, 0x18, 0x28))
    p.circle(x + 8, base - 7, 5, (0xE8, 0x30, 0x40))
    p.circle(x + 8, base - 7, 2, GOLD)
    x = U(67)                                                       # a powder compact
    p.ellipse((x, base - 6, x + 15, base - 1), GOLD)
    p.ellipse((x + 3, base - 8, x + 12, base - 4), (0xF8, 0xB8, 0xC8))
    x = U(71)                                                       # a brush
    p.rect((x + 3, base - 9, x + 4, base - 1), GOLD)
    p.rrect((x + 1, base - 18, x + 6, base - 9), 2, (0x50, 0x3C, 0x30))
    # the cabinet: wood, gold-trimmed doors (flat: the menu), on the floor
    p.rect((cx0, V(14), cx1, V(22) + 3), WOOD)
    p.rect((cx0, V(22) + 4, cx1, UH - 1), WOOD_DARK)
    dx0, dx1, dy0, dy1 = U(CABINET[0]), U(CABINET[1] + 1) - 1, V(CABINET[2]), V(CABINET[3] + 1) - 1
    p.rect((dx0 - 2, dy0, dx1 + 2, dy1 + 2), GOLD)
    p.rect((dx0, dy0, dx1, dy1), GLASS_RGB, R_GLASS)


# ---------------------------------------------------------------------------
# the chair: black leather, chrome

LEATHER = (0x26, 0x24, 0x2C)
LEATHER_HI = (0x5A, 0x56, 0x66)
CHROME = (0xB8, 0xBC, 0xC8)
CHROME_HI = (0xF4, 0xF6, 0xFC)
CHROME_LO = (0x60, 0x64, 0x70)


def chrome_bar(p, x0, y0, x1, y1):
    """a vertical chrome bar: dark edges, a light stripe"""
    w = x1 - x0 + 1
    for i in range(w):
        t = abs((i + 0.5) / w - 0.35) * 2
        p.rect((x0 + i, y0, x0 + i, y1), mix(CHROME_HI, CHROME_LO, min(1.0, t)))


def chair(p):
    # the base and the shadow under it
    p.ellipse((70, V(22) - 6, 300, V(22) + 22), (0x16, 0x14, 0x18))
    p.ellipse((96, V(21) + 2, 268, V(22) + 8), CHROME_LO)
    p.ellipse((98, V(21), 266, V(22) + 4), CHROME)
    p.ellipse((130, V(21) + 2, 220, V(21) + 10), CHROME_HI)
    # the pole, the pedal, the footrest
    chrome_bar(p, 168, V(16), 184, V(21) + 6)
    p.rect((150, V(17) + 4, 200, V(17) + 10), CHROME_LO)
    p.rect((150, V(17) + 2, 200, V(17) + 7), CHROME)
    p.line([(184, V(20)), (226, V(20) - 4)], CHROME, 5)
    p.line([(184, V(18) + 8), (300, V(19) + 6)], CHROME, 5)
    p.rrect((290, V(19) + 4, 372, V(19) + 11), 3, CHROME_LO)
    p.rrect((290, V(19) + 2, 372, V(19) + 8), 3, CHROME)
    # the back: tall, buttoned (under the sign)
    p.rrect((18, V(4) - 4, 100, V(16) + 2), 26, (0x10, 0x10, 0x14))
    p.rrect((22, V(4), 96, V(16) - 2), 24, LEATHER)
    p.rrect((30, V(4) + 8, 54, V(15)), 12, mix(LEATHER, LEATHER_HI, 0.35))
    for r in range(5, 15, 2):
        for x in (44, 74):
            p.circle(x, V(r) + 8, 3, (0x10, 0x10, 0x14))
            p.circle(x - 1, V(r) + 7, 1.5, LEATHER_HI)
    # the seat and its arm
    p.rrect((50, V(14) - 2, 330, V(16) + 4), 14, (0x10, 0x10, 0x14))
    p.rrect((52, V(14), 328, V(16)), 12, LEATHER)
    p.rrect((70, V(14) + 2, 320, V(14) + 10), 6, mix(LEATHER, LEATHER_HI, 0.5))
    p.rect((120, V(16) + 2, 240, V(16) + 8), CHROME_LO)


# ---------------------------------------------------------------------------
# the sheep in profile (facing right, sitting)

FACE = (0x3A, 0x34, 0x38)
FACE_HI = (0x6A, 0x60, 0x66)
FACE_LO = (0x22, 0x1E, 0x22)
EAR_IN = (0x8A, 0x5C, 0x66)
HORN = (0xC8, 0xA8, 0x78)
HORN_LO = (0x8A, 0x6C, 0x46)
EYE_WHITE = (0xF8, 0xF6, 0xF0)
IRIS = (0x40, 0x28, 0x18)
INK = (0x08, 0x06, 0x08)
BOW = (0xE0, 0x24, 0x40)
BOW_LO = (0x98, 0x10, 0x28)
BOW_HI = (0xFF, 0x70, 0x80)


HAT_PINK, HAT_PINK_LO = (0xFF, 0x7C, 0xC0), (0xC0, 0x40, 0x88)
PETAL, PETAL_LO, PETAL_MID = (0xFF, 0xD8, 0xEC), (0xE0, 0x88, 0xB8), (0xF8, 0xD0, 0x30)
GEM_BLUE, GEM_PINK = (0x50, 0xA0, 0xFF), (0xFF, 0x60, 0xA0)
HAT_BLACK, HAT_GREY = (0x18, 0x16, 0x1C), (0x50, 0x4C, 0x58)
PEARL, PEARL_LO = (0xF8, 0xF4, 0xEC), (0xA8, 0xA0, 0x98)
RIBBON, RIBBON_LO = (0xFF, 0x60, 0xA8), (0xB0, 0x30, 0x70)


def tone(p, box, role, steps=4):
    """an ellipse in a tone's shades (the game colours them): darker at the edge"""
    x0, y0, x1, y1 = box
    for i in range(steps):
        t = i / (steps - 1)
        sx, sy = (x1 - x0) * 0.22 * t, (y1 - y0) * 0.22 * t
        v = int(48 + 190 * t)
        p.ellipse((x0 + sx - 0.6 * sx, y0 + sy - 0.6 * sy, x1 - sx - 0.6 * sx, y1 - sy - 0.6 * sy), (v, v, v), role)


def eye(p, ex, ey, rad, look_dx, lash, blink, side=1):
    """a big cartoon eye under its lid (the eyeshadow's tone); lashes curl
    out to one side (side: 1 right, -1 left); a blink shows the lid"""
    tone(p, (ex - rad - 3, ey - rad - 6, ex + rad + 3, ey + rad - 1), R_SHADOW)
    if blink:
        tone(p, (ex - rad - 1, ey - rad - 2, ex + rad + 1, ey + rad), R_SHADOW)
        p.arc((ex - rad, ey - rad + 2, ex + rad, ey + rad - 1), 20, 160, INK, 2.5)
    else:
        p.ellipse((ex - rad - 1, ey - rad, ex + rad + 1, ey + rad), INK)
        p.ellipse((ex - rad, ey - rad + 1, ex + rad, ey + rad - 1), EYE_WHITE)
        ix = ex + look_dx
        p.ellipse((ix - rad * 0.5, ey - rad * 0.6, ix + rad * 0.5, ey + rad * 0.6), IRIS)
        p.ellipse((ix - rad * 0.3, ey - rad * 0.4, ix + rad * 0.3, ey + rad * 0.4), INK)
        p.circle(ix - rad * 0.15, ey - rad * 0.3, max(1.2, rad * 0.15), (0xFF, 0xFF, 0xFF))
    if lash:
        p.arc((ex - rad - 1, ey - rad - 1, ex + rad + 1, ey + rad + 1), 190, 350, INK, 3)
        for a in (236, 253, 270, 287, 304):
            ra = math.radians(a)
            l = rad * (0.8 + 0.5 * math.sin(math.radians(a - 180)))
            sx, sy = ex + (rad + 1) * math.cos(ra), ey + (rad + 1) * math.sin(ra)
            tx, ty = ex + (rad + 1 + l) * math.cos(ra), ey + (rad + 1 + l) * math.sin(ra)
            p.line([(sx, sy), ((sx + tx) / 2 + 1 * side, (sy + ty) / 2 - 2), (tx + 2 * side, ty)], INK, 2)


def bow_at(p, bx, by, size=1.0, col=BOW, lo=BOW_LO, hi=BOW_HI, dots=False):
    s = size
    p.poly([(bx, by), (bx - 26 * s, by - 14 * s), (bx - 22 * s, by + 16 * s)], lo)
    p.poly([(bx, by), (bx + 26 * s, by - 14 * s), (bx + 22 * s, by + 16 * s)], lo)
    p.poly([(bx, by), (bx - 23 * s, by - 11 * s), (bx - 20 * s, by + 12 * s)], col)
    p.poly([(bx, by), (bx + 23 * s, by - 11 * s), (bx + 20 * s, by + 12 * s)], col)
    if dots:
        for dx, dy in ((-15, -4), (-11, 5), (-18, 4), (15, -4), (11, 5), (18, 4)):
            p.circle(bx + dx * s, by + dy * s, 2.2 * s, (0xFF, 0xFF, 0xFF))
    p.poly([(bx - 3 * s, by + 2 * s), (bx - 12 * s, by + 22 * s), (bx - 6 * s, by + 24 * s)], lo)
    p.poly([(bx + 3 * s, by + 2 * s), (bx + 12 * s, by + 22 * s), (bx + 6 * s, by + 24 * s)], lo)
    p.circle(bx, by, 6 * s, lo)
    p.circle(bx - 1, by - 1, 4 * s, hi)


def hat(p, kind, x, y, s=1.0):
    """headwear sitting at x, y (its bottom middle): 1 a red bow, 2 a dotty
    bow, 3 a flower, 4 a tiara, 5 a top hat"""
    if kind == 1:
        bow_at(p, x, y - 10 * s, s * 0.9)
    elif kind == 2:
        bow_at(p, x, y - 10 * s, s * 0.9, HAT_PINK, HAT_PINK_LO, (0xFF, 0xC0, 0xE0), True)
    elif kind == 3:
        cy = y - 12 * s
        for i in range(6):
            a = math.radians(60 * i + 15)
            p.circle(x + 9 * s * math.cos(a), cy + 9 * s * math.sin(a), 7 * s, PETAL_LO)
        for i in range(6):
            a = math.radians(60 * i + 15)
            p.circle(x + 8 * s * math.cos(a) - 1, cy + 8 * s * math.sin(a) - 1, 5.5 * s, PETAL)
        p.circle(x, cy, 6 * s, (0xC8, 0x90, 0x10))
        p.circle(x - 1, cy - 1, 4.5 * s, PETAL_MID)
        p.poly([(x + 10 * s, cy + 8 * s), (x + 22 * s, cy + 4 * s), (x + 14 * s, cy + 14 * s)], (0x40, 0x90, 0x40))
    elif kind == 4:
        p.rect((x - 22 * s, y - 6 * s, x + 22 * s, y), GOLD_DARK)
        p.rect((x - 21 * s, y - 5 * s, x + 21 * s, y - 2 * s), GOLD)
        for dx, h in ((-14, 10), (0, 18), (14, 10)):
            p.poly([(x + (dx - 7) * s, y - 5 * s), (x + dx * s, y - (5 + h) * s), (x + (dx + 7) * s, y - 5 * s)], GOLD)
            p.poly([(x + (dx - 4) * s, y - 6 * s), (x + dx * s, y - (5 + h) * s + 3), (x + (dx + 1) * s, y - 6 * s)], GOLD_LIGHT)
        p.circle(x, y - 13 * s, 3.5 * s, GEM_BLUE)
        p.circle(x - 14 * s, y - 9 * s, 2.5 * s, GEM_PINK)
        p.circle(x + 14 * s, y - 9 * s, 2.5 * s, GEM_PINK)
        p.circle(x - 1, y - 14 * s, 1.2, (0xFF, 0xFF, 0xFF))
    elif kind == 5:
        p.ellipse((x - 24 * s, y - 6 * s, x + 24 * s, y + 2 * s), HAT_BLACK)
        p.rect((x - 14 * s, y - 34 * s, x + 14 * s, y - 2 * s), HAT_BLACK)
        p.ellipse((x - 14 * s, y - 38 * s, x + 14 * s, y - 30 * s), HAT_GREY)
        p.rect((x - 14 * s, y - 12 * s, x + 14 * s, y - 6 * s), HAT_PINK)
        p.rect((x - 10 * s, y - 30 * s, x - 7 * s, y - 14 * s), HAT_GREY)


def necklace(p, kind, pts):
    """along pts: 1 pearls, 2 a ribbon collar with a bow"""
    if kind == 1:
        for (ax, ay), (bx, by) in zip(pts, pts[1:]):
            n = max(1, int(math.hypot(bx - ax, by - ay) / 6))
            for i in range(n):
                t = i / n
                x, y = ax + (bx - ax) * t, ay + (by - ay) * t
                p.circle(x, y, 3.4, PEARL_LO)
                p.circle(x - 0.8, y - 0.8, 2.4, PEARL)
        x, y = pts[-1]
        p.circle(x, y, 3.4, PEARL_LO)
        p.circle(x - 0.8, y - 0.8, 2.4, PEARL)
    elif kind == 2:
        p.line(pts, RIBBON_LO, 8)
        p.line([(x, y - 1) for x, y in pts], RIBBON, 5)
        mx, my = pts[len(pts) // 2]
        bow_at(p, mx, my + 2, 0.45, RIBBON, RIBBON_LO, (0xFF, 0xC0, 0xE0))


def horn_spiral(p, cx, cy, size, side):
    """a ram's horn curling out from the head (side: -1 to the left, 1 right)"""
    for i in range(14):
        t = i / 13
        a = math.radians(-100 + 320 * t) if side < 0 else math.radians(280 - 320 * t)
        rad = size * (1 - 0.55 * t)
        x, y = cx + side * size * 0.2 + rad * math.cos(a), cy + rad * math.sin(a) * 1.05
        w = size * (0.42 - 0.24 * t)
        p.circle(x, y, w, HORN_LO)
        p.circle(x - 1, y - 1, w * 0.75, HORN)


def lips(p, x, y, rx, ry):
    """lips in the lipstick's shades (dark at the edge, a shine)"""
    for i in range(N_LIPS):
        t = i / (N_LIPS - 1)
        p.ellipse((x - rx * (1 - 0.6 * t) - 1 * t, y - ry * (1 - 0.6 * t) - 1 * t,
                   x + rx * (1 - 0.6 * t) - 1 * t, y + ry * (1 - 0.6 * t) - 1 * t), (30 + 32 * i,) * 3, R_LIPS)


def hoof(p, hx, top, bottom):
    """a hoof in the polish's tone (shaded top to bottom)"""
    p.rrect((hx - 9, top, hx + 9, bottom + 1), 4, FACE_LO)
    for i in range(N_TONE):
        v = int(60 + 60 * i)
        p.rrect((hx - 8 + i, top + 1 + i * 0.5, hx + 8 - 2 * i, bottom - i * 0.5), 1, (v, v, v), R_HOOF)


def sheep(p, look):
    ram, lash, blink, kind, neck = look["ram"], look["lash"], look["blink"], look["hat"], look["neck"]
    # the front legs: from the chest, a bend at the knee, to the footrest
    for x0, dx in ((262, 0), (290, 6)):
        pts = [(x0, V(14)), (x0 + 50 + dx, V(15) + 8), (x0 + 52 + dx, V(19) - 2)]
        p.limb(pts, 18, FACE_LO)
        p.limb(pts, 14, FACE)
        p.line([(x0 + 2, V(14) - 2), (x0 + 48 + dx, V(15) + 2)], FACE_HI, 3)
        hoof(p, x0 + 52 + dx, V(19) - 7, V(19) + 2)
    # the body: a big fleece over the seat, the haunch, the tail, the chest
    body = in_ellipse(196, V(9) + 4, 126, 84)
    haunch = in_ellipse(150, V(13), 74, 44)
    chest = in_ellipse(300, V(7), 46, 64)
    p.ellipse((70, V(4) - 8, 322, V(15) + 4), WOOL_DARK, R_WOOL)
    fleece(p, (70, V(4) - 4, 330, V(15) + 4, lambda x, y: body(x, y) or haunch(x, y) or chest(x, y)), 9, 1)
    for x, y, r in ((66, V(7), 15), (56, V(8), 12)):                        # the tail
        curl(p, x, y, r)
    # the ear or the horn behind the head
    hx, hy = 356, V(5) + 6                  # the skull's centre
    if ram:
        horn_spiral(p, hx - 24, hy - 5, 20, -1)
    else:
        p.poly([(hx - 10, hy - 14), (hx - 52, hy - 30), (hx - 58, hy - 22), (hx - 22, hy + 2)], FACE_LO)
        p.poly([(hx - 14, hy - 12), (hx - 50, hy - 26), (hx - 52, hy - 22), (hx - 22, hy - 2)], EAR_IN)
    # the head: a long face toward the mirror
    p.ellipse((hx - 34, hy - 30, hx + 30, hy + 28), FACE_LO)
    p.poly([(hx - 10, hy - 20), (hx + 40, hy - 4), (hx + 58, hy + 34), (hx + 46, hy + 56), (hx + 4, hy + 40)], FACE_LO)
    p.ellipse((hx - 32, hy - 28, hx + 28, hy + 26), FACE)
    p.poly([(hx - 8, hy - 18), (hx + 38, hy - 2), (hx + 55, hy + 33), (hx + 44, hy + 53), (hx + 4, hy + 38)], FACE)
    p.ellipse((hx - 18, hy - 22, hx + 18, hy + 0), FACE_HI)
    p.ellipse((hx - 14, hy - 20, hx + 22, hy + 6), FACE)
    tone(p, (hx + 10, hy + 10, hx + 32, hy + 24), R_BLUSH)                 # the cheek
    # the necklace round the neck, over the chest's wool
    necklace(p, neck, [(hx - 26, hy + 30), (hx - 14, hy + 40), (hx, hy + 46), (hx + 12, hy + 48)])
    # the forelock, the eye, the headwear
    for x, y, r in ((hx - 26, hy - 26, 11), (hx - 10, hy - 34, 12), (hx + 6, hy - 32, 10), (hx - 32, hy - 10, 10)):
        curl(p, x, y, r)
    eye(p, hx + 14, hy - 2, 8, 2, lash, blink)
    hat(p, kind, hx - 6, hy - 40, 0.8)
    # the muzzle: a nostril, the lips
    mx, my = hx + 46, hy + 42
    p.ellipse((mx - 4, my - 18, mx + 2, my - 13), FACE_LO)
    lips(p, mx - 6, my + 2, 10, 5)
    p.line([(mx - 15, my + 2), (mx + 2, my + 1)], FACE_LO, 1.5)


# ---------------------------------------------------------------------------
# the sheep in the mirror: its face from the front

def reflection(p, cx, gy0, gy1, look):
    ram, lash, blink, kind, neck = look["ram"], look["lash"], look["blink"], look["hat"], look["neck"]
    # the shoulders: wool to the bottom of the glass
    sh = in_ellipse(cx, gy1 + 30, 110, 66)
    p.ellipse((cx - 110, gy1 - 34, cx + 110, gy1 + 96), WOOL_DARK, R_WOOL)
    fleece(p, (cx - 110, gy1 - 34, cx + 110, gy1 + 10, sh), 8, 7)
    fy = gy0 + 76                            # the face's middle
    # the ears (or the horns): out to the sides
    if ram:
        for sd in (-1, 1):
            p.limb([(cx + sd * 26, fy - 28), (cx + sd * 42, fy - 30), (cx + sd * 56, fy - 24)], 12, HORN_LO)
            p.limb([(cx + sd * 26, fy - 29), (cx + sd * 42, fy - 31), (cx + sd * 55, fy - 25)], 8, HORN)
            horn_spiral(p, cx + sd * 66, fy - 10, 17, sd)
    else:
        for sd in (-1, 1):
            p.ellipse((cx + sd * 34 - 24, fy - 22, cx + sd * 34 + 24, fy - 6), FACE_LO)
            p.ellipse((cx + sd * 36 - 18, fy - 19, cx + sd * 36 + 18, fy - 9), EAR_IN)
    # the face: wide at the eyes, narrow at the muzzle
    p.poly([(cx - 30, fy - 40), (cx + 30, fy - 40), (cx + 36, fy - 6), (cx + 26, fy + 40),
            (cx + 16, fy + 58), (cx - 16, fy + 58), (cx - 26, fy + 40), (cx - 36, fy - 6)], FACE_LO)
    p.poly([(cx - 28, fy - 38), (cx + 28, fy - 38), (cx + 34, fy - 6), (cx + 24, fy + 40),
            (cx + 14, fy + 56), (cx - 14, fy + 56), (cx - 24, fy + 40), (cx - 34, fy - 6)], FACE)
    p.ellipse((cx - 14, fy - 30, cx + 14, fy + 40), FACE_HI)
    p.ellipse((cx - 10, fy - 28, cx + 18, fy + 44), FACE)
    for sd in (-1, 1):                       # the cheeks
        tone(p, (cx + sd * 22 - 9, fy + 10, cx + sd * 22 + 9, fy + 22), R_BLUSH)
    # the forelock over the brow
    for x, y, r in ((cx - 26, fy - 40, 11), (cx - 9, fy - 46, 12), (cx + 9, fy - 46, 12), (cx + 26, fy - 40, 11),
                    (cx, fy - 34, 10)):
        curl(p, x, y, r)
    # the eyes, looking out of the mirror
    eye(p, cx - 14, fy - 8, 8, 0, lash, blink, -1)
    eye(p, cx + 14, fy - 8, 8, 0, lash, blink, 1)
    # the nose and the lips
    p.ellipse((cx - 9, fy + 34, cx - 3, fy + 39), FACE_LO)
    p.ellipse((cx + 3, fy + 34, cx + 9, fy + 39), FACE_LO)
    lips(p, cx, fy + 48, 12, 5)
    p.line([(cx - 11, fy + 48), (cx + 11, fy + 48)], FACE_LO, 1.5)
    necklace(p, neck, [(cx - 30, fy + 56), (cx - 16, fy + 64), (cx, fy + 67), (cx + 16, fy + 64), (cx + 30, fy + 56)])
    hat(p, kind, cx, fy - 54, 0.9)


LOOK0 = {"ram": 0, "lash": 0, "blink": 0, "hat": 0, "neck": 0}


def scene(look):
    p = Pic()
    room(p)
    vanity(p, look)
    chair(p)
    sheep(p, look)
    return p


# ---------------------------------------------------------------------------
# to pixels: halve the picture vertically, pick each pixel's role

def to_pixels(p):
    """(RGB image PW x PH, role per pixel): a role by the majority of its units"""
    rgb = p.rgb.resize((PW, PH), Image.BOX)
    big = p.role.load()
    bx, by = (UW * SS) // PW, (UH * SS) // PH
    role = []
    for y in range(PH):
        for x in range(PW):
            counts = {}
            for dy in range(by):
                for dx in range(bx):
                    v = big[x * bx + dx, y * by + dy]
                    counts[v] = counts.get(v, 0) + 1
            role.append(max(counts, key=counts.get))
    return rgb, role


def lum(c):
    return (c[0] * 299 + c[1] * 587 + c[2] * 114) / 1000.0


def index_pixels(rgb, role, art_palette, cache):
    """palette indices: the game's colours by their shade, the glass, the rest nearest"""
    px = rgb.load()
    out = []
    for y in range(PH):
        for x in range(PW):
            r = role[y * PW + x]
            c = px[x, y]
            if r == R_WOOL:
                out.append(P_WOOL + min(N_WOOL - 1, int(lum(c) * N_WOOL / 256)))
            elif r == R_LIPS:
                out.append(P_LIPS + min(N_LIPS - 1, int(lum(c) * N_LIPS / 256)))
            elif r in (R_BULBA, R_BULBB):
                out.append((P_BULBA if r == R_BULBA else P_BULBB) + min(N_BULB - 1, int(lum(c) * N_BULB / 256)))
            elif r in TONE_ROLES:
                out.append(TONE_ROLES[r] + min(N_TONE - 1, int(lum(c) * N_TONE / 256)))
            elif r == R_GLASS:
                out.append(P_GLASS)
            else:
                if c not in cache:
                    best, bd = 0, 1 << 30
                    for i, q in enumerate(art_palette):
                        d = (c[0] - q[0]) ** 2 * 3 + (c[1] - q[1]) ** 2 * 4 + (c[2] - q[2]) ** 2 * 2
                        if d < bd:
                            best, bd = i, d
                    cache[c] = P_FIRST + best
                out.append(cache[c])
    return out


def art_palette_of(pics):
    """the picture's own colours: median cut over the plain pixels of all the pictures"""
    n = P_LAST - P_FIRST + 1
    sample = Image.new("RGB", (PW, PH * len(pics)))
    for k, (rgb, role) in enumerate(pics):
        img = rgb.copy()
        px = img.load()
        for y in range(PH):
            for x in range(PW):
                if role[y * PW + x] != R_ART:
                    px[x, y] = WALL             # (counted as wall)
        sample.paste(img, (0, PH * k))
    q = sample.quantize(colors=n, method=Image.Quantize.MEDIANCUT)
    pal = q.getpalette()[:3 * n]
    pal += [0] * (3 * n - len(pal))
    return [tuple(pal[i:i + 3]) for i in range(0, 3 * n, 3)]


def cells_of(idx):
    """per cell (column, picture row): its 64 bytes"""
    out = {}
    for r in range(ROWS):
        for c in range(COLS):
            out[(c, r)] = bytes(idx[(r * 8 + y) * PW + c * 8 + x] for y in range(8) for x in range(8))
    return out


SPARKLE = [
    # a large and a small sparkle: 'W' white, 'Y' light yellow, '.' the picture
    ["...W....", "...W....", ".W.W.W..", "..WYW...", "WWYYYWW.", "..WYW...", ".W.W.W..", "...W...."],
    ["........", "........", "...W....", "..WYW...", "...W....", "........", "........", "........"],
]


def sparkled(cell, k, pal_index):
    out = bytearray(cell)
    for y, row in enumerate(SPARKLE[k]):
        for x, ch in enumerate(row):
            if ch != ".":
                out[y * 8 + x] = pal_index[ch]
    return bytes(out)


def rects_of(cells):
    """cover a set of cells with runs along the rows (rectangles one row high)"""
    runs = []
    for r in sorted({r for c, r in cells}):
        cs = sorted(c for c, rr in cells if rr == r)
        start = prev = cs[0]
        for c in cs[1:] + [None]:
            if c is not None and c == prev + 1:
                prev = c
                continue
            runs.append([start, r, prev, r])
            if c is not None:
                start = prev = c
    return runs


def tones():
    """the tones' palettes: per choice, four shades dark to light"""
    def shades(c, face=False):
        if c is None:                    # the sheep's own: the face's greys
            return [mix(FACE_LO, FACE_HI, i / (N_TONE - 1)) for i in range(N_TONE)]
        return [mix((0, 0, 0), c, 0.45 + 0.55 * i / (N_TONE - 1)) for i in range(N_TONE)]
    return ([shades(c) for c in SHADOWS], [shades(c) for c in BLUSHES], [shades(c) for c in HOOVES])


# ---------------------------------------------------------------------------

def main():
    args = sys.argv[1:]
    preview, look = None, None
    verify = "--fast" not in args
    args = [a for a in args if a != "--fast"]
    if "--preview" in args:
        i = args.index("--preview")
        preview = args[i + 1]
        look = args[i + 2] if i + 2 < len(args) and "=" in args[i + 2] else ""
        del args[i:i + (3 if look else 2)]
    out = args[0] if args else "."

    # every group's looks, the others plain
    looks = []
    for name, n, params in GROUPS:
        for k in range(n):
            lk = dict(LOOK0)
            lk.update(params(k))
            looks.append((name, k, lk))
    pics = {}
    for name, k, lk in looks:
        key = tuple(sorted(lk.items()))
        if key not in pics:
            pics[key] = to_pixels(scene(lk))
    art = art_palette_of(list(pics.values()))
    cache = {}
    cells = {key: cells_of(index_pixels(rgb, role, art, cache)) for key, (rgb, role) in pics.items()}
    base = cells[tuple(sorted(LOOK0.items()))]
    group_cells, group_looks = {}, {}
    for name, n, params in GROUPS:
        gl = []
        for k in range(n):
            lk = dict(LOOK0)
            lk.update(params(k))
            gl.append(cells[tuple(sorted(lk.items()))])
        group_looks[name] = gl
        group_cells[name] = {pos for cm in gl for pos in base if cm[pos] != base[pos]}
    names = [g[0] for g in GROUPS]
    for i, a in enumerate(names):
        for b in names[i + 1:]:
            both = group_cells[a] & group_cells[b]
            if both:
                sys.exit("the %s and the %s change the same cells: %s" % (a, b, sorted(both)))
    # every combination is the base with each group's cells
    if verify:
        import itertools
        for ks in itertools.product(*[range(g[1]) for g in GROUPS]):
            if all(k == 0 for k in ks) or sum(1 for k in ks if k) < 2:
                continue
            lk = dict(LOOK0)
            for (name, n, params), k in zip(GROUPS, ks):
                lk.update(params(k))
            want = cells_of(index_pixels(*to_pixels(scene(lk)), art, cache))
            for pos in base:
                got = base[pos]
                for (name, n, params), k in zip(GROUPS, ks):
                    if pos in group_cells[name]:
                        got = group_looks[name][k][pos]
                if want[pos] != got:
                    sys.exit("look %s: cell %s is not its groups' (they touch)" % (lk, pos))
    runs = {name: rects_of(group_cells[name]) for name in names}

    # the characters: the room's, the looks', the sparkles'
    chars, num = [], {}

    def char(b):
        if b not in num:
            num[b] = len(chars)
            chars.append(b)
        return num[b]
    for r in range(ROWS):
        for c in range(COLS):
            char(base[(c, r)])
    nroom = len(chars)
    for name in names:
        for cm in group_looks[name]:
            for x0, y0, x1, y1 in runs[name]:
                for c in range(x0, x1 + 1):
                    char(cm[(c, y0)])
    nlooks = len(chars) - nroom
    pal_index = {"W": P_FIRST + nearest(art, (0xFF, 0xFF, 0xFF)), "Y": P_FIRST + nearest(art, (0xFF, 0xF4, 0xB0))}

    def covers(rs):
        return {(c, r) for x0, y0, x1, y1 in rs for c in range(x0, x1 + 1) for r in range(y0, y1 + 1)}
    busy = set()
    for name in names:
        busy |= covers(runs[name])
    busy |= {(c, r) for c in range(SIGN[0] + 1, SIGN[1]) for r in range(SIGN[2] + 1, SIGN[3])}
    busy |= {(c, r) for c in range(CABINET[0], CABINET[1] + 1) for r in range(CABINET[2], CABINET[3] + 1)}
    spark_pos = []
    for sname, pts in SPARKS.items():
        for c, r in pts:
            pos = min(((cc, rr) for cc in range(c - 3, c + 4) for rr in range(r - 3, r + 2)
                       if 0 <= cc < COLS and 0 <= rr < ROWS and (cc, rr) not in busy),
                      key=lambda q: (q[0] - c) ** 2 + 4 * (q[1] - (r - 1)) ** 2)
            busy.add(pos)
            spark_pos.append((sname, pos[0], pos[1] + 1,
                              char(sparkled(base[pos], 0, pal_index)), char(sparkled(base[pos], 1, pal_index))))

    # the characters: as many as fit in bank 4, the rest in bank 5
    n1 = min(len(chars), (LIMIT - LOAD_LIN) // 64)

    def number(i):
        return (LOAD_LIN // 64 + i) if i < n1 else (LOAD2_LIN // 64 + i - n1)
    num = {b: number(i) for b, i in num.items()}
    data1 = bytearray(b"".join(chars[:n1]))
    # the second file: the other characters, the screen (rows 1-23:
    # numbers, low byte first), the looks' runs (per group, per look: its
    # cells' numbers), the palettes
    data = bytearray(b"".join(chars[n1:]))
    off = {"SCREEN": len(data)}
    for r in range(ROWS):
        for c in range(COLS):
            n = num[base[(c, r)]]
            data += bytes((n & 0xFF, n >> 8))
    sizes = {}
    for name in names:
        off[name] = len(data)
        sizes[name] = sum((x1 - x0 + 1) * 2 for x0, y0, x1, y1 in runs[name])
        for cm in group_looks[name]:
            for x0, y0, x1, y1 in runs[name]:
                for c in range(x0, x1 + 1):
                    n = num[cm[(c, y0)]]
                    data += bytes((n & 0xFF, n >> 8))

    # palettes (as palette_restore takes them: the reds, the greens, the
    # blues, nibbles swapped as the VIC-IV has them): the picture's colours,
    # each dye's wool shades, each lipstick's, the bulbs' two phases, the
    # eyeshadows', the blushes', the hooves'
    def regs(colours):
        return bytes((c[ch] << 4 | c[ch] >> 4) & 0xFF for ch in range(3) for c in colours)
    off["PALETTE"] = len(data)
    data += regs(art)
    off["DYES"] = len(data)
    for wool in range(4):
        data += regs(wool_shades(wool))
    off["LIPSTICKS"] = len(data)
    for lip in range(4):
        data += regs(lip_shades(lip))
    off["BULBS"] = len(data)
    lit = [mix(BULB_OFF, BULB_ON, 0.4 + 0.6 * i / (N_BULB - 1)) for i in range(N_BULB)]
    dim = [mix((0x50, 0x38, 0x18), BULB_OFF, 0.3 + 0.7 * i / (N_BULB - 1)) for i in range(N_BULB)]
    data += regs(lit + dim) + regs(dim + lit)
    sh, bl, ho = tones()
    for key, tab in (("SHADOWS", sh), ("BLUSHES", bl), ("HOOVES", ho)):
        off[key] = len(data)
        for t in tab:
            data += regs(t)
    data += b"GLAM"                     # (the end: the game checks it)
    if len(data1) > LIMIT - LOAD_LIN or LOAD2_LIN + len(data) > LIMIT2:
        sys.exit("the salon is %d + %d bytes: more than %d + %d" % (len(data1), len(data), LIMIT - LOAD_LIN, LIMIT2 - LOAD2_LIN))
    for fname, d in (("salon1.dat", data1), ("salon2.dat", data)):
        with open(out + "/" + fname, "wb") as f:
            f.write(b"\x00\x00" + d)    # (the load header: the game gives the address)

    with open(out + "/salon.h", "w") as f:
        w = f.write
        w("/* salon.h -- generated by gen_salon.py with salon1.dat and salon2.dat: do not edit */\n\n")
        w("#define SAL_LIN 0x%XUL             /* where the game loads salon1.dat (GLAMSALON) */\n" % LOAD_LIN)
        w("#define SAL_SIZE %du\n" % len(data1))
        w("#define SAL2_LIN 0x%XUL            /* salon2.dat (GLAMSALON2): the SAL_* offsets are in it */\n" % LOAD2_LIN)
        w("#define SAL2_SIZE %du\n#define SAL_CHARS %d\n" % (len(data), len(chars)))
        for k, v in off.items():
            w("#define SAL_%s %du\n" % (k, v))
        w("#define SAL_PAL_FIRST %d\n#define SAL_PAL_COUNT %d\n" % (P_FIRST, len(art)))
        w("#define SAL_WOOL %d\n#define SAL_LIPS %d\n#define SAL_BULBA %d\n#define SAL_BULBB %d\n#define SAL_GLASS %d\n"
          % (P_WOOL, P_LIPS, P_BULBA, P_BULBB, P_GLASS))
        w("#define SAL_SHADOW %d\n#define SAL_BLUSH %d\n#define SAL_HOOF %d\n#define SAL_TONE %d\n"
          % (P_SHADOW, P_BLUSH, P_HOOF, N_TONE))
        w("#define SAL_SIGN_X %d\n#define SAL_SIGN_Y %d\n#define SAL_SIGN_W %d\n"
          % (SIGN[0] + 1, SIGN[2] + 2, SIGN[1] - SIGN[0] - 1))
        w("#define SAL_DOOR_X %d\n#define SAL_DOOR_Y %d\n#define SAL_DOOR_W %d\n#define SAL_DOOR_H %d\n"
          % (CABINET[0], CABINET[2] + 1, CABINET[1] - CABINET[0] + 1, CABINET[3] - CABINET[2] + 1))
        for name, n, params in GROUPS:
            w("/* the %s's looks (%d): runs of cells (column, screen row, width), a look's numbers %d bytes */\n"
              % (name.lower(), n, sizes[name]))
            w("#define SAL_%s_N %d\n#define SAL_%s_LOOK %d\n" % (name, len(runs[name]), name, sizes[name]))
            w("#define SAL_%s_RUNS %s\n" % (name, ", ".join("%d, %d, %d" % (x0, y0 + 1, x1 - x0 + 1)
                                                          for x0, y0, x1, y1 in runs[name])))
        w("/* sparkles: column, screen row, the large one's number, the small one's */\n")
        for sname in SPARKS:
            pts = [sp for sp in spark_pos if sp[0] == sname]
            w("#define SAL_SPARK_%s %s\n" % (sname, ", ".join("%d, %d, 0x%X, 0x%X" % (sp[1], sp[2], number(sp[3]), number(sp[4]))
                                                           for sp in pts)))
    print("salon: %d characters (room %d, looks %d, sparkles %d), %d + %d bytes; runs: %s"
          % (len(chars), nroom, nlooks, len(chars) - nroom - nlooks, len(data1), len(data),
             ", ".join("%s %d" % (n, len(runs[n])) for n in names)))
    if preview:
        render(preview, base, group_looks, runs, art, look)


def wool_shades(wool):
    return [mix((0, 0, 0), WOOL_RGB[wool], 0.30 + 0.70 * i / (N_WOOL - 1)) for i in range(N_WOOL)]


def lip_shades(lip):
    if lip == 0:
        return [mix((0x22, 0x1E, 0x22), (0x6A, 0x50, 0x58), i / (N_LIPS - 1)) for i in range(N_LIPS)]
    return [mix((0, 0, 0), LIPS_RGB[lip], 0.45 + 0.55 * i / (N_LIPS - 1)) for i in range(N_LIPS)]


def nearest(art, c):
    return min(range(len(art)), key=lambda i: sum((art[i][j] - c[j]) ** 2 for j in range(3)))


def render(path, base, group_looks, runs, art, spec):
    """the picture as shown (each pixel 2 x 4 in the PNG): spec 'wool=1,lips=2,lash=1,blink=0,ram=1,hat=3,neck=1,
    shadow=2,blush=1,hoof=3'"""
    lk = {"wool": 1, "lips": 1, "lash": 1, "blink": 0, "ram": 0, "hat": 1, "neck": 1, "shadow": 1, "blush": 1, "hoof": 3}
    for kv in spec.split(","):
        if "=" in kv:
            k, v = kv.split("=")
            lk[k] = int(v)
    ks = {"EYES": lk["lash"] << 1 | lk["blink"], "HORNS": lk["ram"], "HAT": lk["hat"], "NECK": lk["neck"]}
    cm = dict(base)
    for name, rs in runs.items():
        for x0, y0, x1, y1 in rs:
            for c in range(x0, x1 + 1):
                cm[(c, y0)] = group_looks[name][ks[name]][(c, y0)]
    pal = [(0, 0, 0)] * 256
    pal[P_GLASS] = GLASS_RGB
    pal[P_WOOL:P_WOOL + N_WOOL] = wool_shades(lk["wool"])
    pal[P_LIPS:P_LIPS + N_LIPS] = lip_shades(lk["lips"])
    for i in range(N_BULB):
        pal[P_BULBA + i] = mix(BULB_OFF, BULB_ON, 0.4 + 0.6 * i / (N_BULB - 1))
        pal[P_BULBB + i] = mix((0x50, 0x38, 0x18), BULB_OFF, 0.3 + 0.7 * i / (N_BULB - 1))
    sh, bl, ho = tones()
    pal[P_SHADOW:P_SHADOW + N_TONE] = sh[lk["shadow"]]
    pal[P_BLUSH:P_BLUSH + N_TONE] = bl[lk["blush"]]
    pal[P_HOOF:P_HOOF + N_TONE] = ho[lk["hoof"]]
    for i, c in enumerate(art):
        pal[P_FIRST + i] = c
    img = Image.new("RGB", (PW, PH))
    px = img.load()
    for (c, r), b in cm.items():
        for y in range(8):
            for x in range(8):
                px[c * 8 + x, r * 8 + y] = pal[b[y * 8 + x]]
    img.resize((PW * 2, PH * 4), Image.NEAREST).save(path)


if __name__ == "__main__":
    main()
