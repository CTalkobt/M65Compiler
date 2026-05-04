; strcpy.s — Copy string (ZP calling convention)
;
; char *strcpy(char *dest, char *src);
;   Copies src including NUL terminator to dest. Returns dest.

.global _strcpy

.segment "code"

proc _strcpy
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; dest=$03/$04, src=$05/$06 — use directly as indirect pointers
    ldy #0
@loop:
    lda ($05),y
    sta ($03),y
    beq @done
    iny
    bne @loop
    inc $04
    inc $06
    bra @loop
@done:
    lda $03
    ldx $04
    rts
    endproc
