; memcpy.s — Copy memory area
;
; void *memcpy(void *dest, void *src, int n);
;   Copies n bytes from src to dest. Areas must not overlap.
;   Returns dest.

.global _memcpy
.extern __sp_base

.segment "code"

proc _memcpy, W#_p_dest, W#_p_src, W#_p_n
    .var _fp = 0
    ; Save ZP $02-$03 to stack (+2 bytes on stack)
    lda $02
    pha
    lda $03
    pha

    ; Load dest into ZP $02/$03 (offsets +2 for the PHA saves)
    tsx
    lda __sp_base+_p_dest+2, x
    sta $02
    lda __sp_base+_p_dest+3, x
    sta $03
    ldy #0
@loop:
    ; SP-relative offsets include +2 for the PHA saves
    lda _p_n+2, sp
    bne @do_copy
    lda _p_n+3, sp
    beq @done
@do_copy:
    lda (_p_src+2, sp), y   ; load from src via stack-relative indirect
    sta ($02),y         ; store to dest
    iny
    bne @no_inc
    inc $03
    ; increment src high byte on stack
    inc _p_src+3, sp
@no_inc:
    ; decrement count in-place on stack
    lda _p_n+2, sp
    bne @dec_lo
    dec _p_n+3, sp
@dec_lo:
    dec _p_n+2, sp
    bra @loop
@done:
    ; Restore ZP (pops the 2 PHA saves)
    plz
    stz $03
    plz
    stz $02
    ; Return dest — no +2 adjustment needed here, PHA saves are gone
    tsx
    lda __sp_base+_p_dest+1, x     ; hi
    pha
    lda __sp_base+_p_dest, x       ; lo
    plx                             ; X = hi
    rts
    endproc
