#!/usr/bin/env python3
"""Generate gfx.h for Glam Lambs: the custom glyphs, from ASCII pictures.

Usage: python3 tools/gen_gfx.py <outdir>

Hi-res glyphs are 8 x 8 ('#' set). Multicolour glyphs are 4 x 8 double-wide
pixels: '.' the background ($D021), 'd' $D022 (dark grey: faces, legs),
'g' $D023 (light green: grass), 'c' the cell's own colour (wool, lipstick,
eyes). In 80 columns a multicolour pixel is square.

A multicolour glyph named *_R faces right; its mirror image follows it as
*_L (the game relies on that order). A picture several cells wide is cut
into cells and each cell gets its _R / _L pair.
"""
import sys

MC = {".": 0, "d": 1, "g": 2, "c": 3}

# --- sheep: 4 x 2 cells (16 x 16 pixels) -----------------------------------
# columns 0-2 the wool (top row; bottom row: wool and legs, two steps; full,
# short and shorn),
# column 3 the head (top: ears, eye; bottom: the muzzle, lipstick, bow)

WOOL = {
    "WOOL_T": """
............
..cc.ccc.cc.
.ccccccccccc
cccccccccccc
cccccccccccc
cccccccccccc
cccccccccccc
cccccccccccc
""",
    "WOOL_BA": """
cccccccccccc
cccccccccccc
.ccccccccccc
..cccccccccc
..dd....dd..
..dd....dd..
..dd....dd..
.ddd...ddd..
""",
    "WOOL_BB": """
cccccccccccc
cccccccccccc
.ccccccccccc
..cccccccccc
...dd..dd...
...dd..dd...
..dd....dd..
.dd....dd...
""",
    "SHORT_T": """
............
............
..cc.cc.cc..
.ccccccccccc
cccccccccccc
cccccccccccc
cccccccccccc
cccccccccccc
""",
    "SHORT_BA": """
cccccccccccc
.ccccccccccc
..cccccccccc
..dd....dd..
..dd....dd..
..dd....dd..
..dd....dd..
.ddd...ddd..
""",
    "SHORT_BB": """
cccccccccccc
.ccccccccccc
..cccccccccc
...dd..dd...
...dd..dd...
...dd..dd...
..dd....dd..
.dd....dd...
""",
    "SHORN_T": """
............
............
............
............
............
...ccccccccc
..cccccccccc
.cc.c.c.cccc
""",
    "SHORN_BA": """
.ccccccccccc
..cccccccccc
...ccccccccc
...d.....d..
...d.....d..
...d.....d..
...d.....d..
..dd....dd..
""",
    "SHORN_BB": """
.ccccccccccc
..cccccccccc
...ccccccccc
....d...d...
....d...d...
...d.....d..
...d.....d..
..d.....d...
""",
    "LAMB_T": """
............
............
............
............
............
.....cc.cc..
....cccccccc
...ccccccccc
""",
    "LAMB_BA": """
...ccccccccc
...ccccccccc
....cccccccc
.....d...d..
.....d...d..
.....d...d..
.....d...d..
....dd..dd..
""",
    "LAMB_BB": """
...ccccccccc
...ccccccccc
....cccccccc
......d.d...
......d.d...
.....d...d..
.....d...d..
....d...d...
""",
}

HEAD_T = {
    "EWE": """
....
dd..
.dd.
dddd
dccd
dddd
dddd
dddd
""",
    "EWELASH": """
.c.c
ddc.
.dd.
dddd
dccd
dddd
dddd
dddd
""",
    "RAM": """
cc..
c.c.
c.cd
cddd
dccd
dddd
dddd
dddd
""",
    "RAMLASH": """
cc.c
c.c.
c.cd
cddd
dccd
dddd
dddd
dddd
""",
    "LAMBTOP": """
....
....
....
....
....
dd..
.dd.
dddd
""",
    "LAMBTOPLASH": """
....
....
....
....
....
dd.c
.ddc
dddd
""",
}

MUZZLE = {
    "MUZ": """
dddd
.ddd
..dd
..dd
....
....
....
....
""",
    "MUZLIPS": """
dddd
.ddd
..cc
..dc
....
....
....
....
""",
    "MUZBOW": """
dddd
c.cd
.cdd
c.dd
....
....
....
....
""",
    "MUZLIPSBOW": """
dddd
c.dd
.ccc
c.dc
....
....
....
....
""",
    "LAMBMUZ": """
d.dd
dddd
.ddd
..dd
....
....
....
....
""",
    "LAMBMUZLIPS": """
d.dd
dddd
.dcc
..dd
....
....
....
....
""",
    "LAMBMUZBOW": """
d.dd
dddd
c.cd
.cdd
c...
....
....
....
""",
    "LAMBMUZLIPSBOW": """
d.dd
dddd
c.cc
.cdd
c...
....
....
....
""",
}

# --- the stylist: 2 x 2 cells, top yellow hair, bottom a pink suit ----------
PLAYER = {
    "PLAYER_A": """
..cccc..
.cccccc.
ccdddccc
cddddddc
cd.dd.dc
.dddddd.
..dddd..
...dd...
.cccccc.
cccccccc
dccccccd
dccccccd
.cccccc.
.cc..cc.
.dd..dd.
dd....dd
""",
    "PLAYER_B": """
..cccc..
.cccccc.
ccdddccc
cddddddc
cd.dd.dc
.dddddd.
..dddd..
...dd...
.cccccc.
cccccccc
dccccccd
dccccccd
.cccccc.
..cccc..
...dd...
..dddd..
""",
}

# --- hi-res singles ----------------------------------------------------------
SINGLE = [
    ("FENCE_H", """
........
........
########
.#....#.
########
.#....#.
.#....#.
........
"""),
    ("FENCE_V", """
...##...
..####..
...##...
...##...
..####..
...##...
...##...
...##...
"""),
    ("POST", """
..####..
.######.
.######.
..####..
..####..
..####..
..####..
..####..
"""),
    ("FLOWER", """
........
...#....
..#.#...
...#....
...#.#..
..##....
...#....
........
"""),
    ("HEART", """
.##.##..
#######.
#######.
.#####..
..###...
...#....
........
........
"""),
    ("GATE", """
#.#.#.#.
........
.#.#.#.#
........
#.#.#.#.
........
.#.#.#.#
........
"""),
    ("BLOCK", """
########
########
########
########
########
########
########
########
"""),
    ("BULLET", """
........
........
..###...
.#####..
.#####..
..###...
........
........
"""),
    ("SPARKLE", """
....#...
....#...
..#.#.#.
...###..
.#######
...###..
..#.#.#.
....#...
"""),
]

MC_SINGLE = [
    ("GRASS1", """
....
....
....
....
.g..
g.g.
.g..
....
"""),
    ("GRASS2", """
....
..g.
.g.g
..g.
....
....
....
....
"""),
]

# --- the salon's big sheep: 6 x 3 cells (24 x 24) ---------------------------
# columns 0-3 the wool and legs (cell colour: the wool), 4-5 the head: rows
# 0-1 the face (cell colour: black, for the eye), row 2 the muzzle (cell
# colour: the lipstick)
BIG = """
.....cc..cc.cc..........
...cccccccccccc...dd....
..cccccccccccccc.dddd...
.cccccccccccccccdd.ddd..
ccccccccccccccccddddddd.
cccccccccccccccccdddcccd
cccccccccccccccccdddcccd
ccccccccccccccccdddddddd
cccccccccccccccccddddddd
cccccccccccccccccddddddd
cccccccccccccccccddddddd
cccccccccccccccccddddddd
.cccccccccccccccc.dddddd
..ccccccccccccccc..ddddd
...ccccccccccccc....dddd
...dd.cccccc.dd......ddd
...dd........dd.....dddd
...dd........dd.....cccc
...dd........dd.....dccd
...dd........dd......cc.
...dd........dd.........
...dd........dd.........
..ddd.......ddd.........
........................
"""
BIG_NAMES = [["BIG0", "BIG1", "BIG2", "BIG3", "BEAR_A", "BEAR_B"],
             ["BIG4", "BIG5", "BIG6", "BIG7", "BFACE_A", "BFACE_B"],
             ["BIG8", "BIG9", "BIG10", "BIG11", "BMUZ_A", "BMUZ_B"]]

# two-cell variants of row 0's head (the ram's horns, the eyelashes), and
# the bow over it
BIG_EXTRA = [
    ("BHORN", """
.ccc....
c...c...
c.cc.cd.
c.c.ddd.
.c.dddd.
.cdddccc
.ddddccc
dddddddd
"""),
    ("BLASH", """
........
.....c.c
..dd..c.
.dddd.c.
dd.ddcd.
.dddcccd
.dddcccd
dddddddd
"""),
    ("BBOW", """
........
........
cc....cc
ccc..ccc
cccccccc
ccc..ccc
cc....cc
........
"""),
]


def rows(art, w):
    return [l.ljust(w, ".")[:w] for l in art.strip("\n").split("\n")]


def hires(art):
    r = rows(art, 8)
    assert len(r) == 8, art
    return [sum(1 << (7 - i) for i, c in enumerate(line) if c == "#") for line in r]


def mc_cell(lines):
    """8 lines of 4 multicolour pixels"""
    assert len(lines) == 8 and all(len(l) == 4 for l in lines), lines
    return [sum(MC[c] << (6 - 2 * i) for i, c in enumerate(l)) for l in lines]


def mc_cells(lines, w):
    """cut multicolour lines (w cells of 4 pixels) into w glyphs"""
    return [mc_cell([l[c * 4:c * 4 + 4] for l in lines]) for c in range(w)]


def mirror_mc(g):
    return [(b & 3) << 6 | (b >> 2 & 3) << 4 | (b >> 4 & 3) << 2 | b >> 6 for b in g]


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    names, data = [], []

    def add(name, g):
        names.append(name)
        data.append(g)

    def add_rl(name, g):                  # a multicolour cell and its mirror
        add(name + "_R", g)
        add(name + "_L", mirror_mc(g))

    for name, art in WOOL.items():
        for i, g in enumerate(mc_cells(rows(art, 12), 3)):
            add_rl("%s%d" % (name, i), g)
    for name, art in list(HEAD_T.items()) + list(MUZZLE.items()):
        add_rl(name, mc_cell(rows(art, 4)))
    for name, art in PLAYER.items():
        lines = rows(art, 8)
        assert len(lines) == 16
        for r in range(2):
            for c, g in enumerate(mc_cells(lines[r * 8:r * 8 + 8], 2)):
                add("%s%d" % (name, r * 2 + c), g)
    for name, art in SINGLE:
        add(name, hires(art))
    for name, art in MC_SINGLE:
        add(name, mc_cell(rows(art, 4)))
    big = rows(BIG, 24)
    assert len(big) == 24
    for cr in range(3):
        for cc, g in enumerate(mc_cells(big[cr * 8:cr * 8 + 8], 6)):
            add(BIG_NAMES[cr][cc], g)
    for name, art in BIG_EXTRA:
        a, b = mc_cells(rows(art, 8), 2)
        add(name + "_A", a)
        add(name + "_B", b)
    assert len(names) <= 0xA0, len(names)   # from $60 to $FF

    with open(out + "/gfx.h", "w") as f:
        f.write("/* gfx.h -- generated by tools/gen_gfx.py: do not edit */\n\n")
        f.write("#define G_COUNT %d\n" % len(names))
        for i, n in enumerate(names):
            f.write("#define G_%s (CH_BASE + %d)\n" % (n, i))
        # (the glyphs' data: in the one file that defines GFX_DATA)
        f.write("\n#ifdef GFX_DATA\nstatic const unsigned char glyph_data[%d] = {\n" % (len(data) * 8))
        for n, g in zip(names, data):
            f.write("    " + ", ".join("0x%02X" % b for b in g) + ",   /* %s */\n" % n)
        f.write("};\n#endif\n")


if __name__ == "__main__":
    main()
