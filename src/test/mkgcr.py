#!/usr/bin/env python3
"""Test images for disk45's GCR formats, from a D64, written independently of
disk45's C++ coder (Schepers' G64.TXT; nibtools' NIB/NBZ layout):

  mkgcr.py g64 in.d64 out.g64        standard G64 (84 entries, 7928-byte slots)
  mkgcr.py nib in.d64 out.nib        nibtools NIB: "MNIB-1541-RAW" header, 8 KB
                                     track reads holding 1.x revolutions, each
                                     shifted by a few bits (not byte-aligned)
  mkgcr.py nbz in.d64 out.nbz        the NIB compressed with LZ77 (lz.c format)
"""
import sys

GCR = [0x0A, 0x0B, 0x12, 0x13, 0x0E, 0x0F, 0x16, 0x17, 0x09, 0x19, 0x1A, 0x1B, 0x0D, 0x1D, 0x1E, 0x15]
SPT = [21] * 17 + [19] * 7 + [18] * 6 + [17] * 5
ZONE_LEN = {3: 7692, 2: 7142, 1: 6666, 0: 6250}


def zone(t):
    return 3 if t <= 17 else 2 if t <= 24 else 1 if t <= 30 else 0


def gcr4(b):
    v = 0
    for x in b:
        v = (v << 10) | (GCR[x >> 4] << 5) | GCR[x & 15]
    return v.to_bytes(5, 'big')


def gcr(data):
    return b''.join(gcr4(data[i:i + 4]) for i in range(0, len(data), 4))


def track_bytes(d64, t, id1, id2):
    off = sum(SPT[:t - 1]) * 256
    n = SPT[t - 1]
    out = bytearray()
    length = ZONE_LEN[zone(t)]
    gap = (length - n * 354) // n
    for s in range(n):
        data = d64[off + s * 256: off + s * 256 + 256]
        hdr = bytes([0x08, s ^ t ^ id2 ^ id1, s, t, id2, id1, 0x0F, 0x0F])
        chk = 0
        for x in data:
            chk ^= x
        out += b'\xff' * 5 + gcr(hdr) + b'\x55' * 9 + b'\xff' * 5
        out += gcr(bytes([0x07]) + data + bytes([chk, 0, 0])) + b'\x55' * gap
    out += b'\x55' * (length - len(out))
    return bytes(out)


def g64(d64):
    id1, id2 = d64[0x16500 + 0xA2], d64[0x16500 + 0xA3]
    head = bytearray(b'GCR-1541' + bytes([0, 84]) + (7928).to_bytes(2, 'little'))
    offs, speeds, body = [0] * 84, [0] * 84, bytearray()
    base = 12 + 84 * 8
    for t in range(1, 36):
        tb = track_bytes(d64, t, id1, id2)
        offs[(t - 1) * 2] = base + len(body)
        speeds[(t - 1) * 2] = zone(t)
        body += len(tb).to_bytes(2, 'little') + tb + b'\xff' * (7928 - len(tb))
    for o in offs:
        head += o.to_bytes(4, 'little')
    for s in speeds:
        head += s.to_bytes(4, 'little')
    return bytes(head + body)


def shifted(data, bits):
    # the stream delayed by `bits` 1-bits (as an unaligned read would see it)
    v = int.from_bytes(data, 'big')
    n = len(data) * 8
    v = (((1 << bits) - 1) << n | v) >> bits if bits else v
    return v.to_bytes(len(data), 'big')


def nib(d64):
    id1, id2 = d64[0x16500 + 0xA2], d64[0x16500 + 0xA3]
    head = bytearray(256)
    head[0:13] = b'MNIB-1541-RAW'
    head[13] = 3
    body = bytearray()
    for i, t in enumerate(range(1, 36)):
        head[0x10 + 2 * i] = t * 2            # half-track number
        head[0x11 + 2 * i] = zone(t)          # density
        tb = track_bytes(d64, t, id1, id2)
        start = (t * 997) % len(tb)            # the read starts anywhere
        rev = tb[start:] + tb + tb             # more than one revolution
        body += shifted(rev[:0x2000], t % 8)
    return bytes(head + body)


def var_size(x):
    b = []
    while True:
        b.append(x & 0x7F)
        x >>= 7
        if not x:
            break
    b.reverse()
    return bytes([c | 0x80 for c in b[:-1]] + [b[-1]])


def lz(data):
    # lz.c's stream: the least common byte as marker, matches of 4+ bytes
    hist = [0] * 256
    for x in data:
        hist[x] += 1
    marker = hist.index(min(hist))
    out = bytearray([marker])
    i, n = 0, len(data)
    last = {}
    while i < n:
        best_len, best_off = 0, 0
        key = data[i:i + 4]
        j = last.get(key)
        if j is not None and i - j <= 0xFFFF:
            l = 0
            while i + l < n and data[j + l] == data[i + l] and l < 0xFFFF:
                l += 1
            best_len, best_off = l, i - j
        if best_len >= 6:
            out += bytes([marker]) + var_size(best_len) + var_size(best_off)
            for k in range(i, i + best_len):
                last[data[k:k + 4]] = k
            i += best_len
        else:
            if data[i] == marker:
                out += bytes([marker, 0])
            else:
                out.append(data[i])
            last[key] = i
            i += 1
    return bytes(out)


if __name__ == '__main__':
    kind, src, dst = sys.argv[1:4]
    d64 = open(src, 'rb').read()
    out = {'g64': g64, 'nib': nib, 'nbz': lambda d: lz(nib(d))}[kind](d64)
    open(dst, 'wb').write(out)
