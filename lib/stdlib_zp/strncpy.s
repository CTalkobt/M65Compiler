; strncpy.s — Copy string with length limit (ZP calling convention)
;
; char *strncpy(char *dest, char *src, int n);
;   Copies at most n bytes from src to dest. Pads with NUL if shorter.

.global _strncpy

.segment "code"

proc _strncpy
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; dest=$03/$04, src=$05/$06, n=$07/$08
    ldy #0
@copy:
    lda $07
    ora $08
    beq @done
    lda ($05),y
    sta ($03),y
    beq @pad_start
    iny
    bne @no_inc_copy
    inc $04
    inc $06
@no_inc_copy:
    lda $07
    bne @dec_lo_copy
    dec $08
@dec_lo_copy:
    dec $07
    bra @copy
@pad_start:
    lda $07
    bne @dec_lo_pad_first
    dec $08
@dec_lo_pad_first:
    dec $07
@pad:
    lda $07
    ora $08
    beq @done
    iny
    bne @no_inc_pad
    inc $04
@no_inc_pad:
    lda #0
    sta ($03),y
    lda $07
    bne @dec_lo_pad
    dec $08
@dec_lo_pad:
    dec $07
    bra @pad
@done:
    lda $03
    ldx $04
    rts
    endproc
