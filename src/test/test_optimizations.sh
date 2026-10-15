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
CC="./bin/cc45 -O2"
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

# post-increment-sink (*p++), on a source that has *p++
ptrace() { $CC -S -Roptir "$@" src/test-resources/test_zp_pointers_xemu.c -o $T/p.s 2>&1 >/dev/null; }
ptrace | grep -q "^\[postinc\]" && pass || fail "post-increment-sink does not run at the default level"
ptrace -O0 -fpost-increment-sink | grep -q "^\[postinc\]" && pass || fail "-O0 -fpost-increment-sink does not run it"
ptrace -fno-post-increment-sink | grep -q "^\[postinc\]" && fail "-fno-post-increment-sink still runs it" || pass
ptrace -O0 | grep -q "^\[postinc\]" && fail "post-increment-sink runs at -O0" || pass

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

# Loop interchange (AST pass, -O4): exactly the nests it may swap
LI=src/test-resources/test_interchange_xemu.c
li() { $CC -S -Roptir "$@" $LI -o $T/li.s 2>&1 >/dev/null | grep "^\[interchange\]" | sed -E 's/^\[interchange\] _([a-z_]+) .*/\1/' | tr '\n' ' '; }
[ "$(li -floop-interchange)" = "colcopy colsum " ] && pass || fail "-floop-interchange swapped: '$(li -floop-interchange)' (expected colcopy colsum)"
[ -z "$(li)" ] && pass || fail "loop interchange runs at the default -O2"
[ "$(li -O4)" = "colcopy colsum " ] && pass || fail "loop interchange does not run at -O4"
[ -z "$(li -O4 -fno-loop-interchange)" ] && pass || fail "-fno-loop-interchange does not stop it"
printf 'int a[4][5];\n#pragma cc45 optimize(no-loop-interchange)\nvoid f(void) { for (int i = 0; i < 5; i++) for (int j = 0; j < 4; j++) a[j][i] = 1; }\n' > $T/li2.c
$CC -S -Roptir -floop-interchange $T/li2.c -o $T/li2.s 2>&1 | grep -q "^\[interchange\]" && fail "#pragma cc45 optimize(no-loop-interchange) does not stop it" || pass

# Loop unrolling (IR pass, -O4): exactly these loops, the 300-iteration one is not
UN=src/test-resources/test_unroll_xemu.c
un() { $CC -S -Roptir "$@" $UN -o $T/un.s 2>&1 >/dev/null | grep "^\[unroll\]" | sed -E 's/^\[unroll\] _([a-z_]+)__test_unroll_xemu: .* unrolled ([0-9]+) times/\1x\2/' | tr '\n' ' '; }
want="simplex8 simplex4 cont_breakx6 cont_breakx6 nested_down_stepx4 nested_down_stepx4 nested_down_stepx5 nested_down_stepx4 char_ifx5 zero_callsx0 zero_callsx3 "
[ "$(un -floop-unrolling)" = "$want" ] && pass || fail "-floop-unrolling unrolled: '$(un -floop-unrolling)'"
[ -z "$(un)" ] && pass || fail "loop unrolling runs at the default -O2"
[ "$(un -O4)" = "$want" ] && pass || fail "loop unrolling does not run at -O4"
[ -z "$(un -O4 -fno-loop-unrolling)" ] && pass || fail "-fno-loop-unrolling does not stop it"
printf 'int a[4];\nvoid f(void) {\n#pragma cc45 optimize(no-loop-unrolling)\n    for (int i = 0; i < 4; i++) a[i] = i; }\n' > $T/un2.c
$CC -S -Roptir -floop-unrolling $T/un2.c -o $T/un2.s 2>&1 | grep -q "^\[unroll\]" && fail "#pragma cc45 optimize(no-loop-unrolling) does not stop it" || pass

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
out=$($CC -S --future -fprocedure-inlining $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "optimization 'procedure-inlining' is not implemented yet" && pass || fail "--future does not warn for -fprocedure-inlining"
out=$($CC -S -fprocedure-inlining $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "not implemented" && fail "unimplemented optimization warns without --future" || pass
printf '#pragma cc45 optimize(cross-function-inlining)\nint main(void) { return 0; }\n' > $T/fut.c
out=$($CC -S --future $T/fut.c -o $T/fut.s 2>&1)
echo "$out" | grep -q "optimization 'cross-function-inlining' is not implemented yet" && pass || fail "--future does not warn for the pragma"
out=$($CC -S -fno-constant-folding $SRC -o $T/t.s 2>&1)
echo "$out" | grep -q "cannot be disabled" && pass || fail "-fno-constant-folding does not warn"
out=$($CC -S -fbogus-option $SRC -o $T/t.s 2>&1); rc=$?
echo "$out" | grep -q "unknown option '-fbogus-option'" && [ $rc -eq 0 ] && pass || fail "an unknown -f option is not a warning"

# a name that is both a file-scope variable and a function is an error
printf 'int x;\nint x(void) { return 1; }\n' > $T/dup.c
out=$($CC -c $T/dup.c -o $T/dup.o45 2>&1); rc=$?
echo "$out" | grep -q "'x' redeclared as a different kind of symbol" && [ $rc -ne 0 ] && pass || fail "a variable and a function of the same name are not an error"

# a constant first operand of a commutative op is seen like a second one:
# 0 ^ x, 1 * x (also long) and 4 * x simplify like x ^ 0, x * 1, x * 4; a
# constant of another width (x ^ 0 on a char or long: a zext of the 0) too
printf 'unsigned a(unsigned x) { return 0 ^ x; }\nunsigned b(unsigned x) { return 1 * x; }\nunsigned long c(unsigned long x) { return 1 * x; }\nunsigned d(unsigned x) { return 4 * x; }\nunsigned char e(unsigned char x) { return x ^ 0; }\nunsigned long f(unsigned long x) { return x ^ 0; }\n' > $T/alg.c
out=$(CC45_DUMP_IR=1 $CC -c $T/alg.c -o $T/alg.o45 2>&1)
if ! echo "$out" | grep -qE "= (xor|mul_u|mul) " && echo "$out" | grep -q "= shl i16"; then pass; else fail "0^x / 1*x / 4*x not simplified"; fi

# all ones on either side of & and | (also written -1), and x & x / x | x on
# a char (the same operand only after type narrowing)
printf 'unsigned a(unsigned x) { return 0xFFFF & x; }\nsigned b(signed x) { return x & -1; }\nunsigned long c(unsigned long x) { return x & 0xFFFFFFFFUL; }\nunsigned char d(unsigned char x) { return x & x; }\nunsigned char e(unsigned char x) { return x | x; }\n' > $T/ones.c
out=$(CC45_DUMP_IR=1 $CC -c $T/ones.c -o $T/ones.o45 2>&1)
if ! echo "$out" | grep -qE "= (and|or) "; then pass; else fail "all-ones & / x&x on a char not simplified"; fi
printf 'unsigned a(unsigned x) { return 0xFFFF | x; }\nsigned b(signed x) { return -1 | x; }\n' > $T/ones2.c
out=$(CC45_DUMP_IR=1 $CC -c $T/ones2.c -o $T/ones2.o45 2>&1)
if ! echo "$out" | grep -qE "= or " && [ "$(echo "$out" | grep -c "const i16 65535")" = 2 ]; then pass; else fail "all-ones | not simplified"; fi

echo "Optimization switch tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
