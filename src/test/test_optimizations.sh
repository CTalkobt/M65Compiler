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
             copy-propagation:copy addr-elem-fusion:fusion rotate-loops:rotate; do
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
# ... also one name of a declaration of several (#331: extern unsigned long a,
# save_line; with a static function save_line used the function's address)
printf 'extern unsigned long a, x, b;\nstatic void x(unsigned int n) { (void)n; }\nunsigned long f(void) { x(1); return a; }\n' > $T/dup2.c
out=$($CC -c $T/dup2.c -o $T/dup2.o45 2>&1); rc=$?
echo "$out" | grep -q "'x' redeclared as a different kind of symbol" && [ $rc -ne 0 ] && pass || fail "a variable of a declaration group and a function of the same name are not an error"

# const between the stars (#334): T *const *p is a modifiable pointer to const
# pointers (*p and p[i] are read-only, p and **p are not); int **const *p's
# *p is the const one; the const pointers of a declaration of several names
# (int *a, *const q) stay read-only after it
n334=0
while IFS='|' read -r want code; do
    n334=$((n334 + 1))
    printf '%s\n' "$code" > $T/c334_$n334.c
    if $CC -c $T/c334_$n334.c -o $T/c334.o45 >/dev/null 2>&1; then got=ok; else got=err; fi
    [ "$got" = "$want" ] && pass || fail "#334 case $n334 ($want expected): $code"
done <<'EOF334'
ok|static const int a = 1, b = 2; static const int *const tbl[2] = {&a, &b}; static const int *const *p; int main(void) { p = tbl; return **p; }
ok|int main(void) { static const int a = 1; const int *const t[1] = {&a}; const int *const *p; p = t; return **p; }
err|static int *const *p; static int *q; int main(void) { *p = q; return 0; }
err|int main(void) { static int *q; int *const *p = 0; p[0] = q; return 0; }
ok|static int **pp; int main(void) { static int x; int *const *p = pp; **p = 1; return x; }
err|static int **const r; int main(void) { r = 0; return 0; }
err|int main(void) { static int *y; int *a, **const r = &y; r = 0; return 0; }
err|int main(void) { static int x; int *a, *const q = &x; q++; return 0; }
err|int main(void) { static int *q; int *a, *const *p = 0; *p = q; return 0; }
ok|int main(void) { static int *q, *r; int *const *p = &q; p = &r; return 0; }
err|int main(void) { static int *xp; int **const *p = 0; *p = &xp; return 0; }
ok|int main(void) { static int x; int **const *p = 0; **p = &x; return 0; }
ok|typedef int *const cip; static cip *p; static cip t[1]; int main(void) { p = t; return 0; }
ok|int f(int *const *p) { p = 0; return 0; } int main(void) { return f(0); }
EOF334

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

# a variable assigned a constant once, before any read, is that constant
printf 'unsigned a(unsigned x) { unsigned one = 1; return one * x; }\nunsigned b(unsigned x, unsigned k) { k = 0; return k ^ x; }\n' > $T/cvar.c
out=$(CC45_DUMP_IR=1 $CC -c $T/cvar.c -o $T/cvar.o45 2>&1)
if ! echo "$out" | grep -qE "= (xor|mul_u|mul) "; then pass; else fail "constant through a variable not propagated"; fi

# an int parameter every call passes 0-255 is a byte when its uses cost
# nothing zero-extended (an add, a compare), not when one would (a multiply),
# nor in a function other files can call
printf 'volatile unsigned w = 1000;\nstatic unsigned add(unsigned p, unsigned x) { return x + p > 1200; }\nstatic unsigned mul(unsigned p, unsigned x) { return x * p; }\nunsigned pub(unsigned p, unsigned x) { return x + p; }\nint main(void) { return add(200, w) + add(7, w) + mul(3, w) + pub(4, w); }\n' > $T/pn.c
$CC -S -fno-inline-small-functions $T/pn.c -o $T/pn.s 2>/dev/null
if grep -q "proc _add__pn, B#@_p_p, W#@_p_x" $T/pn.s && grep -q "proc _mul__pn, W#@_p_p" $T/pn.s &&
   grep -q "proc _pub, W#@_p_p" $T/pn.s; then pass; else fail "param narrowing ($(grep 'proc _' $T/pn.s | tr '\n' ' '))"; fi
$CC -S -fno-inline-small-functions -fno-param-narrowing $T/pn.c -o $T/pn2.s 2>/dev/null
grep -q "proc _add__pn, W#@_p_p" $T/pn2.s && pass || fail "-fno-param-narrowing still narrows"

# a temporary's register that nothing reads is not written (a[i]: the index
# and the element were stored to $30/$31 and $34/$35 and never read)
printf 'int a[10];\nvolatile unsigned i;\nint f(void) { return a[i]; }\n' > $T/dps.c
$CC -S $T/dps.c -o $T/dps.s 2>/dev/null
if awk '/proc _f/{p=1} /endproc/{p=0} p' $T/dps.s | grep -qE "^ +st[axyzq] [$][3-7][0-9A-F]"; then
    fail "dead stores to pool registers kept ($(awk '/proc _f/{p=1} /endproc/{p=0} p' $T/dps.s | grep -E '^ +st' | tr '\n' ' '))"
else pass; fi
CC45_NO_POOL_PRUNE=1 $CC -S $T/dps.c -o $T/dps2.s 2>/dev/null
awk '/proc _f/{p=1} /endproc/{p=0} p' $T/dps2.s | grep -qE "^ +sta [$]3" && pass || fail "CC45_NO_POOL_PRUNE=1 does not keep the stores"

# Loop rotation (#138): the test after the body, entered by a branch: one
# backward conditional branch and the entry's bra, no branch back
printf 'unsigned char buf[100];\nvoid fill(unsigned char n) { unsigned char i; for (i = 0; i < n; i++) buf[i] = i; }\n' > $T/rot.c
$CC -c $T/rot.c -o $T/rot.o45 2>/dev/null
dis=$(./bin/objdump45 -d $T/rot.o45 | grep -E "^\s+[0-9a-f]{4}:" | cut -f3- | awk '{print $1}' | tr '\n' ' ')
case "$dis" in
    *"bra "*"bcc rts"*) pass ;;
    *) fail "a for loop is not rotated: $dis" ;;
esac
# ... kept only where the function is no larger (both ways assembled), none
# under -Os, always with -frotate-loops
printf 'unsigned int count(unsigned int n) { unsigned int c = 0, k = 0; while (k < n) { if (k & 1) c++; k++; } return c; }\n' > $T/rot2.c
$CC -Roptir -c $T/rot2.c -o $T/rot2.o45 2>&1 | grep -q "^\[rotate\] _count: kept" && pass || fail "the size trial does not keep count's rotation"
./bin/cc45 -Os -Roptir -c $T/rot2.c -o $T/rot2.o45 2>&1 | grep -q "^\[rotate\]" && fail "-Os rotates loops" || pass
./bin/cc45 -Os -frotate-loops -Roptir -c $T/rot2.c -o $T/rot2.o45 2>&1 | grep -q "^\[rotate\] _count .*@while_cond0 after" && pass || fail "-Os -frotate-loops does not rotate"
ls temp_*_trial* >/dev/null 2>&1 && fail "trial files left behind" || pass

# ca45: a label reached only by a backward branch (a reset label between)
# starts with the registers at the branch: f's body does not reload A; g's
# label has two branches to it, so it does
cat > $T/onebr.s45 <<'EOF'
proc _f
    bra @t
@b:
    lda $30
    sta $40
    sta $41
    sta $42
    ldy #0
@t:
    lda $30
    cmp #10
    bcc @b
    rts
endproc
proc _g
    bra @t
@b:
    lda $30
    sta $40
    sta $41
    sta $42
    ldy #0
@t:
    lda $30
    cmp #10
    bcc @b
    beq @b
    rts
endproc
EOF
./bin/ca45 -c $T/onebr.s45 -o $T/onebr.o45 >/dev/null 2>&1
n=$(./bin/objdump45 -d $T/onebr.o45 | grep -c 'lda	\$30')
[ "$n" = 3 ] && pass || fail "ca45: a one-branch label's registers (lda \$30 left: $n, expected 3)"

echo "Optimization switch tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
