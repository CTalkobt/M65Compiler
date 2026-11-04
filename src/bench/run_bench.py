#!/usr/bin/env python3
"""run_bench.py — the 6502 C compiler benchmarks (c-bench-64, after gglabs'
bench8) built with cc45 and run in xemu, beside the published results of
cc65, llvm-mos, Oscar64, sdcc, vbcc and Calypsi (#129).

The suite's sources are not in this repository (they have no licence):
they are downloaded, at a pinned commit, into build/bench/. cc45's harness
(main45.c, out45.c here) replaces the suite's main.c, cia_timer.c and out.c.

  python3 src/bench/run_bench.py                 # all, at 1 MHz, -O2
  python3 src/bench/run_bench.py --speed 40 pi crc8
  python3 src/bench/run_bench.py --cflags "-O3" --json out.json

See doc/architecture/benchmarks.md.
"""
import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys
import tarfile
import urllib.request
import zlib

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
HERE = os.path.dirname(os.path.abspath(__file__))

SUITE_COMMIT = "17f7ca21ac5b3f6602c12fbddc4250302761dd1e"
SUITE_SHA256 = "7bab1925c935a8d387a2f4cb6e84a2bfad3315f9ddc2083289367c0c0be4c916"
SUITE_URL = "https://codeload.github.com/thred/c-bench-64/tar.gz/" + SUITE_COMMIT

# name in the results: the source file
BENCHMARKS = {
    "aes256": "aes256.c", "crc8": "crc8.c", "crc16": "crc16.c", "crc32": "crc32.c",
    "dhrystone": "dhrystone.c", "fact": "fact.c", "pi": "pi.c", "pow": "pow.c",
    "puff2": "puff2.c", "sieve": "sieve.c", "sieve_bit": "sieve_bit.c",
}
# the ones that read the KERNAL at $E000 (the C64's in the published runs)
KERNAL_READERS = {"aes256", "crc8", "crc16", "crc32"}
COMPILERS = ["cc65", "llvm", "o64", "sdcc", "vbcc", "calypsi"]
CIA_HZ = 1000000.0   # CIA 2 timer A counts phi2: 1 MHz on the MEGA65

RESULT_ADDR = 0xC0C0
LOG_ADDR = 0xC100


def fail(msg):
    sys.stderr.write("run_bench: " + msg + "\n")
    sys.exit(1)


def fetch_suite(work):
    """The suite at the pinned commit, unpacked under work/; its directory."""
    top = os.path.join(work, "c-bench-64-" + SUITE_COMMIT)
    if os.path.isdir(os.path.join(top, "benchmarks", "src")):
        return top
    os.makedirs(work, exist_ok=True)
    tgz = os.path.join(work, "c-bench-64-" + SUITE_COMMIT[:12] + ".tar.gz")
    if not os.path.exists(tgz):
        print("Downloading c-bench-64 " + SUITE_COMMIT[:12] + " ...", flush=True)
        try:
            with urllib.request.urlopen(SUITE_URL, timeout=120) as r, open(tgz + ".part", "wb") as f:
                shutil.copyfileobj(r, f)
        except Exception as e:
            fail("could not download %s: %s" % (SUITE_URL, e))
        os.replace(tgz + ".part", tgz)
    h = hashlib.sha256(open(tgz, "rb").read()).hexdigest()
    if h != SUITE_SHA256:
        os.remove(tgz)
        fail("%s: sha256 %s, expected %s (removed; run again)" % (tgz, h, SUITE_SHA256))
    with tarfile.open(tgz) as t:
        for m in t.getmembers():
            p = os.path.normpath(m.name)
            if p.startswith("..") or os.path.isabs(p) or not (m.isfile() or m.isdir()):
                fail("unexpected member in the archive: " + m.name)
        t.extractall(work)
    return top


def find_rom(arg):
    cands = [arg] if arg else []
    cands += [os.environ.get("MEGA65_ROM", ""), "/rom/MEGA65.ROM",
              os.path.expanduser("~/.local/share/xemu-lgb/mega65/MEGA65.ROM")]
    for c in cands:
        if c and os.path.isfile(c):
            return c
    return None


# --- the expected results of the KERNAL readers, from the bytes at $E000 ---

def crc8(d):
    crc = 0
    for b in d:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1D) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


def crc16(d):
    crc = 0
    for b in d:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def crc32_cksum(d):
    crc = 0
    for b in d:
        crc ^= b << 24
        for _ in range(8):
            crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF if crc & 0x80000000 else (crc << 1) & 0xFFFFFFFF
    return crc ^ 0xFFFFFFFF


AES_KEY = "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"
AES_IV = "000102030405060708090a0b0c0d0e0f"


def aes_expected(d):
    """aes256.c: the 8 KB encrypted (AES-256-CBC), then its CRC-32/CKSUM; None without openssl"""
    if not shutil.which("openssl"):
        return None
    r = subprocess.run(["openssl", "enc", "-aes-256-cbc", "-nopad", "-K", AES_KEY, "-iv", AES_IV],
                       input=d, capture_output=True)
    if r.returncode != 0 or len(r.stdout) != len(d):
        return None
    return crc32_cksum(r.stdout)


def kernal_expected(name, kernal, ram):
    """(the text benchmark_check prints before the value, the value as it prints it);
    aes256 copies $E000 with memcpy, which is a DMA copy: the RAM there, not the ROM"""
    if name == "crc8":
        return "CRC8=", "%02X" % crc8(kernal)
    if name == "crc16":
        return "CRC16=", "%04X" % crc16(kernal)
    if name == "crc32":
        return "CRC32=", "%08X" % crc32_cksum(kernal)
    v = aes_expected(ram)
    return "CRC32=", (None if v is None else "%08X" % v)


def printed_value(log, prefix):
    i = log.find(prefix)
    if i < 0:
        return None
    j = i + len(prefix)
    k = j
    while k < len(log) and log[k] in "0123456789ABCDEF":
        k += 1
    return log[j:k]


def petscii(b):
    """the text the benchmark printed (cc45's strings are PETSCII) as ASCII"""
    out = []
    for c in b:
        if 0xC1 <= c <= 0xDA:
            out.append(chr(c - 0x80))          # upper case
        elif 0x41 <= c <= 0x5A:
            out.append(chr(c + 0x20))          # lower case
        elif c == 0x0D:
            out.append("\n")
        elif 0x20 <= c < 0x7F:
            out.append(chr(c))
        else:
            out.append("?")
    return "".join(out)


def run(cmd, cwd=None, timeout=None):
    return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout)


def build(name, src_dir, out_dir, args):
    """The program for one benchmark: (prg path, None) or (None, error text)"""
    cc45 = os.path.join(ROOT, "bin", "cc45")
    ln45 = os.path.join(ROOT, "bin", "ln45")
    lib = os.path.join(ROOT, "lib", "build", "c45_stack.lib" if "-fno-zpcall" in args.cflags.split() else "c45.lib")
    flags = args.cflags.split() + ["-fsigned-default", "-I", src_dir, "-DBENCH_SPEED=%d" % args.speed]
    objs = []
    for src in [os.path.join(src_dir, BENCHMARKS[name]), os.path.join(HERE, "main45.c"), os.path.join(HERE, "out45.c")]:
        obj = os.path.join(out_dir, "%s-%s.o45" % (name, os.path.splitext(os.path.basename(src))[0]))
        r = run([cc45] + flags + ["-c", src, "-o", obj])
        if r.returncode != 0:
            return None, (r.stdout + r.stderr).strip().splitlines()[-1:] or ["cc45 failed"]
        objs.append(obj)
    prg = os.path.join(out_dir, name + ".prg")
    r = run([ln45, "-basic", "-o", prg] + objs + [lib])
    if r.returncode != 0:
        return None, (r.stdout + r.stderr).strip().splitlines()[-1:] or ["ln45 failed"]
    return prg, None


def execute(prg, out_dir, xemu, timeout):
    """(check byte, ticks, printed text) or an error string"""
    mem = prg[:-4] + ".mem"
    if os.path.exists(mem):
        os.remove(mem)
    try:
        run([xemu, "-headless", "-besure", "-sleepless", "-prgmode", "65", "-prgexit",
             "-dumpmem", mem, "-prg", prg], cwd=out_dir, timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timed out after %d s" % timeout
    if not os.path.exists(mem):
        return "no memory dump"
    d = open(mem, "rb").read()
    res = d[RESULT_ADDR:RESULT_ADDR + 6]
    if len(res) < 6 or res[5] != 0xAA:
        return "did not finish (no result)"
    log = petscii(d[LOG_ADDR:LOG_ADDR + 0xC00].split(b"\0")[0])
    return res[0], struct.unpack("<I", res[1:5])[0], log, d[0xE000:0x10000]


def reference(suite):
    """{compiler: {config key: (optimization, {bench: (size, time, status)})}}"""
    out = {}
    for c in COMPILERS:
        p = os.path.join(suite, "report", "public", c + ".json")
        if not os.path.exists(p):
            continue
        d = json.load(open(p))
        for cfg in d.get("configs", []):
            res = d.get("results", {}).get(cfg["key"], {})
            out.setdefault(c, {})[cfg["key"]] = (cfg.get("optimization"),
                {k: (v.get("size"), v.get("time"), v.get("status")) for k, v in res.items()})
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("benchmarks", nargs="*", help="some of: " + " ".join(BENCHMARKS))
    ap.add_argument("--speed", type=int, choices=(1, 3, 40), default=1,
                    help="CPU speed in MHz while the benchmark runs (1: as the published results; default)")
    ap.add_argument("--cflags", default="-O2", help="cc45 options (default -O2)")
    ap.add_argument("--xemu", default=os.environ.get("XEMU", "xemu-xmega65"), help="the xemu binary")
    ap.add_argument("--rom", help="the MEGA65 ROM xemu uses (for the KERNAL readers' expected values)")
    ap.add_argument("--timeout", type=int, default=900, help="seconds per run in xemu (default 900)")
    ap.add_argument("--work", default=os.path.join(ROOT, "build", "bench"), help="download and build directory")
    ap.add_argument("--json", help="write the results here as JSON")
    ap.add_argument("--fetch-only", action="store_true", help="download the suite and stop")
    args = ap.parse_args()

    for b in args.benchmarks:
        if b not in BENCHMARKS:
            fail("unknown benchmark '%s' (%s)" % (b, " ".join(BENCHMARKS)))
    names = args.benchmarks or list(BENCHMARKS)
    suite = fetch_suite(args.work)
    if args.fetch_only:
        print(suite)
        return 0
    if not shutil.which(args.xemu):
        fail("%s not found (--xemu, XEMU)" % args.xemu)
    for t in ("cc45", "ln45"):
        if not os.path.exists(os.path.join(ROOT, "bin", t)):
            fail("bin/%s missing: run make" % t)
    src_dir = os.path.join(suite, "benchmarks", "src")
    out_dir = os.path.join(args.work, "out-%dmhz" % args.speed)
    os.makedirs(out_dir, exist_ok=True)

    rom = find_rom(args.rom)
    kernal = None
    if rom:
        data = open(rom, "rb").read()
        if len(data) >= 0x20000:
            kernal = data[0x1E000:0x20000]   # $3E000: the C65 KERNAL, at $E000 in C65 mode
    ref = reference(suite)

    results = {}
    for name in names:
        print("%-10s " % name, end="", flush=True)
        prg, err = build(name, src_dir, out_dir, args)
        if err:
            results[name] = {"status": "build failed", "error": err[0]}
            print("build failed: " + err[0])
            continue
        size = os.path.getsize(prg)
        r = execute(prg, out_dir, args.xemu, args.timeout)
        if isinstance(r, str):
            results[name] = {"status": "run failed", "error": r, "size": size}
            print("%5d bytes  %s" % (size, r))
            continue
        check, ticks, log, ram = r
        # benchmark_check()'s result; dhrystone's always 1: its asserts print [OK] / [FAIL]
        if "[FAIL]" in log:
            status = "fail"
        elif check == 0 or (name == "dhrystone" and "[OK]" in log):
            status = "pass"
        else:
            status = "fail"
        note = ""
        if name in KERNAL_READERS:
            # the published values are of the C64's KERNAL; here the result is
            # checked against the same computation on the bytes at $E000
            if kernal is None:
                status, note = "unchecked", "no MEGA65 ROM (--rom): the C64 KERNAL's value expected"
            else:
                prefix, want = kernal_expected(name, kernal, ram)
                got = printed_value(log, prefix)
                src = "the RAM at $E000" if name == "aes256" else "the C65 KERNAL"
                if want is None:
                    status, note = "unchecked", "openssl needed for the expected value"
                elif got == want:
                    status, note = "pass", "%s%s of %s" % (prefix, want, src)
                else:
                    status, note = "fail", "%s%s, expected %s (of %s)" % (prefix, got, want, src)
        secs = ticks / CIA_HZ
        results[name] = {"status": status, "size": size, "ticks": ticks, "time": round(secs, 3),
                         "output": log.strip(), "note": note}
        print("%5d bytes  %9.3f s  %s%s" % (size, secs, status, ("  (" + note + ")") if note else ""))

    # the comparison: cc45 beside each compiler's performance build
    print()
    print("At %d MHz, cc45 %s; the others at the C64's 1 MHz (c-bench-64's results)." % (args.speed, args.cflags))
    cols = [(c, k) for c in COMPILERS for k, (opt, _) in ref.get(c, {}).items() if opt == "performance"]
    head = "%-10s %16s" % ("benchmark", "cc45") + "".join("%16s" % k for _, k in cols)
    print(head)
    print("%-10s %16s" % ("", "bytes / s") + "".join("%16s" % "bytes / s" for _ in cols))
    for name in names:
        r = results[name]
        mine = "%d / %.1f" % (r["size"], r["time"]) if "time" in r else r["status"]
        if r.get("status") == "fail":
            mine += "!"
        row = "%-10s %16s" % (name, mine)
        for c, k in cols:
            s, t, st = ref[c][k][1].get(name, (None, None, None))
            cell = "-" if s is None else ("%d / %s" % (s, "-" if t is None else ("%.1f" % t)))
            if st and st not in ("pass", "unknown"):
                cell += "!"
            row += "%16s" % cell
        print(row)
    print("(!: the result was wrong)")

    if args.json:
        with open(args.json, "w") as f:
            json.dump({"speed": args.speed, "cflags": args.cflags, "suite": SUITE_COMMIT,
                       "results": results}, f, indent=1)
    return 1 if any(r["status"] in ("fail", "build failed", "run failed") for r in results.values()) else 0


if __name__ == "__main__":
    sys.exit(main())
