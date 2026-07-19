; strcat.s — Concatenate strings (ZP calling convention)
;
; char *strcat(char *dest, char *src);
;   Appends src to the end of dest. Returns dest.

.global _strcat

.segment "code"

proc _strcat
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; dest=$03/$04, src=$05/$06
    ; Save original dest for return
    lda $03
    pha
    lda $04
    pha
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
    ; Advance $03/$04 by Y to point at the NUL
    tya
    clc
    adc $03
    sta $03
    lda #0
    adc $04
    sta $04
    ; Copy src to dest end
    ldy #0
@copy:
    lda ($05),y
    sta ($03),y
    beq @done
    iny
    bne @copy
    inc $04
    inc $06
    bra @copy
@done:
    pla
    tax             ; original dest hi
    pla             ; original dest lo
    rts
    endproc
