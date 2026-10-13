# Build GEOS Convert (CVT) files per Schepers' CVT.TXT / cbmconvert:
# 30-byte directory entry + "PRG formatted GEOS file V1.0", the info block
# (sector bytes 2-255), for VLIR the record block (count, last-byte index)
# and the records padded to 254-byte blocks (the last one unpadded)
import sys, random
def rounddiv(a, b): return (a + b - 1) // b
def make(name, vlir, records, seed, geostype=6):
    rnd = random.Random(seed)
    sig = bytearray(254)
    sig[0] = 0x83                                   # USR, closed
    n = name.encode('ascii')[:16]
    sig[3:3+16] = n + b'\xa0' * (16 - len(n))
    sig[0x15] = 1 if vlir else 0                    # structure
    sig[0x16] = geostype                            # GEOS type
    sig[0x17:0x1c] = bytes([88, 8, 19, 13, 35])     # 1988-08-19 13:35
    s = b'PRG formatted GEOS file V1.0\0'
    sig[30:30+len(s)] = s
    info = bytearray(254)
    info[0:3] = b'\x03\x15\xbf'
    for i in range(3, 66): info[i] = rnd.randrange(256)
    info[0x42] = 0x83; info[0x43] = geostype; info[0x44] = 1 if vlir else 0
    info[0x45:0x47] = b'\x00\x04'
    cls = b'Test V1.0'; info[0x4b:0x4b+len(cls)] = cls
    desc = b'A test file for disk45'; info[0x9e:0x9e+len(desc)] = desc
    out = sig + info
    if not vlir:
        data = bytes(rnd.randrange(256) for _ in range(records))
        out += data
        blocks = 1 + rounddiv(len(data), 254) if data else 2
    else:
        rec = bytearray(254); body = bytearray(); total = 2
        for i, r in enumerate(records):
            if r is None: rec[2*i:2*i+2] = b'\x00\xff'; continue
            data = bytes(rnd.randrange(256) for _ in range(r))
            body = body + bytearray(254 * rounddiv(len(body), 254) - len(body))
            body += data if r else bytes(254)
            b = max(1, rounddiv(r, 254)); total += b
            rec[2*i] = b
            rec[2*i+1] = 1 if r == 0 else (r % 254 + 1 if r % 254 else 0xff)
        out += rec + body
        blocks = total
    out[0x1c] = blocks & 0xff; out[0x1d] = blocks >> 8
    return bytes(out)
open(sys.argv[1] + '/vlir.cvt', 'wb').write(make('GeoTest App', True, [700, 254, None, 0, 1, 5000], 1))
open(sys.argv[1] + '/seq.cvt', 'wb').write(make('a seq file', False, 3000, 2, 7))
