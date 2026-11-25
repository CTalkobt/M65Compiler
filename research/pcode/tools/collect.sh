#!/usr/bin/env bash
# research/pcode: CC45_PCODE_STATS over the library and the examples (the
# library with lib/Makefile's flags), then the size tables:
#   tools/collect.sh [OUTDIR]     (default /tmp/pcode-stats)
cd "$(dirname "$0")/../../.." || exit 1
O=${1:-/tmp/pcode-stats}
mkdir -p "$O"
rm -f "$O/lib.stats" "$O/ex.stats"
for f in lib/stdlib/*.c; do
    CC45_PCODE_STATS="$O/lib.stats" ./bin/cc45 -O2 -fimplicit-function-declaration -fsigned-default -funsigned-char \
        -ffunction-sections -fzp-locals-max=8 -Ilib/include -c "$f" -o "$O/x.o45" 2>/dev/null
done
for f in examples/c/*/*.c; do
    CC45_PCODE_STATS="$O/ex.stats" ./bin/cc45 -O2 -I"$(dirname "$f")" -c "$f" -o "$O/x.o45" 2>/dev/null
done
python3 research/pcode/tools/stats_ab.py "$O/lib.stats" "$O/ex.stats"
python3 research/pcode/tools/stats_a.py "$O/lib.stats" "$O/ex.stats"
