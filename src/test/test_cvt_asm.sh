#!/bin/bash
# cvt_asm tests (make test): ca45 input converted to each dialect
#
# - ca45 -> ca45 round trip: every src/test-resources/*.s45 that ca45 assembles
#   as a flat binary assembles to the same bytes after conversion
# - each writer: a label and an instruction on one line, data lists with their
#   commas, a label spelled like an opcode (tab:), constants, imports/exports
#   (comments for ACME/KickAssembler, which have no linker), comments
# - -o with one input names a file; -v's summary goes to stderr; unknown
#   directives and ca45-only syntax are reported

cd "$(dirname "$0")/../.."
CVT=./bin/cvt_asm
CA45=./bin/ca45
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
passed=0
failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "=== cvt_asm ==="

# ca45 -> ca45 round trip over the test sources
n=0; bad=""
for f in src/test-resources/*.s45; do
    b=$(basename "$f" .s45)
    $CA45 "$f" -o "$T/$b.orig" >/dev/null 2>&1 || continue
    n=$((n + 1))
    $CVT -t ca45 "$f" -o "$T/$b.rt.s45" 2>/dev/null
    if ! (cd "$(dirname "$f")" && "$OLDPWD/$CA45" "$T/$b.rt.s45" -o "$T/$b.rt" >/dev/null 2>&1) ||
       ! cmp -s "$T/$b.orig" "$T/$b.rt"; then
        bad="$bad $b"
    fi
done
[ -z "$bad" ] && [ "$n" -gt 20 ] && pass "ca45 -> ca45 -> ca45: $n test sources assemble to the same bytes" ||
    fail "ca45 round trip:$bad"

cat > "$T/rt.s45" << 'EOF'
; round-trip test
    .global start
    .extern putc
    .org $2000
start:  lda #$01        ; load
tab:    tax
    ldy #<table
loop:   lda table,y
    sta $0400,x
    bne loop
    lda ($fb),y
    sta ($fb,x)
    jmp (vector)
    jsr putc
    rts
vector = $3000
table: .byte 1, 2, $03
words: .word $1234, start
EOF
check() {   # check <target> <pattern>...
    local t=$1; shift
    local out
    out=$($CVT -t "$t" "$T/rt.s45" 2>/dev/null)
    for p in "$@"; do
        if ! grep -qF -- "$p" <<< "$out"; then
            fail "$t: missing '$p'"
            echo "$out" | sed 's/^/      /'
            return
        fi
    done
    pass "$t writer"
}
check ca45 "start:" "lda #\$01 ;" "tab:" "tax" "lda table, Y" "jmp (vector)" "vector = \$3000" \
    ".byte 1, 2, \$03" ".word \$1234, start" ".global start" ".extern putc"
check ca65 "tab:" "tax" ".byte 1, 2, \$03" "vector = \$3000" ".export start" ".import putc"
check acme "tab:" "tax" "!byte 1, 2, \$03" "*= \$2000" "vector = \$3000" "; global: start" "; extern: putc"
check kickassembler "tab:" "tax" ".byte 1, 2, \$03" "*= \$2000" ".const vector = \$3000" "// global: start"
check x65 "tab:" "tax" ".byte 1, 2, \$03" ".export start" ".import putc"
check merlin64 "tab" "tax" "BYTE 1, 2, \$03" "GLOBAL start"
check oscar "tab" "tax" "BYTE 1, 2, \$03" "EXTERN putc"
OUT=$($CVT -t acme "$T/rt.s45" 2>/dev/null)
! grep -qE '^!(global|extern)|^TAB' <<< "$OUT" && pass "no bare !global/!extern, label case kept" ||
    fail "bare declarations or upper-cased label"

# -o with one input is a file; -v's summary is on stderr
$CVT -t acme "$T/rt.s45" -o "$T/out.a65" >/dev/null 2>&1
[ -f "$T/out.a65" ] && pass "-o names the output file" || fail "-o made a directory"
A=$($CVT -t ca65 "$T/rt.s45" 2>/dev/null)
B=$($CVT -v -t ca65 "$T/rt.s45" 2>/dev/null)
E=$($CVT -v -t ca65 "$T/rt.s45" 2>&1 >/dev/null)
[ "$A" = "$B" ] && grep -q 'Performance Metrics' <<< "$E" && pass "-v: the summary goes to stderr" ||
    fail "-v output mixed into stdout"

# warnings: unknown directives, ca45-only syntax
printf '    .org $2000\n    .bogus 1\n    lda ($20),z\n' > "$T/w.s45"
W=$($CVT -t acme "$T/w.s45" 2>&1 >/dev/null)
grep -q "unknown directive '.bogus'" <<< "$W" && grep -q "ca45-specific and kept as written" <<< "$W" &&
    pass "unknown directives and ca45-only syntax are reported" || fail "warnings ($W)"

# the older format tests
if bash src/test-resources/test_cvt_asm.sh >"$T/old.log" 2>&1; then
    pass "test_cvt_asm.sh (format pairs)"
else
    fail "test_cvt_asm.sh"; tail -5 "$T/old.log"
fi

echo "cvt_asm tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
