; isspace.s — Test if character is whitespace (ZP calling convention)
;
; int isspace(int c);
;   Returns 1 if c is whitespace ($20, $09-$0D), 0 otherwise.

.global _isspace

.segment "code"

proc _isspace
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$20
    beq @yes
    cmp #$09
    bcc @no
    cmp #$0e
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
