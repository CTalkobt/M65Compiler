; __builtin_bswap32.s
;
; uint32_t __builtin_bswap32(uint32_t x);

.global ___builtin_bswap32

.segment "code"

proc ___builtin_bswap32, D#_p_x
    .var _fp = 0
    ldaxyz.fp _p_x
    ; Input in A, X, Y, Z (A=b0, X=b1, Y=b2, Z=b3)
    
    ; Swap A with Z, X with Y
    sta $08 ; b0
    stx $09 ; b1
    sty $0A ; b2
    stz $0B ; b3
    
    lda $0B ; b3
    ldx $0A ; b2
    ldy $09 ; b1
    ldz $08 ; b0
    
    ; Result returned in A, X, Y, Z
    rts
    endproc
