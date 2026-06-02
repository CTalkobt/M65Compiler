; crt0_zp.s — Default C Runtime Startup for cc45 (ZP calling convention)
;
; Entry point for all C programs compiled with -fzpcall.
; Same as crt0.s — main takes no parameters via ZP.

.global __init
.global __exit
.global __sp_base
.weak _init_features
.extern _main

.segment "init"

__sp_base = $0101

__init:
    ; Save caller's stack pointer for clean return
    tsx
    stx __saved_spl + 1
    tsy
    sty __saved_sph + 1

    ; Save ZP $08-$FF to BSS buffer via static DMA job
    ldz #0              ; Z must be 0 for stz clears below
    lda #>__dma_save
    sta $D702
    stz $D703
    lda #<__dma_save
    sta $D701
    stz $D700

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore ZP $08-$FF from BSS buffer via static DMA job
    lda #>__dma_restore
    sta $D702
    stz $D703
    lda #<__dma_restore
    sta $D701
    stz $D700

__saved_spl:
    ldx #$FF
    txs
__saved_sph:
    ldy #$01
    tys
    rts

; Default init_features — does nothing. Override with a strong definition.
_init_features:
    rts

.segment "data"
__dma_save:
    .byte $00
    .word 248
    .word $08
    .byte $00
    .word __zp_save_buf
    .byte $00
    .byte $00
    .byte $00

__dma_restore:
    .byte $00
    .word 248
    .word __zp_save_buf
    .byte $00
    .word $08
    .byte $00
    .byte $00
    .byte $00

.segment "bss"
__zp_save_buf:
    .res 248
