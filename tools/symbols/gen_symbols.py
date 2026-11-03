#!/usr/bin/env python3
"""Make objdump45's shipped symbol files (lib/symbols/*.sym, #144).

  gen_symbols.py c64 <c64rom> <roms> > lib/symbols/c64.sym
      <c64rom>: a clone of https://github.com/mist64/c64rom (Commodore's
      C64 BASIC and KERNAL source, its symbols as Commodore wrote them);
      <roms>: a directory with the real basic.bin ($A000) and kernal.bin
      ($E000). The source is put together as its linker config places it,
      converted from ca65 with asmCvt45 and assembled with ca45 --cpu 6502;
      the bytes must be the ROMs' (else nothing is written), so every
      address is the ROM's. Written: the KERNAL jump table (the names of
      Commodore's Programmer's Reference Guide), the ROM routines a JSR or
      JMP in the source reaches (their source names, upper case), the RAM
      vectors at $0300-$0333 and the I/O registers.

  gen_symbols.py mega65 <mega65.h> > lib/symbols/mega65.sym
      The KERNAL jump table (the C65 KERNAL keeps the C64's) and every I/O
      register of <mega65.h>: its struct overlays' fields (STRUCTVAR_FIELD,
      array elements numbered) and its register #defines.

The file format objdump45 -S reads: "$addr NAME [comment]" lines, # comments.
"""
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
BIN = os.path.join(HERE, "..", "..", "bin")

# the KERNAL jump table, $FF81 on: the documented names
JUMP_TABLE = [
    "CINT", "IOINIT", "RAMTAS", "RESTOR", "VECTOR", "SETMSG", "SECOND", "TKSA", "MEMTOP", "MEMBOT",
    "SCNKEY", "SETTMO", "ACPTR", "CIOUT", "UNTLK", "UNLSN", "LISTEN", "TALK", "READST", "SETLFS",
    "SETNAM", "OPEN", "CLOSE", "CHKIN", "CHKOUT", "CLRCHN", "CHRIN", "CHROUT", "LOAD", "SAVE",
    "SETTIM", "RDTIM", "STOP", "GETIN", "CLALL", "UDTIM", "SCREEN", "PLOT", "IOBASE"]


def jump_table(comments=None):
    out = []
    for k, name in enumerate(JUMP_TABLE):
        a = 0xFF81 + 3 * k
        c = (comments or {}).get(a, "")
        out.append((a, name, "KERNAL jump table" + (": " + c if c else "")))
    return out


def c64_io():
    out = [(0x0000, "D6510", "6510 data direction register"), (0x0001, "R6510", "6510 I/O port (memory map)")]
    for n in range(8):
        out.append((0xD000 + 2 * n, "VIC_SPR%dX" % n, "sprite %d X (bits 0-7)" % n))
        out.append((0xD001 + 2 * n, "VIC_SPR%dY" % n, "sprite %d Y" % n))
    vic = ["SPRXMSB", "CTRL1", "RASTER", "LPENX", "LPENY", "SPRENA", "CTRL2", "SPRYEXP", "MEMPTR", "IRQ",
           "IRQMASK", "SPRPRIO", "SPRMC", "SPRXEXP", "SPRCOLL", "SPRBGCOLL", "BORDERCOL", "BGCOL0", "BGCOL1",
           "BGCOL2", "BGCOL3", "SPRMC0", "SPRMC1"]
    for k, n in enumerate(vic):
        out.append((0xD010 + k, "VIC_" + n, "VIC-II"))
    for n in range(8):
        out.append((0xD027 + n, "VIC_SPR%dCOL" % n, "sprite %d colour" % n))
    for v in range(3):
        for k, n in enumerate(["FREQLO", "FREQHI", "PWLO", "PWHI", "CTRL", "AD", "SR"]):
            out.append((0xD400 + 7 * v + k, "SID_V%d%s" % (v + 1, n), "SID voice %d" % (v + 1)))
    for k, n in enumerate(["FCLO", "FCHI", "RESFILT", "MODEVOL", "POTX", "POTY", "OSC3", "ENV3"]):
        out.append((0xD415 + k, "SID_" + n, "SID"))
    out.append((0xD800, "COLORRAM", "colour RAM"))
    for c, base in ((1, 0xDC00), (2, 0xDD00)):
        for k, n in enumerate(["PRA", "PRB", "DDRA", "DDRB", "TALO", "TAHI", "TBLO", "TBHI", "TOD10TH",
                               "TODSEC", "TODMIN", "TODHR", "SDR", "ICR", "CRA", "CRB"]):
            out.append((base + k, "CIA%d_%s" % (c, n), "CIA %d" % c))
    return out


def c64(src, roms):
    cfg = open(os.path.join(src, "rom.cfg")).read()
    starts = {m.group(1): int(m.group(2)[1:], 16)
              for m in re.finditer(r"^\s*(\w+):\s*start\s*=\s*(\$[0-9A-Fa-f]+)", cfg, re.M)}
    ram = {"ZPKERNAL", "STACK", "KVAR", "KVECTORS", "ZPBASIC", "STRTMP", "BVECTORS"}
    ramsyms = {}

    def flat(top, d, out, st):
        for line in open(os.path.join(src, d, top)):
            m = re.match(r'\s*\.include\s+"([^"]+)"', line)
            if m:
                flat(m.group(1), d, out, st)
                continue
            m = re.match(r'\s*\.segment\s+"(\w+)"', line)
            if m:
                st["seg"] = m.group(1)
                st["pc"] = starts[st["seg"]]
                if st["seg"] not in ram:
                    out.append("*=$%04X\n" % st["pc"])
                continue
            if re.match(r"\s*\.feature", line):
                continue
            if st["seg"] in ram:   # (RAM: its names as equates; ca45 makes .res under *= absolute)
                code = line.split(";")[0].rstrip()
                m = re.match(r"^([A-Za-z_]\w*)?:?\s*\.res\s+(\S+)", code) or re.match(r"^([A-Za-z_]\w*):?()$", code)
                if m:
                    if m.group(1):
                        out.append("%s = $%04X\n" % (m.group(1), st["pc"]))
                        ramsyms[m.group(1)] = (st["pc"], line.split(";", 1)[1].strip() if ";" in line else "")
                    if m.group(2):
                        st["pc"] += int(m.group(2).replace("$", "0x"), 0)
                    continue
                if code.strip():
                    out.append(line)
                continue
            m = re.match(r"^([A-Za-z_]\w*)(?!\w|:)(.*)$", line.rstrip("\n"))   # (labels without colons)
            if m and not re.match(r"\s*=", m.group(2)):
                line = m.group(1) + ":" + m.group(2) + "\n"
            out.append(line)

    labels, called, comments = {}, set(), {}
    rom = {}
    for base, f in ((0xA000, "basic.bin"), (0xE000, "kernal.bin")):
        for k, v in enumerate(open(os.path.join(roms, f), "rb").read()):
            rom[base + k] = v
    with tempfile.TemporaryDirectory() as tmp:
        for top, d in (("kernal.s", "kernal"), ("basic.s", "basic")):
            out = []
            flat(top, d, out, {"seg": None, "pc": 0})
            s65, s45, lst = (os.path.join(tmp, d + x) for x in (".s", ".s45", ".lst"))
            open(s65, "w").writelines(out)
            subprocess.run([os.path.join(BIN, "asmCvt45"), "-f", "ca65", "-t", "ca45", "-o", s45, s65],
                           check=True, capture_output=True)
            r = subprocess.run([os.path.join(BIN, "ca45"), "--cpu", "6502", "-O0", "-L", lst, s45, "-o",
                                os.path.join(tmp, d + ".bin")], capture_output=True, text=True)
            if r.returncode:
                sys.exit("ca45: " + r.stdout + r.stderr)
            bad, cur = 0, None
            for line in open(lst):
                m = re.match(r"^([0-9a-f]{8}):\s+([A-Za-z_]\w*):\s*$", line)
                if m:
                    labels[m.group(2)] = int(m.group(1), 16)
                    continue
                m = re.match(r"^([0-9a-f]{8}): ((?:[0-9a-f]{2} )+)", line)
                if m:
                    cur, bs = int(m.group(1), 16), m.group(2).split()
                else:
                    m = re.match(r"^ {10}((?:[0-9a-f]{2} )+)", line)
                    if not m or cur is None:
                        continue
                    bs = m.group(1).split()
                for b in bs:   # (the checksum bytes, BASIC's $BF52 and the KERNAL's $E4AC, are set after the build; the fill after $BF52)
                    if cur in rom and rom[cur] != int(b, 16) and not (0xBF52 <= cur < 0xC000) and cur != 0xE4AC:
                        bad += 1
                        if bad <= 5:
                            sys.stderr.write("$%04X: %s, the ROM's $%02X\n" % (cur, b, rom[cur]))
                    cur += 1
            if bad:
                sys.exit("%s: %d bytes are not the ROM's: not written" % (d, bad))
            for line in open(s45):
                m = re.match(r"^\s+(jsr|jmp)\s+([A-Za-z_]\w*)\s*(;.*)?$", line, re.I)
                if m:
                    called.add(m.group(2))
    # the jump table's comments, in order
    tcom, k = {}, 0
    for line in open(os.path.join(src, "kernal", "vectors.s")):
        m = re.match(r"^\S*\s+jmp\s+(\(?\w+\)?)\s*(;(.*))?$", line)
        if m:
            to = m.group(1)
            tcom[0xFF81 + 3 * k] = ((m.group(3) or "").strip() + (", through " + to if to.startswith("(") else ", " + to)).lstrip(", ")
            k += 1
    out = jump_table(tcom)
    out += [(0xFFFA, "NMIVEC", "NMI vector"), (0xFFFC, "RESETVEC", "RESET vector"), (0xFFFE, "IRQVEC", "IRQ/BRK vector")]
    taken = {a for a, _, _ in out}
    names = {n for _, n, _ in out} | {n for _, n, _ in c64_io()}
    for n, a in sorted(labels.items(), key=lambda t: t[1]):
        if n in called and (0xA000 <= a < 0xC000 or 0xE000 <= a < 0xFF81) and a not in taken:
            basic = a < 0xC000
            name = n.upper()
            if name in names:   # (the routine a jump table entry reaches has its name: KRN_CINT; BASIC's STOP: BAS_STOP)
                name = ("BAS_" if basic else "KRN_") + name
            out.append((a, name, "BASIC" if basic else "KERNAL"))
            taken.add(a)
            names.add(name)
    for n, (a, c) in sorted(ramsyms.items(), key=lambda t: t[1]):
        if 0x0300 <= a < 0x0334:
            out.append((a, n.upper(), "vector" + (": " + c if c else "")))
    out += c64_io()
    return out, "C64: BASIC V2 and KERNAL (901226-01, 901227-03) from Commodore's source (mist64/c64rom), " \
                 "checked against the ROMs; the I/O registers"


def mega65(header):
    text = open(header).read()
    structs = {}
    for m in re.finditer(r"struct\s+(\w+)\s*\{(.*?)\};", text, re.S):
        fields = []
        for f in re.finditer(r"^\s*(?:volatile\s+)?(unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long|"
                             r"char|short|int|long|struct\s+\w+)\s+(\w+)\s*(?:\[(\d+)\])?\s*;(?:\s*/\*(.*?)\*/)?",
                             m.group(2), re.M):
            fields.append((f.group(1), f.group(2), int(f.group(3) or 1), (f.group(4) or "").strip()))
        structs[m.group(1)] = fields

    def size(t):
        if t.startswith("struct"):
            return sum(size(ft) * n for ft, _, n, _ in structs[t.split()[1]])
        return {"char": 1, "short": 2, "int": 2, "long": 4}[t.split()[-1]]

    out = jump_table()
    taken = {a for a, _, _ in out}

    def walk(sname, base, prefix, out):
        off = 0
        for t, name, n, com in structs[sname]:
            sz = size(t)
            for k in range(n):
                nm = prefix + "_" + name.upper() + (str(k) if n > 1 else "")
                a = base + off + k * sz
                if t.startswith("struct"):
                    walk(t.split()[1], a, nm, out)
                else:
                    c = re.sub(r"^\+?\$[0-9A-Fa-f]{2,4}(-\$[0-9A-Fa-f]{2,4})?:\s*", "", com)
                    out.append((a, nm, c))
            off += sz * n

    for m in re.finditer(r"#define\s+(\w+)\s+\(\(volatile\s+struct\s+(\w+)\s*\*\)\s*0x([0-9A-Fa-f]+)\)", text):
        regs = []
        walk(m.group(2), int(m.group(3), 16), m.group(1).upper(), regs)
        for a, n, c in regs:
            if a not in taken:
                out.append((a, n, c))
                taken.add(a)
    for m in re.finditer(r"#define\s+(\w+)\s+\(\*\(volatile\s+unsigned\s+(?:char|short)\s*\*\)\s*0x([0-9A-Fa-f]+)\)"
                         r"(?:\s*/\*(.*?)\*/)?", text):
        a = int(m.group(2), 16)
        if a not in taken:
            out.append((a, m.group(1), (m.group(3) or "").strip()))
            taken.add(a)
    return out, "MEGA65: the KERNAL jump table and the I/O registers of <mega65.h>"


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "c64":
        syms, what = c64(sys.argv[2], sys.argv[3])
    elif len(sys.argv) >= 3 and sys.argv[1] == "mega65":
        syms, what = mega65(sys.argv[2])
    else:
        sys.exit(__doc__)
    print("# objdump45 symbols (-S %s): %s" % (sys.argv[1], what))
    print("# made by tools/symbols/gen_symbols.py; $addr NAME comment")
    for a, n, c in sorted(set(syms), key=lambda t: (t[0], t[1])):
        print(("$%04X %s %s" % (a, n, c)).rstrip())


if __name__ == "__main__":
    main()
