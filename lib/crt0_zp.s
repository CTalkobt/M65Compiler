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

    ; Enable MEGA65 I/O before DMA — GS knock must precede $D700 access
    lda #$47            ; 'G'
    sta $D02F
    lda #$53            ; 'S'
    sta $D02F

    ; Save ZP $08-$FF to BSS buffer via static DMA job
    ldz #0              ; Z must be 0 for stz bank clears below
    stz $D704           ; megabyte bank = 0 (clear stale KERNAL value)
    stz $D702           ; bank = 0
    lda #>__dma_save
    sta $D701           ; DMA list address MSB
    lda #<__dma_save
    sta $D700           ; DMA list address LSB — triggers DMA

    jsr _init_features
    jsr _main

    ; Fall through to __exit
__exit:
    ; Restore ZP $08-$FF from BSS buffer via static DMA job
    ldz #0              ; Z may have changed during program execution
    stz $D704           ; megabyte bank = 0
    stz $D702           ; bank = 0
    lda #>__dma_restore
    sta $D701           ; DMA list address MSB
    lda #<__dma_restore
    sta $D700           ; DMA list address LSB — triggers DMA

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
