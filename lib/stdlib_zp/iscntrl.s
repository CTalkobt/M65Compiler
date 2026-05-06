; iscntrl.s — Test if character is a control character (PETSCII, ZP calling convention)
;
; int iscntrl(int c);
;   Returns 1 if c is $00-$1F or $80-$9F, 0 otherwise.

.global _iscntrl

.segment "code"

proc _iscntrl
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$20
    bcc @yes
    cmp #$80
    bcc @no
    cmp #$a0
    bcc @yes
@no:
    lda #0
    ldx #0
    rts
@yes:
    lda #1
    ldx #0
    rts
    endproc
