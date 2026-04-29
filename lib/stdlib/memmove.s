; memmove.s — Copy memory area (overlap-safe)
;
; void *memmove(void *dest, void *src, int n);
;   Copies n bytes from src to dest. Handles overlapping areas
;   by copying backwards when dest > src.

.global _memmove
.extern __sp_base

.segment "code"

proc _memmove, W#_p_dest, W#_p_src, W#_p_n
    .var _fp = 0
    ldax _p_n, s
    stax $06            ; $06/$07 = count
    lda $06
    ora $07
    beq @done           ; n == 0, nothing to do

    ldax _p_dest, s
    stax $02
    ldax _p_src, s
    stax $04

    ; Compare dest vs src to decide direction
    ; If dest > src, copy backwards to handle overlap
    lda $03
    cmp $05
    bcc @forward        ; dest_hi < src_hi → forward
    bne @backward       ; dest_hi > src_hi → backward
    lda $02
    cmp $04
    bcc @forward        ; dest_lo < src_lo → forward
    beq @done           ; dest == src → nothing to do
    ; fall through to backward

@backward:
    ; Point both pointers to last byte: base + n - 1
    lda $06
    sec
    sbc #1
    tay                 ; Y = (n-1) & 0xFF
    lda $07
    sbc #0
    ; Add (n-1) to dest and src base
    tya
    clc
    adc $02
    sta $02
    lda $07
    adc $03
    sta $03
    tya
    clc
    adc $04
    sta $04
    lda $07
    adc $05
    sta $05
    ldy #0
@back_loop:
    lda $06
    ora $07
    beq @done
    lda ($04),y
    sta ($02),y
    ; Decrement pointers
    lda $02
    bne @no_borrow_d
    dec $03
@no_borrow_d:
    dec $02
    lda $04
    bne @no_borrow_s
    dec $05
@no_borrow_s:
    dec $04
    ; Decrement count
    lda $06
    bne @dec_back
    dec $07
@dec_back:
    dec $06
    bra @back_loop

@forward:
    ldy #0
@fwd_loop:
    lda $06
    ora $07
    beq @done
    lda ($04),y
    sta ($02),y
    iny
    bne @no_inc_fwd
    inc $03
    inc $05
@no_inc_fwd:
    lda $06
    bne @dec_fwd
    dec $07
@dec_fwd:
    dec $06
    bra @fwd_loop
@done:
    ldax _p_dest, s
    rtn #0
    endproc
