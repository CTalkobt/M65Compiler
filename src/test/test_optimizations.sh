#!/bin/bash
# test_optimizations.sh — cc45's optimization switches (no emulator needed)
#
# For every implemented optimization of OptimizationCatalog:
#   - IR passes: -Roptir reports the pass at the default level and with
#     -O0 -f<pass>, and does not with -fno-<pass>, -O0 or
#     #pragma cc45 optimize(no-<pass>)
#   - ca45 passes: -f/-fno- reach ca45 (tail-call: JMP vs JSR in the object)
#   - inline-small-functions, devirtualization: the call disappears / stays
# and the diagnostics: --future warns for unimplemented optimizations (only
# with --future), -fno-<always on> warns, unknown -f options warn and are
# not taken as the input file.
# The runtime check of the passes is test_iropt_xemu.c in test_xemu.sh.

cd "$(dirname "$0")/../.." || exit 1
CC=./bin/cc45
SRC=src/test-resources/test_iropt_xemu.c
T=build/test/opt
mkdir -p $T
passed=0
failed=0

pass() { passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

# trace FLAGS... — the -Roptir report of SRC
trace() { $CC -S -fsigned-default -Roptir "$@" $SRC -o $T/t.s 2>&1 >/dev/null; }

# pass name : -Roptir tag
for entry in strength-reduction:strength algebraic-simplify:algebraic type-narrowing:narrow \
             branch-folding:branch cse:cse loop-invariant-code-motion:licm \
             copy-propagation:copy addr-elem-fusion:fusion; do
    p=${entry%%:*}; tag=${entry##*:}
    trace | grep -q "^\[$tag\]" && pass || fail "$p does not run at the default level"
    # (the copies that copy propagation removes come from the other passes)
    extra=""; [ $p = copy-propagation ] && extra=-fcse
    trace -O0 $extra -f$p | grep -q "^\[$tag\]" && pass || fail "-O0 -f$p does not run it"
    trace -fno-$p | grep -q "^\[$tag\]" && fail "-fno-$p still runs it" || pass
    trace -O0 | grep -q "^\[$tag\]" && fail "$p runs at -O0" || pass
done

# #pragma cc45 optimize(no-licm) for the functions that follow
cat > $T/prag.c <<'EOF'
int g[4];
int f1(int n, int k) { int s = 0, i; for (i = 0; i < n; i++) s += k * 7; return s; }
#pragma cc45 optimize(no-licm)
int f2(int n, int k) { int s = 0, i; for (i = 0; i < n; i++) s += k * 7; return s; }
int main(void) { return f1(2, 3) + f2(2, 3); }
EOF
out=$($CC -S -Roptir $T/prag.c -o $T/prag.s 2>&1)
echo "$out" | grep -q "^\[licm\] _f1" && pass || fail "LICM does not run on f1"
echo "$out" | grep -q "^\[licm\] _f2" && fail "#pragma cc45 optimize(no-licm) did not stop LICM in f2" || pass

# ... and inside a function body, for that function only
cat > $T/prag2.c <<'EOF'
int f1(int n, int k) {
#pragma cc45 optimize(no-licm)
    int s = 0, i; for (i = 0; i < n; i++) s += k * 7; return s; }
int f2(int n, int k) { int s = 0, i; for (i = 0; i < n; i++) s += k * 7; return s; }
int main(void) { return f1(2, 3) + f2(2, 3); }
EOF
out=$($CC -S -Roptir $T/prag2.c -o $T/prag2.s 2>&1)
echo "$out" | grep -q "^\[licm\] _f1" && fail "optimize(no-licm) inside f1 did not stop LICM there" || pass
echo "$out" | grep -q "^\[licm\] _f2" && pass || fail "optimize(no-licm) inside f1 leaked into f2"
printf '#pragma cc45 optimize(no-tail-call-optimization)\nint main(void) { return 0; }\n' > $T/fw.c
$CC -S $T/fw.c -o $T/fw.s 2>&1 | grep -q "applies to the whole file" && pass || fail "optimize(no-<ca45 pass>) does not warn"

# ca45 pass: tail calls (JSR + RTS -> JMP)
printf 'void g(void);\nvoid f(void) { g(); }\n' > $T/tail.c
$CC -c $T/tail.c -o $T/tail.o45 >/dev/null 2>&1
./bin/objdump45 -d $T/tail.o45 2>/dev/null | grep -qi "jmp" && pass || fail "tail call is not a JMP by default"
$CC -c -fno-tail-call-optimization $T/tail.c -o $T/tail.o45 >/dev/null 2>&1
./bin/objdump45 -d $T/tail.o45 2>/dev/null | grep -qi "jsr" && pass || fail "-fno-tail-call-optimization keeps the JMP"

# inline-small-functions: on at -O2, off with -fno-
printf 'static int sq(int x) { return x * x; }\nint f(int a) { return sq(a) + 1; }\n' > $T/inl.c
$CC -S -O2 $T/inl.c -o $T/inl.s >/dev/null 2>&1
grep -q "jsr _sq" $T/inl.s && fail "sq is not inlined at -O2" || pass
$CC -S -O2 -fno-inline-small-functions $T/inl.c -o $T/inl.s >/dev/null 2>&1
grep -q "jsr _sq" $T/inl.s && pass || fail "-fno-inline-small-functions still inlines"

# devirtualization: a single implementation is called directly
cat > $T/dv.c <<'EOF'
struct A { int v; virtual int get() { return this->v; } };
int f(struct A *a) { return a->get(); }
EOF
$CC -S $T/dv.c -o $T/dv.s >/dev/null 2>&1
grep -q "jsr _A__get" $T/dv.s && pass || fail "the virtual call is not devirtualized"
$CC -S -fno-devirtualization $T/dv.c -o $T/dv.s >/dev/null 2>&1
grep -q "jsr _A__get" $T/dv.s && fail "-fno-devirtualization still calls directly" || pass

# Diagnostics
out=$($CC -S --future -floop-unrolling $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "optimization 'loop-unrolling' is not implemented yet" && pass || fail "--future does not warn for -floop-unrolling"
out=$($CC -S -floop-unrolling $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "not implemented" && fail "unimplemented optimization warns without --future" || pass
printf '#pragma cc45 optimize(loop-interchange)\nint main(void) { return 0; }\n' > $T/fut.c
out=$($CC -S --future $T/fut.c -o $T/fut.s 2>&1)
echo "$out" | grep -q "optimization 'loop-interchange' is not implemented yet" && pass || fail "--future does not warn for the pragma"
out=$($CC -S -fno-constant-folding $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "cannot be disabled" && pass || fail "-fno-constant-folding does not warn"
out=$($CC -S -fbogus-option $SRC -o $T/t.s 2>&1); rc=$?
echo "$out" | grep -q "unknown option '-fbogus-option'" && [ $rc -eq 0 ] && pass || fail "an unknown -f option is not a warning"

echo "Optimization switch tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
