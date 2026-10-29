#!/bin/bash
# bench_tgi.sh — <tgi.h>'s speed in xemu: frames for each workload of
# src/test-resources/benchmark_tgi.c and a checksum of what it drew (equal
# checksums: the same pixels). Not part of make test.
#
# Usage: bash src/test/bench_tgi.sh ["-fno-zpcall -DTGI_DRIVER=tgi_static_ncm640"] [> results.txt]
#   (the lib must be built: make lib)
set -e
cd "$(dirname "$0")/../.."
flags="${1:-}"
out=build/test/bench_tgi
mkdir -p "$out"
./bin/cc45 -O2 $flags src/test-resources/benchmark_tgi.c -o "$out/bench.prg" >/dev/null
rm -f "$out/bench.mem"
timeout 120 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit \
    -dumpmem "$out/bench.mem" -prg "$out/bench.prg" </dev/null >/dev/null 2>&1 || true
python3 - "$out/bench.mem" <<'EOF'
import sys
names = ["setpixel (128000)", "getpixel (128000)", "lines, long (4160)",
         "lines, short (12000)", "circles (240)", "text 1x (4000 chars)",
         "text 2x (1920 chars)", "bars (8000)"]
d = open(sys.argv[1], "rb").read()
r = d[0xC0C0:0xC0C0 + 4 * len(names) + 1]
if r[-1] != 0xAA:
    sys.exit("benchmark did not finish (no $AA sentinel)")
print("%-22s %7s  %s" % ("workload", "frames", "checksum"))
for i, n in enumerate(names):
    f = r[4 * i] | r[4 * i + 1] << 8
    s = r[4 * i + 2] | r[4 * i + 3] << 8
    print("%-22s %7d  %04X" % (n, f, s))
EOF
