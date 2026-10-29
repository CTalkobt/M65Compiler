#!/usr/bin/env python3
"""make_tap.py: TAP tape images in the KERNAL's tape format, for disk45's tests.

  make_tap.py OUT.tap --platform c64|vic20|c16 [--version 1|2] [--speed F]
              [--jitter F] [--seed N] [--damage BLOCK:COPY ...]
              --prg NAME:ADDR:FILE ... --seq NAME:FILE ...

Each file is a 192-byte header block and its data (a program's one block,
or a SEQ file's 192-byte blocks of type 2), every block written twice
(countdown $89..$81, then $09..$01), its bytes framed by a long+medium
marker with odd parity, then the checksum and an end-of-data marker
(c64-wiki "Datassette Encoding"). Pulse lengths in TAP units (8 cycles):
C64 short/medium/long $30/$42/$56, VIC-20 $2C/$3C/$4F, C16 $35/$6A/$D4
(version 2: half-waves). --speed scales them (a slow or fast tape),
--jitter varies each pulse by up to that fraction. --damage 3:1 swaps one
bit's pulses in the first copy of the fourth block (counted from 0, both
copies of a block are one block): a parity and checksum error there.
"""
import argparse
import random
import struct

NOMINAL = {"c64": (0x30, 0x42, 0x56), "vic20": (0x2C, 0x3C, 0x4F), "c16": (0x35, 0x6A, 0xD4)}
PLATFORM = {"c64": 0, "vic20": 1, "c16": 2}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--platform", default="c64", choices=NOMINAL)
    ap.add_argument("--version", type=int, default=None)
    ap.add_argument("--speed", type=float, default=1.0)
    ap.add_argument("--jitter", type=float, default=0.0)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--damage", action="append", default=[])
    ap.add_argument("--prg", action="append", default=[])
    ap.add_argument("--seq", action="append", default=[])
    ap.add_argument("--leader", type=int, default=1500, help="pilot pulses before a file's first block")
    a = ap.parse_args()
    version = a.version if a.version is not None else (2 if a.platform == "c16" else 1)
    rnd = random.Random(a.seed)
    S, M, L = NOMINAL[a.platform]
    damage = {tuple(int(x) for x in d.split(":")) for d in a.damage}

    waves = []   # full waves, in TAP units (floats until written)

    def pulse(n):
        waves.append(n * a.speed * (1 + rnd.uniform(-a.jitter, a.jitter)))

    def bit(b):
        if b:
            pulse(M); pulse(S)
        else:
            pulse(S); pulse(M)

    def byte(v, flip=False):
        pulse(L); pulse(M)
        parity = 1
        for k in range(8):
            b = (v >> k) & 1
            parity ^= b
            if flip and k == 3:
                b ^= 1   # this bit's pulses swapped: a wrong bit, a parity error
            bit(b)
        bit(parity)

    block_no = [0]

    def block(payload, leader):
        n = block_no[0]
        block_no[0] += 1
        for copy, start in ((1, 0x89), (2, 0x09)):
            for _ in range(leader if copy == 1 else 60):
                pulse(S)
            for k in range(9):
                byte(start - k)
            for i, v in enumerate(payload):
                byte(v, flip=(n, copy) in damage and i == 5)
            chk = 0
            for v in payload:
                chk ^= v
            byte(chk)
            pulse(L); pulse(S)   # end-of-data marker
        for _ in range(80):
            pulse(S)
        waves.append(None)       # a pause

    def header(kind, name, start, end):
        h = bytes([kind]) + struct.pack("<HH", start, end)
        n = name.upper().encode("ascii")[:16]
        return (h + n.ljust(16, b" ")).ljust(192, b" ")

    first = True
    for spec in a.prg:
        name, addr, path = spec.split(":")
        addr = int(addr, 16)
        data = open(path, "rb").read()
        block(header(3, name, addr, addr + len(data)), a.leader if first else 400)
        block(data, 300)
        first = False
    for spec in a.seq:
        name, path = spec.split(":")
        data = open(path, "rb").read()
        block(header(4, name, 0, 0), a.leader if first else 400)
        for i in range(0, max(len(data), 1), 191):
            chunk = data[i:i + 191]
            block(bytes([2]) + chunk.ljust(191, b"\0"), 300)
        first = False

    out = bytearray()
    for w in waves:
        if w is None:
            out += b"\0" + struct.pack("<I", 30000)[:3]   # a pause, in cycles
            continue
        if version == 2:
            h = max(1, round(w / 2))
            out += bytes([min(h, 255), min(round(w) - h, 255)])
        else:
            out.append(max(1, min(round(w), 255)))
    sig = b"C16-TAPE-RAW" if a.platform == "c16" else b"C64-TAPE-RAW"
    hdr = sig + bytes([version, PLATFORM[a.platform], 0, 0]) + struct.pack("<I", len(out))
    open(a.out, "wb").write(hdr + out)


if __name__ == "__main__":
    main()
