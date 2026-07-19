; isxdigit.s — Test if character is a hexadecimal digit (PETSCII, ZP calling convention)
;
; int isxdigit(int c);
;   Returns 1 if c is '0'-'9', 'a'-'f', or 'A'-'F'. Returns 0 otherwise.

.global _isxdigit

.segment "code"

proc _isxdigit
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$30
    bcc @no
    cmp #$3a
    bcc @yes
    cmp #$41
    bcc @no
    cmp #$47
    bcc @yes
    cmp #$c1
    bcc @no
    cmp #$c7
    bcs @no
@yes:
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc
