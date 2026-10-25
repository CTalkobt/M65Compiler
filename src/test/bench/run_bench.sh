#!/bin/bash
# run_bench.sh [cc45 flags...] — build bench_xemu.c, run it on xemu, print
# each benchmark's time in ms and checksum (both conventions unless flags
# are given). Not part of make test: a tool for comparing code generation.
#
#   bash src/test/bench/run_bench.sh            # ZP and stack conventions
#   bash src/test/bench/run_bench.sh -O1        # with other flags
#
# Expected checksums: sieve 0x404 (1028 primes), fib 0xa18, bubble 0xa255,
# strings 0x3a90, sprintf 0x24b9, arith 0x6aad, life 0x132e, rows 0x433.

cd "$(dirname "$0")/../../.."
OUT=build/bench
mkdir -p "$OUT"
NAMES="sieve fib bubble strings sprintf arith life rows"
EXPECT="0x404 0xa18 0xa255 0x3a90 0x24b9 0x6aad 0x132e 0x433"

run() {
    local tag=$1; shift
    if ! ./bin/cc45 -O2 "$@" src/test/bench/bench_xemu.c -o "$OUT/bench_$tag.prg" > "$OUT/build_$tag.log" 2>&1; then
        echo "$tag: build failed"; tail -3 "$OUT/build_$tag.log"; return 1
    fi
    rm -f "$OUT/bench_$tag.mem"
    timeout 60 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit \
        -dumpmem "$OUT/bench_$tag.mem" -prg "$OUT/bench_$tag.prg" >/dev/null 2>&1
    python3 - "$OUT/bench_$tag.mem" "$tag" "$(stat -c %s "$OUT/bench_$tag.prg")" "$NAMES" "$EXPECT" <<'PY'
import sys
m = open(sys.argv[1], 'rb').read()
tag, size, names, expect = sys.argv[2], sys.argv[3], sys.argv[4].split(), sys.argv[5].split()
n = len(names)
st = [int.from_bytes(m[0xC100 + 4 * i:0xC104 + 4 * i], 'little') for i in range(n + 1)]
ms = [(st[i] - st[i + 1]) // 1000 for i in range(n)]
ck = [hex(m[0xC0E0 + 2 * i] | m[0xC0E1 + 2 * i] << 8) for i in range(n)]
bad = [n for n, c, e in zip(names, ck, expect) if c != e]
print(f"{tag:8} {size:>6} bytes  " + "  ".join(f"{n} {t}" for n, t in zip(names, ms)) +
      f"  total {sum(ms)} ms" + ("" if m[0xC0CC] == 0xAA else "  (did not finish)") +
      (f"  WRONG: {' '.join(bad)}" if bad else ""))
PY
}

if [ $# -gt 0 ]; then
    run custom "$@"
else
    run zp
    run stack -fno-zpcall
fi
