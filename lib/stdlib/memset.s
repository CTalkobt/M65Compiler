; memset.s — Fill memory with a byte value
;
; void *memset(void *s, int c, int n);
;   Fills n bytes at address s with byte c.
;   Returns s.

.global _memset

.segment "code"

proc _memset, W#_p_s, W#_p_c, W#_p_n
    .var _fp = 0
    ; Load destination pointer into ZP
    ldax _p_s, s
    stax $02            ; $02/$03 = dest pointer
    ; Load fill byte
    lda.sp _p_c
    sta $04             ; $04 = fill byte
    ; Load count
    ldax _p_n, s
    stax $05            ; $05/$06 = count

    ldy #0
    lda $04
@loop:
    ldx $06
    bne @do
    cpy $05
    beq @done
@do:
    sta ($02),y
    iny
    bne @noinc
    inc $03
@noinc:
    ldx $05
    bne @declow
    dec $06
@declow:
    dec $05
    lda $04
    bra @loop
@done:
    ; return s
    ldax _p_s, s
    rtn #0
    endproc
