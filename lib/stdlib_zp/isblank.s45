; isblank.s — Test if character is blank (ZP calling convention)
;
; int isblank(int c);
;   Returns 1 if c is space ($20) or tab ($09), 0 otherwise.

.global _isblank

.segment "code"

proc _isblank
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$20
    beq @yes
    cmp #$09
    beq @yes
    lda #0
    ldx #0
    rts
@yes:
    lda #1
    ldx #0
    rts
    endproc
