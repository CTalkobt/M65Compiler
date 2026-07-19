; strncat.s — Concatenate at most n characters (ZP calling convention)
;
; char *strncat(char *dest, char *src, int n);

.global _strncat

.segment "code"

proc _strncat
    .zp_uses $03, $04, $05, $06, $07, $08, $09, $0a
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09, $0a
    .zp_release $07, $08, $09, $0a
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; $03/$04 = dest, $05/$06 = src, $07/$08 = n
    ; Save dest for return
    lda $03
    sta $09
    lda $04
    sta $0a
    ; Find end of dest
    ldy #0
@find_end:
    lda ($03),y
    beq @found
    iny
    bne @find_end
    inc $04
    bra @find_end
@found:
    ; Advance $03 by Y
    tya
    clc
    adc $03
    sta $03
    lda #0
    adc $04
    sta $04
    ldy #0
@copy:
    lda $07
    ora $08
    beq @terminate
    lda ($05),y
    sta ($03),y
    beq @done
    lda $07
    bne @nodec_hi
    dec $08
@nodec_hi:
    dec $07
    iny
    bne @copy
    inc $04
    inc $06
    bra @copy
@terminate:
    lda #0
    sta ($03),y
@done:
    lda $09
    ldx $0a
    rts
    endproc
