"""float_fast_model.py — a model of lib/stdlib_asm/float_fast.s45 (cc45
-ffast-float): the expected values of src/test-resources/test_float_fast_xemu.c.
Checked in xemu against the BASIC 65 ROM with its float rounding fixed
(mega65-rom "Option A") and against float_fast.s45 on random operands.

CBM 40-bit packed floats.

Packed: e, m1 (sign bit 7 | top 7 bits), m2, m3, m4 (big-endian mantissa,
hidden 1). add/sub follow the BASIC 65 ROM's faddt (shift into an 8-bit
guard; QUIRK: the carry left by the shift is added to the guard), mul/div
are correctly rounded (half up), as the fixed ROM's fmultt / fdivt.
"""

QUIRK = True
MAXF = bytes([0xFF, 0x7F, 0xFF, 0xFF, 0xFF])


def unpack(p):
    e = p[0]
    s = p[1] & 0x80
    m = ((p[1] | 0x80) << 24) | (p[2] << 16) | (p[3] << 8) | p[4]
    return e, s, m


def pack(e, s, m, g):
    """round (guard bit 7, half up), then exponent range: saturate / zero"""
    if g & 0x80:
        m += 1
        if m == 1 << 32:
            m = 1 << 31
            e += 1
    if e <= 0:
        return bytes(5)
    if e > 255:
        return bytes([0xFF, 0x7F | s, 0xFF, 0xFF, 0xFF])
    return bytes([e, (m >> 24 & 0x7F) | s, m >> 16 & 0xFF, m >> 8 & 0xFF, m & 0xFF])


def add(a, b, sub=False):
    ea, sa, ma = unpack(a)
    eb, sb, mb = unpack(b)
    if sub:
        sb ^= 0x80
    if eb == 0:
        return bytes(a)
    if ea == 0:
        return pack(eb, sb, mb, 0)
    # (ROM: FAC = b, ARG = a; the operand with the smaller exponent shifted)
    if ea >= eb:
        big_m, big_s, small_m, n, E = ma, sa, mb, ea - eb, ea
    else:
        big_m, big_s, small_m, n, E = mb, sb, ma, eb - ea, eb
    t, g, c = small_m, 0, 0
    if n == 0:
        c = 1                   # (sbc with equal exponents: no borrow)
    else:
        while n >= 8:
            g = t & 0xFF
            t >>= 8
            n -= 8
            c = 1 if n == 0 else 0  # (the last adc #8 wraps to 0)
        while n > 0:
            c = g & 1
            g = (g >> 1) | ((t & 1) << 7)
            t >>= 1
            n -= 1
    if not QUIRK:
        c = 0
    if (sa ^ sb) == 0:
        g2 = g + c
        carry = g2 >> 8
        g2 &= 0xFF
        r = big_m + t + carry
        s = big_s
        if r >> 32:
            # rndshf: exponent + 1, shift right with the carry
            g2 = ((r & 1) << 7) | (g2 >> 1)
            r >>= 1
            E += 1
        return pack(E, s, r & 0xFFFFFFFF, g2)
    # different signs: big - shifted small (40 bits), guard = 0 - g
    v = (big_m << 8) - ((t << 8) | g)
    s = big_s
    if v < 0:
        v = -v
        s ^= 0x80
    if v >> 8 == 0:
        return bytes(5)         # (ROM normal: four byte shifts give 0, the guard not looked at)
    # normal: shift left until the top bit is set
    k = 0
    while not (v >> 39) & 1:
        v <<= 1
        k += 1
    if k >= E:
        return bytes(5)
    return pack(E - k, s, v >> 8, v & 0xFF)


def mul(a, b):
    ea, sa, ma = unpack(a)
    eb, sb, mb = unpack(b)
    if ea == 0 or eb == 0:
        return bytes(5)
    p = ma * mb
    e = ea + eb - 128
    if not (p >> 63) & 1:
        p <<= 1
        e -= 1
    return pack(e, sa ^ sb, p >> 32, (p >> 24) & 0xFF)


def div(a, b):
    ea, sa, ma = unpack(a)
    eb, sb, mb = unpack(b)
    if eb == 0:
        if ea == 0:
            return bytes(5)
        return bytes([0xFF, 0x7F | (sa ^ sb), 0xFF, 0xFF, 0xFF])
    if ea == 0:
        return bytes(5)
    e = ea - eb + 128
    if ma >= mb:
        q = (ma << 32) // mb          # 33 bits: 1.xxxx
        e += 1
        m = q >> 1
        rbit = q & 1                  # the bit below the 32 kept
    else:
        q = (ma << 32) // mb          # 32 bits: 0.1xxx
        m = q
        rbit = ((ma << 33) // mb) & 1
    return pack(e, sa ^ sb, m, 0x80 if rbit else 0)


def cmp(a, b):
    def key(p):
        e, s, m = unpack(p)
        if e == 0:
            return 0
        v = (e << 32) | m
        return -v if s else v
    ka, kb = key(a), key(b)
    return 0xFF if ka < kb else (1 if ka > kb else 0)
