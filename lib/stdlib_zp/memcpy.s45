; memcpy.s — Copy memory area (ZP calling convention)
;
; void *memcpy(void *dest, void *src, int n);
;   Copies n bytes from src to dest. Returns dest.

.global _memcpy

.segment "code"

proc _memcpy
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; dest=$03/$04, src=$05/$06, n=$07/$08
    ldy #0
@loop:
    lda $07
    ora $08
    beq @done
    lda ($05),y
    sta ($03),y
    iny
    bne @no_inc
    inc $04
    inc $06
@no_inc:
    lda $07
    bne @dec_lo
    dec $08
@dec_lo:
    dec $07
    bra @loop
@done:
    lda $03
    ldx $04
    rts
    endproc
