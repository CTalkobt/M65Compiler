#!/bin/bash
# crt0's start-up code, checked in the built objects (lib/build/crt0*.o45; a
# directory as $1 checks that one's instead):
#  1. the knock that enables the MEGA65's I/O ($47 then $53 to $D02F) runs with
#     interrupts masked -- php, sei, the two writes, plp. An IRQ between the
#     writes found $D611 reading $FF and the KERNAL's keyboard scan turned its
#     Ctrl-S pause ($1119) on: the next PRINT waited for ever (#667). The
#     window is two instructions, too narrow for a runtime test to hit.

DIR=${1:-lib/build}
OBJDUMP=./bin/objdump45

passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

n=0
for o in "$DIR"/crt0*.o45; do
    [ -f "$o" ] || continue
    name=$(basename "$o")
    # the instructions as "mnemonic operand", one a line (a register objdump45
    # names, "sta VIC4_KEY ; $d02f", by its address)
    ops=$($OBJDUMP -d "$o" 2>/dev/null | awk -F'\t' '/^ *[0-9a-f]+:\t/ {
            op = $4; if ($5 ~ /^; \$[0-9a-fA-F]+$/) op = substr($5, 3)
            gsub(/[ \t]+$/, "", op); s = $3; if (op != "") s = s " " op; print tolower(s) }')
    # files without the knock (crt0_mega65*: crt0 did it) have nothing to check
    grep -q '^sta \$d02f' <<< "$ops" || continue
    n=$((n + 1))
    seq=$(grep -B3 -A1 -m2 '^sta \$d02f' <<< "$ops" | grep -v '^--' | tr '\n' ';')
    if [ "$seq" = 'php;sei;lda #$47;sta $d02f;lda #$53;sta $d02f;plp;' ]; then
        pass "$name: the \$D02F knock with interrupts masked"
    else
        fail "$name: the \$D02F knock is not php; sei; lda #\$47; sta \$d02f; lda #\$53; sta \$d02f; plp (got: $seq)"
    fi
done
[ $n -ge 2 ] || fail "no crt0 object with the \$D02F knock in $DIR (crt0.o45 and crt0_zp.o45 have it)"

echo "crt0 tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
