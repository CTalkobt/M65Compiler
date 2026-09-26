; memset.s — Fill memory with a byte value (ZP calling convention)
;
; void *memset(void *s, int c, int n);
;   Fills n bytes at address s with byte c. Returns s.

.global _memset

.segment "code"

proc _memset
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s=$03/$04, c=$05 (low byte), n=$07/$08
    ldy #0
    lda $05             ; fill byte
@loop:
    ldx $08
    bne @do
    ldx $07
    beq @done
@do:
    sta ($03),y
    iny
    bne @noinc
    inc $04
@noinc:
    ldx $07
    bne @declow
    dec $08
@declow:
    dec $07
    lda $05
    bra @loop
@done:
    lda $03
    ldx $04
    rts
    endproc
