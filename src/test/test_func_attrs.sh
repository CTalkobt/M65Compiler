#!/bin/bash
# The attributes cc45 declares for a function (.reg_clobbers, .func_flags
# leaf) against what its code does: ca45's optimizer and ln45 believe them.
# #607: a 32-bit shift, comparison or truncation changes Y and Z too; the
# lists are what the function's instructions change (measured).
# #608: a call in inline assembly or of an operator method: not a leaf.
# al45's mega65-clobber-mismatch finds neither in cc45's output.
cd "$(dirname "$0")/../.."
CC="./bin/cc45 -O2"
T=build/test/func_attrs
mkdir -p $T
passed=0
failed=0
pass() { echo "PASS: $1"; passed=$((passed + 1)); }
fail() { echo "FAIL: $1"; failed=$((failed + 1)); }

# $1 the description, $2 the function's name, $3 a pattern its nm45 -f line
# must match, $4 one it must not (or ""), $5 the code
attrs() {
    printf '%s\n' "$5" > $T/a.c
    for f in "" "-fno-zpcall"; do
        if ! $CC $f -c $T/a.c -o $T/a.o45 > $T/a.log 2>&1; then fail "$1: does not compile ($f)"; head -3 $T/a.log; return; fi
        line=$(./bin/nm45 -f $T/a.o45 | grep " _$2  ")
        if ! grep -q "$3" <<< "$line" || { [ -n "$4" ] && grep -q "$4" <<< "$line"; }; then
            fail "$1 ($f): $line"; return
        fi
    done
    pass "$1"
}

attrs "a long shifted and cut down to a char: Z" f "regs:AXYZ" "" \
    'void f(unsigned long l) { *(volatile unsigned char *)0xD069 = (unsigned char)(l >> 8); }'
attrs "a comparison of longs: Z" f "regs:AXYZ" "" \
    'unsigned long a, b; char f(void) { return a < b; }'
attrs "a long negated: Z" f "regs:AXYZ" "" \
    'long a; void f(void) { a = -a; }'
attrs "a char function stays A (X): no Y, no Z" f "regs:AX " "regs:AXY" \
    'unsigned char g; unsigned char f(void) { return g + 1; }'
attrs "asm(jsr): not a leaf" f "zp_call\|stack_call" " leaf" \
    'void f(void) { asm("jsr $FFD2"); }'
attrs "__jsr_raw: not a leaf" f "zp_call\|stack_call" " leaf" \
    'void f(unsigned char c) { __jsr_raw(0xFFD2, .A = c); }'
attrs "assembly without a call: still a leaf" f " leaf" "" \
    'void f(void) { asm("sei"); asm("cli"); }'

# al45 over cc45's assembly for these: no clobber mismatch
printf 'void f(unsigned long l) { *(volatile unsigned char *)0xD069 = (unsigned char)(l >> 8); }\nstatic void p(unsigned char c) { __jsr_raw(0xFFD2, .A = c); }\nvoid g(void) { p(65); }\n' > $T/b.c
if $CC -c -S $T/b.c -o $T/b.s45 > $T/b.log 2>&1 &&
   [ -z "$(./bin/al45 -c --checks='-*,mega65-clobber-mismatch' $T/b.s45 2>/dev/null)" ]; then
    pass "al45 finds no clobber mismatch in cc45's output"
else
    fail "al45 on cc45's output: $(./bin/al45 -c --checks='-*,mega65-clobber-mismatch' $T/b.s45 2>&1 | head -3)"
fi

# ... nor in the shapes the estimate had wrong: a nested function reaching
# its parent's frame (tsy), a variadic call (plz), an indexed static (asl),
# a chain of calls (clc)
ok=1
for f in test_nested_decl_xemu test_zpcall_mixed_xemu test_static_init_xemu test_const_chain_xemu; do
    if ! $CC -c -S src/test-resources/$f.c -o $T/$f.s45 > $T/c.log 2>&1 ||
       [ -n "$(./bin/al45 -c --checks='-*,mega65-clobber-mismatch' $T/$f.s45 2>/dev/null)" ]; then
        ok=0; echo "  $f: $(./bin/al45 -c --checks='-*,mega65-clobber-mismatch' $T/$f.s45 2>&1 | head -2)"
    fi
done
[ $ok = 1 ] && pass "what a function's code changes is in its lists (measured, #607)" || fail "clobber lists against the code"

# a struct's operator method called: not a leaf, the callee's clobbers its own
attrs "an operator method's call: not a leaf" f "zp_call\|stack_call" " leaf" \
    'struct v { int x; void operator+=(struct v o) { this->x = this->x + o.x; } }; struct v a, b; void f(void) { a += b; }'

echo ""
echo "function attribute tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
