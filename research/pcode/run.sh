#!/usr/bin/env bash
# research/pcode: build the three xemu tests and run them
#   test_pcode_xemu.c  the interpreters (v1, v2) on hand-written p-code
#   test_jit_xemu.c    the translators (absolute, base page) beside the
#                      interpreter and native code, timed
#   test_far_xemu.c    sum from bank 0, bank 4 and attic RAM, timed
# Each prints its $C0C0 results (01 = pass) and, for the timed ones, the
# CIA ticks at $C100 (words; 1 tick = 40 cycles at 40 MHz).
#   run.sh [OUTDIR]     (default /tmp/pcode)
cd "$(dirname "$0")" || exit 1
B=../../bin
O=${1:-/tmp/pcode}
mkdir -p "$O"
LIB=../../lib/build/c45.lib
for s in pcode_vm pcode_vm_v1 pcode_vmf pcode_jit pcode_jit_abs pcode_jitf pcode_progs; do
    $B/ca45 -c $s.s45 -o "$O/$s.o45" >/dev/null || exit 1
done
run() {   # name, objects...
    local t=$1; shift
    $B/cc45 -O2 -c $t.c -o "$O/$t.o45" 2>/dev/null &&
    $B/ln45 -basic -M "$O/$t.map" -o "$O/$t.prg" "$O/$t.o45" "$@" $LIB >/dev/null || { echo "$t: build failed"; return; }
    timeout 20 xemu-xmega65 -headless -besure -sleepless -prgmode 65 -prgexit -dumpmem "$O/$t.mem" -prg "$O/$t.prg" </dev/null >/dev/null 2>&1
    echo "== $t"
    xxd -s 0xC0C0 -l 16 "$O/$t.mem"
    xxd -s 0xC100 -l 24 "$O/$t.mem"
}
run test_pcode_xemu "$O/pcode_vm.o45" "$O/pcode_vm_v1.o45" "$O/pcode_progs.o45"
run test_jit_xemu "$O/pcode_jit.o45" "$O/pcode_jit_abs.o45" "$O/pcode_vm.o45" "$O/pcode_progs.o45"
run test_far_xemu "$O/pcode_vmf.o45" "$O/pcode_jitf.o45" "$O/pcode_vm.o45" "$O/pcode_progs.o45"
# the interpreters' disassembly with cycles (tools/cycles.py walks it)
$B/objdump45 -d --cycles "$O/pcode_vm_v1.o45" > "$O/vm1.dis"
$B/objdump45 -d --cycles "$O/pcode_vm.o45" > "$O/vm2.dis"
