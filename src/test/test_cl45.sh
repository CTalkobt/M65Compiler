#!/bin/bash
# test_cl45.sh — cl45, the C linter (#199; doc/architecture/lint.md): each
# fixture of src/test-resources/cl45 reports exactly the rules its
# "// EXPECT <rule>" comments name, on their lines; the library's C lints
# clean; suppression, the configuration file, the command line, the output
# formats, exit codes; cc45 compiles the fixtures as before
cd "$(dirname "$0")/../.."
CL=./bin/cl45
F=src/test-resources/cl45
D=build/test/cl45
mkdir -p $D
# (a personal ~/.config/m65/cc45-lint.yml or cc45.conf must not change the results)
export HOME=$PWD/$D/home
mkdir -p $HOME
passed=0; failed=0
pass() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

for f in $F/*.c; do
    n=$(basename $f .c)
    opts=""
    case $n in io_personality|io_knock) opts="--profile=standalone" ;; esac
    # (a line can expect several findings)
    want=$(grep -n "EXPECT mega65-" $f | awk -F: '{ l = $1; s = $0
        while (match(s, /EXPECT mega65-[a-z0-9-]+/)) { print l, substr(s, RSTART + 7, RLENGTH - 7); s = substr(s, RSTART + RLENGTH) } }' | sort)
    got=$($CL -O2 $opts $f 2>$D/$n.err | sed -nE 's/^[^:]+:([0-9]+):[0-9]+: [a-z]+: .* \[(mega65-[a-z0-9-]+)\]$/\1 \2/p' | sort)
    if [ "$want" = "$got" ]; then
        pass "cl45 $n ($(echo "$want" | grep -c mega65) findings)"
    else
        fail "cl45 $n: want [$(echo $want)] got [$(echo $got)] $(head -3 $D/$n.err)"
    fi
done

# the library's C, with the options lib/Makefile compiles it with, lints clean
# (make lint-lib: the same, and its assembly with al45)
if make -s lint-lib > $D/lib.txt 2>&1 && [ ! -s $D/lib.txt ]; then
    pass "cl45: the library lints clean (make lint-lib)"
else
    fail "cl45 library: $(head -5 $D/lib.txt)"
fi

# a finding in a header of the project is reported, one in a system header not
C=$D/hdr
mkdir -p $C
printf 'static void poke_border(unsigned char v) { *(char *)0xD020 = v; }\n' > $C/mine.h
printf '#include "mine.h"\n#include <mega65.h>\nvoid f(void) { poke_border(1); vic4->border = 2; }\n' > $C/a.c
$CL -O2 $C/a.c > $D/hdr.txt 2>&1
if [ "$(grep -c mega65- $D/hdr.txt)" = 1 ] && grep -q "mine.h:1:[0-9]*: warning: .*\[mega65-io-not-volatile\]" $D/hdr.txt; then
    pass "cl45: a project header's finding is reported at its line; system headers are not"
else
    fail "cl45 headers: $(head -3 $D/hdr.txt)"
fi

# the same with the project's header in a -I directory (the last include
# path is not the system headers')
rm -rf $C/inc
mkdir -p $C/inc
cp $C/mine.h $C/inc/theirs.h
printf '#include <theirs.h>\n#include <mega65.h>\nvoid f(void) { poke_border(1); vic4->border = 2; }\n' > $C/c.c
$CL -O2 -I$C/inc $C/c.c > $D/hdr3.txt 2>&1
if [ "$(grep -c mega65- $D/hdr3.txt)" = 1 ] && grep -q "inc/theirs.h:1:[0-9]*: warning: .*\[mega65-io-not-volatile\]" $D/hdr3.txt; then
    pass "cl45: a project header in a -I directory is reported; system headers are not"
else
    fail "cl45 -I headers: $(head -3 $D/hdr3.txt)"
fi

# ... nor one in a copy of the system headers that CC45_INCLUDE names (the
# Docker image's /usr/local/include/cc45, #630); another CC45_INCLUDE
# directory is the project's
rm -rf $C/sys
cp -r lib/include $C/sys
cp $C/mine.h $C/inc/theirs.h
printf '#include <theirs.h>\n#include <mega65.h>\nvoid f(void) { poke_border(1); vic4->border = 2; }\n' > $C/b.c
CC45_INCLUDE=$C/sys:$C/inc $CL -O2 $C/b.c > $D/hdr2.txt 2>&1
if [ "$(grep -c mega65- $D/hdr2.txt)" = 1 ] && grep -q "inc/theirs.h:1:[0-9]*: warning: .*\[mega65-io-not-volatile\]" $D/hdr2.txt; then
    pass "cl45: CC45_INCLUDE: a copy of the system headers is not reported, another directory is"
else
    fail "cl45 CC45_INCLUDE headers: $(head -3 $D/hdr2.txt)"
fi

# configuration: a project .cc45-lint.yml (found from the source's directory):
# c_checks.enabled, a rule's option, a severity; then the command line
C=$D/cfg
mkdir -p $C/sub
cp $F/frames.c $C/sub/a.c
printf 'c_checks:\n  enabled: "-mega65-recursion-frame"\n  options:\n    mega65-stack-frame-limit: 32\noverrides:\n  mega65-stack-frame-limit: error\n' > $C/.cc45-lint.yml
$CL -O2 $C/sub/a.c > $D/cfg1.txt 2>&1; rc1=$?
$CL -O2 --disable=mega65-stack-frame-limit $C/sub/a.c > $D/cfg2.txt 2>&1; rc2=$?
$CL -O2 --config=/dev/null --checks='-*,mega65-recursion-frame' --severity=mega65-recursion-frame=error $C/sub/a.c > $D/cfg3.txt 2>&1; rc3=$?
rm -f $C/.cc45-lint.yml
if [ $rc1 = 1 ] && [ "$(grep -c ": error: .*\[mega65-stack-frame-limit\]" $D/cfg1.txt)" = 2 ] &&
   ! grep -q recursion-frame $D/cfg1.txt && [ $rc2 = 0 ] && [ ! -s $D/cfg2.txt ] &&
   [ $rc3 = 1 ] && [ "$(grep -c ": error: .*\[mega65-recursion-frame\]" $D/cfg3.txt)" = 3 ] &&
   ! grep -q stack-frame-limit $D/cfg3.txt; then
    pass "cl45: .cc45-lint.yml (enabled, an option, a severity), --disable, --checks, --severity; an error fails the run"
else
    fail "cl45 configuration: rc $rc1 $rc2 $rc3: $(cat $D/cfg1.txt $D/cfg2.txt $D/cfg3.txt | head -5)"
fi

# output formats; --list-rules, --explain; the -v level does not change the output
$CL -O2 --format=json $F/dma_order.c > $D/j.json 2>&1
$CL -O2 --format=sarif $F/dma_order.c > $D/s.json 2>&1
$CL -O2 -vv $F/dma_order.c > $D/v.txt 2>/dev/null
$CL -O2 $F/dma_order.c > $D/t.txt 2>/dev/null
if python3 -c "
import json,sys
j=json.load(open('$D/j.json')); s=json.load(open('$D/s.json'))
assert j[0]['rule']=='mega65-dma-trigger-order' and j[0]['line']==18 and len(j)==3
r=s['runs'][0]
assert s['version']=='2.1.0' and r['results'][0]['ruleId']=='mega65-dma-trigger-order'
assert r['tool']['driver']['name']=='cl45'
assert any(x['id']=='mega65-signedness' for x in r['tool']['driver']['rules'])
" 2>/dev/null && cmp -s $D/v.txt $D/t.txt && $CL --list-rules | grep -q "^mega65-signedness " &&
   $CL --explain=mega65-sac-interrupt | grep -q "ln45" && ! $CL --explain=mega65-nope >/dev/null 2>&1 &&
   $CL --help | grep -q -- "--checks"; then
    pass "cl45: JSON and SARIF output, --list-rules, --explain, --help, -v leaves the output alone"
else
    fail "cl45 output formats"
fi

# the level of optimization is the file's as compiled: at -O0 nothing is
# inlined, so a small static function an interrupt handler calls is reached
printf 'volatile unsigned char t;\nstatic unsigned char two(unsigned char v) { return v + v; }\n__interrupt void h(void) { t = two(t); }\n' > $D/inl.c
$CL -O0 $D/inl.c > $D/inl0.txt 2>&1
$CL -O2 $D/inl.c > $D/inl2.txt 2>&1
if grep -q "inl.c:2:.*\[mega65-sac-interrupt\]" $D/inl0.txt && [ ! -s $D/inl2.txt ]; then
    pass "cl45: lints the file as its -O level compiles it (an inlined function is not called)"
else
    fail "cl45 -O: $(head -2 $D/inl0.txt) / $(head -2 $D/inl2.txt)"
fi

# cc45 takes no notice of #pragma cc45 lint and NOLINT, and keeps its own
# warnings (the ones a rule covers: on stderr, as before)
./bin/cc45 -O2 -c $F/suppress.c -o $D/suppress.o45 > $D/cc1.txt 2>&1; rc1=$?
./bin/cc45 -O2 -c $F/signedness.c -o $D/signedness.o45 > $D/cc2.txt 2>&1; rc2=$?
if [ $rc1 = 0 ] && [ ! -s $D/cc1.txt ] && [ $rc2 = 0 ] && grep -q "signedness.c:10:.*warning: 'x' is unsigned" $D/cc2.txt &&
   ! grep -q "\[mega65-" $D/cc2.txt && [ ! -e src/test-resources/cl45/suppress.o45 ]; then
    pass "cc45 compiles the fixtures as before (its own warnings, no rule ids; cl45 writes no file)"
else
    fail "cc45 on the fixtures: rc $rc1 $rc2: $(head -3 $D/cc1.txt $D/cc2.txt)"
fi

echo ""
echo "cl45 tests: $passed passed, $failed failed"
[ $failed = 0 ]
