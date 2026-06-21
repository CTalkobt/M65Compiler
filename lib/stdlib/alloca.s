; alloca.s — stack allocation for cc45
;
; void* alloca(size_t size);
; void* __builtin_alloca(size_t size);

.global _alloca
.global ___builtin_alloca

.segment "code"

_alloca:
___builtin_alloca:
    ; Pop return address
    pla                 ; PCL
    sta $0A             ; ret_lo
    pla                 ; PCH
    sta $0B             ; ret_hi
    
    ; Pop size
    pla                 ; size_lo
    sta $08
    pla                 ; size_hi
    sta $09
    
    ; SP now at original top. Grow it down by size.
    tsx
    tsy
    
    sec
    txa
    sbc $08
    tax
    tya
    sbc $09
    tay
    
    ; Set new SP
    txs
    tys
    
    ; Push return address back onto new stack
    lda $0B
    pha
    lda $0A
    pha
    
    ; Result pointer in AX is new SP + 2 (points past the pushed return address)
    clc
    txa
    adc #2
    tax
    tya
    adc #0
    
    rts
