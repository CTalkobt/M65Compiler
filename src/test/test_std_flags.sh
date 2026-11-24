#!/bin/bash
# test_std_flags.sh — the flags every tool answers the same way (#641,
# include/Version.hpp): -? and --help print the usage, -V and --version print
# "<tool> v<version> (<commit>)", each with exit status 0, wherever the flag
# is among the arguments; -h is help too, but in nm45 and objdump45 (headers)
cd "$(dirname "$0")/../.."
passed=0; failed=0
pass() { passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }
ver=$(sed -n 's/^#define SUITE_VERSION "\(.*\)"/\1/p' include/Version.hpp)

for t in cc45 cp45 cl45 ca45 al45 af45 ln45 ar45 nm45 objdump45 basic45 disk45 asmCvt45 pack45 font45 tune45 deploy45 dbg45; do
    for f in '-?' --help -h; do
        case "$t$f" in nm45-h|objdump45-h) continue ;; esac
        out=$(./bin/$t "$f" 2>&1 </dev/null); rc=$?
        if [ $rc = 0 ] && echo "$out" | head -3 | grep -qi "usage\|$t"; then pass; else fail "$t $f: exit $rc, $(echo "$out" | head -1)"; fi
    done
    for f in -V --version; do
        out=$(./bin/$t "$f" 2>&1 </dev/null); rc=$?
        if [ $rc = 0 ] && echo "$out" | grep -q "^$t v$ver ([0-9a-f]*\(unknown\)*)\$"; then pass; else fail "$t $f: exit $rc, $out"; fi
    done
done

# anywhere among the arguments, not only first
./bin/ar45 t nothing.lib --version 2>&1 | grep -q "^ar45 v" && pass || fail "ar45 t x.lib --version"
./bin/deploy45 m65ether --help 2>&1 | grep -q "^Usage: deploy45" && pass || fail "deploy45 m65ether --help"
./bin/disk45 list nothing.d81 --help 2>&1 | grep -qi "usage" && pass || fail "disk45 list x.d81 --help"
./bin/cc45 -c nothing.c -V 2>&1 | grep -q "^cc45 v" && pass || fail "cc45 -c x.c -V"
# al45 has a help of its own; disk45 sd keeps its own
./bin/al45 --help | grep -q -- "--suggest-clobbers" && pass || fail "al45 --help lists the lint options"
./bin/disk45 sd --help 2>&1 | grep -q "Usage: disk45 sd" && pass || fail "disk45 sd --help"
# nm45 -h and objdump45 -h are still their header options
./bin/nm45 -h nothing.o45 2>&1 | grep -qi "usage" && fail "nm45 -h prints the usage" || pass
./bin/objdump45 -h nothing.o45 2>&1 | grep -qi "^usage" && fail "objdump45 -h prints the usage" || pass

# each tool's help lists the options its parser takes (#638)
helphas() { # tool text...: the tool's --help has each text
    local t=$1 h; shift; h=$(./bin/$t --help 2>&1)
    for x in "$@"; do echo "$h" | grep -qF -- "$x" && pass || fail "$t --help lacks '$x'"; done
}
helphas ln45 "-V, --version" "-Woverride " "--section=NAME=ADDR" "--stack-size=N"
helphas nm45 "-A, -o" "-V, --version"
helphas ar45 "-V, --version"
helphas objdump45 "--entry ADDR" "--data A-B" "--html=FILE" "-V, --version"
# ln45: the --stack-size default under --stack-size, not after --zp-plan-out
./bin/ln45 --help | grep -A1 -- "--stack-size=N" | grep -q "default 4096" && pass || fail "ln45 --help: the --stack-size default is not under it"
./bin/ln45 --help | grep -A2 -- "--zp-plan-out" | grep -q "default 4096" && fail "ln45 --help: the --stack-size default after --zp-plan-out" || pass

# bad option values are errors (exit 1), not aborts (#722 cc45 --prg-base, #728 ca45 -D, #740 asmCvt45 default_threads)
SF=build/test/stdflags_bad; mkdir -p $SF/rc
printf 'int main(void){return 0;}\n' > $SF/h.c; printf '    .org $2000\n    rts\n' > $SF/f.s45
./bin/cc45 -O2 --prg-base zz $SF/h.c -o $SF/h.prg > $SF/o.txt 2>&1; rc=$?
[ $rc = 1 ] && grep -q "invalid --prg-base value 'zz'" $SF/o.txt && pass || fail "cc45 --prg-base zz: rc $rc (#722)"
./bin/cc45 -O2 --prg-base '$3000' $SF/h.c -o $SF/h.prg >/dev/null 2>&1 && [ "$(xxd -p -l 2 $SF/h.prg)" = 0030 ] && pass || fail "cc45 --prg-base \$3000 (#722)"
./bin/ca45 -DV=abc $SF/f.s45 -o $SF/f.bin > $SF/o.txt 2>&1; rc=$?
[ $rc = 1 ] && grep -q "ca45: error: -DV=abc: the value must be a number" $SF/o.txt && pass || fail "ca45 -DV=abc: rc $rc (#728)"
./bin/ca45 -DV='$1F' $SF/f.s45 -o $SF/f.bin >/dev/null 2>&1 && pass || fail "ca45 -DV=\$1F (#728)"
printf '[general]\ndefault_threads = x\n' > $SF/rc/.asmCvt45rc
(cd $SF/rc && ../../../../bin/asmCvt45 -t acme ../f.s45 -o out.a65 > o.txt 2>&1); rc=$?
[ $rc = 0 ] && grep -q "default_threads = 'x' is not a number of threads" $SF/rc/o.txt && pass || fail "asmCvt45 default_threads = x: rc $rc (#740)"

# #726: cp45 / cc45 -E without -o (or -o -) print to stdout; a program without -o is a.prg
SO=build/test/stdflags_out726; rm -rf $SO; mkdir -p $SO
printf '#define N 3\nint main(void){ return N - 3; }\n' > $SO/p.c
[ "$(./bin/cp45 $SO/p.c | grep main)" = "int main(void){ return 3 - 3; }" ] && pass || fail "cp45 without -o: stdout (#726)"
(cd $SO && ../../../bin/cc45 -O2 -E p.c >/dev/null 2>&1) && grep -q 'return 3 - 3' $SO/a.out && pass || fail "cc45 -E without -o: a.out (#726)"
[ "$(./bin/cc45 -O2 -E $SO/p.c -o - | grep main)" = "int main(void){ return 3 - 3; }" ] && pass || fail "cc45 -E -o -: stdout (#726)"
[ "$(./bin/cp45 $SO/p.c -o - | grep -c main)" = 1 ] && pass || fail "cp45 -o -: stdout (#726)"
(cd $SO && ../../../bin/cc45 -O2 p.c >/dev/null 2>&1) && [ -s $SO/a.prg ] && [ ! -e $SO/p.prg ] && pass || fail "cc45 p.c without -o: a.prg (#726)"
# #754: cc45 -S -o -: the assembly on stdout (no file named "-"); -c / a program to -o -: an error
(cd $SO && ../../../bin/cc45 -O2 -S p.c -o - > s.out 2>/dev/null) && grep -q 'proc _main' $SO/s.out && [ ! -e $SO/- ] && pass || fail "cc45 -S -o -: stdout (#754)"
(cd $SO && ../../../bin/cc45 -O2 -c p.c -o - > c.out 2>&1); rc=$?
[ $rc = 1 ] && grep -q 'only for -E and -S' $SO/c.out && [ ! -e $SO/- ] && pass || fail "cc45 -c -o -: rc $rc (#754)"
(cd $SO && ../../../bin/cc45 -O2 p.c -o - > c.out 2>&1); rc=$?
[ $rc = 1 ] && [ ! -e $SO/- ] && pass || fail "cc45 p.c -o -: rc $rc (#754)"
# #723: -D NAME (with a space) as -DNAME; -D without a name an error
printf 'int main(void){return FOO;}\n' > $SO/d.c
[ "$(./bin/cp45 $SO/d.c -D FOO=3 | grep main)" = "int main(void){return 3;}" ] && pass || fail "cp45 -D FOO=3 (#723)"
[ "$(./bin/cp45 -D FOO $SO/d.c | grep main)" = "int main(void){return 1;}" ] && pass || fail "cp45 -D FOO (#723)"
./bin/cc45 -O2 $SO/d.c -o $SO/d.prg -D >/dev/null 2>&1 && fail "cc45 -D without a name accepted (#723)" || pass
# #729: ca45: an unknown option, a second input file are errors
./bin/ca45 -bogus $SF/f.s45 -o $SF/f.bin > $SF/o.txt 2>&1; rc=$?
[ $rc = 1 ] && grep -q "ca45: error: unknown option '-bogus'" $SF/o.txt && pass || fail "ca45 -bogus: rc $rc (#729)"
./bin/ca45 $SF/f.s45 $SF/f.s45 -o $SF/f.bin > $SF/o.txt 2>&1; rc=$?
[ $rc = 1 ] && grep -q "more than one input file" $SF/o.txt && pass || fail "ca45 two inputs: rc $rc (#729)"
./bin/al45 -bogus $SF/f.s45 2>&1 | grep -q "al45: warning: unknown option '-bogus' ignored" && pass || fail "al45 -bogus still a warning (#729)"

# #731: ca45 --help=opt says what the levels do (1-3 the same passes)
./bin/ca45 --help=opt | grep -q '1, 2, 3 = every pass' && ! ./bin/ca45 --help=opt | grep -q 'aggressive' && pass || fail "ca45 --help=opt levels (#731)"

echo "standard flag tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
