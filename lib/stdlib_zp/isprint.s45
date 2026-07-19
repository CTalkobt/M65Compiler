; isprint.s — Test if character is printable (ZP calling convention)
;
; int isprint(int c);
;   Returns 1 if c is printable PETSCII ($20-$7E or $A0-$FF).

.global _isprint

.segment "code"

proc _isprint
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$20
    bcc @no
    cmp #$7f
    bcc @yes
    cmp #$a0
    bcs @yes
@no:
    lda #0
    ldx #0
    rts
@yes:
    lda #1
    ldx #0
    rts
    endproc
