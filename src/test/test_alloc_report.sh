#!/usr/bin/env bash
# test_alloc_report.sh — where each variable is placed, and why (#248):
# cc45 -Ralloc (on stderr; with -S also as "; alloc:" comments), the same
# code with and without it; ln45 -M's Static Allocation section and the base
# page blocks' sharing (the programs' call graphs), and SAC slot sharing
# happening at all (#790)
cd "$(dirname "$0")/../.." || exit 1
BIN=./bin
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
passed=0
failed=0
ok() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
bad() { echo "FAIL: $1"; failed=$((failed + 1)); }

cat > "$TMP/a.c" <<'EOF'
void g(int);
int h(void);
void fill(char *d, int n);
int sum(const int *p, int n) { int s = 0; for (int i = 0; i < n; i++) s += p[i]; return s; }
/* a and b never live together: one slot for both (#324) */
void branches(int c) {
    if (c) { int a = h(); g(1); g(a); }
    else   { int b = h(); g(2); g(b); }
}
int work(int k) {
    char buf[8];
    fill(buf, 8);
    return sum((int *)buf, 4) + k;
}
EOF
cat > "$TMP/b.c" <<'EOF'
void g(int x) { (void)x; }
int h(void) { return 7; }
void fill(char *d, int n) { while (n--) *d++ = 0; }
int sum(const int *p, int n);
void branches(int c);
int work(int k);
/* first and second never active together: their static blocks share bytes
   (work and branches: their base-page blocks) */
int first(int x) { int t = h(); g(x); return t + x; }
int second(int x) { int t = h(); g(x); return t - x; }
int main(void) { branches(1); return work(2) + first(1) + second(2); }
EOF

for conv in "zp|" "stack|-fno-zpcall"; do
    IFS='|' read -r cname cflags <<< "$conv"
    # the report on stderr, nothing on stdout
    if $BIN/cc45 -O2 $cflags -fno-zp-locals -Ralloc -c "$TMP/a.c" -o "$TMP/a_r.o45" >"$TMP/out" 2>"$TMP/err" &&
       [ ! -s "$TMP/out" ]; then
        ok "-Ralloc ($cname): compiles, nothing on stdout"
    else
        bad "-Ralloc ($cname): failed or wrote to stdout: $(cat "$TMP/out" "$TMP/err" | head -5)"
    fi
    grep -q '^_sum: static allocation (SAC), .*, leaf; frame 0 bytes, static [0-9]* bytes' "$TMP/err" &&
        ok "-Ralloc ($cname): a function's line" || bad "-Ralloc ($cname): no _sum line: $(head -3 "$TMP/err")"
    grep -Eq '^  buf +local +8 +_work__local_[0-9]+ +memory: its address is taken' "$TMP/err" &&
        ok "-Ralloc ($cname): an address-taken array in its static slot" || bad "-Ralloc ($cname): buf: $(grep buf "$TMP/err")"
    grep -Eq '^  a +local .*its bytes shared with (.*, )?b(, .*)? \(never live together, #324\)' "$TMP/err" &&
        grep -Eq '^_branches: .*static [0-9]+ bytes \([0-9]+ before sharing\)' "$TMP/err" &&
        ok "-Ralloc ($cname): two locals of branches share a slot (#324, #790)" ||
        bad "-Ralloc ($cname): no sharing in _branches: $(grep -A4 '^_branches' "$TMP/err")"
    # the same object without the report
    $BIN/cc45 -O2 $cflags -fno-zp-locals -c "$TMP/a.c" -o "$TMP/a_n.o45" 2>/dev/null
    cmp -s "$TMP/a_r.o45" "$TMP/a_n.o45" && ok "-Ralloc ($cname): the same object without it" ||
        bad "-Ralloc ($cname): the object differs"
    # with the base-page blocks too (the default)
    $BIN/cc45 -O2 $cflags -Ralloc -c "$TMP/b.c" -o "$TMP/b_r.o45" 2>"$TMP/errb"
    $BIN/cc45 -O2 $cflags -c "$TMP/b.c" -o "$TMP/b_n.o45" 2>/dev/null
    cmp -s "$TMP/b_r.o45" "$TMP/b_n.o45" && ok "-Ralloc ($cname): the same object (base-page blocks)" ||
        bad "-Ralloc ($cname): the object differs (base-page blocks)"
    # -S: the report as comments, the code otherwise the same
    $BIN/cc45 -O2 $cflags -Ralloc -S "$TMP/a.c" -o "$TMP/a_r.s45" 2>/dev/null
    $BIN/cc45 -O2 $cflags -S "$TMP/a.c" -o "$TMP/a_n.s45" 2>/dev/null
    if grep -q '^; alloc: _work: ' "$TMP/a_r.s45" && ! grep -q '@alloc' "$TMP/a_r.s45" &&
       diff <(grep -v '^; alloc:' "$TMP/a_r.s45") "$TMP/a_n.s45" >/dev/null; then
        ok "-Ralloc -S ($cname): '; alloc:' comments, the assembly otherwise the same"
    else
        bad "-Ralloc -S ($cname): comments missing or the assembly differs"
    fi
done
grep -q -- '-Ralloc' <($BIN/cc45 --help 2>&1) && ok "cc45 --help names -Ralloc" || bad "cc45 --help: no -Ralloc"

# ln45 -M: each static-allocation function's block
$BIN/cc45 -O2 -fno-zp-locals -c "$TMP/a.c" -o "$TMP/a.o45" 2>/dev/null
$BIN/cc45 -O2 -fno-zp-locals -c "$TMP/b.c" -o "$TMP/b.o45" 2>/dev/null
if $BIN/ln45 -basic -M "$TMP/p.map" -o "$TMP/p.prg" "$TMP/a.o45" "$TMP/b.o45" lib/build/c45.lib >/dev/null 2>&1; then
    sed -n '/^Static Allocation$/,/^$/p' "$TMP/p.map" > "$TMP/sac"
    grep -Eq '^  _first at \$[0-9A-F]{4} len [0-9]+, shares its bytes with .*_second' "$TMP/sac" &&
        ok "ln45 -M: two functions never active together share their block" ||
        bad "ln45 -M: _first does not share with _second: $(cat "$TMP/sac")"
    grep -Eq '^  _main at \$[0-9A-F]{4} len [0-9]+, bytes of its own: active at the same time as ' "$TMP/sac" &&
        ok "ln45 -M: main's block is its own, the reason given" ||
        bad "ln45 -M: _main's line: $(grep _main "$TMP/sac")"
else
    bad "ln45 -M: the link failed"
fi
$BIN/ln45 -basic --no-sac-overlay -M "$TMP/q.map" -o "$TMP/q.prg" "$TMP/a.o45" "$TMP/b.o45" lib/build/c45.lib >/dev/null 2>&1
grep -Eq '^  _first at .*, bytes of its own: --no-sac-overlay$' "$TMP/q.map" &&
    ok "ln45 -M --no-sac-overlay: every block its own, said so" || bad "ln45 -M --no-sac-overlay: $(grep _first "$TMP/q.map")"

# the base-page blocks (the default build): the existing lines kept, the sharing below them
$BIN/cc45 -O2 -c "$TMP/a.c" -o "$TMP/a2.o45" 2>/dev/null
$BIN/cc45 -O2 -c "$TMP/b.c" -o "$TMP/b2.o45" 2>/dev/null
if $BIN/ln45 -basic -M "$TMP/r.map" -o "$TMP/r.prg" "$TMP/a2.o45" "$TMP/b2.o45" lib/build/c45.lib >/dev/null 2>&1 &&
   grep -Eq '^  base-page block: _work at \$[0-9a-f]+ len [0-9]+$' "$TMP/r.map" &&
   grep -A1 '^  base-page block: _work ' "$TMP/r.map" | grep -q '^    shares its bytes with .*_branches'; then
    ok "ln45 -M: base-page blocks of functions never active together share bytes, said below the line"
else
    bad "ln45 -M: base-page sharing: $(sed -n '/^Base Page$/,/^$/p' "$TMP/r.map")"
fi

echo "allocation report tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
