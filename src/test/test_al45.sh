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
    case $n in rom_call|zp_param|clobber_mismatch) opts="-c" ;; esac
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
   [ $rc3 = 1 ] && grep -q "the mega65- ids are the built-in rules'" $D/cfg3.txt; then
    pass "al45: .cc45-lint.yml searched upward, severity error fails the run, --disable wins, a pattern under a built-in id refused"
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

# --suggest-clobbers: the lists al45 works out, a line per proc; a register
# pushed and pulled is not in them
$AL -c --suggest-clobbers $F/clobber_mismatch.s45 > $D/sug.txt 2>&1
if grep -q "proc _under: .reg_clobbers A, X  .flag_clobbers C, N, Z  ; leaf" $D/sug.txt &&
   grep -q "proc _saves: .reg_clobbers A  .flag_clobbers N, Z  ; leaf" $D/sug.txt &&
   grep -q "proc _calls: .reg_clobbers A, X, Y, Z  .flag_clobbers C, N, Z, V  ; (calls _other: everything)" $D/sug.txt; then
    pass "al45 --suggest-clobbers"
else
    fail "al45 --suggest-clobbers: $(head -6 $D/sug.txt)"
fi

# #708: an option al45 does not know is a warning, as in cl45 (it took it as
# the input file's name); so is what the configuration names that nothing
# reads: an unknown option or rule id, a glob matching no rule. cl45's rule
# ids are known too (one file serves both linters)
C=$D/cfg708
mkdir -p $C
cp $F/no_bp_mode.s45 $C/a.s45
printf 'options:\n  mega65-style-coment-column: 30\noverrides:\n  mega65-stack-frame-limit: error\nasm_rules:\n  - id: mega65-no-bp-mod\n    severity: error\n' > $C/.cc45-lint.yml
$AL $C/a.s45 > $D/w708.txt 2>&1; rc=$?
rm -f $C/.cc45-lint.yml
$AL --foo --option=mega65-style-comment-column=30 $C/a.s45 > $D/w708b.txt 2>&1; rc2=$?
$AL $C/a.s45 --bar > $D/w708c.txt 2>&1; rc3=$?
if [ $rc = 0 ] && [ $rc2 = 0 ] && [ $rc3 = 0 ] &&
   grep -qF "al45: warning: $C/.cc45-lint.yml:2: options: unknown option 'mega65-style-coment-column' ignored" $D/w708.txt &&
   grep -qF "al45: warning: $C/.cc45-lint.yml:6: asm_rules: unknown rule 'mega65-no-bp-mod' ignored" $D/w708.txt &&
   ! grep -q "stack-frame-limit" $D/w708.txt && grep -q "\[mega65-no-bp-mode\]" $D/w708.txt &&
   grep -qF "al45: warning: unknown option '--foo' ignored" $D/w708b.txt &&
   grep -qF "al45: warning: unknown option '--option=mega65-style-comment-column=30' ignored" $D/w708b.txt &&
   grep -q "\[mega65-no-bp-mode\]" $D/w708b.txt &&
   grep -qF "al45: warning: unknown option '--bar' ignored" $D/w708c.txt && grep -q "\[mega65-no-bp-mode\]" $D/w708c.txt; then
    pass "al45: unknown options on the command line and in the configuration, unknown rule ids: warnings (#708)"
else
    fail "al45 #708 warnings: rc $rc $rc2 $rc3: $(head -4 $D/w708.txt $D/w708b.txt $D/w708c.txt)"
fi

# #714: jmp / jsr through a page-0 pointer ($xx, ($xx,x)) warn as ldz $xx does; $00xx and pointers above page 0 do not
printf '        .org $2000\nptr = $50\n        jmp ($40)\n        jsr ($41,x)\n        jmp (ptr)\n        jmp ($0040)\n        jmp ($1234)\n        jsr (vec)\n        phw $0040\nvec:    .word 0\n' > $D/j714.s45
./bin/ca45 $D/j714.s45 -o $D/j714.bin > $D/j714a.txt 2>&1
$AL $D/j714.s45 > $D/j714b.txt 2>&1
if [ "$(grep -c "has no base-page mode: its pointer is read at absolute" $D/j714a.txt)" = 3 ] &&
   grep -q ":3:.*'jmp (\$40)'" $D/j714a.txt && grep -q ":4:.*'jsr (\$41,x)'" $D/j714a.txt && grep -q ":5:.*'jmp (\$50)'" $D/j714a.txt &&
   [ "$(grep -c "\[mega65-no-bp-mode\]" $D/j714b.txt)" = 3 ]; then
    pass "jmp / jsr (\$xx) and (\$xx,x): ca45 warns, al45's mega65-no-bp-mode agrees; (\$0040), (\$1234), phw \$0040 do not (#714)"
else
    fail "#714 jmp (\$xx) warnings: $(cat $D/j714a.txt $D/j714b.txt)"
fi

# #765: user-written rules (pattern: in an asm_rules entry), the fixture's
# .cc45-lint.yml beside it: the findings on the EXPECT lines; an override
# before the rule makes one an error (exit 1); --checks turns on one that is
# off; JSON and SARIF carry them; --list-rules and --explain show them
P=src/test-resources/al45/patterns
want=$(grep -n "EXPECT proj-" $P/patterns.s45 | sed -E 's/^([0-9]+):.*EXPECT (proj-[a-z0-9-]+).*/\1 \2/' | sort)
got=$($AL --disable='mega65-*' $P/patterns.s45 2>$D/pat.err | sed -nE 's/^[^:]+:([0-9]+):[0-9]+: [a-z]+: .* \[(proj-[a-z0-9-]+)\]$/\1 \2/p' | sort)
$AL --disable='mega65-*' $P/patterns.s45 > $D/pat.txt 2>&1; rc=$?
$AL --disable='mega65-*' --checks=proj-any-then-rts --severity=proj-shift-a=warning $P/patterns.s45 > $D/pat2.txt 2>&1; rc2=$?
$AL --disable='mega65-*' --format=json $P/patterns.s45 > $D/pat.json 2>&1
$AL --disable='mega65-*' --format=sarif $P/patterns.s45 > $D/pat.sarif 2>&1
if [ "$want" = "$got" ] && [ ! -s $D/pat.err ] && [ $rc = 1 ] && [ $rc2 = 0 ] &&
   grep -qF "patterns.s45:4:1: warning: lda #0 then sta \$d020: stz \$d020 needs no A [proj-use-stz]" $D/pat.txt &&
   grep -qF "patterns.s45:16:1: error: a shift of A [proj-shift-a]" $D/pat.txt &&
   grep -qF "patterns.s45:24:1: warning: an instruction then rts [proj-any-then-rts]" $D/pat2.txt &&
   grep -qF "patterns.s45:16:1: warning: a shift of A [proj-shift-a]" $D/pat2.txt &&
   python3 -c "
import json
j=json.load(open('$D/pat.json')); s=json.load(open('$D/pat.sarif'))
assert any(d['rule']=='proj-inc-twice' and d['severity']=='info' and d['line']==12 for d in j)
r=s['runs'][0]
assert any(x['ruleId']=='proj-use-stz' for x in r['results'])
assert any(x['id']=='proj-use-stz' for x in r['tool']['driver']['rules'])
" && $AL --config=$P/.cc45-lint.yml --list-rules | grep -q "^proj-any-then-rts  *off " &&
   $AL --config=$P/.cc45-lint.yml --explain=proj-use-stz | grep -q "sta {dst}"; then
    pass "al45: user-written rules: findings, captures, NOLINT, overrides, --checks / --severity, JSON / SARIF, --list-rules, --explain (#765)"
else
    fail "al45 user-written rules: want [$(echo $want)] got [$(echo $got)] rc $rc $rc2 $(head -3 $D/pat.err $D/pat2.txt)"
fi

# #765: malformed user-written rules are errors naming the file and line;
# cl45 reads asm_rules' patterns and leaves them to al45, and refuses one in c_checks
C=$D/cfg765
mkdir -p $C
cp $P/patterns.s45 $C/a.s45
bad765() {   # yml, the error's text
    printf "$1" > $C/.cc45-lint.yml
    $AL $C/a.s45 > $C/out.txt 2>&1; local rc=$?
    if [ $rc = 1 ] && grep -qF "al45: error: $C/.cc45-lint.yml:$2" $C/out.txt; then return 0; fi
    echo "  (rc $rc: $(head -2 $C/out.txt))"
    return 1
}
ok765=1
bad765 'asm_rules:\n  - id: mega65-mine\n    message: m\n    pattern: nop\n' "2: 'mega65-mine': the mega65- ids are the built-in rules'" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    pattern: nop\n' "2: user-written rule 'proj-x' needs a message" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: m\n    pattern:\n      - nop\n      - "lda {x"\n' "6: pattern: 'lda {x': a capture is {name}" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: m\n    pattern: "l$da"\n' "4: pattern: 'l\$da': a mnemonic" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: "{y}"\n    pattern: "lda {x}"\n' "3: message: {y} is not a capture of the pattern" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: m\n    options:\n      a: 1\n    pattern: nop\n' "5: unknown key 'options' in a user-written rule" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: m\n    pattern: ""\n' "4: pattern: no instructions" || ok765=0
bad765 'asm_rules:\n  - id: proj-x\n    message: m\n    pattern: nop\n  - id: proj-x\n    message: m\n    pattern: nop\n' "5: a second user-written rule 'proj-x'" || ok765=0
printf 'int main(void) { return 0; }\n' > $C/m.c
printf 'asm_rules:\n  - id: proj-x\n    message: m\n    pattern: nop\noverrides:\n  proj-x: error\n' > $C/.cc45-lint.yml
./bin/cl45 -O2 $C/m.c > $C/cl1.txt 2>&1; rc1=$?
printf 'c_checks:\n  pattern: "x"\n' > $C/.cc45-lint.yml
./bin/cl45 -O2 $C/m.c > $C/cl2.txt 2>&1; rc2=$?
rm -f $C/.cc45-lint.yml
if [ $ok765 = 1 ] && [ $rc1 = 0 ] && [ ! -s $C/cl1.txt ] && [ $rc2 = 1 ] &&
   grep -qF "cl45: error: $C/.cc45-lint.yml:2: c_checks: user-written rules (pattern:) are al45's only" $C/cl2.txt; then
    pass "al45: malformed user-written rules are errors at their line; cl45 leaves asm_rules' to al45, refuses c_checks.pattern (#765)"
else
    fail "al45 #765 errors: ok $ok765, cl45 rc $rc1 $rc2: $(head -2 $C/cl1.txt $C/cl2.txt)"
fi

echo ""
echo "al45 tests: $passed passed, $failed failed"
[ $failed = 0 ]
