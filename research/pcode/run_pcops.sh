#!/usr/bin/env bash
# research/pcode: test_pcops_xemu.c native and as p-code (CC45_PCODE=all),
# both run in xemu (the p-code's frames overlaid by ln45, its SAC overlay); their results at $A000 compared, the first group that
# differs named
#   run_pcops.sh [OUTDIR]
#   FAR=2048 run_pcops.sh [OUTDIR]   the p-code in far memory (here attic RAM,
#       bank $800; 5: bank 5): the runtime built with -DPCODE_FAR, cc45 with
#       CC45_PCODE_FAR, the p-code in the program's overlay file, on a disk
cd "$(dirname "$0")" || exit 1
B=../../bin
O=${1:-/tmp/pcops}
mkdir -p "$O"
RTF=; STORE=; [ -n "$FAR" ] && { RTF=-DPCODE_FAR; STORE="--overlay-store=$(( FAR << 16 ))"; }
$B/ca45 $RTF -c pcode_rt.s45 -o "$O/rt.o45" >/dev/null || exit 1
$B/cc45 -O2 -c test_pcops_xemu.c -o "$O/n.o45" 2>/dev/null &&
$B/ln45 -basic -o "$O/n.prg" "$O/n.o45" ../../lib/build/c45.lib >/dev/null || { echo "native build failed"; exit 1; }
rm -f "$O/p.prg" "$O/p.ov1"
CC45_PCODE_FAR=$FAR CC45_PCODE=all CC45_PCODE_REPORT=1 $B/cc45 -O2 -c test_pcops_xemu.c -o "$O/p.o45" 2>"$O/p.rep" &&
$B/ln45 -v -basic $STORE -o "$O/p.prg" "$O/p.o45" "$O/rt.o45" ../../lib/build/c45.lib 2>&1 | grep -E "SAC overlay:|error|Code overlays" ; [ -s "$O/p.prg" ] || { echo "p-code build failed"; exit 1; }
grep native "$O/p.rep"
DISK=
if [ -n "$FAR" ]; then
    rm -f "$O/p.d81"
    $B/disk45 create "$O/p.d81" -n "PCOPS" -i "PC" >/dev/null && $B/disk45 add "$O/p.d81" "$O/p.prg" "p" >/dev/null &&
    $B/disk45 add "$O/p.d81" "$O/p.ov1" "p.ov1" >/dev/null || { echo "disk failed"; exit 1; }
    DISK="-8 $O/p.d81"
fi
timeout 60 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$O/n.mem" -prg "$O/n.prg" </dev/null >/dev/null 2>&1
timeout 60 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$O/p.mem" $DISK -prg "$O/p.prg" </dev/null >/dev/null 2>&1
python3 - "$O/n.mem" "$O/p.mem" <<'PY'
import sys
n, p = open(sys.argv[1], "rb").read(), open(sys.argv[2], "rb").read()
print("ended: native", hex(n[0xC0C0]), "p-code", hex(p[0xC0C0]))
# groups: EE g, then a 2-byte checksum per call
k, g, call, bad = 0xA000, None, 0, 0
end = n.index(bytes([0xEE, 8]), 0xA000) + 2
while k < end:
    if n[k] == 0xEE and n[k + 1] in range(1, 9) and (k == 0xA000 or (k - 0xA000) % 2 == 0):
        g, call, k = n[k + 1], 0, k + 2
        continue
    if n[k:k + 2] != p[k:k + 2]:
        if bad < 12: print(f"group {g}, call {call} (${k:04X}): native {n[k:k+2].hex()}, p-code {p[k:k+2].hex()}")
        bad += 1
    call += 1
    k += 2
print(f"{bad} calls differ" if bad else f"the same: {end - 0xA000} bytes of results")
sys.exit(1 if bad else 0)
PY
