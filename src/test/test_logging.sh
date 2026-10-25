#!/usr/bin/env bash
# test_logging.sh — the toolchain's diagnostic log (include/Logger.hpp):
# -v ... -vvvv levels (cumulative, capped at 4, -v0 off) in cc45, ca45 and
# ln45; every log line on stderr as "[tool:LEVEL] ...", none on stdout;
# cc45 runs ca45 and ln45 at its own level; the program the same whatever
# the level. ar45, nm45, objdump45, disk45, cvt_asm, basic45 and tune45 log
# with -v and write the same output (dbg45: test_dbg45.sh needs xemu)
cd "$(dirname "$0")/../.." || exit 1
BIN=./bin
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
passed=0
failed=0
ok() { echo "SUCCESS: $1"; passed=$((passed + 1)); }
bad() { echo "FAIL: $1"; failed=$((failed + 1)); }

cat > "$TMP/t.c" <<'EOF'
int twice(int x) { return x * 2; }
int main(void) { return twice(21); }
EOF

# levels of cc45 alone (-c: no ca45/ln45 summary on stdout)
for lv in v vv vvv vvvv; do
    $BIN/cc45 -O2 -$lv -c "$TMP/t.c" -o "$TMP/t.o45" >"$TMP/out" 2>"$TMP/err"
    n=${#lv}
    names=(INFO DEBUG DEEP TRACE)
    good=1
    for k in 0 1 2 3; do
        c=$(grep -c "^\[cc45:${names[$k]}\]" "$TMP/err")
        if [ $k -lt $n ] && [ "$c" -eq 0 ] && [ $k -le 1 ]; then good=0; fi   # INFO/DEBUG always have lines
        if [ $k -ge $n ] && [ "$c" -ne 0 ]; then good=0; fi
    done
    [ $n -eq 4 ] && ! grep -q '^\[cc45:TRACE\] Token: ' "$TMP/err" && good=0
    grep -q '^\[' "$TMP/out" && good=0
    [ $good = 1 ] && ok "cc45 -$lv logs its levels only, on stderr" || bad "cc45 -$lv levels"
done

# nothing by default or with -v0; -v -v is -vv; -vvvvvv is -vvvv
$BIN/cc45 -O2 -c "$TMP/t.c" -o "$TMP/t.o45" 2>"$TMP/err0"
$BIN/cc45 -O2 -vv -v0 -c "$TMP/t.c" -o "$TMP/t.o45" 2>"$TMP/errv0"
if ! grep -q '^\[' "$TMP/err0" && ! grep -q '^\[' "$TMP/errv0"; then ok "no log by default or after -v0"; else bad "log without -v"; fi
$BIN/cc45 -O2 -v -v -c "$TMP/t.c" -o "$TMP/t.o45" >/dev/null 2>"$TMP/errc"
if grep -q '^\[cc45:DEBUG\]' "$TMP/errc" && ! grep -q '^\[cc45:DEEP\]' "$TMP/errc"; then ok "-v -v is -vv"; else bad "cumulative -v"; fi
$BIN/cc45 -O2 -vvvvvv -c "$TMP/t.c" -o "$TMP/t.o45" >/dev/null 2>"$TMP/errm"
if grep -q '^\[cc45:TRACE\]' "$TMP/errm"; then ok "-vvvvvv is -vvvv"; else bad "-vvvvvv"; fi

# a one-step build: ca45 and ln45 at cc45's level, every stderr line a
# log line or a diagnostic, the same program at every level
$BIN/cc45 -O2 "$TMP/t.c" -o "$TMP/p0.prg" >/dev/null 2>&1
$BIN/cc45 -O2 -vvv "$TMP/t.c" -o "$TMP/p3.prg" >"$TMP/out3" 2>"$TMP/err3"
if grep -q '^\[ca45:INFO\]' "$TMP/err3" && grep -q '^\[ln45:DEEP\]' "$TMP/err3"; then
    ok "cc45 -vvv runs ca45 and ln45 at -vvv"
else
    bad "cascade to ca45/ln45"
fi
if ! grep -vE '^\[(cc45|ca45|ln45):(INFO|DEBUG|DEEP|TRACE)\] |^(ln45|ca45|cc45): warning|warning:|note:|^ *[0-9]* \||^ *\| ' "$TMP/err3" | grep -q .; then
    ok "every stderr line is a log line or a diagnostic"
else
    bad "untagged stderr lines:"; grep -vE '^\[(cc45|ca45|ln45):' "$TMP/err3" | head -5
fi
if cmp -s "$TMP/p0.prg" "$TMP/p3.prg"; then ok "the program is the same at -vvv"; else bad "-vvv changed the program"; fi

# ca45 and ln45 on their own
$BIN/cc45 -O2 -c "$TMP/t.c" -o "$TMP/t.o45" >/dev/null 2>&1
$BIN/cc45 -O2 -S "$TMP/t.c" -o "$TMP/t.s45" >/dev/null 2>&1
$BIN/ca45 -vvvv -c "$TMP/t.s45" -o "$TMP/a.o45" >"$TMP/aout" 2>"$TMP/aerr"
if grep -q '^\[ca45:INFO\] Lexing' "$TMP/aerr" && grep -q '^\[ca45:TRACE\] Token: ' "$TMP/aerr" && ! grep -q '^\[' "$TMP/aout"; then
    ok "ca45 -vvvv: phases and tokens on stderr"
else
    bad "ca45 -vvvv"
fi
$BIN/ln45 -v -basic -o "$TMP/l.prg" "$TMP/t.o45" lib/build/c45.lib >"$TMP/lout" 2>"$TMP/lerr"
if grep -q '^\[ln45:INFO\]' "$TMP/lerr" && ! grep -q '^\[ln45:DEEP\]' "$TMP/lerr" && ! grep -q '^\[' "$TMP/lout"; then
    ok "ln45 -v: INFO only, on stderr"
else
    bad "ln45 -v"
fi

# the other tools: "[tool:INFO]" lines on stderr with -v (anywhere among
# the arguments), none without it, and the same standard output either way
other() {   # other TOOL "ARGS before -v" "ARGS after -v"
    local tool=$1 pre=$2 post=$3
    eval "$BIN/$tool $pre $post" >"$TMP/o0" 2>"$TMP/e0"
    eval "$BIN/$tool $pre -v $post" >"$TMP/o1" 2>"$TMP/e1"
    if grep -q "^\[$tool:INFO\] " "$TMP/e1" && ! grep -q '^\[' "$TMP/e0" && cmp -s "$TMP/o0" "$TMP/o1"; then
        ok "$tool -v logs on stderr, the same output"
    else
        bad "$tool -v (stderr: $(head -c 200 "$TMP/e1"))"
    fi
}
other ar45 "t" "lib/build/c45.lib"
other nm45 "" "\"$TMP/t.o45\""
other objdump45 "-f" "\"$TMP/t.o45\""
$BIN/disk45 create "$TMP/d.d81" -n LOGTEST >/dev/null 2>&1
other disk45 "list" "\"$TMP/d.d81\""
other cvt_asm "-t acme" "\"$TMP/t.s45\""
other basic45 "src/test-resources/test_basic45_xemu.bas" "-o \"$TMP/b.prg\""
other tune45 "list" "examples/c/tunes/music.txt"
# disk45: a lone -v first is still --version
if $BIN/disk45 -v | grep -q '^disk45 v'; then ok "disk45 -v (alone) is --version"; else bad "disk45 -v"; fi

echo "logging tests: $passed passed, $failed failed"
[ $failed -eq 0 ]
