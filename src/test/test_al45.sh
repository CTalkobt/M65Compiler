#!/bin/bash
# test_al45.sh — al45, the assembly linter (#64, #199; doc/architecture/lint.md):
# each fixture of src/test-resources/al45 reports exactly the rules its
# "; EXPECT <rule>" comments name, on their lines; the library lints clean;
# the configuration file, the command line, the output formats, exit codes
cd "$(dirname "$0")/../.."
AL=./bin/al45
F=src/test-resources/al45
D=build/test/al45
mkdir -p $D
# (a personal ~/.config/m65/cc45-lint.yml or ca45.conf must not change the results)
export HOME=$PWD/$D/home
mkdir -p $HOME
passed=0; failed=0
pass() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# (relocatable fixtures: the cc45 profile)
for f in $F/*.s45; do
    n=$(basename $f .s45)
    opts=""
    case $n in rom_call) opts="-c" ;; esac
    want=$(grep -n "EXPECT mega65-" $f | sed -E 's/^([0-9]+):.*EXPECT (mega65-[a-z0-9-]+).*/\1 \2/' | sort)
    got=$($AL $opts $f 2>$D/$n.err | sed -nE 's/^[^:]+:([0-9]+):[0-9]+: [a-z]+: .* \[(mega65-[a-z0-9-]+)\]$/\1 \2/p' | sort)
    if [ "$want" = "$got" ] && [ ! -s $D/$n.err ]; then
        pass "al45 $n ($(echo "$want" | grep -c mega65) findings)"
    else
        fail "al45 $n: want [$(echo $want)] got [$(echo $got)] $(head -3 $D/$n.err)"
    fi
done

# the library (its assembly as lib/Makefile builds it, -c) lints clean
: > $D/lib.txt
for f in lib/stdlib_asm/*.s45 lib/crt0*.s45; do
    $AL -c -Ilib/stdlib_asm -Ilib/include $f >> $D/lib.txt 2>&1
done
if [ ! -s $D/lib.txt ]; then
    pass "al45: the library's assembly lints clean"
else
    fail "al45 library: $(head -5 $D/lib.txt)"
fi

# configuration: a project .cc45-lint.yml (found from the source's directory),
# then the command line; an error makes the run fail
C=$D/cfg
mkdir -p $C/sub
cp $F/no_bp_mode.s45 $C/sub/a.s45
printf 'asm_rules:\n  - id: mega65-no-bp-mode\n    severity: error\n' > $C/.cc45-lint.yml
$AL $C/sub/a.s45 > $D/cfg1.txt 2>&1; rc1=$?
$AL --disable=mega65-no-bp-mode $C/sub/a.s45 > $D/cfg2.txt 2>&1; rc2=$?
printf 'asm_rules:\n  - id: mega65-no-bp-mode\n    pattern: "x"\n' > $C/.cc45-lint.yml
$AL $C/sub/a.s45 > $D/cfg3.txt 2>&1; rc3=$?
rm -f $C/.cc45-lint.yml
if [ $rc1 = 1 ] && grep -q ": error: .*\[mega65-no-bp-mode\]" $D/cfg1.txt && [ $rc2 = 0 ] && [ ! -s $D/cfg2.txt ] &&
   [ $rc3 = 1 ] && grep -q "not supported yet" $D/cfg3.txt; then
    pass "al45: .cc45-lint.yml searched upward, severity error fails the run, --disable wins, pattern: refused"
else
    fail "al45 configuration: rc $rc1 $rc2 $rc3: $(cat $D/cfg1.txt $D/cfg2.txt $D/cfg3.txt | head -5)"
fi

# output formats; --list-rules, --explain; the -v level does not change the output
$AL --format=json $F/dma_order.s45 > $D/j.json 2>&1
$AL --format=sarif $F/dma_order.s45 > $D/s.json 2>&1
$AL -vv $F/dma_order.s45 > $D/v.txt 2>/dev/null
$AL $F/dma_order.s45 > $D/t.txt 2>/dev/null
if python3 -c "
import json,sys
j=json.load(open('$D/j.json')); s=json.load(open('$D/s.json'))
assert j[0]['rule']=='mega65-dma-trigger-order' and j[0]['line']==14
r=s['runs'][0]
assert s['version']=='2.1.0' and r['results'][0]['ruleId']=='mega65-dma-trigger-order'
assert any(x['id']=='mega65-dma-trigger-order' for x in r['tool']['driver']['rules'])
" 2>/dev/null && cmp -s $D/v.txt $D/t.txt && $AL --list-rules | grep -q "^mega65-unreachable " &&
   $AL --explain=mega65-rom-call-bp | grep -q "__bp_rom" && ! $AL --explain=mega65-nope >/dev/null 2>&1; then
    pass "al45: JSON and SARIF output, --list-rules, --explain, -v leaves the output alone"
else
    fail "al45 output formats"
fi

echo ""
echo "al45 tests: $passed passed, $failed failed"
[ $failed = 0 ]
