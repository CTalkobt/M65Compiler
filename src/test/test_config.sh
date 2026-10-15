#!/bin/bash
# test_config.sh — configuration files and environment variables do not
# disturb options that take a value (-o FILE, -L FILE, -l N, --source-map):
# ca45 with CA45_OPTIMIZE / CA45_WARNINGS and a .ca45rc, cc45 and cp45 with
# their ~/.config/m65 files (a temporary HOME)
cd "$(dirname "$0")/../.." || exit 1
ROOT=$PWD
T=build/test/config
rm -rf $T; mkdir -p $T/h/.config/m65
passed=0; failed=0
check() { if eval "$2"; then passed=$((passed + 1)); else echo "  FAIL: $1"; failed=$((failed + 1)); fi; }
printf '.org $2000\n    lda #1\n    rts\n' > $T/c.s45
printf 'int main(void) { return 0; }\n' > $T/m.c

(cd $T && CA45_OPTIMIZE=2 $ROOT/bin/ca45 -c c.s45 -o c1.o45 >/dev/null 2>&1)
check "CA45_OPTIMIZE with -o" "[ -s $T/c1.o45 ]"
(cd $T && CA45_OPTIMIZE=1 CA45_WARNINGS=overflow,underflow $ROOT/bin/ca45 -L c2.lst -l 2 -c c.s45 -o c2.o45 >/dev/null 2>&1)
check "CA45_* with -L, -l, -o" "[ -s $T/c2.o45 ] && [ -s $T/c2.lst ]"
printf -- '-O1\n-Woverflow\n' > $T/.ca45rc
(cd $T && $ROOT/bin/ca45 -c c.s45 -o c3.o45 -L c3.lst --source-map c3.map >/dev/null 2>&1)
check ".ca45rc with -o, -L, --source-map" "[ -s $T/c3.o45 ] && [ -s $T/c3.lst ] && [ -s $T/c3.map ]"
rm -f $T/.ca45rc
printf -- '-O1\n-fsigned-default\n' > $T/h/.config/m65/cc45.conf
printf -- '-DFOO=1\n' > $T/h/.config/m65/cp45.conf
HOME=$ROOT/$T/h ./bin/cc45 -c $T/m.c -o $T/m.o45 >/dev/null 2>&1
check "cc45.conf with -c -o" "[ -s $T/m.o45 ]"
HOME=$ROOT/$T/h ./bin/cc45 $T/m.c -o $T/m.prg >/dev/null 2>&1
check "cc45.conf with a program" "[ -s $T/m.prg ]"
HOME=$ROOT/$T/h ./bin/cp45 $T/m.c -o $T/m.i >/dev/null 2>&1
check "cp45.conf with -o" "[ -s $T/m.i ]"

echo "Configuration tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
