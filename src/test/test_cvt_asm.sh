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
check merlin64 "tab" "tax" "DFB 1, 2, \$03" "ENT start" "EXT putc"
check oscar "tab" "tax" "BYTE 1, 2, \$03" "EXTERN putc"
OUT=$($CVT -t acme "$T/rt.s45" 2>/dev/null)
! grep -qE '^!(global|extern)|^TAB' <<< "$OUT" && pass "no bare !global/!extern, label case kept" ||
    fail "bare declarations or upper-cased label"

# Each dialect's reader takes its writer's output back: ca45 -> dialect ->
# ca45 assembles every flat test source to the same bytes (the dialects in
# parallel)
rtd() {
    local d=$1 n=0 bad=""
    mkdir -p "$T/rt_$d"
    for f in src/test-resources/*.s45; do
        local b; b=$(basename "$f" .s45)
        [ -f "$T/$b.orig" ] || continue
        n=$((n + 1))
        $CVT -t "$d" "$f" -o "$T/rt_$d/$b.x" 2>/dev/null
        $CVT -f "$d" -t ca45 "$T/rt_$d/$b.x" -o "$T/rt_$d/$b.s45" 2>/dev/null
        if ! (cd "$(dirname "$f")" && "$OLDPWD/$CA45" "$T/rt_$d/$b.s45" -o "$T/rt_$d/$b.bin" >/dev/null 2>&1) ||
           ! cmp -s "$T/$b.orig" "$T/rt_$d/$b.bin"; then
            bad="$bad $b"
        fi
    done
    echo "$n$bad" > "$T/rt_$d.result"
}
DIALECTS="ca65 acme kickassembler merlin64 oscar x65"
for d in $DIALECTS; do rtd "$d" & done
wait
for d in $DIALECTS; do
    read -r n bad < "$T/rt_$d.result"
    [ -z "$bad" ] && [ "$n" -gt 20 ] && pass "ca45 -> $d -> ca45: $n test sources assemble to the same bytes" ||
        fail "$d round trip: $bad"
done

# Programs in each dialect's own syntax (labels without ':', EQU, .pc =,
# !text/ASC/.byte strings, HEX, @local labels, 'c' constants) assemble as
# ca45 to the bytes of the same program written for ca45
cat > "$T/ref.s45" << 'END'
.org $2000
start:  ldx #0
loop:   lda msg,x
        beq done
        sta $0400,x
        inx
        bne loop
done:   lda #<msg
        ldy #>msg
        lda ($fb),y
        sta ($fb,x)
        jmp ($0314)
        asl
        lda #$41
        rts
msg:    .byte $48, $49, 0
        .word start, $1234
        .byte $ea, $ea, $ea
END
cat > "$T/p.ca65" << 'END'
; ca65
        .export start
        .org $2000
ptr     = $fb
vec     := $0314
start:  ldx #0
@loop:  lda msg,x
        beq @done
        sta $0400,x
        inx
        bne @loop
@done:  lda #<msg
        ldy #>msg
        lda (ptr),y
        sta (ptr,x)
        jmp (vec)
        asl a
        lda #'A'
        rts
msg:    .byte "HI", 0
        .word start, $1234
        .byte $ea, $ea, $ea
END
cat > "$T/p.acme" << 'END'
; ACME
!to "x.prg", cbm
!macro nops {
        nop
}
*= $2000
!zone main
ptr = $fb
start   ldx #0
loop    lda msg,x
        beq done
        sta $0400,x
        inx
        bne loop
done    lda #<msg
        ldy #>msg
        lda (ptr),y
        sta (ptr,x)
        jmp ($0314)
        asl
        lda #'A'
        rts
msg     !text "HI"
        !byte 0
        !word start, $1234
        !fill 3, $ea
END
cat > "$T/p.merlin" << 'END'
* Merlin 64
         ORG $2000
PTR      EQU $FB
START    LDX #0
LOOP     LDA MSG,X
         BEQ DONE
         STA $0400,X
         INX
         BNE LOOP
DONE     LDA #<MSG
         LDY #>MSG
         LDA (PTR),Y
         STA (PTR,X)
         JMP ($0314)
         ASL
         LDA #'A'
         RTS
MSG      ASC "HI"
         DFB 0
         DW START,$1234
         HEX EAEAEA
END
cat > "$T/p.ka" << 'END'
// KickAssembler
.pc = $2000
.const ptr = $fb
/* a block
   comment */
start: ldx #0
loop:  lda msg,x
       beq done
       sta $0400,x
       inx
       bne loop
done:  lda #<msg
       ldy #>msg
       lda (ptr),y
       sta (ptr,x)
       jmp ($0314)
       asl
       lda #$41
       rts
msg:   .text "HI"
       .byte 0
       .word start, $1234
       .fill 3, $ea
END
cat > "$T/p.x65" << 'END'
; x65
    .org $2000
ptr = $fb
start: ldx #0
loop   lda msg,x
       beq done
       sta $0400,x
       inx
       bne loop
done:  lda #<msg
       ldy #>msg
       lda (ptr),y
       sta (ptr,x)
       jmp ($0314)
       asl
       lda #'A'
       rts
msg:   dc.b "HI", 0
       dc.w start, $1234
       .byte $ea, $ea, $ea
END
$CA45 "$T/ref.s45" -o "$T/ref.bin" >/dev/null 2>&1
for x in ca65:ca65 acme:acme merlin:merlin64 ka:kickassembler x65:x65; do
    e=${x%%:*}; f=${x#*:}
    $CVT -f "$f" -t ca45 "$T/p.$e" -o "$T/p.$e.s45" 2>/dev/null
    $CA45 "$T/p.$e.s45" -o "$T/p.$e.bin" >/dev/null 2>&1 && cmp -s "$T/ref.bin" "$T/p.$e.bin" &&
        pass "$f program (its own syntax) assembles as ca45" || fail "$f program: $(cat "$T/p.$e.s45")"
done

# Strings and character constants in each dialect's encoding: ACME !text
# ASCII, !pet PETSCII, !scr screen codes; KickAssembler .text screen codes,
# then .encoding "petscii_mixed"; ca45's own .text (PETSCII) and 'a' written
# out as bytes
printf '*= $1000\n!text "Hi"\n!pet "Hi"\n!scr "Hi@"\nlda #%sa%s\n' "'" "'" > "$T/e.acme"
printf '*= $1000\n.text "Hi"\n.encoding "petscii_mixed"\n.text "Hi"\nlda #%sa%s\n' "'" "'" > "$T/e.ka"
printf '.org $1000\n.text "Hi"\nlda #%sa%s\n' "'" "'" > "$T/e.s45"
$CVT -f acme -t ca45 "$T/e.acme" -o "$T/e.acme.s45" 2>/dev/null
$CVT -f kickassembler -t ca45 "$T/e.ka" -o "$T/e.ka.s45" 2>/dev/null
$CA45 "$T/e.acme.s45" -o "$T/e.acme.bin" >/dev/null 2>&1
$CA45 "$T/e.ka.s45" -o "$T/e.ka.bin" >/dev/null 2>&1
$CVT -t acme "$T/e.s45" -o "$T/e.out.acme" 2>/dev/null
[ "$(xxd -p "$T/e.acme.bin")" = "4869c849480900a961" ] &&
    [ "$(xxd -p "$T/e.ka.bin")" = "4809c849a941" ] &&
    grep -q '!byte \$68, \$49' "$T/e.out.acme" && grep -q 'lda #\$41' "$T/e.out.acme" &&
    pass "strings and 'c' constants keep their bytes (ACME !text/!pet/!scr, KickAssembler .encoding, ca45 .text)" ||
    fail "string encodings (acme $(xxd -p "$T/e.acme.bin"), kickassembler $(xxd -p "$T/e.ka.bin"): $(cat "$T/e.out.acme")"

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
