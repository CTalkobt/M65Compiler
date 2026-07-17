; ispunct.s — Test if character is punctuation (PETSCII, ZP calling convention)
;
; int ispunct(int c);
;   Returns 1 if c is printable but not alphanumeric and not space.

.global _ispunct

.segment "code"

proc _ispunct
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$20
    bcc @no
    cmp #$20
    beq @no
    cmp #$30
    bcc @yes
    cmp #$3a
    bcc @no
    cmp #$41
    bcc @yes
    cmp #$5b
    bcc @no
    cmp #$c1
    bcc @yes
    cmp #$db
    bcc @no
@yes:
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc
