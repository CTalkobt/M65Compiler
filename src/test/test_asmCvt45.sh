#!/bin/bash
# asmCvt45 tests (make test): ca45 input converted to each dialect
#
# - ca45 -> ca45 round trip: every src/test-resources/*.s45 that ca45 assembles
#   as a flat binary assembles to the same bytes after conversion
# - each writer: a label and an instruction on one line, data lists with their
#   commas, a label spelled like an opcode (tab:), constants, imports/exports
#   (comments for ACME/KickAssembler, which have no linker), comments
# - -o with one input names a file; -v's summary goes to stderr; unknown
#   directives and ca45-only syntax are reported

cd "$(dirname "$0")/../.."
CVT=./bin/asmCvt45
CA45=./bin/ca45
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
passed=0
failed=0
pass() { echo "  PASS: $1"; passed=$((passed + 1)); }
fail() { echo "  FAIL: $1"; failed=$((failed + 1)); }

echo "=== asmCvt45 ==="

# ca45 -> ca45 round trip over the test sources
n=0; bad=""
for f in src/test-resources/*.s45; do
    b=$(basename "$f" .s45)
    # (unoptimized: a conversion may turn a @local label into a global one,
    # which changes what ca45's optimizer may do, not what the code means)
    CA45_OPTIMIZE=0 $CA45 "$f" -o "$T/$b.orig" >/dev/null 2>&1 || continue
    n=$((n + 1))
    $CVT -t ca45 "$f" -o "$T/$b.rt.s45" 2>/dev/null
    if ! (cd "$(dirname "$f")" && CA45_OPTIMIZE=0 "$OLDPWD/$CA45" "$T/$b.rt.s45" -o "$T/$b.rt" >/dev/null 2>&1) ||
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
check bsa ".CPU 45GS02" ".STORE __bsa_start,__bsa_end-__bsa_start" "tab:" "tax" ".BYTE 1,2,\$03" "* = \$2000" \
    "vector = \$3000" "; global: start" "; extern: putc"
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
        if ! (cd "$(dirname "$f")" && CA45_OPTIMIZE=0 "$OLDPWD/$CA45" "$T/rt_$d/$b.s45" -o "$T/rt_$d/$b.bin" >/dev/null 2>&1) ||
           ! cmp -s "$T/$b.orig" "$T/rt_$d/$b.bin"; then
            bad="$bad $b"
        fi
    done
    echo "$n$bad" > "$T/rt_$d.result"
}
DIALECTS="ca65 acme kickassembler merlin64 oscar x65 bsa"
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
    grep -q '!byte \$C8, \$49' "$T/e.out.acme" && grep -q 'lda #\$41' "$T/e.out.acme" &&
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

# a macro's parameters replaced as whole words: a parameter a leaves sta
# alone (was st7); a macro used by a macro
printf '.macro inner v\n  lda #v\n.endmacro\n.macro outer a, b\n  inner a\n  sta b\n.endmacro\n  outer 7, $d020\n  rts\n' > "$T/nm.s"
M=$($CVT -f ca65 -t ca45 "$T/nm.s" 2>/dev/null | tr -s ' \n' ' ')
[ "$M" = " lda #7 sta \$d020 rts " ] && pass "macro parameters as whole words, a macro inside a macro" ||
    fail "macro parameters: $M"

# warnings: unknown directives, ca45-only syntax
printf '    .org $2000\n    .bogus 1\n    lda ($20),z\n' > "$T/w.s45"
W=$($CVT -t acme "$T/w.s45" 2>&1 >/dev/null)
grep -q "unknown directive '.bogus'" <<< "$W" && grep -q "kept as written: ca45 syntax or an addressing mode only 65CE02/45GS02" <<< "$W" &&
    pass "unknown directives and ca45-only syntax are reported" || fail "warnings ($W)"

# Native programs in each dialect's idioms (ca65 .proc/@locals/.define/: :- :+/
# .if/.rodata/.asciiz/.lobytes; ACME !zone .locals, + - anonymous labels, !macro with .params,
# !if { } else { }, !pet/!scr, !align mask; KickAssembler .macro(params),
# !: multi-labels, ((zp)),z, .if () { }) convert to ca45 that assembles to
# the bytes the real assembler made of them (src/test-resources/asmCvt45/
# *.bin: ca65 2.18 + ld65 at $2000, ACME 0.97, KickAssembler 65CE02, #224;
# BSA 03-Mar-2025: MACRO, MODULE _locals, & / .BSS, #if, its data forms and
# expression priorities, ^ and LBNE, #371)
NV=src/test-resources/asmCvt45
nbad=""
for spec in "ca65|native_ca65.s65" "acme|native_acme.a65" "kickassembler|native_kick.ka" "bsa|native_bsa.bsa"; do
    IFS='|' read -r nd nf <<< "$spec"
    nb="${nf%.*}"
    $CVT -f "$nd" -t ca45 "$NV/$nf" -o "$T/$nb.s45" 2>/dev/null
    if [ "$nd" = ca65 ]; then
        # (relocatable, as ca65 code is: linked at $2000, code then data)
        (cd "$T" && CA45_OPTIMIZE=0 "$OLDPWD/$CA45" -c "$nb.s45" -o "$nb.o45" >/dev/null 2>&1 &&
            "$OLDPWD/bin/ln45" -prg -t 2000 -o "$nb.prg" "$nb.o45" >/dev/null 2>&1 && tail -c +3 "$nb.prg" > "$nb.bin")
    else
        (cd "$T" && CA45_OPTIMIZE=0 "$OLDPWD/$CA45" "$nb.s45" -o "$nb.bin" >/dev/null 2>&1)
    fi
    cmp -s "$NV/$nb.bin" "$T/$nb.bin" || nbad="$nbad $nd"
done
[ -z "$nbad" ] && pass "native ca65, ACME, KickAssembler and BSA programs convert to the bytes their assemblers made" ||
    fail "native programs:$nbad"

# the older format tests
if bash src/test-resources/test_asmCvt45.sh >"$T/old.log" 2>&1; then
    pass "test_asmCvt45.sh (format pairs)"
else
    fail "test_asmCvt45.sh"; tail -5 "$T/old.log"
fi

echo "asmCvt45 tests: $passed passed, $failed failed"
[ "$failed" -eq 0 ]
