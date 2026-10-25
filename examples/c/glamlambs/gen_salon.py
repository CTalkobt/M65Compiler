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
LIMIT2 = 0x60000

P_WOOL, N_WOOL = 16, 16
P_LIPS, N_LIPS = 32, 8
P_BULBA, P_BULBB, N_BULB = 40, 44, 4
P_GLASS = 14                            # (the text cells' background: $D021)
P_FIRST = 48                            # the picture's own colours: 48-254
P_LAST = 254

GLASS_RGB = (0x2A, 0x16, 0x34)          # the sign's and the doors' background (palette 14)
MIRROR_RGB = (0x30, 0x24, 0x44)         # the mirror's glass
WALL = (0x5A, 0x26, 0x4E)

# the preview's dye and lipstick (the game's colours)
WOOL_RGB = [(0xF4, 0xF0, 0xE8), (0xFF, 0x88, 0xC8), (0x70, 0xA8, 0xFF), (0xF8, 0xD8, 0x30)]
LIPS_RGB = [(0x30, 0x18, 0x20), (0xE0, 0x20, 0x30), (0xA8, 0x48, 0xD0), (0xFF, 0x88, 0xC8)]
BULB_ON, BULB_OFF = (0xFF, 0xF0, 0xA0), (0xC0, 0x90, 0x40)

# roles of a pixel (the role image): what the game recolours
R_ART, R_WOOL, R_LIPS, R_BULBA, R_BULBB, R_GLASS = 0, 1, 2, 3, 4, 5

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


WOOL_DARK, WOOL_LIGHT = (70, 70, 70), (255, 255, 255)


def curl(p, x, y, rad):
    """a curl of wool: a dark ring, a lit ball inside, a shine"""
    p.circle(x, y, rad, WOOL_DARK, R_WOOL)
    shaded_ball(p, x - 0.5, y - 0.5, rad * 0.9, (120, 120, 120), (250, 250, 250), R_WOOL, 4)


def fleece(p, region, rad, seed):
    """wool over a region (a test of a point): rows of curls, in an order
    that leaves the lower ones in front"""
    rnd = random.Random(seed)
    x0, y0, x1, y1, inside = region
    pts = []
    y, row = y0, 0
    while y <= y1:
        x = x0 + (rad * 0.8 if row % 2 else 0)
        while x <= x1:
            cx, cy = x + rnd.uniform(-2.5, 2.5), y + rnd.uniform(-2, 2)
            if inside(cx, cy):
                pts.append((cx, cy, rad * rnd.uniform(0.85, 1.15)))
            x += rad * 1.5
        y += rad * 1.25
        row += 1
    for cx, cy, r in pts:
        curl(p, cx, cy, r)


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


def vanity(p, ram, lash, bow, blink):
    c0, c1, r0, r1 = MIRROR
    x0, x1, y0, y1 = U(c0), U(c1 + 1) - 1, V(r0), V(r1 + 1) - 1
    p.rrect((x0, y0 + 2, x1, y1 - 2), 6, GOLD_DARK)
    p.rrect((x0 + 2, y0 + 4, x1 - 2, y1 - 4), 5, GOLD)
    p.rect((x0 + 6, y0 + 12, x1 - 6, y1 - 12), GOLD_LIGHT)
    # the glass, the sheep's face in it, a shine in two corners
    gx0, gx1, gy0, gy1 = U(GLASS[0]), U(GLASS[1] + 1) - 1, V(GLASS[2]), V(GLASS[3] + 1) - 1
    p.rect((gx0, gy0, gx1, gy1), MIRROR_RGB)
    reflection(p, (gx0 + gx1) / 2.0, gy0, gy1, ram, lash, bow, blink)
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


def eye(p, ex, ey, rad, look_dx, lash, blink, side=1):
    """a big cartoon eye; lashes curl out to one side (side: 1 right, -1 left)"""
    if blink:
        p.arc((ex - rad, ey - rad - 3, ex + rad, ey + rad - 3), 20, 160, INK, 2.5)
    else:
        p.ellipse((ex - rad - 1, ey - rad, ex + rad + 1, ey + rad), INK)
        p.ellipse((ex - rad, ey - rad + 1, ex + rad, ey + rad - 1), EYE_WHITE)
        ix = ex + look_dx
        p.ellipse((ix - rad * 0.5, ey - rad * 0.6, ix + rad * 0.5, ey + rad * 0.6), IRIS)
        p.ellipse((ix - rad * 0.3, ey - rad * 0.4, ix + rad * 0.3, ey + rad * 0.4), INK)
        p.circle(ix - rad * 0.15, ey - rad * 0.3, max(1.2, rad * 0.15), (0xFF, 0xFF, 0xFF))
    if lash:
        p.arc((ex - rad - 1, ey - rad - 1, ex + rad + 1, ey + rad + 1), 190, 350, INK, 3)
        angles = (236, 253, 270, 287, 304)
        for a in angles:
            ra = math.radians(a)
            l = rad * (0.8 + 0.5 * math.sin(math.radians(a - 180)))
            sx, sy = ex + (rad + 1) * math.cos(ra), ey + (rad + 1) * math.sin(ra)
            tx, ty = ex + (rad + 1 + l) * math.cos(ra), ey + (rad + 1 + l) * math.sin(ra)
            p.line([(sx, sy), ((sx + tx) / 2 + 1 * side, (sy + ty) / 2 - 2), (tx + 2 * side, ty)], INK, 2)


def bow_at(p, bx, by, size=1.0):
    s = size
    p.poly([(bx, by), (bx - 26 * s, by - 14 * s), (bx - 22 * s, by + 16 * s)], BOW_LO)
    p.poly([(bx, by), (bx + 26 * s, by - 14 * s), (bx + 22 * s, by + 16 * s)], BOW_LO)
    p.poly([(bx, by), (bx - 23 * s, by - 11 * s), (bx - 20 * s, by + 12 * s)], BOW)
    p.poly([(bx, by), (bx + 23 * s, by - 11 * s), (bx + 20 * s, by + 12 * s)], BOW)
    p.poly([(bx - 3 * s, by + 2 * s), (bx - 12 * s, by + 22 * s), (bx - 6 * s, by + 24 * s)], BOW_LO)
    p.poly([(bx + 3 * s, by + 2 * s), (bx + 12 * s, by + 22 * s), (bx + 6 * s, by + 24 * s)], BOW_LO)
    p.circle(bx, by, 6 * s, BOW_LO)
    p.circle(bx - 1, by - 1, 4 * s, BOW_HI)


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


def sheep(p, ram=False, lash=False, bow=False, blink=False):
    # the front legs: from the chest, a bend at the knee, to the footrest
    for x0, dx in ((262, 0), (290, 6)):
        pts = [(x0, V(14)), (x0 + 50 + dx, V(15) + 8), (x0 + 52 + dx, V(19) - 2)]
        p.limb(pts, 18, FACE_LO)
        p.limb(pts, 14, FACE)
        p.line([(x0 + 2, V(14) - 2), (x0 + 48 + dx, V(15) + 2)], FACE_HI, 3)
        hx = x0 + 52 + dx
        p.rrect((hx - 9, V(19) - 6, hx + 9, V(19) + 3), 4, GOLD_DARK)
        p.rrect((hx - 8, V(19) - 6, hx + 8, V(19) + 1), 4, GOLD)
        p.rect((hx - 6, V(19) - 5, hx - 1, V(19) - 3), GOLD_LIGHT)
    # the body: a big fleece over the seat, the haunch, the tail, the chest
    body = in_ellipse(196, V(9) + 4, 126, 84)
    haunch = in_ellipse(150, V(13), 74, 44)
    chest = in_ellipse(300, V(7), 46, 64)
    p.ellipse((70, V(4) - 8, 322, V(15) + 4), WOOL_DARK, R_WOOL)
    fleece(p, (70, V(4) - 4, 330, V(15) + 4, lambda x, y: body(x, y) or haunch(x, y) or chest(x, y)), 13, 1)
    for x, y, r in ((66, V(7), 15), (56, V(8), 12)):                        # the tail
        curl(p, x, y, r)
    # the ear or the horn behind the head
    hx, hy = 356, V(5) + 6                  # the skull's centre
    if ram:
        horn_spiral(p, hx - 24, hy - 2, 24, -1)
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
    # the forelock
    for x, y, r in ((hx - 26, hy - 26, 11), (hx - 10, hy - 34, 12), (hx + 6, hy - 32, 10), (hx - 32, hy - 10, 10)):
        curl(p, x, y, r)
    eye(p, hx + 14, hy - 2, 8, 2, lash, blink)
    # the muzzle: a nostril, the lips
    mx, my = hx + 46, hy + 42
    p.ellipse((mx - 4, my - 18, mx + 2, my - 13), FACE_LO)
    lips(p, mx - 6, my + 2, 10, 5)
    p.line([(mx - 15, my + 2), (mx + 2, my + 1)], FACE_LO, 1.5)
    if bow:
        bow_at(p, hx - 30, hy - 42)


def lips(p, x, y, rx, ry):
    """lips in the lipstick's shades (dark at the edge, a shine)"""
    for i in range(N_LIPS):
        t = i / (N_LIPS - 1)
        p.ellipse((x - rx * (1 - 0.6 * t) - 1 * t, y - ry * (1 - 0.6 * t) - 1 * t,
                   x + rx * (1 - 0.6 * t) - 1 * t, y + ry * (1 - 0.6 * t) - 1 * t), (30 + 32 * i,) * 3, R_LIPS)


# ---------------------------------------------------------------------------
# the sheep in the mirror: its face from the front

def reflection(p, cx, gy0, gy1, ram, lash, bow, blink):
    # the shoulders: wool to the bottom of the glass
    sh = in_ellipse(cx, gy1 + 30, 110, 66)
    p.ellipse((cx - 110, gy1 - 34, cx + 110, gy1 + 96), WOOL_DARK, R_WOOL)
    fleece(p, (cx - 110, gy1 - 34, cx + 110, gy1 + 10, sh), 11, 7)
    fy = gy0 + 76                            # the face's middle
    # the ears (or the horns): out to the sides
    if ram:
        for sd in (-1, 1):
            # from the top of the head out to the curl
            p.limb([(cx + sd * 22, fy - 38), (cx + sd * 40, fy - 40), (cx + sd * 56, fy - 30)], 12, HORN_LO)
            p.limb([(cx + sd * 22, fy - 39), (cx + sd * 40, fy - 41), (cx + sd * 55, fy - 31)], 8, HORN)
            horn_spiral(p, cx + sd * 66, fy - 14, 17, sd)
    else:
        for s in (-1, 1):
            p.ellipse((cx + s * 34 - 24, fy - 22, cx + s * 34 + 24, fy - 6), FACE_LO)
            p.ellipse((cx + s * 36 - 18, fy - 19, cx + s * 36 + 18, fy - 9), EAR_IN)
    # the face: wide at the eyes, narrow at the muzzle
    p.poly([(cx - 30, fy - 40), (cx + 30, fy - 40), (cx + 36, fy - 6), (cx + 26, fy + 40),
            (cx + 16, fy + 58), (cx - 16, fy + 58), (cx - 26, fy + 40), (cx - 36, fy - 6)], FACE_LO)
    p.poly([(cx - 28, fy - 38), (cx + 28, fy - 38), (cx + 34, fy - 6), (cx + 24, fy + 40),
            (cx + 14, fy + 56), (cx - 14, fy + 56), (cx - 24, fy + 40), (cx - 34, fy - 6)], FACE)
    p.ellipse((cx - 14, fy - 30, cx + 14, fy + 40), FACE_HI)
    p.ellipse((cx - 10, fy - 28, cx + 18, fy + 44), FACE)
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
    if bow:
        bow_at(p, cx + 2, fy - 64, 0.7)


def scene(ram=False, lash=False, bow=False, blink=False):
    p = Pic()
    room(p)
    vanity(p, ram, lash, bow, blink)
    chair(p)
    sheep(p, ram, lash, bow, blink)
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
    """palette indices: wool, lips and bulbs by their shade, the glass, the rest nearest"""
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


# ---------------------------------------------------------------------------

def main():
    args = sys.argv[1:]
    preview, look = None, (1, 1, 1, 1, 0, 0)
    if "--preview" in args:
        i = args.index("--preview")
        preview = args[i + 1]
        if i + 2 < len(args) and "," in args[i + 2]:
            look = tuple(int(v) for v in args[i + 2].split(","))
            del args[i + 2]
        del args[i:i + 2]
    out = args[0] if args else "."

    pics = []
    for k in range(16):                 # ram << 3 | lash << 2 | bow << 1 | blink
        pics.append(to_pixels(scene(ram=bool(k & 8), lash=bool(k & 4), bow=bool(k & 2), blink=bool(k & 1))))
    art = art_palette_of(pics)
    cache = {}
    combos = [cells_of(index_pixels(rgb, role, art, cache)) for rgb, role in pics]
    base = combos[0]

    # the two groups of looks: the eyes (lash, blink), the head's top (ram, bow)
    def k_of(ram, lash, bow, blink):
        return ram << 3 | lash << 2 | bow << 1 | blink
    eye_ks = [k_of(0, lash, 0, blink) for lash in (0, 1) for blink in (0, 1)]     # (index lash << 1 | blink)
    top_ks = [k_of(ram, 0, bow, 0) for ram in (0, 1) for bow in (0, 1)]           # (index ram << 1 | bow)
    eye_cells = {pos for k in eye_ks for pos in base if combos[k][pos] != base[pos]}
    top_cells = {pos for k in top_ks for pos in base if combos[k][pos] != base[pos]}
    eye_rects, top_rects = rects_of(eye_cells), rects_of(top_cells)

    def covers(rects):
        return {(c, r) for x0, y0, x1, y1 in rects for c in range(x0, x1 + 1) for r in range(y0, y1 + 1)}
    if eye_cells & top_cells:
        sys.exit("the eyes' and the head top's looks change the same cells: %s" % sorted(eye_cells & top_cells))
    # every look is the base with its two groups' cells
    for k in range(16):
        ram, lash, bow, blink = k >> 3 & 1, k >> 2 & 1, k >> 1 & 1, k & 1
        e, t = combos[k_of(0, lash, 0, blink)], combos[k_of(ram, 0, bow, 0)]
        for pos in base:
            want = combos[k][pos]
            got = e[pos] if pos in covers(eye_rects) else t[pos] if pos in covers(top_rects) else base[pos]
            if want != got:
                sys.exit("look %d: cell %s is not the groups' (the eyes and the head's top touch)" % (k, pos))

    # the characters: the room's, the looks', the sparkles'
    chars = []
    num = {}

    def char(b):
        if b not in num:
            num[b] = len(chars)
            chars.append(b)
        return num[b]
    for r in range(ROWS):
        for c in range(COLS):
            char(base[(c, r)])
    nroom = len(chars)
    for ks, rects in ((eye_ks, eye_rects), (top_ks, top_rects)):
        for k in ks:
            for x0, y0, x1, y1 in rects:
                for r in range(y0, y1 + 1):
                    for c in range(x0, x1 + 1):
                        char(combos[k][(c, r)])
    nlooks = len(chars) - nroom
    pal_index = {"W": P_FIRST + nearest(art, (0xFF, 0xFF, 0xFF)), "Y": P_FIRST + nearest(art, (0xFF, 0xF4, 0xB0))}
    # a sparkle goes to the cell nearest its place that no look and no text uses
    busy = covers(eye_rects) | covers(top_rects)
    busy |= {(c, r) for c in range(SIGN[0] + 1, SIGN[1]) for r in range(SIGN[2] + 1, SIGN[3])}
    busy |= {(c, r) for c in range(CABINET[0], CABINET[1] + 1) for r in range(CABINET[2], CABINET[3] + 1)}
    spark_pos = []
    for name, pts in SPARKS.items():
        for c, r in pts:
            pos = min(((cc, rr) for cc in range(c - 3, c + 4) for rr in range(r - 3, r + 2)
                       if 0 <= cc < COLS and 0 <= rr < ROWS and (cc, rr) not in busy),
                      key=lambda q: (q[0] - c) ** 2 + 4 * (q[1] - (r - 1)) ** 2)
            busy.add(pos)
            spark_pos.append((name, pos[0], pos[1] + 1,
                              char(sparkled(base[pos], 0, pal_index)), char(sparkled(base[pos], 1, pal_index))))

    # the characters: as many as fit in bank 4, the rest in bank 5
    n1 = min(len(chars), (LIMIT - LOAD_LIN) // 64)

    def number(i):
        return (LOAD_LIN // 64 + i) if i < n1 else (LOAD2_LIN // 64 + i - n1)
    num = {b: number(i) for b, i in num.items()}
    data1 = bytearray(b"".join(chars[:n1]))
    # the second file: the other characters, the screen (rows 1-23:
    # numbers, low byte first), the looks' runs (per group, per look: its
    # cells' numbers), the palette
    data = bytearray(b"".join(chars[n1:]))
    off = {"SCREEN": len(data)}
    for r in range(ROWS):
        for c in range(COLS):
            n = num[base[(c, r)]]
            data += bytes((n & 0xFF, n >> 8))
    rect_defs = []
    for gname, ks, rects in (("EYES", eye_ks, eye_rects), ("TOP", top_ks, top_rects)):
        off[gname] = len(data)
        size = sum((x1 - x0 + 1) * (y1 - y0 + 1) * 2 for x0, y0, x1, y1 in rects)
        for k in ks:
            for x0, y0, x1, y1 in rects:
                for r in range(y0, y1 + 1):
                    for c in range(x0, x1 + 1):
                        n = num[combos[k][(c, r)]]
                        data += bytes((n & 0xFF, n >> 8))
        rect_defs.append((gname, rects, size))
    # palettes (as palette_restore takes them: the reds, the greens, the
    # blues, nibbles swapped as the VIC-IV has them): the picture's colours
    # (entries P_FIRST..), each dye's wool shades, each lipstick's, the two
    # phases of the bulbs (A lit and B dimmed, then the other way)
    def regs(colours):
        return bytes((c[ch] << 4 | c[ch] >> 4) & 0xFF for ch in range(3) for c in colours)
    off["PALETTE"] = len(data)
    data += regs(art)
    off["DYES"] = len(data)
    for wool in range(4):
        data += regs(palette_for(art, (wool, 0, 0, 0, 0, 0))[P_WOOL:P_WOOL + N_WOOL])
    off["LIPSTICKS"] = len(data)
    for lip in range(4):
        data += regs(palette_for(art, (0, lip, 0, 0, 0, 0))[P_LIPS:P_LIPS + N_LIPS])
    off["BULBS"] = len(data)
    lit = [mix(BULB_OFF, BULB_ON, 0.4 + 0.6 * i / (N_BULB - 1)) for i in range(N_BULB)]
    dim = [mix((0x50, 0x38, 0x18), BULB_OFF, 0.3 + 0.7 * i / (N_BULB - 1)) for i in range(N_BULB)]
    data += regs(lit + dim) + regs(dim + lit)
    data += b"GLAM"                     # (the end: the game checks it)
    if LOAD2_LIN + len(data) > LIMIT2:
        sys.exit("the salon is %d bytes in bank 5: more than the %d from $%X" % (len(data), LIMIT2 - LOAD2_LIN, LOAD2_LIN))
    for name, d in (("salon1.dat", data1), ("salon2.dat", data)):
        with open(out + "/" + name, "wb") as f:
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
        w("#define SAL_SIGN_X %d\n#define SAL_SIGN_Y %d\n#define SAL_SIGN_W %d\n"
          % (SIGN[0] + 1, SIGN[2] + 2, SIGN[1] - SIGN[0] - 1))
        w("#define SAL_DOOR_X %d\n#define SAL_DOOR_Y %d\n#define SAL_DOOR_W %d\n#define SAL_DOOR_H %d\n"
          % (CABINET[0], CABINET[2] + 1, CABINET[1] - CABINET[0] + 1, CABINET[3] - CABINET[2] + 1))
        for gname, rects, size in rect_defs:
            w("/* the %s: runs (column, screen row, width, height 1), a look's cells %d bytes */\n"
              % ("eyes (index lash << 1 | blink)" if gname == "EYES" else "head's top (index ram << 1 | bow)", size))
            w("#define SAL_%s_N %d\n#define SAL_%s_LOOK %d\n" % (gname, len(rects), gname, size))
            w("#define SAL_%s_RECTS %s\n" % (gname, ", ".join("%d, %d, %d, %d" % (x0, y0 + 1, x1 - x0 + 1, y1 - y0 + 1)
                                                            for x0, y0, x1, y1 in rects)))
        w("/* sparkles: column, screen row, the large one's number, the small one's */\n")
        for name in SPARKS:
            pts = [s for s in spark_pos if s[0] == name]
            w("#define SAL_SPARK_%s %s\n" % (name, ", ".join("%d, %d, 0x%X, 0x%X" % (s[1], s[2], number(s[3]), number(s[4]))
                                                           for s in pts)))
    print("salon: %d characters (room %d, looks %d, sparkles %d), %d + %d bytes; runs: eyes %d, head top %d"
          % (len(chars), nroom, nlooks, len(chars) - nroom - nlooks, len(data1), len(data), len(eye_rects), len(top_rects)))
    if preview:
        render(preview, base, combos, eye_rects, top_rects, art, look)


def nearest(art, c):
    return min(range(len(art)), key=lambda i: sum((art[i][j] - c[j]) ** 2 for j in range(3)))


def palette_for(art, look):
    wool, lips = look[0], look[1]
    pal = [(0, 0, 0)] * 256
    pal[P_GLASS] = GLASS_RGB
    for i in range(N_WOOL):
        pal[P_WOOL + i] = mix((0, 0, 0), WOOL_RGB[wool], 0.30 + 0.70 * i / (N_WOOL - 1))
    for i in range(N_LIPS):
        if lips == 0:
            pal[P_LIPS + i] = mix((0x22, 0x1E, 0x22), (0x6A, 0x50, 0x58), i / (N_LIPS - 1))
        else:
            pal[P_LIPS + i] = mix((0, 0, 0), LIPS_RGB[lips], 0.45 + 0.55 * i / (N_LIPS - 1))
    for i in range(N_BULB):
        pal[P_BULBA + i] = mix(BULB_OFF, BULB_ON, 0.4 + 0.6 * i / (N_BULB - 1))
        pal[P_BULBB + i] = mix(BULB_OFF, (0xFF, 0xFF, 0xFF), 0.5 + 0.5 * i / (N_BULB - 1))
    for i, c in enumerate(art):
        pal[P_FIRST + i] = c
    return pal


def render(path, base, combos, eye_rects, top_rects, art, look):
    """the picture as shown (each pixel 2 x 4 in the PNG: 80-column pixels)"""
    wool, lips, lash, bow, ram, blink = look
    cm = dict(base)
    for rects, k in ((eye_rects, lash << 2 | blink), (top_rects, ram << 3 | bow << 1)):
        for x0, y0, x1, y1 in rects:
            for r in range(y0, y1 + 1):
                for c in range(x0, x1 + 1):
                    cm[(c, r)] = combos[k][(c, r)]
    pal = palette_for(art, look)
    img = Image.new("RGB", (PW, PH))
    px = img.load()
    for (c, r), b in cm.items():
        for y in range(8):
            for x in range(8):
                px[c * 8 + x, r * 8 + y] = pal[b[y * 8 + x]]
    img.resize((PW * 2, PH * 4), Image.NEAREST).save(path)


if __name__ == "__main__":
    main()
