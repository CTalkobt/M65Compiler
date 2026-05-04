; memcmp.s — Compare memory areas (ZP calling convention)
;
; int memcmp(void *s1, void *s2, int n);
;   Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2.

.global _memcmp

.segment "code"

proc _memcmp
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $03, $04, $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s1=$03/$04, s2=$05/$06, n=$07/$08
    ldy #0
@loop:
    lda $07
    ora $08
    beq @equal
    lda ($03),y
    cmp ($05),y
    bne @diff
    iny
    bne @no_inc
    inc $04
    inc $06
@no_inc:
    lda $07
    bne @dec_lo
    dec $08
@dec_lo:
    dec $07
    bra @loop
@equal:
    lda #0
    tax
    rts
@diff:
    bcs @greater
    lda #$FF
    tax
    rts
@greater:
    lda #1
    ldx #0
    rts
    endproc
