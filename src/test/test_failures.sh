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

section "Shadow execution tests (recursion)"
SHADOW_ONLY="recursion fibonacci" bash src/test/test_shadow_ir.sh | grep -E "^(PASS|FAIL|SKIP)|tests:"
result ${PIPESTATUS[0]} "shadow"

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
