#!/bin/bash
# test_af45.sh — al45's style rules (mega65-style-*) and af45, the assembly
# formatter (#611; doc/bin/af45.md): the fixture's findings and its formatted
# text (src/test-resources/af45), the same bytes assembled before and after,
# idempotence, the options, the library formatted and clean, the errors
cd "$(dirname "$0")/../.."
AL=./bin/al45
AF=./bin/af45
F=src/test-resources/af45
D=build/test/af45
rm -rf $D; mkdir -p $D
# (a personal ~/.config/m65/cc45-lint.yml must not change the results)
export HOME=$PWD/$D/home
mkdir -p $HOME
passed=0; failed=0
pass() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }
findings() { # file [al45 options]: "line rule" of its style findings, sorted
    local f=$1; shift
    $AL --checks='mega65-style-*' "$@" $f 2>&1 | sed -nE 's/^[^:]+:([0-9]+):[0-9]+: [a-z]+: .* \[(mega65-style-[a-z-]+)\]$/\1 \2/p' | sort
}

# the rules: each of the fixture's problems, on its line
got=$(findings $F/messy.s45)
if [ "$got" = "$(sort $F/messy.expect)" ]; then
    pass "al45 style rules: messy.s45 ($(echo "$got" | wc -l) findings)"
else
    fail "al45 style rules: want [$(sort $F/messy.expect | tr '\n' ' ')] got [$(echo $got)]"
fi
# off by default
if ! $AL $F/messy.s45 2>&1 | grep -q "mega65-style-"; then pass "al45: the style rules are off by default"
else fail "al45: a style rule on by default"; fi

# af45: the formatted text; again: the same (idempotent); no findings in it
$AF $F/messy.s45 > $D/messy.out 2>&1
if cmp -s $D/messy.out $F/messy.fmt.s45; then pass "af45 messy.s45"
else fail "af45 messy.s45: $(diff $F/messy.fmt.s45 $D/messy.out | head -6)"; fi
$AF $F/messy.fmt.s45 > $D/again.out 2>&1
if cmp -s $D/again.out $F/messy.fmt.s45; then pass "af45: idempotent"
else fail "af45: a second run changes $(diff $F/messy.fmt.s45 $D/again.out | head -4)"; fi
if [ -z "$(findings $F/messy.fmt.s45)" ]; then pass "al45: no style findings in af45's output"
else fail "al45 on af45's output: $(findings $F/messy.fmt.s45 | head -3)"; fi

# what ca45 assembles does not change
./bin/ca45 $F/messy.s45 -o $D/before.bin >/dev/null 2>&1
./bin/ca45 $D/messy.out -o $D/after.bin >/dev/null 2>&1
if [ -s $D/before.bin ] && cmp -s $D/before.bin $D/after.bin; then pass "af45: the same bytes assembled"
else fail "af45: the bytes assembled differ"; fi

# the library: formatted (af45 --check lists nothing) and clean under the rules
if out=$($AF --check lib/*.s45 lib/stdlib_asm/*.s45 2>&1) && [ -z "$out" ]; then pass "af45 --check: the library is formatted"
else fail "af45 --check on the library: $(echo "$out" | head -3)"; fi

# --check: lists the file, exit status 1; -w rewrites it; -o writes elsewhere
cp $F/messy.s45 $D/w.s45
out=$($AF --check $D/w.s45); rc=$?
if [ $rc = 1 ] && [ "$out" = "$D/w.s45" ] && cmp -s $D/w.s45 $F/messy.s45; then pass "af45 --check"
else fail "af45 --check: exit $rc, '$out'"; fi
$AF -w $D/w.s45 && cmp -s $D/w.s45 $F/messy.fmt.s45 && pass "af45 -w" || fail "af45 -w"
$AF -o $D/o.s45 $F/messy.s45 && cmp -s $D/o.s45 $F/messy.fmt.s45 && pass "af45 -o" || fail "af45 -o"

# the options: from the configuration (options:) and af45 --option
printf 'start:\n    lda #1 ; a\n  rts\n' > $D/opt.s45
printf 'options:\n  mega65-style-instruction-column: 8\n  mega65-style-comment-column: 20\n  mega65-style-operand-column: 12\n' > $D/opt.yml
want=$(printf 'start:\n        lda #1      ; a\n        rts\n')
got=$($AF --config=$D/opt.yml $D/opt.s45)
if [ "$got" = "$want" ]; then pass "af45: the columns from the configuration"
else fail "af45 --config: got [$got]"; fi
got=$($AF --option=mega65-style-comment-column=12 $D/opt.s45 | sed -n 2p)
if [ "$got" = "    lda #1  ; a" ]; then pass "af45 --option"; else fail "af45 --option: [$got]"; fi
$AF --config=$D/opt.yml -o $D/opt.fmt.s45 $D/opt.s45
got=$(findings $D/opt.s45 --config=$D/opt.yml | tr '\n' ';')
if [ "$got" = "2 mega65-style-comment;2 mega65-style-indent;2 mega65-style-operand;3 mega65-style-indent;" ] &&
   [ -z "$(findings $D/opt.fmt.s45 --config=$D/opt.yml)" ]; then
    pass "al45: the columns from the configuration"
else
    fail "al45 with the columns of opt.yml: [$got] then [$(findings $D/opt.fmt.s45 --config=$D/opt.yml)]"
fi

# the names: regular expressions; a name the file exports is not a constant
printf '.global low_exp\nUPPER = 1\nlower = 2\nlow_exp = 3\nBad_Label:\n@Bad:\n    rts\n' > $D/names.s45
printf 'options:\n  mega65-style-constant-name: "^[A-Z][A-Z0-9_]*$"\n  mega65-style-label-name: "^[a-z_][a-z0-9_]*$"\n  mega65-style-local-name: "^@[a-z0-9_]+$"\n' > $D/names.yml
got=$(findings $D/names.s45 --config=$D/names.yml | tr '\n' ';')
if [ "$got" = "3 mega65-style-constant-name;5 mega65-style-label-name;6 mega65-style-local-name;" ]; then
    pass "al45: the naming rules"
else
    fail "al45 naming rules: [$got]"
fi

# NOLINT: a style finding suppressed on its line
printf 'start:\n  rts ; NOLINT(mega65-style-indent)\n' > $D/nolint.s45
if [ "$(findings $D/nolint.s45 | grep -c indent)" = 0 ]; then pass "al45: NOLINT suppresses a style finding"
else fail "al45: NOLINT on a style finding"; fi

# errors: an option that is not a column or a regular expression
printf 'options:\n  mega65-style-comment-column: wide\n' > $D/bad.yml
printf 'options:\n  mega65-style-label-name: "(["\n' > $D/badre.yml
$AF --config=$D/bad.yml $D/opt.s45 >/dev/null 2>$D/e1; r1=$?
$AL --config=$D/badre.yml $D/opt.s45 >/dev/null 2>$D/e2; r2=$?
if [ $r1 = 2 ] && grep -q "is not a column" $D/e1 && [ $r2 = 1 ] && grep -q "is not a regular expression" $D/e2; then
    pass "af45 / al45: a bad option is an error"
else
    fail "bad options: af45 exit $r1 $(cat $D/e1); al45 exit $r2 $(cat $D/e2)"
fi
$AF $D/opt.s45 $D/names.s45 >/dev/null 2>$D/e3; r3=$?
if [ $r3 = 2 ] && grep -q "several files" $D/e3; then pass "af45: several files need -w or --check"
else fail "af45 several files: exit $r3"; fi

echo ""
echo "af45 tests: $passed passed, $failed failed"
[ $failed = 0 ]
