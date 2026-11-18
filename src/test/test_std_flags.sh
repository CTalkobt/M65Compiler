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

for t in cc45 cp45 cl45 ca45 al45 ln45 ar45 nm45 objdump45 basic45 disk45 asmCvt45 pack45 font45 tune45 deploy45 dbg45; do
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

echo "standard flag tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
