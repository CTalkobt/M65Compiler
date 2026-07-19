; crt_bssinit_zp.s — BSS zeroing for cc45 CRT (ZP calling convention)
;
; Zeros the BSS segment at startup. Uses ZP $03-$06 as scratch.
; Called from CRT startup stub via "jsr _init_bss".

.global _init_bss
.extern __bss_start
.extern __bss_end

.segment "code"

_init_bss:
    ; Check if BSS is empty (start == end)
    lda #<__bss_start
    cmp #<__bss_end
    bne @__bss_go
    lda #>__bss_start
    cmp #>__bss_end
    beq @__bss_done

@__bss_go:
    lda #<__bss_start
    sta $03
    lda #>__bss_start
    sta $04
    lda #<__bss_end
    sta $05
    lda #>__bss_end
    sta $06
    lda #0
    ldy #0
@__bss_loop:
    ldx $03
    cpx $05
    bne @__bss_store
    ldx $04
    cpx $06
    beq @__bss_done
@__bss_store:
    sta ($03),y
    inc $03
    bne @__bss_loop
    inc $04
    bra @__bss_loop
@__bss_done:
    rts
