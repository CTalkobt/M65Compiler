; memmove.s — Copy memory area, overlap-safe (ZP calling convention)
;
; void *memmove(void *dest, void *src, int n);
;   Copies n bytes from src to dest. Handles overlapping areas.

.global _memmove

.segment "code"

proc _memmove
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V

    ; dest=$03/$04, src=$05/$06, n=$07/$08
    lda $07
    ora $08
    beq @done

    ; Compare dest vs src to decide direction
    lda $04
    cmp $06
    bcc @forward
    bne @backward
    lda $03
    cmp $05
    bcc @forward
    beq @done

@backward:
    ; Point both pointers to last byte: base + n - 1
    lda $07
    sec
    sbc #1
    tay
    lda $08
    sbc #0
    ; Add (n-1) to dest
    tya
    clc
    adc $03
    sta $03
    lda $08
    adc $04
    sta $04
    ; Add (n-1) to src
    tya
    clc
    adc $05
    sta $05
    lda $08
    adc $06
    sta $06
    ldy #0
@back_loop:
    lda $07
    ora $08
    beq @done
    lda ($05),y
    sta ($03),y
    ; Decrement pointers
    lda $03
    bne @no_borrow_d
    dec $04
@no_borrow_d:
    dec $03
    lda $05
    bne @no_borrow_s
    dec $06
@no_borrow_s:
    dec $05
    ; Decrement count
    lda $07
    bne @dec_back
    dec $08
@dec_back:
    dec $07
    bra @back_loop

@forward:
    ldy #0
@fwd_loop:
    lda $07
    ora $08
    beq @done
    lda ($05),y
    sta ($03),y
    iny
    bne @no_inc_fwd
    inc $04
    inc $06
@no_inc_fwd:
    lda $07
    bne @dec_fwd
    dec $08
@dec_fwd:
    dec $07
    bra @fwd_loop
@done:
    lda $03
    ldx $04
    rts
    endproc
