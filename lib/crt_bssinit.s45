; crt_bssinit.s — BSS zeroing for cc45 CRT
;
; Zeros the BSS segment at startup using linker-provided
; __bss_start and __bss_end symbols.
;
; Called from the CRT startup stub via "jsr _init_bss".
; Skips zeroing if __bss_start == __bss_end (no BSS).

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
    sta $02
    lda #>__bss_start
    sta $03
    lda #<__bss_end
    sta $04
    lda #>__bss_end
    sta $05
    lda #0
    ldy #0
@__bss_loop:
    ldx $02
    cpx $04
    bne @__bss_store
    ldx $03
    cpx $05
    beq @__bss_done
@__bss_store:
    sta ($02),y
    inc $02
    bne @__bss_loop
    inc $03
    bra @__bss_loop
@__bss_done:
    rts
