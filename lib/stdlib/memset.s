; memset.s — Fill memory with a byte value
;
; void *memset(void *s, int c, int n);
;   Fills n bytes at address s with byte c.
;   Returns s.

.global _memset
.extern __sp_base

.segment "code"

proc _memset, W#_p_s, W#_p_c, W#_p_n
    .var _fp = 0
    ; Load all three params with a single TSX
    tsx
    lda __sp_base+_p_s, x      ; dest lo
    sta $02
    lda __sp_base+_p_s+1, x    ; dest hi
    sta $03
    lda __sp_base+_p_c, x      ; fill byte
    sta $04
    lda __sp_base+_p_n, x      ; count lo
    sta $05
    lda __sp_base+_p_n+1, x    ; count hi
    sta $06

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
    ; return s (lo in A, hi in X)
    tsx
    lda __sp_base+_p_s, x
    ldy __sp_base+_p_s+1, x
    sty $08
    ldx $08
    rtn #0
    endproc
