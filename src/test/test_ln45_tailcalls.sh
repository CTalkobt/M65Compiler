#!/bin/bash
# ln45's call graph counts tail calls (#615): a call cc45 made a JMP (JSR +
# RTS) is an edge like a JSR, for the static-allocation recursion check, the
# interrupt-handler check, the calling-convention bridges and the map's
# "Call Graph". A cycle of tail calls alone is a loop, not a recursion.

CC="./bin/cc45 -O2"
LD=./bin/ln45
LIB=lib/build
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT

passed=0
failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "--- Test: ln45 call graph and tail calls ---"

link() { # name objects... : the link's messages in $T/name.log, its status returned
    local n=$1; shift
    $LD -basic -o $T/$n.prg -M $T/$n.map $LIB/crt0_zp.o45 "$@" -l $LIB/c45.lib > $T/$n.log 2>&1
}
tailjmp() { ./bin/objdump45 -d "$1" | grep -q "jmp.*<$2>"; }

# f -> g by JSR, g -> f by a tail JMP: f is entered again while it is active
cat > $T/r.c <<'END'
int g(int x);
int f(int x) {
    volatile int k = x;
    if (!k) return 0;
    return g(k - 1) + k;
}
int main(void) { return f(3); }
END
printf 'int f(int x);\nint g(int x) { return f(x); }\n' > $T/g.c
$CC -c $T/r.c -o $T/r.o45 && $CC -c $T/g.c -o $T/g.o45
tailjmp $T/g.o45 _f && pass "g's call of f is a tail JMP" || fail "g's call of f is not a JMP (the test needs one)"
if link rec $T/r.o45 $T/g.o45; then
    fail "recursion through a tail call linked"
else
    grep -q "'_f' uses static allocation but is recursive" $T/rec.log &&
        pass "recursion through a tail call is a link error" || fail "recursion through a tail call: $(head -1 $T/rec.log)"
fi
# (g only jumps to f: it has finished when f runs, and is not recursive)
grep -q "'_g' uses static allocation but is recursive" $T/rec.log && fail "g reported recursive" || pass "the function that only tail-calls is not reported"

# with a frame for f (#pragma cc45 recurse) it links, and the map has the edge
sed 's/^int f(int x) {/#pragma cc45 recurse\nint f(int x) {/' $T/r.c > $T/r2.c
$CC -c $T/r2.c -o $T/r2.o45
link rec2 $T/r2.o45 $T/g.o45 && pass "links with #pragma cc45 recurse" || fail "#pragma cc45 recurse: $(head -1 $T/rec2.log)"
grep -q '^  _g -> .*_f' $T/rec2.map && pass "the map's call graph has the tail call" || fail "the map lacks _g -> _f"

# a -> b -> a, each a tail JMP: a loop, both with static allocation
printf 'void b(int n);\nvolatile int cnt;\nvoid a(int n) { cnt++; if (!n) return; b(n - 1); }\nint main(void) { a(5); return 0; }\n' > $T/a.c
printf 'void a(int n);\nvoid b(int n) { a(n); }\n' > $T/b.c
$CC -c $T/a.c -o $T/a.o45 && $CC -c $T/b.c -o $T/b.o45
if tailjmp $T/a.o45 _b && tailjmp $T/b.o45 _a; then
    link loop $T/a.o45 $T/b.o45 && pass "a cycle of tail calls alone links" || fail "a cycle of tail calls alone: $(head -1 $T/loop.log)"
    grep -q '^  _a -> .*_b' $T/loop.map && grep -q '^  _b -> .*_a' $T/loop.map &&
        pass "both tail calls in the map" || fail "the map lacks a tail call of the cycle"
else
    fail "a.c / b.c did not compile to tail JMPs (the test needs them)"
fi

# a tail JMP from a zp_call function to a stack_call one: reported, bridged
printf 'int s(int a, int b);\nint h(int x) { return s(x, 7); }\nint main(void) { return h(5); }\n' > $T/zc.c
printf 'int s(int a, int b) { return a + b; }\n' > $T/sc.c
$CC -c $T/zc.c -o $T/zc.o45 && $CC -fno-zpcall -c $T/sc.c -o $T/sc.o45
tailjmp $T/zc.o45 _s || fail "h's call of s is not a JMP (the test needs one)"
$LD --no-thunks -basic -o $T/nt.prg $LIB/crt0_zp.o45 $T/zc.o45 $T/sc.o45 -l $LIB/c45.lib > $T/nt.log 2>&1
grep -q "calling convention mismatch: '_h' calls '_s'" $T/nt.log &&
    pass "--no-thunks: the mismatch of a tail call is reported" || fail "--no-thunks: no mismatch for the tail call"
link thunk $T/zc.o45 $T/sc.o45 && grep -q "'_h' calls '_s'.*generating thunk" $T/thunk.log &&
    pass "the tail call gets a bridge thunk" || fail "no thunk for the tail call"

# a JMP from one routine of a hand-written object to another is a
# continuation, not a call to bridge: the library's __dma_mem (zp_call) ends
# in jmp __dma_start (no convention of its own)
printf '#include <string.h>\nchar buf[40];\nint main(void) { volatile char v = 7; memset(buf, v, sizeof buf); return buf[39] != 7; }\n' > $T/dm.c
$CC -c $T/dm.c -o $T/dm.o45
link dma $T/dm.o45 && grep -q '__dma_mem -> .*__dma_start' $T/dma.map || fail "the memset program does not reach __dma_start by __dma_mem (the test needs it)"
grep -q "calling convention mismatch\|__thunk_" $T/dma.log $T/dma.map &&
    fail "a hand-written JMP between routines was bridged" || pass "a hand-written JMP between routines is not bridged"

# an interrupt handler reaching a static-allocation function through a tail call
printf 'void helper(void);\n__interrupt void isr(void) { helper(); }\nint main(void) { return 0; }\n' > $T/i1.c
printf 'int work(int n);\nvoid helper(void) { work(3); }\n' > $T/i2.c
printf 'volatile int acc;\nint work(int n) { volatile int k = n; acc += k; return k; }\n' > $T/i3.c
for f in i1 i2 i3; do $CC -c $T/$f.c -o $T/$f.o45; done
tailjmp $T/i2.o45 _work || fail "helper's call of work is not a JMP (the test needs one)"
link isr $T/i1.o45 $T/i2.o45 $T/i3.o45
grep -q "'_work' uses static allocation but is reachable from ISR handler" $T/isr.log &&
    pass "an interrupt handler's tail-called function is checked" || fail "the ISR check missed a tail-called function"

echo "ln45 tail call tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
