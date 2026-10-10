#!/bin/bash
# test_failures.sh — TEMPORARY work list of known failing tests.
#
# Groups everything that still fails so it can be worked down. As items are
# fixed, remove them from here (and from known_failures/gte.txt); once the list
# is empty the underlying suites go into `make test` and this script goes away.

cd "$(dirname "$0")/../.." || exit 1

groups_failed=0
section() { echo ""; echo "=== $1 ==="; }
result() { if [ "$1" -eq 0 ]; then echo "--> $2: all passing (remove from test_failures.sh)"; else echo "--> $2: still failing"; groups_failed=$((groups_failed + 1)); fi; }

section "GTE (src/test/known_failures/gte.txt)"
bash src/test/run_gte_all.sh -l src/test/known_failures/gte.txt | tail -8
result ${PIPESTATUS[0]} "GTE"

section "Stack frames with -fno-staticalloc (int -> long widening)"
f=src/test-resources/test_stackframe_xemu.c
./bin/cc45 -c -fno-staticalloc $f -o build/test/stackframe_nosac.o45 >/dev/null 2>&1 &&
./bin/ln45 -basic -o build/test/stackframe_nosac.prg lib/build/crt0.o45 build/test/stackframe_nosac.o45 lib/build/c45.lib >/dev/null 2>&1
rm -f build/test/stackframe_nosac.mem
timeout 30 xemu-xmega65 -headless -besure -prgmode 65 -prgexit -dumpmem build/test/stackframe_nosac.mem -prg build/test/stackframe_nosac.prg </dev/null >/dev/null 2>&1
got=$(xxd -s $((16#C0B0)) -l 7 -p build/test/stackframe_nosac.mem 2>/dev/null)
echo "got $got, expected 157900561234aa"
[ "$got" = "157900561234aa" ]
result $? "stackframe -fno-staticalloc"

section "Calling convention interop"
bash src/test/test_cc_interop.sh | grep -E "PASS|FAIL|Expected|Summary"
result ${PIPESTATUS[0]} "cc_interop"

section "Library: unsigned < 0 comparisons (should be 0)"
make -C lib clean >/dev/null 2>&1
n=$(make -C lib 2>&1 | grep -c "unsigned expression < 0 is always false")
echo "warnings: $n"
result "$n" "library < 0"

echo ""
echo "test_failures: $groups_failed group(s) still failing"
[ $groups_failed -eq 0 ]
